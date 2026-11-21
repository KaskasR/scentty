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

  // Integer percentages that always add up to 100 (largest remainder).
  function wholePercents(fracs) {
    var raw = SCENTS.map(function (k) { return fracs[k] * 100; });
    var out = raw.map(Math.floor), left = 100 - out.reduce(function (a, b) { return a + b; }, 0);
    raw.map(function (v, i) { return [v - out[i], i]; })
      .sort(function (a, b) { return b[0] - a[0]; })
      .slice(0, left).forEach(function (r) { out[r[1]] += 1; });
    var res = {}; SCENTS.forEach(function (k, i) { res[k] = out[i]; });
    return res;
  }

  // Scent shares with the 10% floor: share = floor + (1 − 3·floor) · p, then
  // rounded to whole percents (SPEC §2) so the pour matches what the result
  // screen shows and a recipe code reproduces it exactly. Rounding down never
  // takes a share below 10%, so the floor still holds.
  function percents(scores) {
    var p = proportions(scores), raw = {};
    var spread = 1 - SCENTS.length * BLEND.floor;
    SCENTS.forEach(function (k) { raw[k] = BLEND.floor + spread * p[k]; });
    return wholePercents(raw);
  }

  function shares(scores) {
    var pct = percents(scores), out = {};
    SCENTS.forEach(function (k) { out[k] = pct[k] / 100; });
    return out;
  }

  // The pump recipe in mL, rounded to 0.1 mL, from whole-percent shares
  // ({orange, lavender, cedar} adding to 100). Works in integer tenths so
  // floating-point error can't leave a bottle at 23.9999 mL.
  function recipeFromPercents(pct, strength) {
    var oil = BLEND.strengths[strength];
    if (oil === undefined) throw new Error("unknown strength: " + strength);
    var totalT = Math.round(BLEND.blendMl * 10);
    var scentT = Math.round((BLEND.blendMl * oil / BLEND.oilInBase) * 10);
    var t = {}, sh = {};
    SCENTS.forEach(function (k) { t[k] = Math.round(scentT * pct[k] / 100); sh[k] = pct[k] / 100; });
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
      shares: sh, // fraction of the scent portion (whole percents)
    };
  }

  function recipe(scores, strength) {
    return recipeFromPercents(percents(scores), strength);
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

  // A preset from quiz.presets as a result (same shape the result screen uses),
  // or null if there's no preset with that id.
  function presetResult(quiz, id) {
    var p = (quiz.presets || []).filter(function (x) { return x.id === id; })[0];
    if (!p) return null;
    return {
      profileId: p.id,
      presetId: p.id,
      profile: { name: p.name, tagline: p.tagline, description: p.description, why: p.why },
      recipe: recipeFromPercents(p.mix, p.strength),
    };
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

  // ---- recipe codes ------------------------------------------------------------
  // A result as a short code the guest screenshots, e.g. "GOLD-4231-S8":
  //   GOLD  profile word        42  orange %       31  lavender %   (cedar = the rest)
  //   S     strength (W/S/B)    7   Damm check digit over all of it
  // The code stores the recipe itself, not quiz answers, so editing the quiz or
  // its thresholds never changes what an old code pours. Never renumber these.
  var CODE_WORDS = {
    sunchaser: "SUN", dreamer: "DREAM", wanderer: "WANDER",
    goldenHour: "GOLD", gardenParty: "GARDEN", oldSoul: "SOUL", harmony: "HARMONY",
  };
  var CODE_PROFILES = ["sunchaser", "dreamer", "wanderer", "goldenHour", "gardenParty", "oldSoul", "harmony"];
  var CODE_STRENGTHS = { whisper: "W", signature: "S", bold: "B" };
  var STRENGTH_ORDER = ["whisper", "signature", "bold"];

  // Damm algorithm: one check digit that catches every single-digit typo and
  // every swap of two neighbouring digits.
  var DAMM = [
    [0, 3, 1, 7, 5, 9, 8, 6, 4, 2], [7, 0, 9, 2, 1, 5, 4, 8, 6, 3],
    [4, 2, 0, 6, 8, 7, 1, 3, 5, 9], [1, 7, 5, 0, 9, 8, 3, 4, 2, 6],
    [6, 1, 2, 3, 0, 4, 5, 9, 7, 8], [3, 6, 7, 4, 2, 0, 9, 5, 8, 1],
    [5, 8, 6, 9, 7, 2, 0, 1, 3, 4], [8, 9, 4, 5, 3, 6, 2, 0, 1, 7],
    [9, 4, 3, 8, 6, 1, 7, 2, 0, 5], [2, 5, 8, 1, 4, 3, 6, 7, 9, 0],
  ];
  function damm(digits) {
    return digits.reduce(function (c, d) { return DAMM[c][d]; }, 0);
  }
  // Everything the check digit covers: profile, strength, both percents. The
  // percents go last so the digits sit in the same order as on screen, with the
  // check digit right after them: that's what lets Damm catch a swap of the last
  // percent digit and the check digit.
  function codeDigits(profileId, o, l, strength) {
    return [CODE_PROFILES.indexOf(profileId), STRENGTH_ORDER.indexOf(strength), (o / 10) | 0, o % 10, (l / 10) | 0, l % 10];
  }

  // Presets have no code (they're always on the welcome screen): returns null.
  function encodeCode(res) {
    if (!CODE_WORDS[res.profileId]) return null;
    var r = res.recipe;
    var o = Math.round(r.shares.orange * 100), l = Math.round(r.shares.lavender * 100);
    var check = damm(codeDigits(res.profileId, o, l, r.strength));
    var pad = function (n) { return (n < 10 ? "0" : "") + n; };
    return CODE_WORDS[res.profileId] + "-" + pad(o) + pad(l) + "-" + CODE_STRENGTHS[r.strength] + check;
  }

  // Forgiving about case, spaces and dashes. Returns the same shape as result()
  // (minus scores), or null if the code isn't valid.
  function decodeCode(quiz, str) {
    var m = /^([A-Z]+)(\d\d)(\d\d)([WSB])(\d)$/.exec(String(str).toUpperCase().replace(/[\s\-_.·]/g, ""));
    if (!m) return null;
    var profileId = null;
    for (var id in CODE_WORDS) if (CODE_WORDS[id] === m[1]) profileId = id;
    var strength = null;
    for (var st in CODE_STRENGTHS) if (CODE_STRENGTHS[st] === m[4]) strength = st;
    var o = +m[2], l = +m[3], c = 100 - o - l;
    if (!profileId || !strength || !quiz.profiles[profileId]) return null;
    if (o < BLEND.floor * 100 || l < BLEND.floor * 100 || c < BLEND.floor * 100) return null;
    if (damm(codeDigits(profileId, o, l, strength)) !== +m[5]) return null;
    return {
      profileId: profileId,
      profile: quiz.profiles[profileId],
      recipe: recipeFromPercents({ orange: o, lavender: l, cedar: c }, strength),
    };
  }

  return {
    SCENTS: SCENTS, BLEND: BLEND,
    tally: tally, proportions: proportions, shares: shares, wholePercents: wholePercents,
    recipe: recipe, recipeFromPercents: recipeFromPercents, archetype: archetype, result: result,
    presetResult: presetResult, encodeCode: encodeCode, decodeCode: decodeCode,
  };
})();

if (typeof module !== "undefined") module.exports = SCENTTY_SCORING;
