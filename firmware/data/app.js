// ScenTTy phone app: screen flow, rendering, triangle chart, machine polling.
// Content comes from quiz.js, math from scoring.js, the machine from api.js.
//
// Dev shortcuts (URL):  ?quick=1  pre-fills random answers and jumps to the strength question

(function () {
  "use strict";

  var QUIZ = SCENTTY_QUIZ;
  var S = SCENTTY_SCORING;

  var SCENTS = {
    orange:   { name: "Sweet orange", short: "Orange",   note: "top note" },
    lavender: { name: "Lavender",     short: "Lavender", note: "heart note" },
    cedar:    { name: "Cedarwood",    short: "Cedar",    note: "base note" },
    alcohol:  { name: "Alcohol base", short: "Base",     note: "sets the strength" },
  };
  var TINTS = ["orange", "lavender", "cedar"];
  var STRENGTH_LABELS = { whisper: "Whisper", signature: "Signature", bold: "Bold" };
  var POLL_MS = 400, RETRY_BUSY_MS = 2000, MAX_POLL_FAILS = 15;

  var NQ = QUIZ.questions.length;   // personality questions
  var TOTAL = NQ + 1;               // + strength

  var state = {
    i: 0,                 // current question index (NQ = strength question)
    answers: [],
    strength: null,
    result: null,
    job: null,
    machine: null,
    pollGen: 0,           // bump to invalidate any running poll/retry loop
  };

  // ---- helpers ---------------------------------------------------------------
  var $ = function (id) { return document.getElementById(id); };
  function color(id) { return "var(--c-" + id + ")"; }
  function soft(id) { return id === "alcohol" ? "var(--c-surface-2)" : "var(--c-" + id + "-soft)"; }
  function el(tag, cls, html) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (html !== undefined) e.innerHTML = html;
    return e;
  }
  function esc(s) {
    return String(s).replace(/[&<>"]/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]; });
  }
  var reducedMotion = window.matchMedia && matchMedia("(prefers-reduced-motion: reduce)").matches;

  // Integer percentages that always add up to 100 (largest remainder).
  function wholePercents(shares) {
    var keys = S.SCENTS, raw = keys.map(function (k) { return shares[k] * 100; });
    var out = raw.map(Math.floor), left = 100 - out.reduce(function (a, b) { return a + b; }, 0);
    raw.map(function (v, i) { return [v - out[i], i]; })
      .sort(function (a, b) { return b[0] - a[0]; })
      .slice(0, left).forEach(function (r) { out[r[1]] += 1; });
    var res = {}; keys.forEach(function (k, i) { res[k] = out[i]; });
    return res;
  }

  // ---- screens ---------------------------------------------------------------
  var current = "welcome";
  function show(name) {
    if (name !== current) {
      document.querySelectorAll(".screen").forEach(function (s) {
        s.classList.toggle("is-active", s.dataset.screen === name);
      });
      current = name;
      window.scrollTo(0, 0);
    }
  }

  function stopLoops() { state.pollGen += 1; }

  // ---- questions ---------------------------------------------------------------
  function setTint(i) {
    var t = TINTS[i % 3];
    var root = document.documentElement.style;
    root.setProperty("--tint", color(t));
    root.setProperty("--tint-soft", soft(t));
  }

  function renderQuestion(direction) {
    var i = state.i, isStrength = i === NQ;
    var q = isStrength ? QUIZ.strengthQuestion : QUIZ.questions[i];
    var picked = isStrength ? null : state.answers[i];

    setTint(i);
    $("q-num").textContent = i + 1;
    $("q-total").textContent = TOTAL;
    $("q-progress").style.width = (i / TOTAL) * 100 + "%";
    $("q-kicker").textContent = isStrength ? (q.kicker || "") : ((QUIZ.checkpoints || {})[i + 1] || "");
    $("q-text").textContent = q.q;
    $("q-text").classList.toggle("question__text--long", q.q.length > 44);

    var list = $("q-answers");
    list.innerHTML = "";
    q.answers.forEach(function (a, n) {
      var li = el("li");
      li.style.setProperty("--i", n);
      var b = el("button", "answer");
      b.type = "button";
      b.dataset.index = n;
      if (isStrength) {
        var level = { whisper: 1, signature: 2, bold: 3 }[a.strength];
        b.innerHTML = '<span class="answer__meter" data-level="' + level + '"><i></i><i></i><i></i></span>' +
          '<span><span class="answer__label">' + esc(a.label) + "</span>" + esc(a.text) + "</span>";
        if (state.strength === a.strength) b.classList.add("is-picked");
      } else {
        b.innerHTML = '<span class="answer__mark">' + "ABCDEFG"[n] + "</span><span>" + esc(a.text) + "</span>";
        if (picked === n) b.classList.add("is-picked");
      }
      li.appendChild(b);
      list.appendChild(li);
    });

    var box = $("question");
    box.classList.remove("is-entering", "is-leaving", "is-back");
    void box.offsetWidth; // restart the animation
    if (direction === "back") box.classList.add("is-back");
    box.classList.add("is-entering");
  }

  var advancing = false;
  function pickAnswer(n, button) {
    if (advancing) return;
    advancing = true;
    document.querySelectorAll("#q-answers .answer").forEach(function (b) { b.classList.remove("is-picked"); });
    button.classList.add("is-picked");

    if (state.i === NQ) state.strength = QUIZ.strengthQuestion.answers[n].strength;
    else state.answers[state.i] = n;

    setTimeout(function () {
      $("question").classList.add("is-leaving");
      setTimeout(function () {
        advancing = false;
        if (state.i === NQ) return reveal();
        state.i += 1;
        renderQuestion("forward");
      }, reducedMotion ? 0 : 140);
    }, reducedMotion ? 60 : 260);
  }

  function goBack() {
    if (advancing) return;
    if (state.i === 0) return show("welcome");
    state.i -= 1;
    renderQuestion("back");
  }

  function startQuiz() {
    state.i = 0; state.answers = []; state.strength = null; state.result = null;
    resetAmbient();
    show("question");
    renderQuestion("forward");
  }

  // ---- reveal ---------------------------------------------------------------------
  var BUILDUP_LINES = ["Reading your answers…", "Weighing spark, calm and anchor…", "Balancing top, heart and base…"];

  function reveal() {
    state.result = S.result(QUIZ, state.answers, state.strength);
    renderResult(state.result);
    $("q-progress").style.width = "100%";

    var buildup = $("buildup"), result = $("result"), line = $("buildup-line");
    buildup.hidden = false; result.hidden = true;
    // restart the orb animation
    buildup.querySelectorAll(".orb").forEach(function (o) { o.style.animation = "none"; void o.offsetWidth; o.style.animation = ""; });
    show("reveal");

    var step = reducedMotion ? 150 : 820;
    BUILDUP_LINES.forEach(function (txt, n) {
      setTimeout(function () {
        line.textContent = txt;
        line.style.animation = "none"; void line.offsetWidth; line.style.animation = "";
      }, n * step);
    });
    setTimeout(function () {
      buildup.hidden = true;
      result.hidden = false;
      setAmbient(state.result.recipe.shares);
      placeDot(state.result.recipe.shares);
    }, BUILDUP_LINES.length * step);
  }

  function renderResult(res) {
    var p = res.profile, r = res.recipe, sh = r.shares;
    var ranked = S.SCENTS.slice().sort(function (a, b) { return sh[b] - sh[a]; });

    $("r-name").textContent = p.name;
    $("r-name").style.setProperty("--name-a", color(ranked[0]));
    $("r-name").style.setProperty("--name-b", color(res.profileId === "harmony" ? ranked[2] : ranked[1]));
    $("r-tagline").textContent = p.tagline;
    $("r-desc").textContent = p.description;
    $("r-why").textContent = p.why;

    var pct = wholePercents(sh), shares = $("r-shares");
    shares.innerHTML = "";
    S.SCENTS.forEach(function (k) {
      var li = el("li", "share",
        '<span class="share__pct">' + pct[k] + '%</span>' +
        '<span class="share__name">' + SCENTS[k].short + "</span>" +
        '<span class="share__note">' + SCENTS[k].note + "</span>" +
        '<span class="share__bar"><i></i></span>');
      li.style.setProperty("--soft", soft(k));
      li.style.setProperty("--col", color(k));
      li.style.setProperty("--w", (sh[k] / sh[ranked[0]]) * 100 + "%");
      shares.appendChild(li);
    });

    var pctOil = Math.round(S.BLEND.strengths[r.strength] * 100);
    $("r-strength").textContent = STRENGTH_LABELS[r.strength] + " · " + pctOil + "% oil";

    var list = $("r-recipe");
    list.innerHTML = "";
    ["orange", "lavender", "cedar", "alcohol"].forEach(function (k) {
      var ml = r[k + "_ml"];
      var row = el("li", "recipe__row",
        '<span class="recipe__swatch"></span>' +
        '<span class="recipe__name">' + SCENTS[k].name + "</span>" +
        '<span class="recipe__ml">' + ml.toFixed(1) + "<small>mL</small></span>");
      row.style.setProperty("--col", k === "alcohol" ? "var(--c-alcohol)" : color(k));
      list.appendChild(row);
    });

    // stagger the result blocks in
    Array.prototype.forEach.call($("result").children, function (c, n) { c.style.setProperty("--i", n); });
    drawTriangle();
  }

  // ---- triangle chart ------------------------------------------------------------------
  // Equilateral triangle; orange on top, lavender bottom-left, cedar bottom-right.
  var V = { orange: [160, 44], lavender: [30, 269], cedar: [290, 269] };
  function bary(sh) {
    return [
      sh.orange * V.orange[0] + sh.lavender * V.lavender[0] + sh.cedar * V.cedar[0],
      sh.orange * V.orange[1] + sh.lavender * V.lavender[1] + sh.cedar * V.cedar[1],
    ];
  }
  function pts(list) { return list.map(function (p) { return p.join(","); }).join(" "); }

  function drawTriangle() {
    var f = S.BLEND.floor, hi = 1 - 2 * f;
    var inner = [
      bary({ orange: hi, lavender: f, cedar: f }),
      bary({ orange: f, lavender: hi, cedar: f }),
      bary({ orange: f, lavender: f, cedar: hi }),
    ];
    var outer = [V.orange, V.lavender, V.cedar];
    var c = bary({ orange: 1 / 3, lavender: 1 / 3, cedar: 1 / 3 });
    var grad = function (id, k) {
      return '<radialGradient id="g-' + id + '" gradientUnits="userSpaceOnUse" cx="' + V[k][0] + '" cy="' + V[k][1] + '" r="250">' +
        '<stop offset="0" style="stop-color:' + color(k) + ';stop-opacity:.55"/>' +
        '<stop offset="1" style="stop-color:' + color(k) + ';stop-opacity:0"/></radialGradient>';
    };
    var grid = outer.map(function (v, n) {
      var a = outer[(n + 1) % 3], b = outer[(n + 2) % 3];
      var mid = [(a[0] + b[0]) / 2, (a[1] + b[1]) / 2];
      return '<line class="tri__grid" x1="' + v[0] + '" y1="' + v[1] + '" x2="' + mid[0] + '" y2="' + mid[1] + '"/>';
    }).join("");

    $("tri").innerHTML =
      "<defs>" + grad("o", "orange") + grad("l", "lavender") + grad("c", "cedar") + "</defs>" +
      '<polygon points="' + pts(outer) + '" fill="var(--c-surface)"/>' +
      '<polygon points="' + pts(outer) + '" fill="url(#g-o)"/>' +
      '<polygon points="' + pts(outer) + '" fill="url(#g-l)"/>' +
      '<polygon points="' + pts(outer) + '" fill="url(#g-c)"/>' +
      grid +
      '<polygon class="tri__inner" points="' + pts(inner) + '"/>' +
      '<polygon class="tri__edge" points="' + pts(outer) + '"/>' +
      '<text class="tri__label" x="160" y="18" text-anchor="middle">Orange</text>' +
      '<text class="tri__note" x="160" y="33" text-anchor="middle">spark · top note</text>' +
      '<text class="tri__label" x="30" y="292" text-anchor="start">Lavender</text>' +
      '<text class="tri__note" x="30" y="307" text-anchor="start">calm · heart</text>' +
      '<text class="tri__label" x="290" y="292" text-anchor="end">Cedar</text>' +
      '<text class="tri__note" x="290" y="307" text-anchor="end">anchor · base</text>' +
      '<g class="tri__you" id="tri-you" style="transform:translate(' + c[0] + 'px,' + c[1] + 'px)">' +
      '<circle class="tri__halo" r="16"/><circle class="tri__dot" r="9"/>' +
      '<text class="tri__tag" y="-17" text-anchor="middle">YOU</text></g>';
  }

  function placeDot(sh) {
    var p = bary(sh), g = $("tri-you");
    void g.getBoundingClientRect(); // make sure the start position is painted first
    requestAnimationFrame(function () {
      g.style.transform = "translate(" + p[0] + "px," + p[1] + "px)";
    });
  }

  // ambient background leans toward your blend
  function setAmbient(sh) {
    var root = document.documentElement.style;
    S.SCENTS.forEach(function (k) { root.setProperty("--amb-" + k, (0.6 + sh[k] * 1.6).toFixed(2)); });
  }
  function resetAmbient() {
    var root = document.documentElement.style;
    S.SCENTS.forEach(function (k) { root.setProperty("--amb-" + k, "1"); });
  }

  // ---- get ready + blending ---------------------------------------------------------------
  function toReady(notice) {
    stopLoops();
    var n = $("ready-notice");
    n.hidden = !notice;
    n.textContent = notice || "";
    show("ready");
  }

  function blendPayload() {
    var r = state.result.recipe;
    return {
      orange_ml: r.orange_ml, lavender_ml: r.lavender_ml, cedar_ml: r.cedar_ml, alcohol_ml: r.alcohol_ml,
      profile: state.result.profileId, strength: r.strength,
    };
  }

  function startBlend() {
    if (!state.machine) return setTimeout(startBlend, 300); // still connecting
    stopLoops();
    var gen = state.pollGen;
    var startBtn = document.querySelector('[data-action="start-blend"]');
    startBtn.disabled = true;

    (function attempt() {
      if (gen !== state.pollGen) return;
      state.machine.blend(blendPayload()).then(function (r) {
        if (gen !== state.pollGen) return;
        startBtn.disabled = false;
        if (r.accepted) {
          state.job = r.job;
          resetMaking();
          show("making");
          poll(gen, 0);
        } else if (r.busy) {
          show("waiting");
          renderWaiting(r.status);
          setTimeout(attempt, RETRY_BUSY_MS);
        } else {
          showError(r.error || "The machine didn't accept the blend.");
        }
      }, function () {
        startBtn.disabled = false;
        if (gen !== state.pollGen) return;
        showError("Can't reach the machine. Make sure you're still on the ScenTTy WiFi.");
      });
    })();
  }

  function renderWaiting(st) {
    var p = 0;
    if (st && st.pumps) {
      var t = 0, d = 0;
      st.pumps.forEach(function (x) { t += x.target_ml; d += x.done_ml; });
      p = t > 0 ? (d / t) * 100 : 0;
    }
    $("wait-ring").style.setProperty("--p", p.toFixed(1));
  }

  var PUMP_ORDER = ["orange", "lavender", "cedar", "alcohol"];

  function resetMaking() {
    var name = state.result.profile.name;
    $("m-title").textContent = name;
    $("m-pct").textContent = "0";
    $("m-step").textContent = "Warming up…";
    disarmCancel();

    var r = state.result.recipe, fill = $("m-fill"), list = $("m-pumps");
    fill.innerHTML = ""; list.innerHTML = "";
    PUMP_ORDER.forEach(function (k) {
      var layer = el("span", "bottle__layer");
      layer.style.setProperty("--col", k === "alcohol" ? "var(--c-alcohol)" : color(k));
      fill.appendChild(layer);

      var target = r[k + "_ml"];
      var li = el("li", "pump",
        '<span class="pump__name">' + SCENTS[k].name + "</span>" +
        '<span class="pump__ml">' + (target > 0 ? "0.0 / " + target.toFixed(1) + " mL" : "none") + "</span>" +
        '<span class="pump__bar"><i></i></span>');
      li.style.setProperty("--col", k === "alcohol" ? "var(--c-ink-3)" : color(k));
      li.dataset.state = target > 0 ? "waiting" : "skip";
      list.appendChild(li);
    });
  }

  function renderMaking(st) {
    var total = S.BLEND.blendMl, done = 0;
    var layers = $("m-fill").children, rows = $("m-pumps").children;
    st.pumps.forEach(function (p, n) {
      done += p.done_ml;
      layers[n].style.height = (p.done_ml / total) * 100 + "%";
      var row = rows[n];
      if (!row) return;
      if (p.target_ml > 0) {
        row.dataset.state = n === st.step ? "active" : p.done_ml >= p.target_ml - 1e-6 ? "done" : "waiting";
        row.querySelector(".pump__bar i").style.width = (p.done_ml / p.target_ml) * 100 + "%";
        row.querySelector(".pump__ml").textContent = p.done_ml.toFixed(1) + " / " + p.target_ml.toFixed(1) + " mL";
      }
    });
    $("m-pct").textContent = Math.min(100, Math.round((done / total) * 100));
    var k = PUMP_ORDER[st.step];
    $("m-step").textContent = !k ? "Finishing up…" :
      k === "alcohol" ? "Adding the alcohol base" : "Pouring " + SCENTS[k].name.toLowerCase() + " · " + SCENTS[k].note;
  }

  function poll(gen, fails) {
    if (gen !== state.pollGen) return;
    state.machine.status().then(function (st) {
      if (gen !== state.pollGen) return;
      if (st.job !== state.job) return showError("Your blend was interrupted.");
      renderMaking(st);
      if (st.state === "done") return finish();
      if (st.state === "error") return showError(st.error || "Something went wrong mid-blend.");
      if (st.state === "idle") return toReady("Stopped. Empty the bottle before you start again.");
      setTimeout(function () { poll(gen, 0); }, POLL_MS);
    }, function () {
      if (gen !== state.pollGen) return;
      if (fails + 1 >= MAX_POLL_FAILS) return showError("Lost the connection to the machine. Make sure you're still on the ScenTTy WiFi.");
      setTimeout(function () { poll(gen, fails + 1); }, POLL_MS * 1.5);
    });
  }

  // Stop needs two taps so nobody cancels by accident.
  var armTimer = null;
  function disarmCancel() {
    clearTimeout(armTimer);
    var b = $("m-cancel");
    b.classList.remove("is-armed");
    b.textContent = "Stop";
  }
  function cancelTapped() {
    var b = $("m-cancel");
    if (!b.classList.contains("is-armed")) {
      b.classList.add("is-armed");
      b.textContent = "Tap again to stop";
      armTimer = setTimeout(disarmCancel, 3000);
      return;
    }
    disarmCancel();
    stopLoops();
    state.machine.cancel().catch(function () {});
    toReady("Stopped. Empty the bottle before you start again.");
  }

  function finish() {
    stopLoops();
    $("d-name").textContent = state.result.profile.name;
    var fill = $("d-fill"), r = state.result.recipe;
    fill.innerHTML = "";
    PUMP_ORDER.forEach(function (k) {
      var layer = el("span", "bottle__layer");
      layer.style.setProperty("--col", k === "alcohol" ? "var(--c-alcohol)" : color(k));
      layer.style.height = (r[k + "_ml"] / S.BLEND.blendMl) * 100 + "%";
      fill.appendChild(layer);
    });
    show("done");
  }

  function showError(msg) {
    stopLoops();
    $("e-msg").textContent = msg;
    show("error");
  }

  // ---- wiring ------------------------------------------------------------------------------------
  var actions = {
    start: startQuiz,
    back: goBack,
    retake: startQuiz,
    "to-ready": function () { toReady(); },
    "to-result": function () { stopLoops(); show("reveal"); },
    "start-blend": startBlend,
    "leave-queue": function () { toReady(); },
    cancel: cancelTapped,
    restart: function () { stopLoops(); resetAmbient(); show("welcome"); },
  };

  document.addEventListener("click", function (e) {
    var answer = e.target.closest(".answer");
    if (answer) return pickAnswer(+answer.dataset.index, answer);
    var a = e.target.closest("[data-action]");
    if (a && actions[a.dataset.action]) actions[a.dataset.action]();
  });

  document.querySelectorAll('[data-bind="question-count"]').forEach(function (n) { n.textContent = NQ; });

  ScenttyMachine.connect().then(function (m) {
    state.machine = m;
    if (m.mock) $("mode-tag").hidden = false;
    window.scentty = { state: state, machine: m }; // handy in the browser console
  });

  if (new URLSearchParams(location.search).has("quick")) {
    state.answers = QUIZ.questions.map(function (q) { return Math.floor(Math.random() * q.answers.length); });
    state.i = NQ;
    show("question");
    renderQuestion("forward");
  }
})();
