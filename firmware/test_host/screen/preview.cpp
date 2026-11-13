// Laptop preview of the machine's screen: renders each scene with the real drawing
// code (src/display.cpp + Adafruit GFX) and saves PNGs, 2× size.
// Run:  firmware/test_host/screen/run.sh   → PNGs in firmware/test_host/screen/out/
#include "Arduino.h"
#include "WiFi.h"
#include "pumps.h"
#include <zlib.h>
#include <vector>

FakeSerial Serial;
FakeWiFi WiFi;
DispStatus Dispenser::status() { return DispStatus{}; }
float Calibration::mlPerS(int) const { return 1.17f; }

void displayHostRender(int scene, uint32_t t, uint32_t now, const DispStatus &s, const char *name,
                       const char *strength, int stations, float etaS, uint16_t *out);

enum { IDLE, REVEAL, POUR, DONE, STOPPED, ERROR };

static void put32(std::vector<uint8_t> &v, uint32_t x) { for (int i = 3; i >= 0; i--) v.push_back(x >> (8 * i)); }
static void chunk(FILE *f, const char *type, const std::vector<uint8_t> &data) {
  std::vector<uint8_t> c; put32(c, data.size()); c.insert(c.end(), type, type + 4); c.insert(c.end(), data.begin(), data.end());
  uint32_t crc = crc32(0, c.data() + 4, c.size() - 4); put32(c, crc);
  fwrite(c.data(), 1, c.size(), f);
}
static void savePng(const char *path, const uint16_t *px, int scale) {
  int w = 320 * scale, h = 240 * scale;
  std::vector<uint8_t> raw;
  for (int y = 0; y < h; y++) {
    raw.push_back(0);
    for (int x = 0; x < w; x++) {
      uint16_t c = px[(y / scale) * 320 + x / scale];
      raw.push_back(((c >> 11) & 31) * 255 / 31); raw.push_back(((c >> 5) & 63) * 255 / 63); raw.push_back((c & 31) * 255 / 31);
    }
  }
  uLongf zn = compressBound(raw.size()); std::vector<uint8_t> z(zn);
  compress(z.data(), &zn, raw.data(), raw.size()); z.resize(zn);
  FILE *f = fopen(path, "wb");
  const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10}; fwrite(sig, 1, 8, f);
  std::vector<uint8_t> ihdr; put32(ihdr, w); put32(ihdr, h); ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
  chunk(f, "IHDR", ihdr); chunk(f, "IDAT", z); chunk(f, "IEND", {});
  fclose(f);
}

static DispStatus blend(const float target[4], const float done[4], int step, DispState st = DispState::Dispensing) {
  DispStatus s{};
  s.state = st; s.job = 1; s.step = step;
  for (int i = 0; i < 4; i++) { s.pumps[i].target_ml = target[i]; s.pumps[i].done_ml = done[i]; }
  return s;
}

int main() {
  static uint16_t px[320 * 240];
  const float golden[4] = {6.5f, 2.2f, 5.7f, 9.6f};
  const float bold[4] = {13.1f, 5.4f, 5.5f, 0};
  const float none[4] = {0, 0, 0, 0};
  const float mid[4] = {6.5f, 1.1f, 0, 0};
  const float late[4] = {6.5f, 2.2f, 5.7f, 5.0f};
  const float boldMid[4] = {13.1f, 5.4f, 2.0f, 0};
  DispStatus idle{}; idle.state = DispState::Idle; idle.step = -1;
  DispStatus err = blend(golden, mid, -1, DispState::Error);
  strcpy(err.error, "orange stayed on 2.0 s past its deadline (loop stalled?). All pumps off.");

  struct Shot { const char *file; int scene; uint32_t t; DispStatus s; const char *name, *strength; int stations; float eta; };
  Shot shots[] = {
    {"01_idle", IDLE, 3000, idle, "", "", 0, 0},
    {"02_idle_guests", IDLE, 3000, idle, "", "", 3, 0},
    {"03_reveal_burst", REVEAL, 500, blend(golden, none, 0), "Golden Hour", "signature", 0, 0},
    {"04_reveal", REVEAL, 3000, blend(golden, none, 0), "Golden Hour", "signature", 0, 0},
    {"05_reveal_long", REVEAL, 3000, blend(bold, none, 0), "The Sunchaser", "bold", 0, 0},
    {"06_pour_lavender", POUR, 6000, blend(golden, mid, 1), "Golden Hour", "signature", 0, 14.2f},
    {"07_pour_alcohol", POUR, 9000, blend(golden, late, 3), "Golden Hour", "signature", 0, 3.9f},
    {"08_pour_bold_long", POUR, 9000, blend(bold, boldMid, 2), "The Sunchaser", "bold", 0, 3.1f},
    {"09_done_burst", DONE, 1300, blend(golden, golden, -1, DispState::Done), "Golden Hour", "signature", 0, 0},
    {"10_done", DONE, 4000, blend(bold, bold, -1, DispState::Done), "The Sunchaser", "bold", 0, 0},
    {"11_stopped", STOPPED, 2000, idle, "", "", 0, 0},
    {"12_error", ERROR, 2000, err, "", "", 0, 0},
  };
  for (auto &sh : shots) {
    displayHostRender(sh.scene, sh.t, 20000 + sh.t, sh.s, sh.name, sh.strength, sh.stations, sh.eta, px);
    char path[64];
    snprintf(path, sizeof path, "out/%s.png", sh.file);
    savePng(path, px, 2);
  }
  // 1:1 idle for the QR decode check
  displayHostRender(IDLE, 3000, 23000, idle, "", "", 0, 0, px);
  savePng("out/qr_check.png", px, 1);
  printf("wrote %zu screens to out/\n", sizeof shots / sizeof shots[0]);
}
