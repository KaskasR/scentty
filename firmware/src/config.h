// ScenTTy - every pin and number in one place. See SPEC.md §1, §2, §5, §6.
#pragma once
#include <Arduino.h>

// ---- pumps (SPEC §1) --------------------------------------------------------
// Order matters: this is also the dispense order and the order of `pumps` in /api/status.
// Never use GPIO 16/17 (PSRAM on the WROVER).
constexpr int NUM_PUMPS = 4;
constexpr int PUMP_PINS[NUM_PUMPS] = {25, 26, 27, 32};
constexpr const char *PUMP_IDS[NUM_PUMPS] = {"orange", "lavender", "cedar", "alcohol"};

// ---- blend (SPEC §2) ----------------------------------------------------------
constexpr float BLEND_ML = 24.0f;
constexpr float BLEND_TOLERANCE_ML = 0.2f;   // a blend's total must be BLEND_ML ± this

// ---- calibration ------------------------------------------------------------------
// Pump spec sheet says 70 mL/min = 1.17 mL/s. Used until a pump is calibrated.
constexpr float DEFAULT_ML_PER_S = 1.17f;
constexpr float MIN_ML_PER_S = 0.05f;
constexpr float MAX_ML_PER_S = 10.0f;

// ---- safety ---------------------------------------------------------------------
constexpr uint32_t MAX_BLEND_PUMP_MS = 45000;   // refuse a blend if any pump would run longer
constexpr uint32_t MAX_MANUAL_RUN_MS = 60000;   // cap for /api/admin/run (calibration runs)
constexpr uint32_t WATCHDOG_SLACK_MS = 2000;    // a pin HIGH this long past its deadline = fault
constexpr uint32_t GAP_BETWEEN_PUMPS_MS = 300;  // let drips settle between pumps

// ---- WiFi / captive portal (SPEC §5) ------------------------------------------------
constexpr const char *AP_SSID = "ScenTTy";       // open network
