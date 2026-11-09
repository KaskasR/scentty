// WiFi access point + captive portal + web app + JSON API (SPEC §5).
#pragma once
#include "pumps.h"
#include "calibration.h"

void webBegin(Dispenser &pumps, Calibration &cal);
void webLoop();   // call every loop(): answers DNS queries
