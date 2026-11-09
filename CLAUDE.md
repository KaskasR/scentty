# ScenTTy — notes for Claude Code

Read `SPEC.md` first. It is the decision log for this project: liquids, blend math, quiz design, architecture, pin map, chassis, timeline. Do not contradict it; if something needs to change, propose the change, and once approved update `SPEC.md` in the same commit.

## Repo layout
- `firmware/` — PlatformIO project (ESP32, Arduino framework). `src/` is the real firmware (AP + captive portal + API + pump state machine); `bench/bench.cpp` is the pump bench test (env `bench`); `data/` is the web app (uploaded to LittleFS with "Upload Filesystem Image"); `test_host/` runs the pump state machine on a laptop.
- `tools/simulate.js` — quiz simulation / threshold tuning (`node tools/simulate.js [--sweep]`).
- `cad/scentty_chassis.fs` — Onshape FeatureScript for the chassis. `cad/tools/mirror.py` + `render.py` — CadQuery mirror for collision checks and preview renders (needs `pip install cadquery trimesh`).
- `docs/` — renders and reference images.

## How Kasra likes to work
- Plan first and wait for approval before building. Each feature on its own git branch.
- Teach as you go for hardware/firmware: define terms, one step at a time, check it landed. Keep full technical rigor, just as a walkthrough rather than a wall of text.
- Casual, direct tone.
- Verify before saying something works: build the firmware, open the web app (mock mode), run the simulation/tests.

## Commands
- `pio` isn't on PATH on Kasra's Mac (installed by the VS Code extension): use `~/.platformio/penv/bin/pio`.
- Build firmware: `pio run -d firmware`
- Bench test: `pio run -d firmware -e bench -t upload`
- Pump state machine tests (laptop, no ESP32): `firmware/test_host/run.sh`
- Upload firmware: `pio run -d firmware -t upload`
- Upload web app to the ESP32 flash: `pio run -d firmware -t uploadfs`
- Serial monitor: `pio device monitor -d firmware`
- Web app in mock mode: serve `firmware/data/` locally (e.g. `python3 -m http.server -d firmware/data`) and open it in a browser.

## Hard constraints
- The web app must work with **no internet** (no CDNs, no web fonts, no external requests) and inside iOS's captive-portal mini-browser.
- Never use GPIO 16/17 (PSRAM on the WROVER). Pumps 25/26/27/32, LEDs 13, TFT 18/23/5/21/4.
- Pump control must be non-blocking; all pump pins LOW at boot; cancel must always work.
