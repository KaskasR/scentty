#include "pumps.h"
#include <math.h>

const char *stateName(DispState s) {
  switch (s) {
    case DispState::Idle: return "idle";
    case DispState::Dispensing: return "dispensing";
    case DispState::Done: return "done";
    case DispState::Error: return "error";
    case DispState::Busy: return "busy";
  }
  return "error";
}

// millis() wraps around after ~49 days; comparing with a signed difference
// keeps "is it time yet?" correct across the wrap.
static bool reached(uint32_t now, uint32_t when) { return (int32_t)(now - when) >= 0; }

void Dispenser::begin() {
  for (int i = 0; i < NUM_PUMPS; i++) {
    digitalWrite(PUMP_PINS[i], LOW);  // set the level before switching to output: no glitch
    pinMode(PUMP_PINS[i], OUTPUT);
    digitalWrite(PUMP_PINS[i], LOW);
  }
}

// ---- pin control: only these two functions touch the pump pins ----------------------
void Dispenser::setPump(int pump, uint32_t now, uint32_t allowedMs) {
  for (int i = 0; i < NUM_PUMPS; i++) {
    if (i != pump) digitalWrite(PUMP_PINS[i], LOW);  // others off first
  }
  digitalWrite(PUMP_PINS[pump], HIGH);
  activePin_ = pump;
  pinDeadline_ = now + allowedMs;
}

void Dispenser::allOff() {
  for (int i = 0; i < NUM_PUMPS; i++) digitalWrite(PUMP_PINS[i], LOW);
  activePin_ = -1;
}

// ---- the loop side ---------------------------------------------------------------------
void Dispenser::update() {
  std::lock_guard<std::mutex> lock(mu_);
  uint32_t now = millis();

  // Watchdog: independent of the state logic below, so a bug there can't leave a pump on.
  if (activePin_ >= 0 && reached(now, pinDeadline_ + WATCHDOG_SLACK_MS)) {
    char msg[96];
    snprintf(msg, sizeof msg, "Pump %d (%s) ran past its safety time limit.", activePin_ + 1, PUMP_IDS[activePin_]);
    fault(msg);
    return;
  }

  if (state_ == DispState::Busy) {
    if (reached(now, manualUntil_)) {
      allOff();
      state_ = DispState::Idle;
      step_ = -1;
    }
    return;
  }

  if (state_ != DispState::Dispensing) return;

  uint32_t elapsed = now - phaseStart_;
  if (inGap_) {
    if (elapsed >= GAP_BETWEEN_PUMPS_MS) nextStep(now);
    return;
  }

  PumpProgress &p = pumps_[step_];
  if (elapsed >= stepMs_) {
    allOff();
    p.done_ml = p.target_ml;
    Serial.printf("[pump] %s done (%.1f mL in %.1f s)\n", PUMP_IDS[step_], p.target_ml, elapsed / 1000.0f);
    inGap_ = true;
    phaseStart_ = now;
  } else {
    // No flow sensor: progress is estimated from time × calibrated rate.
    p.done_ml = fminf(p.target_ml, rates_[step_] * elapsed / 1000.0f);
  }
}

void Dispenser::startStep(int step, uint32_t now) {
  step_ = step;
  inGap_ = false;
  phaseStart_ = now;
  stepMs_ = (uint32_t)lroundf(pumps_[step].target_ml / rates_[step] * 1000.0f);
  setPump(step, now, stepMs_);
  Serial.printf("[pump] %s on: %.1f mL at %.3f mL/s = %.2f s\n", PUMP_IDS[step], pumps_[step].target_ml,
                rates_[step], stepMs_ / 1000.0f);
}

// Move to the next pump that has something to pour, or finish.
void Dispenser::nextStep(uint32_t now) {
  int s = step_ + 1;
  while (s < NUM_PUMPS && pumps_[s].target_ml <= 0) s++;
  if (s < NUM_PUMPS) {
    startStep(s, now);
  } else {
    allOff();
    step_ = -1;
    inGap_ = false;
    state_ = DispState::Done;
    Serial.printf("[blend] job %lu done\n", (unsigned long)job_);
  }
}

void Dispenser::fault(const char *msg) {
  allOff();
  state_ = DispState::Error;
  step_ = -1;
  strlcpy(error_, msg, sizeof error_);
  Serial.printf("[FAULT] %s All pumps off.\n", msg);
}

// ---- the web side ----------------------------------------------------------------------
StartResult Dispenser::startBlend(const float ml[NUM_PUMPS], uint32_t &jobOut, String &why) {
  // Validate before taking the lock: it only reads the request and calibration.
  float total = 0;
  for (int i = 0; i < NUM_PUMPS; i++) {
    if (!isfinite(ml[i]) || ml[i] < 0 || ml[i] > BLEND_ML) {
      why = String(PUMP_IDS[i]) + "_ml must be between 0 and " + String(BLEND_ML, 0);
      return StartResult::Invalid;
    }
    total += ml[i];
    float ms = ml[i] / cal_.mlPerS(i) * 1000.0f;
    if (ms > MAX_BLEND_PUMP_MS) {
      why = String(PUMP_IDS[i]) + " would run " + String(ms / 1000.0f, 1) + " s (limit " +
            String(MAX_BLEND_PUMP_MS / 1000) + " s). Check calibration.";
      return StartResult::Invalid;
    }
  }
  if (fabsf(total - BLEND_ML) > BLEND_TOLERANCE_ML) {
    why = "volumes add up to " + String(total, 1) + " mL, expected " + String(BLEND_ML, 1);
    return StartResult::Invalid;
  }

  std::lock_guard<std::mutex> lock(mu_);
  if (state_ == DispState::Dispensing || state_ == DispState::Busy) return StartResult::Busy;

  job_ += 1;
  jobOut = job_;
  error_[0] = '\0';
  for (int i = 0; i < NUM_PUMPS; i++) {
    pumps_[i].target_ml = ml[i];
    pumps_[i].done_ml = 0;
    rates_[i] = cal_.mlPerS(i);
  }
  state_ = DispState::Dispensing;
  step_ = -1;
  Serial.printf("[blend] job %lu accepted: %.1f / %.1f / %.1f / %.1f mL\n", (unsigned long)job_, ml[0], ml[1], ml[2], ml[3]);
  nextStep(millis());
  return StartResult::Accepted;
}

void Dispenser::cancel() {
  // Pins off right now, before waiting for the lock. update() holds the lock only
  // for microseconds, and we write LOW again under the lock below, so even if
  // update() was mid-switch the pumps end up off.
  for (int i = 0; i < NUM_PUMPS; i++) digitalWrite(PUMP_PINS[i], LOW);

  std::lock_guard<std::mutex> lock(mu_);
  allOff();
  if (state_ == DispState::Dispensing) Serial.printf("[blend] job %lu cancelled\n", (unsigned long)job_);
  state_ = DispState::Idle;
  step_ = -1;
  inGap_ = false;
  error_[0] = '\0';
}

bool Dispenser::manualRun(int pump, uint32_t ms) {
  if (pump < 0 || pump >= NUM_PUMPS || ms == 0) return false;
  if (ms > MAX_MANUAL_RUN_MS) ms = MAX_MANUAL_RUN_MS;

  std::lock_guard<std::mutex> lock(mu_);
  if (state_ == DispState::Dispensing) return false;
  uint32_t now = millis();
  // Hold-to-run re-sends this every 250 ms with ms=400: each call just pushes the deadline out.
  if (state_ != DispState::Busy || activePin_ != pump) {
    Serial.printf("[manual] %s on (%lu ms)\n", PUMP_IDS[pump], (unsigned long)ms);
  }
  state_ = DispState::Busy;
  step_ = pump;
  error_[0] = '\0';
  manualUntil_ = now + ms;
  setPump(pump, now, ms);
  return true;
}

void Dispenser::manualStop() {
  std::lock_guard<std::mutex> lock(mu_);
  if (state_ != DispState::Busy) return;
  allOff();
  state_ = DispState::Idle;
  step_ = -1;
  Serial.println("[manual] stopped");
}

DispStatus Dispenser::status() {
  std::lock_guard<std::mutex> lock(mu_);
  DispStatus s;
  s.state = state_;
  s.job = job_;
  s.step = step_;
  for (int i = 0; i < NUM_PUMPS; i++) s.pumps[i] = pumps_[i];
  strlcpy(s.error, error_, sizeof s.error);
  return s;
}
