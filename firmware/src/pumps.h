// The dispenser: a non-blocking state machine that runs the pumps.
//
//   IDLE ──blend──▶ DISPENSING ──all pumps done──▶ DONE ──blend──▶ DISPENSING …
//     ▲                │    │
//     └────cancel──────┘    └──watchdog fault──▶ ERROR ──cancel / blend──▶ …
//   IDLE/DONE/ERROR ──admin run──▶ BUSY ──time up / stop──▶ IDLE
//
// "Non-blocking": update() is called from loop() thousands of times a second.
// Each call only looks at millis() and decides whether something should change
// right now, then returns. Nothing ever waits, so the web server stays responsive.
//
// Threading: the web server calls startBlend()/cancel()/manual*() from its own
// FreeRTOS task, while loop() calls update(). A mutex keeps them from changing
// the shared state at the same time.
#pragma once
#include <mutex>
#include "config.h"
#include "calibration.h"

enum class DispState { Idle, Dispensing, Done, Error, Busy };
const char *stateName(DispState s);

struct PumpProgress {
  float target_ml = 0;
  float done_ml = 0;
};

struct DispStatus {
  DispState state;
  uint32_t job;
  int step;                       // pump running now, -1 if none
  PumpProgress pumps[NUM_PUMPS];
  char error[96];                 // empty when no error
};

enum class StartResult { Accepted, Busy, Invalid };

class Dispenser {
public:
  explicit Dispenser(Calibration &cal) : cal_(cal) {}

  void begin();     // FIRST thing in setup(): all pump pins LOW
  void update();    // call every loop()

  // --- called from the web task ---
  // ml[] in pump order. On Accepted, jobOut is the new job id; on Invalid, why says why.
  StartResult startBlend(const float ml[NUM_PUMPS], uint32_t &jobOut, String &why);
  void cancel();                            // always works: pins LOW immediately
  bool manualRun(int pump, uint32_t ms);    // false while a blend is dispensing
  void manualStop();
  DispStatus status();

private:
  void setPump(int pump, uint32_t now, uint32_t allowedMs);  // the ONLY code that writes pump pins
  void allOff();
  void startStep(int step, uint32_t now);
  void nextStep(uint32_t now);
  void fault(const char *msg);

  Calibration &cal_;
  std::mutex mu_;

  DispState state_ = DispState::Idle;
  uint32_t job_ = 0;
  int step_ = -1;
  PumpProgress pumps_[NUM_PUMPS];
  float rates_[NUM_PUMPS] = {0};   // mL/s snapshot taken when the job starts
  char error_[96] = "";

  bool inGap_ = false;             // pausing between pumps
  uint32_t phaseStart_ = 0;        // when the current pump (or gap) started
  uint32_t stepMs_ = 0;            // how long the current pump should run

  int activePin_ = -1;             // index of the pump that's on, -1 if none
  uint32_t pinDeadline_ = 0;       // watchdog: it must be off by this time (+ slack)
  uint32_t manualUntil_ = 0;
};
