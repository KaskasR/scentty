// ScenTTy operator page: prime (hold-to-run), flush, calibrate, test blend, LEDs, E-stop.

(function () {
  "use strict";

  var S = SCENTTY_SCORING;
  var PUMPS = [
    { id: "orange", name: "Orange", col: "var(--c-orange)", soft: "var(--c-orange-soft)" },
    { id: "lavender", name: "Lavender", col: "var(--c-lavender)", soft: "var(--c-lavender-soft)" },
    { id: "cedar", name: "Cedar", col: "var(--c-cedar)", soft: "var(--c-cedar-soft)" },
    { id: "alcohol", name: "Alcohol", col: "var(--c-ink-3)", soft: "var(--c-surface-2)" },
  ];
  // Hold-to-run: each request runs the pump for HOLD_MS; we re-send every HOLD_EVERY_MS
  // while the finger is down. If the phone disappears, the pump stops on its own.
  var HOLD_MS = 400, HOLD_EVERY_MS = 250;

  var $ = function (id) { return document.getElementById(id); };
  var machine = null;
  var flushGen = 0;
  var sleep = function (ms) { return new Promise(function (r) { setTimeout(r, ms); }); };
  function log(id, msg) { $(id).textContent = msg; }

  // ---- status bar --------------------------------------------------------------
  function pollStatus() {
    machine.status().then(function (st) {
      var chip = $("s-state");
      chip.textContent = st.state;
      chip.dataset.state = st.state;
      var detail = "";
      if (st.state === "dispensing") {
        var p = st.pumps[st.step];
        detail = "job " + st.job + " · " + (p ? p.id + " " + p.done_ml.toFixed(1) + "/" + p.target_ml.toFixed(1) + " mL" : "");
      } else if (st.state === "busy" && st.step >= 0) {
        detail = "running " + PUMPS[st.step].name.toLowerCase();
      } else if (st.state === "error") {
        detail = st.error || "";
      }
      $("s-detail").textContent = detail;
      $("s-detail").hidden = !detail;
    }, function () {
      $("s-state").textContent = "no connection";
      $("s-state").dataset.state = "error";
    }).then(function () { setTimeout(pollStatus, 700); });
  }

  // ---- prime: hold-to-run ---------------------------------------------------------
  function holdToRun(btn, pump) {
    var timer = null;
    function send() { machine.admin.run(pump, HOLD_MS).catch(function () {}); }
    function start(e) {
      e.preventDefault();
      if (timer) return;
      btn.classList.add("is-running");
      send();
      timer = setInterval(send, HOLD_EVERY_MS);
      try { btn.setPointerCapture(e.pointerId); } catch (err) { /* old browsers */ }
    }
    function end() {
      if (!timer) return;
      clearInterval(timer);
      timer = null;
      btn.classList.remove("is-running");
      machine.admin.stop().catch(function () {});
    }
    btn.addEventListener("pointerdown", start);
    ["pointerup", "pointercancel", "lostpointercapture"].forEach(function (ev) { btn.addEventListener(ev, end); });
    btn.addEventListener("contextmenu", function (e) { e.preventDefault(); });
    document.addEventListener("visibilitychange", function () { if (document.hidden) end(); });
  }

  PUMPS.forEach(function (p, i) {
    var b = document.createElement("button");
    b.className = "hold";
    b.type = "button";
    b.innerHTML = p.name + "<small>pump " + (i + 1) + " · hold</small>";
    b.style.setProperty("--soft", p.soft);
    b.style.setProperty("--col", p.col);
    holdToRun(b, i);
    $("prime").appendChild(b);

    var o = document.createElement("option");
    o.value = i;
    o.textContent = (i + 1) + " · " + p.name;
    $("cal-pump").appendChild(o);
  });

  // ---- flush -------------------------------------------------------------------------
  $("flush-go").addEventListener("click", function () {
    var secs = Math.min(60, Math.max(1, +$("flush-s").value || 8));
    var gen = ++flushGen;
    (function next(i) {
      if (gen !== flushGen) return;
      if (i >= PUMPS.length) return log("flush-log", "Flush done.");
      log("flush-log", "Flushing " + PUMPS[i].name.toLowerCase() + " (" + secs + " s)…");
      machine.admin.run(i, secs * 1000)
        .then(function () { return sleep(secs * 1000 + 300); })
        .then(function () { next(i + 1); }, function (e) { log("flush-log", "Stopped: " + e.message); });
    })(0);
  });

  // ---- calibrate ------------------------------------------------------------------------
  var calRunSecs = null;

  function renderCalibration(c) {
    $("cal-table").innerHTML = PUMPS.map(function (p, i) {
      return "<tr><td>" + (i + 1) + " · " + p.name + "</td><td>" + (+c.ml_per_s[i]).toFixed(3) + " mL/s</td></tr>";
    }).join("");
  }
  function loadCalibration() {
    machine.admin.getCalibration().then(renderCalibration, function () { log("cal-log", "Couldn't load calibration."); });
  }

  $("cal-run").addEventListener("click", function () {
    var pump = +$("cal-pump").value;
    var secs = Math.min(60, Math.max(1, +$("cal-s").value || 10));
    calRunSecs = null;
    log("cal-log", "Running " + PUMPS[pump].name.toLowerCase() + " for " + secs + " s…");
    machine.admin.run(pump, secs * 1000).then(function () {
      return sleep(secs * 1000 + 300);
    }).then(function () {
      calRunSecs = secs;
      log("cal-log", "Done. Measure it and enter the mL.");
      $("cal-ml").focus();
    }, function (e) { log("cal-log", "Couldn't run: " + e.message); });
  });

  $("cal-save").addEventListener("click", function () {
    var pump = +$("cal-pump").value, ml = +$("cal-ml").value;
    if (!calRunSecs) return log("cal-log", "Run the pump first.");
    if (!(ml > 0)) return log("cal-log", "Enter the measured mL.");
    var rate = ml / calRunSecs;
    machine.admin.setCalibration(pump, +rate.toFixed(4)).then(function () {
      log("cal-log", "Saved " + PUMPS[pump].name.toLowerCase() + ": " + rate.toFixed(3) + " mL/s");
      $("cal-ml").value = "";
      calRunSecs = null;
      loadCalibration();
    }, function (e) { log("cal-log", "Save failed: " + e.message); });
  });

  // ---- test blend ------------------------------------------------------------------------
  $("tb-go").addEventListener("click", function () {
    var mix = $("tb-mix").value.split(",").map(Number);
    var r = S.recipe({ orange: mix[0], lavender: mix[1], cedar: mix[2] }, $("tb-strength").value);
    machine.blend({
      orange_ml: r.orange_ml, lavender_ml: r.lavender_ml, cedar_ml: r.cedar_ml, alcohol_ml: r.alcohol_ml,
      profile: "test", strength: r.strength,
    }).then(function (res) {
      log("tb-log", res.accepted
        ? "Started job " + res.job + ": " + [r.orange_ml, r.lavender_ml, r.cedar_ml, r.alcohol_ml].map(function (v) { return v.toFixed(1); }).join(" / ") + " mL"
        : "Machine is busy.");
    }, function (e) { log("tb-log", "Failed: " + e.message); });
  });

  // ---- LEDs ---------------------------------------------------------------------------------
  document.querySelectorAll("[data-led]").forEach(function (b) {
    b.addEventListener("click", function () { machine.admin.led(b.dataset.led).catch(function () {}); });
  });

  // ---- E-stop: cancels everything, including a customer's blend ---------------------------------
  $("estop").addEventListener("click", function () {
    flushGen++;
    machine.cancel().catch(function () {});
    machine.admin.stop().catch(function () {});
    log("flush-log", "");
  });

  ScenttyMachine.connect().then(function (m) {
    machine = m;
    if (m.mock) $("mode-tag").hidden = false;
    pollStatus();
    loadCalibration();
  });
})();
