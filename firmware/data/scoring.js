// ScenTTy scoring + blend math (SPEC §2, §3). Pure functions, no DOM, so the
// same file runs in the browser and in tools/simulate.js.

var SCENTTY_SCORING = (function () {
  var SCENTS = ["orange", "lavender", "cedar"];

  var BLEND = {
    blendMl: 24,          // total liquid in the bottle
    oilInBase: 0.10,      // each reservoir is 10% essential oil
    floor: 0.10,          // every scent gets at least 10% of the scent portion
    strengths: { whisper: 0.03, signature: 0.06, bold: 0.10 }, // final oil %
  };

  // answers: array of answer indexes, one per personality question.
  function tally(quiz, answers) {
    var s = { orange: 0, lavender: 0, cedar: 0 };
    quiz.questions.forEach(function (question, i) {
      var a = question.answers[answers[i]];
      if (!a) return;
      SCENTS.forEach(function (k) { s[k] += a.points[k] || 0; });
    });
    return s;
  }

  // Raw proportions of the quiz score; an all-zero score counts as an even split.
  function proportions(scores) {
    var total = scores.orange + scores.lavender + scores.cedar;
    var p = {};
    SCENTS.forEach(function (k) { p[k] = total > 0 ? scores[k] / total : 1 / 3; });
    return p;
  }

  // Scent shares with the 10% floor: share = floor + (1 − 3·floor) · p.
  function shares(scores) {
    var p = proportions(scores), out = {};
    var spread = 1 - SCENTS.length * BLEND.floor;
    SCENTS.forEach(function (k) { out[k] = BLEND.floor + spread * p[k]; });
    return out;
  }

  // The pump recipe in mL, rounded to 0.1 mL. Works in integer tenths so
  // floating-point error can't leave a bottle at 23.9999 mL.
  function recipe(scores, strength) {
    var pct = BLEND.strengths[strength];
    if (pct === undefined) throw new Error("unknown strength: " + strength);
    var sh = shares(scores);
    var totalT = Math.round(BLEND.blendMl * 10);
    var scentT = Math.round((BLEND.blendMl * pct / BLEND.oilInBase) * 10);
    var t = {};
    SCENTS.forEach(function (k) { t[k] = Math.round(scentT * sh[k]); });
    var sumT = t.orange + t.lavender + t.cedar;
    var alcoholT = totalT - sumT;
    if (scentT >= totalT) {
      // Bold: no alcohol, so the rounding remainder goes to the largest scent.
      var big = SCENTS.reduce(function (a, b) { return t[b] > t[a] ? b : a; });
      t[big] += alcoholT;
      alcoholT = 0;
    }
    return {
      orange_ml: t.orange / 10,
      lavender_ml: t.lavender / 10,
      cedar_ml: t.cedar / 10,
      alcohol_ml: alcoholT / 10,
      strength: strength,
      shares: sh, // fraction of the scent portion, before rounding
    };
  }

  var SINGLE = { orange: "sunchaser", lavender: "dreamer", cedar: "wanderer" };
  var PAIR = {
    "cedar+orange": "goldenHour",
    "lavender+orange": "gardenParty",
    "cedar+lavender": "oldSoul",
  };

  function archetype(scores, thresholds) {
    var p = proportions(scores);
    var ranked = SCENTS.slice().sort(function (a, b) { return p[b] - p[a]; });
    var top = p[ranked[0]], bottom = p[ranked[2]];
    if (top - bottom < thresholds.harmonySpread) return "harmony";
    if (top >= thresholds.singleShare) return SINGLE[ranked[0]];
    return PAIR[[ranked[0], ranked[1]].sort().join("+")];
  }

  // Everything the result screen needs, in one call.
  function result(quiz, answers, strength) {
    var scores = tally(quiz, answers);
    var id = archetype(scores, quiz.thresholds);
    return {
      scores: scores,
      profileId: id,
      profile: quiz.profiles[id],
      recipe: recipe(scores, strength),
    };
  }

  return {
    SCENTS: SCENTS, BLEND: BLEND,
    tally: tally, proportions: proportions, shares: shares,
    recipe: recipe, archetype: archetype, result: result,
  };
})();

if (typeof module !== "undefined") module.exports = SCENTTY_SCORING;
