// Just enough fake Arduino to compile src/display.cpp and Adafruit GFX on a laptop.
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#define PROGMEM
#define radians(d) ((d) * 0.017453292519943295)
#define F(s) s
typedef bool boolean;
class __FlashStringHelper;

inline uint32_t millis() { return 0; }
inline void delay(uint32_t) {}
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
#define HIGH 1
#define LOW 0
#define OUTPUT 1

// FreeRTOS (the preview never starts the screen task)
typedef void (*TaskFunction_t)(void *);
inline int xTaskCreatePinnedToCore(TaskFunction_t, const char *, int, void *, int, void *, int) { return 0; }
inline void vTaskDelay(int) {}
#define pdMS_TO_TICKS(ms) (ms)

class String {
public:
  String(const char *s = "") : s_(s) {}
  String(float v, int d) { char b[32]; snprintf(b, sizeof b, "%.*f", d, v); s_ = b; }
  const char *c_str() const { return s_.c_str(); }
  unsigned length() const { return s_.size(); }
  char operator[](unsigned i) const { return s_[i]; }
private:
  std::string s_;
};

class Print {
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t *b, size_t n) { size_t k = 0; while (n--) k += write(*b++); return k; }
  size_t print(const char *s) { return write((const uint8_t *)s, strlen(s)); }
  size_t println(const char *s = "") { size_t n = print(s); return n + write('\n'); }
  size_t printf(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt); int n = vprintf(fmt, ap); va_end(ap); return n;
  }
};

struct FakeSerial {
  void printf(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); }
  void println(const char *s = "") { puts(s); }
};
extern FakeSerial Serial;
