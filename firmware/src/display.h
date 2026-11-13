// The machine's screen: an animated mirror of what the guest is doing (Phase 4).
//
//   IDLE ──blend accepted──▶ REVEAL (profile name) ──▶ POUR (bottles filling) ──▶ DONE (confetti)
//     ▲                                      │                                      │
//     └──── STOPPED (cancel) / ERROR ◀───────┘                     back to IDLE ◀───┘
//
// It runs as its own FreeRTOS task on core 0, so drawing frames can never delay
// the pump timing in loop(). It only *reads* the dispenser status (mutex-protected).
#pragma once
#include "pumps.h"
#include "calibration.h"

void displayBegin(Dispenser &pumps, Calibration &cal);  // no-op if SCREEN_ENABLED is false

// Called by the web server when a blend is accepted, so the screen can show the
// guest's profile name ("Golden Hour") and strength. Safe to call from any task.
void displayNoteBlend(uint32_t job, const char *profileName, const char *strength);

// Plays every screen once (reveal, pour, done, stopped, error) without running a pump.
void displayStartDemo();
