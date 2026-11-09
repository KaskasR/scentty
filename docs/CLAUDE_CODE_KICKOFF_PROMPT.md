Paste everything below the line into Claude Code, opened in the `scentty` folder.

---

We're building the software for ScenTTy, a personalized perfume machine for my Theta Tau build project. It's due Friday Nov 13 (feature freeze Tuesday Nov 10). Read `CLAUDE.md` and `SPEC.md` first; they contain every decision made so far (liquids, blend math, quiz design, captive-portal architecture, pin map, hardware status). Don't contradict them; if you think something should change, tell me why and wait for my OK.

Hardware status: the pump bench test in `firmware/src/main.cpp` works (pump 1 verified; I'm wiring pumps 2–4). The LED strip and screen are not wired yet.

Your job right now is Phases 1 and 2 from SPEC §9, in this order:

**Phase 1: quiz content + scoring**
- Draft the full quiz: 8–10 questions plus the separate strength question, every answer's point split across orange / lavender / cedar, and the copy for all 7 result profiles (name, 2–3 sentence description, one line on why their blend fits them). Include a hobbies/interests question. The writing should be playful and clever, but not tacky: no emoji spam. Keep questions quick to read on a phone.
- Put all content in its own file that my teammates can edit without touching logic.
- Implement scoring and blend math exactly as SPEC §2 describes (10% floor per scent, 24 mL blend, 3/6/10% strength, round to 0.1 mL).
- Write a small simulation (random answers, plus a few hand-made "extreme" personalities) that prints the archetype distribution and example recipes, and use it to choose the archetype thresholds so all 7 profiles are reachable and reasonably balanced. Show me the results.

**Phase 2: the web app, in mock mode**
- Build the full phone flow from SPEC §4 in `firmware/data/`: welcome, questions, reveal (with the triangle chart), get ready, making it (live progress), done, plus the hidden operator page.
- Plain HTML/CSS/JS, zero external resources (it runs with no internet inside iOS's captive-portal browser). Total size small.
- Design: very playful but genuinely well-designed and modern, with personality from typography, color (scent colors: warm orange, soft purple, amber-brown), motion, and custom CSS/SVG shapes. Not emojis. Put every design decision in CSS variables (colors, type scale, spacing, radii, motion timing) and keep markup clean, because I want to keep polishing the visuals after it works.
- Mock mode: if `/api` isn't reachable (or with `?mock=1`), fake the machine so the whole flow, including progress and "machine busy", works in a laptop browser. Write the API calls against the endpoints in SPEC §5 so the real firmware drops in later.
- Mobile-first (iPhone-sized), and it should still look fine on desktop.

Before writing code: give me a short plan for both phases (file structure, the screen list, how scoring and mock mode will work) and wait for my approval. Work on a git branch (`feature/quiz-webapp`); initialize git first if needed. When Phase 1 is done, show me the quiz and the simulation results before starting Phase 2. When Phase 2 is done, tell me exactly how to open it in mock mode on my laptop.
