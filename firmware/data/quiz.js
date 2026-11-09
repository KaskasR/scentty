// ScenTTy quiz content. EDIT ME, no logic lives here.
//
// How points work
//   Each answer adds points to orange / lavender / cedar. Keep every answer at
//   3 points total (e.g. {orange: 3} or {orange: 2, cedar: 1}) so no single
//   question outweighs the others. Re-run `node tools/simulate.js` after edits
//   to check that all 7 profiles still come up reasonably often.
//
//   orange   = the spark  (outgoing, spontaneous, optimistic, social)
//   lavender = the calm   (thoughtful, empathetic, creative, recharges alone)
//   cedar    = the anchor (steady, confident, outdoorsy, dependable)
//
// Keep questions short: they're read on a phone, one per screen.

var SCENTTY_QUIZ = {
  questions: [
    {
      q: "Saturday, zero plans. You…",
      answers: [
        { text: "Text the group chat. Something's happening tonight.", points: { orange: 3 } },
        { text: "Sleep in, then a long café sit with a sketchbook.", points: { lavender: 3 } },
        { text: "Drive until you find a trailhead.", points: { cedar: 3 } },
        { text: "Farmers market, then see where the day goes.", points: { orange: 1, lavender: 1, cedar: 1 } },
      ],
    },
    {
      // the hobbies / interests question
      q: "Which hobby would eat your whole weekend?",
      answers: [
        { text: "Concerts, road trips, the new restaurant everyone's talking about", points: { orange: 3 } },
        { text: "Making things: music, art, writing", points: { lavender: 3 } },
        { text: "Hiking, climbing, anything with a campfire at the end", points: { cedar: 3 } },
        { text: "Taking something apart to see how it works", points: { cedar: 2, lavender: 1 } },
      ],
    },
    {
      q: "Your friends call you the one who…",
      answers: [
        { text: "has the most chaotic stories", points: { orange: 3 } },
        { text: "actually listens", points: { lavender: 3 } },
        { text: "stays calm when everything's on fire", points: { cedar: 3 } },
        { text: "turns \"we should\" into an actual plan", points: { orange: 1, cedar: 2 } },
      ],
    },
    {
      q: "Pick a place to be right now.",
      answers: [
        { text: "A beach at noon", points: { orange: 3 } },
        { text: "A rooftop at sunset", points: { orange: 1, cedar: 2 } },
        { text: "A garden right after the rain", points: { lavender: 2, orange: 1 } },
        { text: "A cabin in the woods, fire going", points: { lavender: 2, cedar: 1 } },
      ],
    },
    {
      q: "Group project. You're the one who…",
      answers: [
        { text: "pitches the big idea in the first five minutes", points: { orange: 3 } },
        { text: "makes it look good", points: { lavender: 3 } },
        { text: "builds the part that actually works", points: { cedar: 3 } },
        { text: "keeps everyone on track and on speaking terms", points: { lavender: 1, cedar: 2 } },
      ],
    },
    {
      q: "Surprise free hour. You…",
      answers: [
        { text: "find whoever's around", points: { orange: 3 } },
        { text: "headphones in, walk alone", points: { lavender: 3 } },
        { text: "squeeze in a run", points: { cedar: 2, orange: 1 } },
        { text: "nap. Obviously.", points: { lavender: 2, cedar: 1 } },
      ],
    },
    {
      q: "Pick a soundtrack.",
      answers: [
        { text: "Anything you can dance to", points: { orange: 3 } },
        { text: "Soft, with lyrics that hit a little too hard", points: { lavender: 3 } },
        { text: "Classic rock with the windows down", points: { cedar: 3 } },
        { text: "Shuffle. You like a bit of everything.", points: { orange: 1, lavender: 1, cedar: 1 } },
      ],
    },
    {
      q: "Big decision. How do you make it?",
      answers: [
        { text: "Gut call. Figure out the details later.", points: { orange: 3 } },
        { text: "Sleep on it and see how it feels", points: { lavender: 3 } },
        { text: "Pros, cons, maybe a spreadsheet", points: { cedar: 3 } },
        { text: "Ask three friends, then do what you wanted anyway", points: { orange: 1, lavender: 1, cedar: 1 } },
      ],
    },
    {
      q: "Your usual order?",
      answers: [
        { text: "Iced, fruity, a little extra", points: { orange: 3 } },
        { text: "Matcha or chamomile", points: { lavender: 3 } },
        { text: "Black coffee", points: { cedar: 3 } },
        { text: "Whatever the barista recommends", points: { orange: 2, lavender: 1 } },
      ],
    },
  ],

  // Asked last. `strength` must be whisper / signature / bold (amounts are in scoring.js).
  strengthQuestion: {
    q: "You walk into a room. How much should people notice your scent?",
    answers: [
      { text: "Only if they lean in", label: "Whisper", strength: "whisper" },
      { text: "Noticed, not announced", label: "Signature", strength: "signature" },
      { text: "They'll know I was here", label: "Bold", strength: "bold" },
    ],
  },

  // Archetype thresholds, tuned with tools/simulate.js. Applied to the raw quiz
  // proportions (each scent's points / all points):
  //   max − min < harmonySpread        → Harmony
  //   else top share ≥ singleShare     → the single profile
  //   else                             → the pair profile for the top two
  thresholds: { harmonySpread: 0.16, singleShare: 0.54 },

  // The 7 result profiles. `why` is one line on why the blend fits.
  profiles: {
    sunchaser: {
      name: "The Sunchaser",
      tagline: "All spark, no off switch.",
      description:
        "You say yes first and work out the details on the way there. Rooms get a little louder when you walk in, in the good way, and most of your plans end up as stories.",
      why: "Mostly sweet orange, bright and impossible to ignore, with just enough lavender and cedar to linger after you've left for the next thing.",
    },
    dreamer: {
      name: "The Dreamer",
      tagline: "Quiet on the outside. A whole world inside.",
      description:
        "You notice what everyone else walks past: the light, the lyric, the friend who's a little quieter than usual. You recharge on your own and come back with ideas nobody saw coming.",
      why: "Lavender up front, soft and floral, with a flicker of orange on top and cedar underneath to keep your feet on the ground.",
    },
    wanderer: {
      name: "The Wanderer",
      tagline: "Steady hands, long trails.",
      description:
        "You're the calm in the group chat and the first one at the trailhead. People lean on you without thinking twice, and you'd rather be outside than talk about it.",
      why: "Built on cedarwood, the base note that lasts longest. Like you, it's still there at the end of the day.",
    },
    goldenHour: {
      name: "Golden Hour",
      tagline: "Warm, bright, and dependable.",
      description:
        "You can start the party and still be the one who gets everyone home. Big energy, but with a spine: people follow you because you actually come through.",
      why: "Orange for the spark, cedar for the staying power. Sunlight on old wood.",
    },
    gardenParty: {
      name: "Garden Party",
      tagline: "Social, but soft-hearted.",
      description:
        "You bring people together and quietly make sure everyone's having a good time. Fun with you never comes at anyone's expense.",
      why: "Citrus and lavender together: fresh, friendly, a little floral, like a long afternoon outside.",
    },
    oldSoul: {
      name: "Old Soul",
      tagline: "Calm, grounded, a little mysterious.",
      description:
        "You'd take one long conversation over a loud room any day. You remember what people tell you, and you're more sure of yourself than you let on.",
      why: "Lavender and cedar: calm and grounded, like a cabin with a good book and nowhere to be.",
    },
    harmony: {
      name: "Harmony",
      tagline: "A little of everything, on purpose.",
      description:
        "The party, the trail, the journal: you fit wherever you land, and you don't apologize for it. People can't pin you down, which is kind of the point.",
      why: "An even split of all three: bright top, calm heart, warm base, none of them shouting over the others.",
    },
  },
};

if (typeof module !== "undefined") module.exports = SCENTTY_QUIZ;
