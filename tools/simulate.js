#!/usr/bin/env node
// ScenTTy quiz simulation. Run: node tools/simulate.js [--sweep]
//
// Uses the real quiz content and scoring code from firmware/data/, so whatever
// it prints is exactly what the phone app will do.
//
// Two simulated crowds:
//   random   — every answer picked uniformly at random (worst case: noisy people)
//   persona  — each person has a hidden "true" mix of spark/calm/anchor (uniform
//              over the triangle) and leans toward answers that match it. Closer
//              to real quiz-takers, so thresholds are tuned mainly on this.

const path = require("path");
const QUIZ = require(path.join(__dirname, "../firmware/data/quiz.js"));
const S = require(path.join(__dirname, "../firmware/data/scoring.js"));

const N = 20000;
const PROFILE_IDS = Object.keys(QUIZ.profiles);
const STRENGTHS = Object.keys(S.BLEND.strengths);

// Small seeded RNG so runs are repeatable.
let seed = 12345;
const rand = () => ((seed = (seed * 1664525 + 1013904223) >>> 0) / 4294967296);

function randomAnswers() {
  return QUIZ.questions.map((q) => Math.floor(rand() * q.answers.length));
}

// Dirichlet(1,1,1) = uniform point on the triangle.
function randomMix() {
  const e = [0, 0, 0].map(() => -Math.log(rand()));
  const t = e[0] + e[1] + e[2];
  return { orange: e[0] / t, lavender: e[1] / t, cedar: e[2] / t };
}

// How well an answer matches a person: cosine similarity between their mix and
// the answer's point vector (direction, not size), so a 50/50 spark+anchor
// person prefers {orange:1, cedar:2} over a pure {orange:3}.
const norm = (v) => Math.sqrt(S.SCENTS.reduce((s, k) => s + (v[k] || 0) ** 2, 0)) || 1;
const fit = (mix, a) => S.SCENTS.reduce((s, k) => s + mix[k] * (a.points[k] || 0), 0) / (norm(mix) * norm(a.points));

// Softmax choice: higher beta = more consistent person.
function personaAnswers(mix, beta) {
  return QUIZ.questions.map((q) => {
    const w = q.answers.map((a) => Math.exp(beta * fit(mix, a)));
    let r = rand() * w.reduce((x, y) => x + y, 0);
    for (let i = 0; i < w.length; i++) if ((r -= w[i]) <= 0) return i;
    return w.length - 1;
  });
}

// Deterministic "always pick the best match" for the hand-made extremes.
function bestAnswers(mix) {
  // Ties rotate question by question, so a 50/50 person doesn't always pick orange.
  let ties = 0;
  return QUIZ.questions.map((q) => {
    const f = q.answers.map((a) => fit(mix, a));
    const top = Math.max(...f);
    const tied = f.map((v, i) => i).filter((i) => top - f[i] < 1e-9);
    return tied[tied.length > 1 ? ties++ % tied.length : 0];
  });
}

function distribution(scoreList, th) {
  const d = Object.fromEntries(PROFILE_IDS.map((id) => [id, 0]));
  for (const s of scoreList) d[S.archetype(s, th)]++;
  for (const id in d) d[id] /= scoreList.length;
  return d;
}

const pct = (x) => (100 * x).toFixed(1).padStart(5) + "%";
const bar = (x) => "█".repeat(Math.round(x * 60));

// ---- build crowds -------------------------------------------------------
const crowds = {
  random: Array.from({ length: N }, () => S.tally(QUIZ, randomAnswers())),
  "persona β=2 (loose)": Array.from({ length: N }, () => S.tally(QUIZ, personaAnswers(randomMix(), 2))),
  "persona β=3": Array.from({ length: N }, () => S.tally(QUIZ, personaAnswers(randomMix(), 3))),
  "persona β=4 (consistent)": Array.from({ length: N }, () => S.tally(QUIZ, personaAnswers(randomMix(), 4))),
};

// ---- invariant checks on every simulated person, every strength ---------
let checked = 0, problems = [];
for (const s of Object.values(crowds).flat()) {
  for (const st of STRENGTHS) {
    const r = S.recipe(s, st);
    const total = r.orange_ml + r.lavender_ml + r.cedar_ml + r.alcohol_ml;
    const scentTarget = S.BLEND.blendMl * S.BLEND.strengths[st] / S.BLEND.oilInBase;
    const bad =
      Math.abs(total - S.BLEND.blendMl) > 1e-9 ||
      [r.orange_ml, r.lavender_ml, r.cedar_ml, r.alcohol_ml].some((v) => v < 0 || Math.abs(v * 10 - Math.round(v * 10)) > 1e-9) ||
      S.SCENTS.some((k) => r.shares[k] < S.BLEND.floor - 1e-12) ||
      Math.abs(r.orange_ml + r.lavender_ml + r.cedar_ml - scentTarget) > 0.15 + 1e-9;
    if (bad) problems.push({ s, st, r });
    checked++;
  }
}

// ---- optional threshold sweep -----------------------------------------
function score(th) {
  // Average, over all crowds, of the worst profile's distance from an even 1/7.
  // Robust to how consistent real people turn out to be.
  const lists = Object.values(crowds);
  return lists.reduce((sum, list) => {
    const d = distribution(list, th);
    return sum + Math.max(...PROFILE_IDS.map((id) => Math.abs(d[id] - 1 / 7)));
  }, 0) / lists.length;
}
if (process.argv.includes("--sweep")) {
  const rows = [];
  for (let h = 0.06; h <= 0.30001; h += 0.01)
    for (let sg = 0.38; sg <= 0.65001; sg += 0.01) {
      const th = { harmonySpread: +h.toFixed(2), singleShare: +sg.toFixed(2) };
      rows.push({ th, err: score(th) });
    }
  rows.sort((a, b) => a.err - b.err);
  console.log("\nTHRESHOLD SWEEP: best 10 (avg over crowds of worst-profile error vs an even 14.3%)");
  for (const { th, err } of rows.slice(0, 10))
    console.log(`  harmonySpread ${th.harmonySpread.toFixed(2)}  singleShare ${th.singleShare.toFixed(2)}  → max error ${pct(err)}`);
}

// ---- report ----------------------------------------------------------------
const th = QUIZ.thresholds;
console.log(`\nTHRESHOLDS in quiz.js: harmonySpread ${th.harmonySpread}, singleShare ${th.singleShare}`);
for (const [name, list] of Object.entries(crowds)) {
  const d = distribution(list, th);
  console.log(`\nDISTRIBUTION: ${name} crowd (${N} people)`);
  for (const id of PROFILE_IDS)
    console.log(`  ${QUIZ.profiles[id].name.padEnd(14)} ${pct(d[id])}  ${bar(d[id])}`);
}

const extremes = {
  "pure spark":        { orange: 1, lavender: 0, cedar: 0 },
  "pure calm":         { orange: 0, lavender: 1, cedar: 0 },
  "pure anchor":       { orange: 0, lavender: 0, cedar: 1 },
  "spark + anchor":    { orange: 0.5, lavender: 0, cedar: 0.5 },
  "spark + calm":      { orange: 0.5, lavender: 0.5, cedar: 0 },
  "calm + anchor":     { orange: 0, lavender: 0.5, cedar: 0.5 },
};
const handMade = Object.entries(extremes).map(([n, mix]) => [n, bestAnswers(mix)]);
handMade.push(["fence-sitter (always last option)", QUIZ.questions.map((q) => q.answers.length - 1)]);
handMade.push(["chaos (o, l, c, o, l, c…)", QUIZ.questions.map((q, i) => i % 3)]);

console.log("\nHAND-MADE PERSONALITIES (Signature strength)");
console.log("  who                                answers              score o/l/c   profile          orange lav  cedar alc  (mL)");
for (const [who, ans] of handMade) {
  const res = S.result(QUIZ, ans, "signature");
  const r = res.recipe, sc = res.scores;
  console.log(
    `  ${who.padEnd(34)} ${ans.join("").padEnd(20)} ${`${sc.orange}/${sc.lavender}/${sc.cedar}`.padEnd(13)} ${res.profile.name.padEnd(16)} ` +
    [r.orange_ml, r.lavender_ml, r.cedar_ml, r.alcohol_ml].map((v) => v.toFixed(1).padStart(5)).join("")
  );
}

console.log("\nEXAMPLE RECIPES: one person at every strength");
for (const [who, ans] of [handMade[0], handMade[5], handMade[6]]) {
  const sc = S.tally(QUIZ, ans);
  for (const st of STRENGTHS) {
    const r = S.recipe(sc, st);
    const sh = S.SCENTS.map((k) => (100 * r.shares[k]).toFixed(0) + "%").join("/");
    console.log(`  ${who.slice(0, 22).padEnd(22)} ${st.padEnd(10)} shares ${sh.padEnd(12)} → orange ${r.orange_ml.toFixed(1)}  lav ${r.lavender_ml.toFixed(1)}  cedar ${r.cedar_ml.toFixed(1)}  alcohol ${r.alcohol_ml.toFixed(1)}  = ${(r.orange_ml + r.lavender_ml + r.cedar_ml + r.alcohol_ml).toFixed(1)} mL`);
  }
}

const rs = crowds.random.map((s) => S.proportions(s));
const mean = (k) => rs.reduce((a, p) => a + p[k], 0) / rs.length;
console.log(`\nQUIZ BIAS CHECK: avg raw share for a random answerer → orange ${pct(mean("orange"))}, lavender ${pct(mean("lavender"))}, cedar ${pct(mean("cedar"))} (ideal 33.3% each)`);

console.log(`\nINVARIANTS: ${checked} recipes checked (24.0 mL total, 0.1 mL steps, ≥10% floor, no negatives) → ${problems.length ? "FAIL" : "all OK"}`);
if (problems.length) { console.log(problems.slice(0, 5)); process.exit(1); }
