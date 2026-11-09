// ScenTTy firmware: WiFi + captive portal + web app + pump control.
// See SPEC.md. The pump bench test lives in firmware/bench/ (env "bench").
//
// Serial monitor (115200) shows every state change, e.g.
//   [blend] job 3 accepted: 3.7 / 2.0 / 8.7 / 9.6 mL
//   [pump] orange on: 3.7 mL at 1.170 mL/s = 3.16 s

#include <Arduino.h>
#include "config.h"
#include "calibration.h"
#include "pumps.h"
#include "web.h"

Calibration calibration;
Dispenser dispenser(calibration);

void setup() {
  dispenser.begin();  // FIRST: every pump pin LOW before anything else happens

  Serial.begin(115200);
  delay(300);  // let the USB serial connect so the boot log isn't lost (setup only)
  Serial.println("\n=== ScenTTy ===");

  calibration.begin();
  webBegin(dispenser, calibration);
  Serial.println("Ready. Join the ScenTTy WiFi.");
}

void loop() {
  dispenser.update();  // non-blocking: checks the clock, flips pumps when it's time
  webLoop();           // answers DNS queries (the web server runs on its own task)
}
