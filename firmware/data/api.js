// ScenTTy machine client. Two interchangeable implementations:
//   real — talks to the ESP32 over the JSON API (SPEC §5)
//   mock — fakes the machine in the browser so the whole flow works on a laptop
// ScenttyMachine.connect() picks one: ?mock (any value) forces mock; otherwise it
// tries GET /api/status and falls back to mock if the API isn't there.
//
// Mock options (URL):  ?mock=1      plain fake machine
//                      ?mock=busy   someone else's bottle is mid-blend when you arrive
//                      ?mock=error  your first blend fails partway through
//
// Every method returns a Promise. Status shape (same for real and mock):
//   { state: "idle"|"dispensing"|"done"|"error"|"busy",   busy = operator is running pumps
//     job: 12,                     id of the current/last blend
//     step: 0..3 | -1,             pump currently running
//     pumps: [{ id: "orange", target_ml, done_ml }, … lavender, cedar, alcohol],
//     error: "message" | null }

var ScenttyMachine = (function () {
  var PUMP_IDS = ["orange", "lavender", "cedar", "alcohol"];

  function request(url, opts, timeoutMs) {
    opts = opts || {};
    var ctrl = typeof AbortController !== "undefined" ? new AbortController() : null;
    var timer = ctrl ? setTimeout(function () { ctrl.abort(); }, timeoutMs || 4000) : null;
    if (ctrl) opts.signal = ctrl.signal;
    return fetch(url, opts).then(
      function (res) {
        clearTimeout(timer);
        return res.text().then(function (txt) {
          var body = null;
          try { body = txt ? JSON.parse(txt) : {}; } catch (e) { /* not JSON */ }
          return { ok: res.ok, status: res.status, body: body };
        });
      },
      function (err) { clearTimeout(timer); throw err; }
    );
  }

  function post(url, data, timeoutMs) {
    return request(url, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(data || {}),
    }, timeoutMs);
  }

  function expectOk(r) {
    if (!r.ok || !r.body) throw new Error((r.body && r.body.error) || "HTTP " + r.status);
    return r.body;
  }

  // ---- real machine ---------------------------------------------------------
  function real() {
    return {
      mock: false,
      status: function () { return request("/api/status").then(expectOk); },
      // → {accepted: true, job} | {accepted: false, busy: true, status}
      blend: function (recipe) {
        return post("/api/blend", recipe).then(function (r) {
          if (r.status === 409) return { accepted: false, busy: true, status: r.body && r.body.status };
          return expectOk(r);
        });
      },
      cancel: function () { return post("/api/cancel").then(expectOk); },
      admin: {
        run: function (pump, ms) { return post("/api/admin/run", { pump: pump, ms: ms }).then(expectOk); },
        stop: function () { return post("/api/admin/stop").then(expectOk); },
        getCalibration: function () { return request("/api/admin/calibration").then(expectOk); },
        setCalibration: function (pump, mlPerS) {
          return post("/api/admin/calibration", { pump: pump, ml_per_s: mlPerS }).then(expectOk);
        },
        led: function (pattern) { return post("/api/admin/led", { pattern: pattern }).then(expectOk); },
      },
    };
  }

  // ---- mock machine ---------------------------------------------------------
  function mock(mode) {
    var RATE = 2.5;   // fake mL per second, so a full blend takes ~10 s
    var TICK = 100;
    var st = {
      state: "idle", job: 0, step: -1, error: null,
      pumps: PUMP_IDS.map(function (id) { return { id: id, target_ml: 0, done_ml: 0 }; }),
    };
    var failJob = mode === "error" ? 1 : 0; // which job id should fail
    var manualUntil = 0;
    var calibration = [1.0, 1.0, 1.0, 1.0];
    var timer = null;

    function startJob(recipe) {
      st.job += 1;
      st.state = "dispensing";
      st.error = null;
      st.step = -1;
      PUMP_IDS.forEach(function (id, i) {
        st.pumps[i].target_ml = recipe[id + "_ml"] || 0;
        st.pumps[i].done_ml = 0;
      });
      nextPump();
      if (!timer) timer = setInterval(tick, TICK);
      return st.job;
    }

    function nextPump() {
      do { st.step += 1; } while (st.step < 4 && st.pumps[st.step].target_ml <= 0);
      if (st.step >= 4) { st.step = -1; st.state = "done"; }
    }

    function tick() {
      if (st.state === "busy" && Date.now() >= manualUntil) { st.state = "idle"; st.step = -1; }
      if (st.state !== "dispensing") return;
      var p = st.pumps[st.step];
      p.done_ml = Math.min(p.target_ml, p.done_ml + RATE * TICK / 1000);
      if (st.job === failJob && st.step === 2 && p.done_ml > p.target_ml / 2) {
        st.state = "error";
        st.error = "Pump 3 (cedarwood) hit its safety time limit.";
        return;
      }
      if (p.done_ml >= p.target_ml) nextPump();
    }

    function snapshot() { return JSON.parse(JSON.stringify(st)); }
    function later(value) {
      return new Promise(function (resolve) { setTimeout(function () { resolve(value); }, 80); });
    }
    var free = function () { return st.state === "idle" || st.state === "done" || st.state === "error"; };

    if (mode === "busy") {
      // someone else's job is already running; bump ids so ours differs
      startJob({ orange_ml: 8, lavender_ml: 5, cedar_ml: 3, alcohol_ml: 8 });
      failJob = 0;
    }

    return {
      mock: true,
      mode: mode,
      status: function () { return later(snapshot()); },
      blend: function (recipe) {
        if (!free()) return later({ accepted: false, busy: true, status: snapshot() });
        return later({ accepted: true, job: startJob(recipe) });
      },
      cancel: function () {
        st.state = "idle"; st.step = -1; st.error = null;
        return later({ ok: true });
      },
      admin: {
        run: function (pump, ms) {
          if (st.state === "dispensing") return later({ ok: false, error: "busy" });
          st.state = "busy"; st.step = pump;
          manualUntil = Date.now() + Math.min(ms, 60000);
          if (!timer) timer = setInterval(tick, TICK);
          return later({ ok: true });
        },
        stop: function () {
          if (st.state === "busy") { st.state = "idle"; st.step = -1; }
          return later({ ok: true });
        },
        getCalibration: function () { return later({ ml_per_s: calibration.slice() }); },
        setCalibration: function (pump, mlPerS) {
          calibration[pump] = mlPerS;
          return later({ ok: true, ml_per_s: calibration.slice() });
        },
        led: function (pattern) { return later({ ok: true, pattern: pattern }); },
      },
    };
  }

  function connect() {
    var mode = new URLSearchParams(location.search).get("mock");
    if (mode !== null) return Promise.resolve(mock(mode));
    return request("/api/status", {}, 1500).then(
      function (r) { return r.ok && r.body && r.body.state ? real() : mock("auto"); },
      function () { return mock("auto"); }
    );
  }

  return { connect: connect, PUMP_IDS: PUMP_IDS };
})();
