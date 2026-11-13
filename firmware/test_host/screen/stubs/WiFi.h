#pragma once
struct FakeWiFi { int softAPgetStationNum() { return 0; } };
extern FakeWiFi WiFi;
