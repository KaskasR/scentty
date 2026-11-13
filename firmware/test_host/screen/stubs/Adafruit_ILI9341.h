// Fake screen: the preview calls displayHostRender() instead of the screen task.
#pragma once
#include <Adafruit_GFX.h>
class Adafruit_ILI9341 : public Adafruit_GFX {
public:
  Adafruit_ILI9341(int, int, int) : Adafruit_GFX(240, 320) {}
  void drawPixel(int16_t, int16_t, uint16_t) override {}
  void begin(uint32_t) {}
  void startWrite() override {}
  void endWrite() override {}
  void setAddrWindow(uint16_t, uint16_t, uint16_t, uint16_t) {}
  void writePixels(uint16_t *, uint32_t, bool, bool) {}
};
