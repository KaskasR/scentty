// Pump flow rates (mL per second), saved in NVS so they survive power-off.
//
// NVS = "non-volatile storage": a small key-value store in the ESP32's flash.
// The Preferences library reads/writes it. We use namespace "scentty",
// keys "mlps0".."mlps3".
#pragma once
#include "config.h"

class Calibration {
public:
  void begin();                         // load saved values (or defaults)
  float mlPerS(int pump) const;         // safe to call from any task
  bool set(int pump, float mlPerS);     // validates + saves; false if out of range
  bool isCalibrated(int pump) const;

private:
  volatile float rates_[NUM_PUMPS] = {DEFAULT_ML_PER_S, DEFAULT_ML_PER_S, DEFAULT_ML_PER_S, DEFAULT_ML_PER_S};
  bool saved_[NUM_PUMPS] = {false, false, false, false};
};
