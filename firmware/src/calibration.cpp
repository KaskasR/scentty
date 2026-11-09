#include "calibration.h"
#include <Preferences.h>

static const char *NVS_NAMESPACE = "scentty";

static String key(int pump) { return String("mlps") + pump; }

static bool inRange(float v) { return v >= MIN_ML_PER_S && v <= MAX_ML_PER_S; }

void Calibration::begin() {
  Preferences prefs;
  // Fails on the very first boot (nothing saved yet): then every pump uses the default.
  bool opened = prefs.begin(NVS_NAMESPACE, /*readOnly=*/true);
  for (int i = 0; i < NUM_PUMPS; i++) {
    float v = opened ? prefs.getFloat(key(i).c_str(), NAN) : NAN;
    saved_[i] = inRange(v);  // NAN (never saved) fails the range check too
    rates_[i] = saved_[i] ? v : DEFAULT_ML_PER_S;
    Serial.printf("[cal] pump %d (%s): %.3f mL/s%s\n", i + 1, PUMP_IDS[i], rates_[i],
                  saved_[i] ? "" : "  (default, not calibrated yet)");
  }
  if (opened) prefs.end();
}

float Calibration::mlPerS(int pump) const {
  return (pump >= 0 && pump < NUM_PUMPS) ? rates_[pump] : DEFAULT_ML_PER_S;
}

bool Calibration::isCalibrated(int pump) const {
  return pump >= 0 && pump < NUM_PUMPS && saved_[pump];
}

bool Calibration::set(int pump, float v) {
  if (pump < 0 || pump >= NUM_PUMPS || !inRange(v)) return false;
  Preferences prefs;
  prefs.begin(NVS_NAMESPACE, /*readOnly=*/false);
  prefs.putFloat(key(pump).c_str(), v);
  prefs.end();
  rates_[pump] = v;
  saved_[pump] = true;
  Serial.printf("[cal] pump %d (%s) set to %.3f mL/s\n", pump + 1, PUMP_IDS[pump], v);
  return true;
}
