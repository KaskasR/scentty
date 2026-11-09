# ScenTTy — Hardware Reference (as built, Nov 9 2026)

This is the authoritative description of the electronics as they are actually wired. Firmware must match it. If wiring changes, update this file first.

Status legend: ✅ verified working on the bench · 🟡 wired, test pending · ⬜ not built yet

---

## 1. Microcontroller

- **Board:** Freenove **ESP32-WROVER** dev board (ESP32-WROVER module, 4 MB flash + PSRAM, USB-C via **CH340** USB-serial chip), plugged into a **Freenove ESP32 GPIO Extension Board** (all pins broken out with printed labels).
- **Camera:** the board shipped with an OV camera — **it is unplugged and not used.** (Its pins would otherwise occupy GPIO 4, 5, 18, 19, 21, 22, 23, 25, 26, 27.)
- **PlatformIO:** `platform = espressif32`, `board = esp32dev`, `framework = arduino`, `monitor_speed = 115200`, `upload_speed = 460800`. Uploading works from VS Code + PlatformIO on Kasra's Mac. If upload stalls at "Connecting…", hold the **IO0** button.
- **Pins that must never be used:** **GPIO 16, 17** (PSRAM on WROVER), **6–11** (flash). **34–39** are input-only.
- **Boot-sensitive pins in use:** GPIO 5 (TFT CS) — fine as a CS line. GPIO 2 has an onboard LED and is intentionally unused.
- **Lesson learned:** a pin the firmware never configures can float/rest HIGH (GPIO 14 did) — every output pin must be explicitly set up and driven.

## 2. Pin map

| GPIO | Function | Connected to | Status |
|---|---|---|---|
| 25 | Pump 1 — **sweet orange** | XY-MOS module 1 TRIG/PWM | ✅ |
| 26 | Pump 2 — **lavender** | XY-MOS module 2 TRIG/PWM | ✅ |
| 27 | Pump 3 — **Atlas cedarwood** | XY-MOS module 3 TRIG/PWM | ✅ |
| 32 | Pump 4 — **perfumer's alcohol** (diluent) | XY-MOS module 4 TRIG/PWM | ✅ |
| 13 | LED strip data | → 300 Ω resistor → strip DIN | 🟡 |
| 18 | TFT SCK | screen SCK | ✅ |
| 23 | TFT MOSI | screen SDI (MOSI) | ✅ |
| 5 | TFT CS | screen CS | ✅ |
| 21 | TFT DC | screen DC | ✅ |
| 4 | TFT RESET | screen RESET | ✅ |
| 3.3V | TFT VCC + TFT backlight (LED pin) | screen VCC, screen LED | ✅ |
| 5V | LED strip power | strip 5V | 🟡 |
| GND | common ground | screen, strip, all XY-MOS GND | ✅ |

Free GPIOs if something new is needed: 33, 14 (only if driven explicitly), 12 (strapping — avoid if possible), 19, 22.

## 3. Pumps (4 channels) ✅

**Driver:** one **XY-MOS dual-MOSFET trigger module** per pump.
- Load supply 5–36 V; trigger 3.3–20 V, **active HIGH** (ESP32 3.3 V works); PWM 0–20 kHz supported.
- The module switches the **low side**: OUT+ is tied to VIN+; OUT− is pulled to ground when triggered.
- The module's signal GND is connected to VIN− on the board, so ESP32 GND → module GND provides the common ground.
- Small onboard LED lights when TRIG is HIGH.

**Per-channel wiring:**
```
12V adapter (+) ──► VIN+        pump red (+)   ──► OUT+
12V adapter (−) ──► VIN−        pump black (−) ──► OUT−
ESP32 GPIO      ──► TRIG/PWM    ESP32 GND      ──► GND
1N5822 Schottky flyback diode across OUT+/OUT−, stripe (cathode) on OUT+
```

**Pumps:** 12 V DC peristaltic dosing pumps (Ø29 mm motor, 54.5 mm flange, 2× Ø3.2 mounting holes 48.5 mm apart, both tube barbs point up). Peristaltic ⇒ no flow and no siphoning when off; reversing motor polarity reverses flow. Flow rate **not yet calibrated** (expect very roughly 0.3–1.5 mL/s; must be measured per pump and stored).

**Tubing:** silicone, 3 mm ID × 5 mm OD. Each channel holds roughly 0.7 mL per 10 cm of tube, so tubes must be **primed** (full of liquid) before dispensing accurate volumes.

**Power:** 12 V 5 A wall adapter (barrel plug → screw-terminal adapter) feeds all four modules' VIN± in parallel. Pumps run **one at a time**.

**Verified:** bench test (`firmware/src/main.cpp`) cycles all four pumps 2 s on / 2 s off. All four work with water.

## 4. LED strip 🟡

- **Part:** SEZO WS2812B, 5 V, 60 LEDs/m (16.67 mm pitch), IP30, black PCB.
- **Layout:** **one continuous, uncut piece of 23 LEDs** (~383 mm). In the box it lies on the base inside the plinth, centered under the row of bottle holes. **Input end (LED 0) is on the RIGHT** (by the divider, nearest the electronics bay); data runs right → left; LED 22 is near the left wall.
- **Wiring:** strip's pre-attached input wires. 5 V → board 5V pin; GND → GND; data → **300 Ω** → GPIO 13. **220 µF** electrolytic capacitor across 5V/GND on the breadboard (stripe to GND).
- **Library:** Adafruit NeoPixel, `NEO_GRB + NEO_KHZ800` (color order **not yet confirmed** — if orange looks green, switch to `NEO_RGB`).
- **Which LEDs sit under each bottle** (GUESSED from CAD geometry; confirm after installation by running the chase test with the bottles in place):

  | Zone | LED indices (guess) | Color |
  |---|---|---|
  | Output bottle | 1, 2, 3 | white / animated |
  | Alcohol (pump 4) | 7, 8 | soft white |
  | Cedarwood (pump 3) | 11, 12, 13 | amber-brown |
  | Lavender (pump 2) | 16, 17 | soft purple |
  | Orange (pump 1) | 20, 21, 22 | warm orange |

  All other LEDs stay off. Keep the zone map in one config table.
- **Power limit:** 23 LEDs at full white ≈ 1.4 A — **too much** for the 5 V source. Cap global brightness (≈60/255) and only light zone LEDs. Never drive the whole strip full white.
- **Test sketch:** `firmware/hwtest/` chases LED 0→22 showing the index on the TFT, then lights the guessed zones.

## 5. Screen ✅

- **Part:** 2.8" SPI TFT, **ILI9341** driver, 240×320, red PCB (MSP2807-style, 86 × 50 mm). Has resistive touch (XPT2046) and an SD slot — **neither is wired or used.**
- **Wiring:** VCC 3.3V, GND, CS 5, RESET 4, DC 21, SDI/MOSI 23, SCK 18, LED (backlight) 3.3V (always on). SDO/MISO, T_* pins, SD pins unconnected.
- **Verified with:** Adafruit_ILI9341 + Adafruit GFX, hardware SPI, `tft.setRotation(1)` → landscape 320×240 looks correct.
- Mounted behind the electronics-bay front panel (landscape), window centered on the panel ~230 mm above the base.

## 6. Power summary

| Rail | Source | Loads |
|---|---|---|
| 12 V | 12 V 5 A adapter | 4 pumps via XY-MOS modules |
| 5 V | USB (bench) / separate 5 V battery (in the box) → board 5V pin | ESP32 board, LED strip |
| 3.3 V | ESP32 board regulator | TFT logic + backlight |

All grounds are common. **No power switch** — power off = unplug the 12 V barrel plug and the 5 V battery.

## 7. Physical layout (for UI/LED mapping)

Left → right, seen from the front: **orange, lavender, cedarwood, alcohol, output bottle**, then the electronics bay (screen on its front). Pumps sit above their bottles on the pump wall in the same order. Reservoir bottles: square 38 mm glass, 140 mm tall. Output bottle: Ø30 mm, 1 oz, filled to 24 mL, with a 28 mm funnel during dispensing. A storage rack for 4 spare output bottles + the funnel is on the outside of the right panel.

## 8. Firmware requirements derived from the hardware

- Drive GPIO 25/26/27/32 **LOW first thing in `setup()`**; never leave a pump pin unconfigured.
- **One pump at a time**, non-blocking timing (`millis()`), a max-runtime cutoff per pump, and a cancel that always works.
- Dispense time = mL ÷ calibrated mL/s per pump; store calibration in NVS (Preferences).
- Provide prime (hold-to-run) and flush routines; tubes must be primed before accurate dispensing.
- LEDs: brightness cap, zone-only lighting, zone table in config.
- TFT: hardware SPI on the pins above, landscape rotation 1.

## 9. Open hardware items

- ⬜ Confirm LED strip test (all 23 in order, correct colors) and the real zone indices once installed in the box.
- ⬜ Pump flow calibration (after tubes are routed and primed in the assembled box).
- ⬜ Measure the screen's visible-area offset for the CAD window (Onshape field "Screen window offset").
- ⬜ Assemble the chassis (plywood cut pending; printed parts in progress).
