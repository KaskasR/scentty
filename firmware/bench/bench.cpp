// ScenTTy - Stage 1 bench test: one pump channel, then all four.
//
// Hardware: Freenove ESP32-WROVER (camera REMOVED) + XY-MOS MOSFET modules.
// Turns each wired pump on for 2 s, one at a time, forever, and prints what
// it's doing to the Serial Monitor (115200 baud). Test with WATER, not oils.
//
// Wiring per pump (one XY-MOS module each):
//   12V supply (+) -> VIN+        12V supply (-) -> VIN-
//   pump (+)       -> OUT+        pump (-)       -> OUT-
//   ESP32 GPIO     -> TRIG/PWM    ESP32 GND      -> module GND
//   flyback diode across the pump: stripe -> OUT+, other end -> OUT-
// The module's GND pin is tied to VIN- on the board, so ESP32 GND -> module GND
// also gives you the shared ground the switch needs.

#include <Arduino.h>

// Freenove WROVER notes:
//  - Unplug the camera ribbon: the camera uses GPIO 4,5,18,19,21,22,23,25,26,27.
//  - GPIO 16 and 17 are used by the WROVER's PSRAM: never use them.
// These four stay clear of the screen's SPI pins and the LED data pin (13).
const int PUMP_PINS[4] = {25, 26, 27, 32};
const char *PUMP_NAMES[4] = {"orange", "lavender", "cedarwood", "alcohol"};

// Start with ONE pump wired (set to 1). When it works, wire the rest and set to 4.
// Build + upload this test with:  pio run -d firmware -e bench -t upload
const int PUMPS_WIRED = 1;

const unsigned long ON_MS = 2000;
const unsigned long OFF_MS = 2000;

void setup() {
  // Drive every pump pin LOW first thing so nothing twitches at boot.
  for (int i = 0; i < 4; i++) {
    pinMode(PUMP_PINS[i], OUTPUT);
    digitalWrite(PUMP_PINS[i], LOW);
  }
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("ScenTTy bench test - pumps cycle 2 s on / 2 s off");
}

void loop() {
  for (int i = 0; i < PUMPS_WIRED; i++) {
    Serial.printf("Pump %d (%s) ON  - GPIO %d\n", i + 1, PUMP_NAMES[i], PUMP_PINS[i]);
    digitalWrite(PUMP_PINS[i], HIGH);
    delay(ON_MS);

    digitalWrite(PUMP_PINS[i], LOW);
    Serial.printf("Pump %d OFF\n", i + 1);
    delay(OFF_MS);
  }
}
