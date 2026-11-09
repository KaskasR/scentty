// Laptop test for the pump state machine (src/pumps.cpp), with a fake clock and fake pins.
// Run:  firmware/test_host/run.sh      (add -v to see the firmware's serial log)

#include "Arduino.h"
#include "pumps.h"
#include <cassert>
#include <vector>

uint32_t fake_millis = 0;
int fake_pin[64] = {0};
bool fake_verbose = false;
FakeSerial Serial;

// Calibration without NVS: same interface, rates set by the test.
static float test_rates[NUM_PUMPS] = {1.17f, 1.17f, 1.17f, 1.17f};
float Calibration::mlPerS(int p) const { return test_rates[p]; }
bool Calibration::isCalibrated(int) const { return true; }

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { failures++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static int pinsHigh() {
  int n = 0;
  for (int i = 0; i < NUM_PUMPS; i++) n += fake_pin[PUMP_PINS[i]];
  return n;
}
static int whichHigh() {
  for (int i = 0; i < NUM_PUMPS; i++) if (fake_pin[PUMP_PINS[i]]) return i;
  return -1;
}

// Advance the fake clock in 1 ms steps, calling update() like loop() would.
// Tracks how long each pump was on and the most pumps ever on at once.
struct Run { uint32_t onMs[NUM_PUMPS] = {0}; int maxHigh = 0; };
static void advance(Dispenser &d, uint32_t ms, Run *r = nullptr) {
  for (uint32_t t = 0; t < ms; t++) {
    fake_millis++;
    d.update();
    if (r) {
      int h = whichHigh();
      if (h >= 0) r->onMs[h]++;
      if (pinsHigh() > r->maxHigh) r->maxHigh = pinsHigh();
    }
  }
}

static void reset(Dispenser &d) {
  for (int i = 0; i < 64; i++) fake_pin[i] = 1;  // pretend pins float HIGH at power-up
  d.begin();
}

static void test(const char *name) { printf("- %s\n", name); }

int main(int argc, char **argv) {
  fake_verbose = argc > 1 && !strcmp(argv[1], "-v");
  Calibration cal;

  {
    test("begin() drives every pump pin LOW");
    Dispenser d(cal);
    reset(d);
    CHECK(pinsHigh() == 0, "%d pins still HIGH", pinsHigh());
  }

  {
    test("full Signature blend: right order, right durations, one pump at a time, ends DONE");
    Dispenser d(cal);
    reset(d);
    float ml[4] = {3.7f, 2.0f, 8.7f, 9.6f};
    uint32_t job = 0; String why;
    CHECK(d.startBlend(ml, job, why) == StartResult::Accepted, "not accepted");
    CHECK(job == 1, "job %u", job);
    CHECK(whichHigh() == 0, "orange should start immediately, got %d", whichHigh());
    Run r;
    advance(d, 30000, &r);
    CHECK(r.maxHigh == 1, "%d pumps on at once", r.maxHigh);
    for (int i = 0; i < 4; i++) {
      uint32_t expect = (uint32_t)lroundf(ml[i] / 1.17f * 1000);
      CHECK(abs((int)r.onMs[i] - (int)expect) <= 2, "pump %d on %u ms, expected %u", i, r.onMs[i], expect);
    }
    DispStatus s = d.status();
    CHECK(s.state == DispState::Done, "state %s", stateName(s.state));
    CHECK(s.step == -1, "step %d", s.step);
    for (int i = 0; i < 4; i++) CHECK(s.pumps[i].done_ml == ml[i], "pump %d done %.2f", i, s.pumps[i].done_ml);
    CHECK(pinsHigh() == 0, "pins left on");
  }

  {
    test("progress is reported mid-pump");
    Dispenser d(cal);
    reset(d);
    float ml[4] = {6, 6, 6, 6};
    uint32_t job; String why;
    d.startBlend(ml, job, why);
    advance(d, 2000);
    DispStatus s = d.status();
    CHECK(s.state == DispState::Dispensing && s.step == 0, "state %s step %d", stateName(s.state), s.step);
    CHECK(fabsf(s.pumps[0].done_ml - 2.34f) < 0.05f, "done_ml %.2f, expected ~2.34", s.pumps[0].done_ml);
  }

  {
    test("Bold blend (0 mL alcohol) skips the alcohol pump");
    Dispenser d(cal);
    reset(d);
    float ml[4] = {12.0f, 6.0f, 6.0f, 0.0f};
    uint32_t job; String why;
    d.startBlend(ml, job, why);
    Run r;
    advance(d, 30000, &r);
    CHECK(r.onMs[3] == 0, "alcohol ran %u ms", r.onMs[3]);
    CHECK(d.status().state == DispState::Done, "not done");
  }

  {
    test("second blend while dispensing → Busy; after DONE → accepted as job 2");
    Dispenser d(cal);
    reset(d);
    float ml[4] = {6, 6, 6, 6};
    uint32_t job; String why;
    d.startBlend(ml, job, why);
    advance(d, 1000);
    CHECK(d.startBlend(ml, job, why) == StartResult::Busy, "should be busy");
    advance(d, 30000);
    CHECK(d.startBlend(ml, job, why) == StartResult::Accepted && job == 2, "job %u", job);
  }

  {
    test("cancel mid-blend: pins LOW immediately, IDLE, nothing restarts");
    Dispenser d(cal);
    reset(d);
    float ml[4] = {6, 6, 6, 6};
    uint32_t job; String why;
    d.startBlend(ml, job, why);
    advance(d, 7000);  // into lavender
    CHECK(whichHigh() == 1, "expected lavender running, got %d", whichHigh());
    d.cancel();
    CHECK(pinsHigh() == 0, "pins on right after cancel");
    Run r;
    advance(d, 30000, &r);
    CHECK(r.maxHigh == 0, "a pump came back on after cancel");
    CHECK(d.status().state == DispState::Idle, "state %s", stateName(d.status().state));
  }

  {
    test("invalid blends are rejected and never touch the pins");
    Dispenser d(cal);
    reset(d);
    uint32_t job; String why;
    float wrongTotal[4] = {3, 3, 3, 3};
    float negative[4] = {-1, 10, 6, 9};
    float nan[4] = {NAN, 8, 8, 8};
    float huge[4] = {24, 0, 0, 0};
    CHECK(d.startBlend(wrongTotal, job, why) == StartResult::Invalid, "wrong total accepted");
    CHECK(d.startBlend(negative, job, why) == StartResult::Invalid, "negative accepted");
    CHECK(d.startBlend(nan, job, why) == StartResult::Invalid, "NaN accepted");
    test_rates[0] = 0.4f;  // 24 mL at 0.4 mL/s = 60 s > 45 s limit
    CHECK(d.startBlend(huge, job, why) == StartResult::Invalid, "60 s pump run accepted");
    printf("    (example reason: %s)\n", why.c_str());
    test_rates[0] = 1.17f;
    CHECK(pinsHigh() == 0, "pins on after rejected blends");
    CHECK(d.status().job == 0, "job counter moved");
  }

  {
    test("hold-to-run: stays on while re-sent every 250 ms, stops ~400 ms after the last one");
    Dispenser d(cal);
    reset(d);
    for (int i = 0; i < 12; i++) {  // 3 s of holding
      CHECK(d.manualRun(2, 400), "manual run refused");
      advance(d, 250);
      CHECK(whichHigh() == 2, "cedar dropped out while held (t=%d)", i);
    }
    // finger lifted / phone gone: no more requests
    uint32_t t0 = fake_millis;
    while (pinsHigh() && fake_millis - t0 < 5000) advance(d, 1);
    uint32_t offAfter = fake_millis - t0;
    CHECK(offAfter <= 400 - 250 + 1, "stopped %u ms after the last tick", offAfter);  // 400 ms after the last request
    CHECK(d.status().state == DispState::Idle, "state %s", stateName(d.status().state));
  }

  {
    test("manual run refused during a blend; manual stop works");
    Dispenser d(cal);
    reset(d);
    float ml[4] = {6, 6, 6, 6};
    uint32_t job; String why;
    d.startBlend(ml, job, why);
    advance(d, 500);
    CHECK(!d.manualRun(3, 400), "manual run allowed during blend");
    d.cancel();
    d.manualRun(3, 10000);
    advance(d, 100);
    d.manualStop();
    CHECK(pinsHigh() == 0 && d.status().state == DispState::Idle, "manual stop failed");
    CHECK(!d.manualRun(9, 400) && !d.manualRun(0, 0), "bad manual args accepted");
  }

  {
    test("manual runs are capped at 60 s");
    Dispenser d(cal);
    reset(d);
    d.manualRun(0, 600000);
    Run r;
    advance(d, 70000, &r);
    CHECK(r.onMs[0] <= MAX_MANUAL_RUN_MS + 1, "ran %u ms", r.onMs[0]);
  }

  {
    test("watchdog: if loop() stalls past a pump's deadline, everything shuts off with an ERROR");
    Dispenser d(cal);
    reset(d);
    float ml[4] = {6, 6, 6, 6};
    uint32_t job; String why;
    d.startBlend(ml, job, why);
    fake_millis += 5128 + WATCHDOG_SLACK_MS + 50;  // jump the clock without calling update()
    d.update();
    DispStatus s = d.status();
    CHECK(s.state == DispState::Error && pinsHigh() == 0, "state %s, %d pins on", stateName(s.state), pinsHigh());
    printf("    (error: %s)\n", s.error);
    CHECK(d.startBlend(ml, job, why) == StartResult::Accepted, "can't start a new blend after an error");
  }

  {
    test("millis() wrap-around (day 49) mid-blend");
    Dispenser d(cal);
    reset(d);
    fake_millis = 0xFFFFFFFFu - 3000;
    float ml[4] = {6, 6, 6, 6};
    uint32_t job; String why;
    d.startBlend(ml, job, why);
    Run r;
    advance(d, 30000, &r);
    CHECK(d.status().state == DispState::Done, "state %s", stateName(d.status().state));
    CHECK(abs((int)r.onMs[0] - 5128) <= 2, "orange ran %u ms across the wrap", r.onMs[0]);
  }

  printf(failures ? "\n%d FAILED\n" : "\nall pump tests passed\n", failures);
  return failures ? 1 : 0;
}
