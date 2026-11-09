// Just enough fake Arduino to compile src/pumps.cpp on a laptop.
// Time and pins are plain variables the test controls.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cmath>
#include <string>

#define HIGH 1
#define LOW 0
#define OUTPUT 1

extern uint32_t fake_millis;
extern int fake_pin[64];
extern bool fake_verbose;

inline uint32_t millis() { return fake_millis; }
inline void pinMode(int, int) {}
inline void digitalWrite(int pin, int v) { fake_pin[pin] = v; }

struct FakeSerial {
  void printf(const char *fmt, ...) {
    if (!fake_verbose) return;
    va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
  }
  void println(const char *s = "") { if (fake_verbose) puts(s); }
};
extern FakeSerial Serial;

class String {
public:
  String(const char *s = "") : s_(s) {}
  String(const std::string &s) : s_(s) {}
  String(float v, int decimals) { char b[32]; snprintf(b, sizeof b, "%.*f", decimals, v); s_ = b; }
  String(uint32_t v) : s_(std::to_string(v)) {}
  String(int v) : s_(std::to_string(v)) {}
  const char *c_str() const { return s_.c_str(); }
  String operator+(const String &o) const { return String(s_ + o.s_); }
  String operator+(const char *o) const { return String(s_ + o); }
  friend String operator+(const char *a, const String &b) { return String(std::string(a) + b.s_); }
private:
  std::string s_;
};
