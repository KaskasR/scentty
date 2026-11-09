# ScenTTy — notes for Claude Code

Read `SPEC.md` first. It is the decision log for this project: liquids, blend math, quiz design, architecture, pin map, chassis, timeline. Do not contradict it; if something needs to change, propose the change, and once approved update `SPEC.md` in the same commit.

## Repo layout
- `firmware/` — PlatformIO project (ESP32, Arduino framework). `src/main.cpp` is currently the working Stage 1/2 pump bench test. `data/` will hold the web app (uploaded to LittleFS with "Upload Filesystem Image").
- `cad/scentty_chassis.fs` — Onshape FeatureScript for the chassis. `cad/tools/mirror.py` + `render.py` — CadQuery mirror for collision checks and preview renders (needs `pip install cadquery trimesh`).
- `docs/` — renders and reference images.

## How Kasra likes to work
- Plan first and wait for approval before building. Each feature on its own git branch.
- Teach as you go for hardware/firmware: define terms, one step at a time, check it landed. Keep full technical rigor, just as a walkthrough rather than a wall of text.
- Casual, direct tone.
- Verify before saying something works: build the firmware, open the web app (mock mode), run the simulation/tests.

## Commands
- Build firmware: `pio run -d firmware`
- Upload firmware: `pio run -d firmware -t upload`
- Upload web app to the ESP32 flash: `pio run -d firmware -t uploadfs`
- Serial monitor: `pio device monitor -d firmware`
- Web app in mock mode: serve `firmware/data/` locally (e.g. `python3 -m http.server -d firmware/data`) and open it in a browser.

## Hard constraints
- The web app must work with **no internet** (no CDNs, no web fonts, no external requests) and inside iOS's captive-portal mini-browser.
- Never use GPIO 16/17 (PSRAM on the WROVER). Pumps 25/26/27/32, LEDs 13, TFT 18/23/5/21/4.
- Pump control must be non-blocking; all pump pins LOW at boot; cancel must always work.
