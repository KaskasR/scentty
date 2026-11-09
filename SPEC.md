# ScenTTy — Project Spec & Decision Log

The single source of truth for the ScenTTy build. If a decision changes, update this file in the same commit.

**What it is:** a personalized perfume machine built by the Theta Tau Fall '26 pledge class. A user scans a QR code, takes a personality quiz on their phone, and the machine pumps a custom blend of three scents plus alcohol into a bottle they take home.

**Deadline:** Build Project Presentation, **Friday Nov 13, 2026**. Feature freeze **Tuesday Nov 10**.

---

## 1. Liquids and pumps

| Pump | GPIO | Reservoir | Character | LED color |
|---|---|---|---|---|
| 1 | 25 | **Sweet orange**, 10% essential oil in perfumer's alcohol | bright, fresh citrus | warm orange |
| 2 | 26 | **Lavender**, 10% in perfumer's alcohol | floral, herbal, calm | soft purple |
| 3 | 27 | **Atlas cedarwood**, 10% in perfumer's alcohol | warm, woody, earthy | amber-brown |
| 4 | 32 | **Pure perfumer's alcohol** (diluent, sets strength) | — | soft white |

Perfume structure (used in quiz copy): orange = **top note** (smelled first, fades fast), lavender = **heart note**, cedarwood = **base note** (lasts longest, anchors the others).

## 2. Blend math (locked)

- **Blend volume:** `BLEND_ML = 24` (1 oz ≈ 30 mL bottle, filled ~80%). Configurable.
- **Strength levels** (no cap, decided):

  | Strength | Final oil % | Scent-base mL (of 24) | Alcohol mL |
  |---|---|---|---|
  | Whisper | 3% | 7.2 | 16.8 |
  | Signature | 6% | 14.4 | 9.6 |
  | Bold | 10% | 24.0 | 0 |

  Formula: `scentTotal = BLEND_ML * strength / 0.10`, `alcohol = BLEND_ML - scentTotal`.
- **Scent shares:** every blend contains **at least 10% of each scent** (of the scent portion) so it has top, heart, and base notes. The remaining 70% is split by quiz score:
  `share_i = 0.10 + 0.70 * (score_i / (score_orange + score_lavender + score_cedar))`
  `mL_i = scentTotal * share_i`
- Smallest possible dispense: Whisper × 10% floor = **0.72 mL**. Pumps must be calibrated so this is accurate.
- Round each pump volume to 0.1 mL; any rounding remainder goes to alcohol (or the largest scent at Bold).

## 3. Quiz design (to be drafted in full — see §9 Phase 1)

- **Tone:** very playful, genuinely well-designed, **not tacky**. No random emojis. Personality comes from writing, motion, color, and custom illustration/CSS.
- **Length:** 25 questions + strength, everyone answers all of them (~5 min). Changed Nov 9 from 8–10 at Kasra's call: more questions = more accurate placement. One question per screen, progress indicator.
- **Scoring:** each answer adds points to `orange`, `lavender`, and/or `cedar` (may split points). One separate question sets **strength** (Whisper / Signature / Bold), e.g. "When you walk into a room, how much do you want people to notice your scent?"
- **Scent personalities:**
  - Orange — *the spark*: outgoing, spontaneous, optimistic, social, loves new things.
  - Lavender — *the calm*: thoughtful, empathetic, creative, recharges alone.
  - Cedarwood — *the anchor*: steady, confident, outdoorsy, dependable.
- Include a hobbies/interests question (from the original proposal).
- **7 result profiles** by position on the scent triangle:
  - single dominant: **The Sunchaser** (orange), **The Dreamer** (lavender), **The Wanderer** (cedar)
  - pairs: **Golden Hour** (orange + cedar), **Garden Party** (orange + lavender), **Old Soul** (lavender + cedar)
  - balanced: **Harmony**
  **Thresholds (decided Nov 9, via `node tools/simulate.js --sweep`):** on the raw quiz proportions p = score_i / total: if max − min < **0.20** → Harmony; else if top ≥ **0.56** → single; else → pair of the top two. Tuned on three simulated "persona" crowds (hidden personality mix, varying answer consistency): for the loosest crowd all 7 land at 13.5–15%; for the most consistent, singles reach ~25% and Harmony drops to ~4.5%. Random answer-tappers average out to the center (~65% Harmony), which is intended. Values live in `firmware/data/quiz.js`.
- **Content:** 25 personality questions (4 answers each, every answer worth 3 points, quiz balanced 100/100/100) + the strength question last. All copy lives in `firmware/data/quiz.js`; logic in `firmware/data/scoring.js` (shared by the app and the simulator).
- **Result screen:** profile name + short description, a **ternary (triangle) chart** with a dot at the user's position, the three percentages, the strength, and the mL recipe.

## 4. Phone experience (screen flow)

1. Welcome ("find your scent")
2. Questions (one per screen)
3. Reveal (short build-up animation → profile, triangle chart, recipe)
4. Get ready: "Grab a bottle from the side rack, set it on the pad, put the funnel on" → confirm
5. Making it: live progress, one bar per pump, synced with the machine
6. Done: "Remove the funnel, cap it, shake gently. Best after a few minutes."

Busy handling: one blend at a time. If the machine is busy, the phone shows "you're next" and starts when free (simple FIFO or retry — keep it simple).

**Operator page** (hidden, e.g. `/admin`): prime each tube (hold-to-run), flush with alcohol, calibrate each pump (run N seconds → type measured mL → store mL/s), run a test blend, emergency stop, LED test.

## 5. Software architecture (locked)

- **No internet, no Firebase.** The ESP32 runs a **WiFi access point** `ScenTTy` (open) and a **captive portal**: a DNS server answers every domain with the ESP32's IP, so phones auto-open the quiz (respond to iOS/Android connectivity-check URLs appropriately).
- **QR code** on the machine: WiFi-join format `WIFI:T:nopass;S:ScenTTy;;` → phone joins → quiz pops up.
- **Web app:** plain HTML/CSS/JS, **zero external resources** (no CDNs, no web fonts, no analytics) since there is no internet. Small (< ~150 KB total). Served from the ESP32's **LittleFS** (`firmware/data/`).
  - Quiz content (questions, answers, points, profile copy) lives in its **own file** so teammates can edit it without touching logic.
  - **Design tokens as CSS variables** (colors, radii, spacing, type scale, motion) so visual polish can be layered on later without restructuring.
  - **Laptop/mock mode:** when the API isn't reachable (or `?mock=1`), the app fakes the machine (fake status/progress) so the team can test the full flow in any browser.
  - Must work inside iOS's captive-portal mini-browser: single page, no reliance on localStorage, no downloads/new tabs.
- **API** (JSON). Contract decided Nov 9; the web app (`firmware/data/api.js`) is written against exactly this, and its mock implements the same shapes.
  - `GET /api/status` → `{state, job, step, pumps, error}`
    - `state`: `idle` | `dispensing` | `done` | `error` | `busy` (`busy` = operator is running pumps by hand)
    - `job`: id of the current/last blend (increments per accepted blend)
    - `step`: index of the pump running now (0 orange, 1 lavender, 2 cedar, 3 alcohol), `-1` if none
    - `pumps`: `[{id: "orange", target_ml, done_ml}, …lavender, cedar, alcohol]`, always 4 in pump order
    - `error`: message or `null`
  - `POST /api/blend` `{orange_ml, lavender_ml, cedar_ml, alcohol_ml, profile, strength}` → `200 {accepted: true, job}`, or `409 {accepted: false, busy: true, status: <same as /api/status>}` when not free. "Free" = `idle`, `done` or `error`. Pumps with 0 mL are skipped. `done` stays until the next blend.
  - `POST /api/cancel` → all pumps off, `state: idle`. Always works (also the operator E-stop).
  - Busy handling is **retry**, not a queue: a phone that gets 409 shows "you're next" (with the current blend's progress) and retries every 2 s.
  - Admin (operator page `admin.html`; firmware should also serve it at `/admin`):
    - `POST /api/admin/run {pump: 0-3, ms}` → run one pump for `ms` (cap 60000), then stop by itself. Rejected (409) while dispensing. Hold-to-run sends `ms: 400` every 250 ms, so a dropped phone stops the pump within 0.4 s.
    - `POST /api/admin/stop` → stop a manual run
    - `GET /api/admin/calibration` → `{ml_per_s: [o, l, c, a]}`; `POST /api/admin/calibration {pump, ml_per_s}` → saved to NVS
    - `POST /api/admin/led {pattern: "scents" | "rainbow" | "white" | "off"}`
- **Firmware** (PlatformIO, Arduino framework, `board = esp32dev`):
  - ESPAsyncWebServer + AsyncTCP (the maintained mathieucarbou forks, now published as `esp32async/*`), DNSServer, ArduinoJson 7, LittleFS, Preferences (NVS) for calibration, Adafruit NeoPixel, Adafruit_ILI9341 + Adafruit GFX (both verified on this hardware, see `HARDWARE.md`).
  - Partition table `huge_app.csv` (3 MB app, 896 KB LittleFS, no OTA): the default layout left the firmware at 93% of its slot.
  - Code layout: `src/config.h` (all pins/limits), `pumps.cpp` (state machine), `calibration.cpp` (NVS), `web.cpp` (AP, captive portal, API). Bench test kept as env `bench` (`firmware/bench/`). Host-side tests for the state machine: `firmware/test_host/run.sh`.
  - **Non-blocking pump state machine** (millis-based; no `delay()` in the dispense path) so the web server stays responsive.
  - Dispense time per pump = `mL / calibrated_mL_per_s`. Run pumps **one at a time**.
  - Safety: all pump pins driven LOW at boot; max-runtime watchdog per pump; cancel always works.
    - One function (`setPump`) writes pump pins and always turns the others off, so two pumps can never run together.
    - A blend is rejected (400) if any volume is outside 0–24 mL, the total isn't 24 ± 0.2 mL, or any pump would run > 45 s.
    - Watchdog: if a pin stays HIGH 2 s past its deadline, all pumps off → `error`.
    - Manual runs capped at 60 s; hold-to-run stops ~0.4 s after the phone stops sending.
    - Cancel writes all pins LOW immediately, before taking the state lock (web handlers run on a separate FreeRTOS task, so shared state is mutex-guarded).
  - Default flow rate until calibrated: **1.17 mL/s** (pump spec sheet: 70 mL/min). Calibrated values stored in NVS, accepted range 0.05–10 mL/s.
  - AP allows up to 10 phones at once (ESP32 maximum).

## 6. Hardware (as built)

**`HARDWARE.md` is the authoritative, as-built wiring reference and wins if it disagrees with this section.** This is a summary.

- **MCU:** Freenove **ESP32-WROVER** dev board (4 MB flash + PSRAM) on the Freenove GPIO extension board, **camera unplugged** (camera occupies GPIO 4,5,18,19,21,22,23,25,26,27). **Never use GPIO 16/17** (PSRAM) or 6–11 (flash); 34–39 are input-only. GPIO 2 (onboard LED) intentionally unused. USB-C, CH340 USB chip. Every output pin must be configured and driven explicitly (an unconfigured GPIO 14 rested HIGH).
- **Pump drivers:** 4× **XY-MOS** dual-MOSFET modules, low-side switching. VIN± from 12 V supply, OUT± to pump, TRIG/PWM from ESP32 (active HIGH, 3.3–20 V), module GND tied to VIN− on-board (gives the shared ground). 1N5822 Schottky flyback diode across each pump (stripe on OUT+). **All four channels verified with water** (bench test, 2 s on / 2 s off each).
- **Pumps:** 12 V peristaltic (Ø29 motor, 54.5 mm flange, screw holes 48.5 mm, tube barbs point up). Tubing 3 mm ID × 5 mm OD (~0.7 mL per 10 cm, so tubes must be primed). Peristaltic = no siphoning when off. Flow not yet calibrated; measure per pump and store in NVS.
- **Power:** 12 V 5 A adapter → pump modules (one pump runs at a time). ESP32 + LED strip from a separate 5 V battery into the board's 5V pin (USB during bench work); TFT from the board's 3.3 V regulator. Grounds are common. No power switch.
- **LEDs (wired, test pending):** **one continuous 23-LED WS2812B strip** (SEZO 60/m, uncut, ~383 mm) on the plinth base under the bottle row. **Data GPIO 13** via 300 Ω, 5 V + GND from the board, 220 µF across 5V/GND. **LED 0 is on the right** (by the electronics bay); data runs right → left. Adafruit NeoPixel, `NEO_GRB + NEO_KHZ800` (color order unconfirmed). **Zone lighting:** only the LEDs under each bottle light up, from one zone table in config; all others stay off. Zones (guessed from CAD, confirm with a chase test once installed): output 1–3, alcohol 7–8, cedar 11–13, lavender 16–17, orange 20–22. **Brightness capped (~60/255)**: full white on all 23 is ~1.4 A, too much for the 5 V source.
- **Screen (verified):** 2.8" ILI9341 SPI TFT, 240×320 (MSP2807-style, 86×50 mm board; touch + SD unused). Pins: SCK 18, MOSI 23, CS 5, **DC 21**, RST 4, VCC 3.3 V, LED (backlight) 3.3 V (always on). Adafruit_ILI9341 + Adafruit GFX on hardware SPI, `setRotation(1)` → landscape 320×240.
- **Pin map summary:** pumps 25/26/27/32 · LED strip 13 · TFT 18/23/5/21/4 · free if needed: 33, 19, 22, 14 (drive explicitly), 12 (strapping, avoid) · never 16/17, 6–11.

## 7. Chassis (CAD done, cutting this week)

- Parametric Onshape custom feature: `cad/scentty_chassis.fs` (paste into a Feature Studio below Onshape's header lines; re-insert the feature after updating code). Builds 12 laser-cut 3 mm plywood panels with finger joints + 3D-printed parts (square diffuser cups, output pad, nozzle holder with tube guide, 4 snap-in tube clips, storage rack for 4 output bottles + funnel, 7 screw blocks) + REF placeholder parts.
- Layout: 560 × 216 × 300 mm. Square reservoir bottles (38.1 mm, 140 mm tall) on a lit plinth; pumps on a wall behind; nozzle over a raised output pad; electronics bay on the right with the TFT window (hole spacing 76.08 × 44 mm); storage rack on the outside right panel. Power switch cutout optional (off — no switch).
- `cad/tools/mirror.py` is a CadQuery mirror of the feature used to check collisions and render previews; keep it in sync if the .fs changes.
- **Open items:** measure actual plywood thickness → `Sheet thickness`; screen visible-area offset → `Screen window offset`; confirm the latest feature regenerates in Onshape (finger joints + rack version not yet run there); drill 5 mm holes in reservoir caps.
- Plywood: 1/8" baltic birch, 12×24" sheets, 7 sheets needed.

## 8. Timeline

| Day | Goal |
|---|---|
| Mon Nov 9 | Quiz content locked; web app working in mock mode; panels cut & printed parts started |
| Tue Nov 10 (freeze) | Firmware: AP + captive portal + serving app + real pump control |
| Wed Nov 11 | Calibration; full runs with several profiles; leak checks |
| Thu Nov 12 | LED/screen polish; rehearse |
| Fri Nov 13 | Present |

## 9. Build phases (software)

1. **Quiz content + scoring:** draft all questions/answers/points, strength question, 7 profile texts; implement scoring + archetype thresholds; simulation script to tune thresholds.
2. **Web app (mock mode):** full screen flow, triangle chart, progress screen, operator page; design system in CSS variables.
3. **Firmware:** AP + captive portal + LittleFS + API + pump state machine + calibration (NVS).
4. **LEDs + TFT** synced to state.
5. **Calibration + end-to-end tests.**

## 10. Working agreements

- Plan first, wait for approval, then build. One git branch per feature; merge when it works.
- Explain hardware/firmware steps as a walkthrough (Kasra is learning): define terms, one step at a time, check it landed.
- Test before claiming done (run the build; open the page; use mock mode).
- Keep this SPEC updated as decisions change.
