// The machine's screen. See display.h for the scene flow.
//
// How drawing works: a full 320×240 frame is 150 KB, too much RAM to spare next
// to WiFi. So each frame is drawn in 5 horizontal strips of 320×48 (30 KB): the
// whole scene is drawn into the strip (anything outside it is clipped), the
// strip is sent to the screen, then the next strip. No flicker, because every
// pixel is sent exactly once per frame.
//
// Look: the web app's design (style.css), redrawn for a small landscape screen:
// warm paper background, plum-black ink, the three scent colors, soft drifting
// blobs and wobbly scent blobs.

#include "display.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <WiFi.h>
#include <qrcode.h>
#include <algorithm>
#include <mutex>

namespace {

constexpr int W = 320, H = 240, STRIP_H = 48;
constexpr uint32_t REVEAL_MS = 4200;

// ---- colors (style.css :root) ------------------------------------------------------------
struct RGB { uint8_t r, g, b; };
constexpr RGB hex(uint32_t v) { return {uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)}; }

const RGB BG = hex(0xfbf5ec);       // --c-bg, warm paper
const RGB SURFACE = hex(0xffffff);  // --c-surface
const RGB INK = hex(0x2a1f2d);      // --c-ink
const RGB INK2 = hex(0x6e6070);     // --c-ink-2
const RGB INK3 = hex(0xa3979f);     // --c-ink-3
const RGB DANGER = hex(0xc8412f);   // --c-danger
// Pump order. Alcohol is a shade darker than the web's #e6e0d6 so it shows up on paper.
const RGB SCENT[NUM_PUMPS] = {hex(0xf0862a), hex(0x9783d6), hex(0xa0603a), hex(0xd6ccbd)};
const RGB SOFT[NUM_PUMPS] = {hex(0xffe2c6), hex(0xe7e0fa), hex(0xf0dccb), hex(0xefeae2)};
const char *const NOTE[NUM_PUMPS] = {"Orange: top note", "Lavender: heart note", "Cedar: base note",
                                     "Alcohol: sets strength"};

uint16_t c565(RGB c) { return ((c.r & 0xF8) << 8) | ((c.g & 0xFC) << 3) | (c.b >> 3); }
float clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }
RGB mix(RGB a, RGB b, float t) {
  t = clamp01(t);
  return {uint8_t(a.r + (b.r - a.r) * t), uint8_t(a.g + (b.g - a.g) * t), uint8_t(a.b + (b.b - a.b) * t)};
}

// ---- motion (style.css --ease-*) -----------------------------------------------------------
float easeOut(float t) { t = clamp01(t); float u = 1 - t; return 1 - u * u * u; }
float easeSpring(float t) {  // overshoots a little, like --ease-spring
  t = clamp01(t);
  const float c1 = 1.70158f, c3 = c1 + 1;
  float u = t - 1;
  return 1 + c3 * u * u * u + c1 * u * u;
}
// 0 before `delay`, then 0→1 over `dur` ms
float phase(uint32_t t, uint32_t delay, uint32_t dur) {
  return t < delay ? 0 : clamp01(float(t - delay) / float(dur));
}

// ---- painter: draws in screen coordinates into the current strip ------------------------------------
enum class Align { Left, Center, Right };

int advance(const GFXfont *f, const char *s, int spacing = 0) {
  int w = 0, n = 0;
  for (; *s; s++, n++) {
    uint8_t ch = *s;
    if (ch >= f->first && ch <= f->last) w += f->glyph[ch - f->first].xAdvance;
  }
  return w + (n > 1 ? spacing * (n - 1) : 0);
}

struct Painter {
  GFXcanvas16 *c;
  int oy;  // screen y of the strip's top row

  bool visible(int y, int h) const { return y + h > oy && y < oy + STRIP_H; }

  void rect(int x, int y, int w, int h, RGB col) {
    if (w > 0 && h > 0 && visible(y, h)) c->fillRect(x, y - oy, w, h, c565(col));
  }
  void rrect(int x, int y, int w, int h, int r, RGB col) {
    if (w > 0 && h > 0 && visible(y, h)) c->fillRoundRect(x, y - oy, w, h, r, c565(col));
  }
  void rrectLine(int x, int y, int w, int h, int r, RGB col) {
    if (visible(y, h)) c->drawRoundRect(x, y - oy, w, h, r, c565(col));
  }
  void circle(float x, float y, float r, RGB col) {
    if (r >= 0.5f && visible(int(y - r) - 1, int(2 * r) + 3)) c->fillCircle(lroundf(x), lroundf(y) - oy, lroundf(r), c565(col));
  }
  void ring(float x, float y, float r, RGB col) {
    if (r >= 1 && visible(int(y - r) - 1, int(2 * r) + 3)) {
      c->drawCircle(lroundf(x), lroundf(y) - oy, lroundf(r), c565(col));
      c->drawCircle(lroundf(x), lroundf(y) - oy, lroundf(r) - 1, c565(col));
    }
  }
  void tri(float x0, float y0, float x1, float y1, float x2, float y2, RGB col) {
    float lo = fminf(y0, fminf(y1, y2)), hi = fmaxf(y0, fmaxf(y1, y2));
    if (!visible(int(lo) - 1, int(hi - lo) + 3)) return;
    c->fillTriangle(lroundf(x0), lroundf(y0) - oy, lroundf(x1), lroundf(y1) - oy, lroundf(x2), lroundf(y2) - oy, c565(col));
  }
  void thick(float x0, float y0, float x1, float y1, float w, RGB col) {
    float dx = x1 - x0, dy = y1 - y0, len = sqrtf(dx * dx + dy * dy);
    if (len < 0.01f) return;
    float nx = -dy / len * w / 2, ny = dx / len * w / 2;
    tri(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, col);
    tri(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, col);
    circle(x0, y0, w / 2, col);
    circle(x1, y1, w / 2, col);
  }
  // Text with its baseline at y. spacing adds px between letters (for small caps kickers).
  int text(const GFXfont *f, const char *s, int x, int y, RGB col, Align a = Align::Left, int spacing = 0) {
    int w = advance(f, s, spacing);
    if (a == Align::Center) x -= w / 2;
    if (a == Align::Right) x -= w;
    if (!visible(y - f->yAdvance, f->yAdvance + 10)) return w;
    uint16_t c16 = c565(col);
    c->setFont(f);  // drawChar draws with the canvas's current font
    for (; *s; s++) {
      uint8_t ch = *s;
      if (ch < f->first || ch > f->last) continue;
      c->drawChar(x, y - oy, ch, c16, c16, 1);
      x += f->glyph[ch - f->first].xAdvance + spacing;
    }
    return w;
  }
};

// Biggest bold font that fits the width.
const GFXfont *fitFont(const char *s, int maxW) {
  if (advance(&FreeSansBold24pt7b, s) <= maxW) return &FreeSansBold24pt7b;
  if (advance(&FreeSansBold18pt7b, s) <= maxW) return &FreeSansBold18pt7b;
  if (advance(&FreeSansBold12pt7b, s) <= maxW) return &FreeSansBold12pt7b;
  return &FreeSansBold9pt7b;
}

// Soft radial blob, like the web's .ambient__blob radial gradients (faked with rings).
void softBlob(Painter &p, float x, float y, float r, RGB col) {
  for (int i = 0; i < 4; i++) p.circle(x, y, r * (1 - i * 0.22f), mix(BG, col, 0.35f + i * 0.21f));
}

// Wobbly organic blob, like the web's morphing .trio__blob.
void wobblyBlob(Painter &p, float cx, float cy, float r, RGB col, float s, float seed) {
  if (!p.visible(int(cy - r * 1.2f), int(r * 2.4f) + 2)) return;
  constexpr int N = 20;
  float px[N + 1], py[N + 1];
  for (int i = 0; i <= N; i++) {
    float a = i * 6.2831853f / N;
    float rr = r * (1 + 0.08f * sinf(3 * a + s * 1.3f + seed) + 0.05f * sinf(2 * a - s * 0.9f + seed * 2));
    px[i] = cx + rr * cosf(a);
    py[i] = cy + rr * sinf(a);
  }
  for (int i = 0; i < N; i++) p.tri(cx, cy, px[i], py[i], px[i + 1], py[i + 1], col);
}

// Three soft blobs drifting slowly behind everything (--dur-drift). weight scales each blob.
void ambient(Painter &p, uint32_t now, const float weight[3]) {
  float s = now / 1000.0f;
  softBlob(p, 20 + 18 * sinf(s * 0.29f), 10 + 14 * cosf(s * 0.23f), 115 * weight[0], SOFT[0]);
  softBlob(p, 310 + 16 * cosf(s * 0.21f), 120 + 18 * sinf(s * 0.27f), 105 * weight[1], SOFT[1]);
  softBlob(p, 110 + 20 * sinf(s * 0.17f + 1), 260 + 12 * cosf(s * 0.25f), 115 * weight[2], SOFT[2]);
}

// Kicker: small caps label with letter spacing, like the web's .kicker
void kicker(Painter &p, const char *s, int x, int y, RGB col, Align a = Align::Left) {
  p.text(&FreeSansBold9pt7b, s, x, y, col, a, 2);
}

// ---- shared state -------------------------------------------------------------------------------
enum class Scene { Idle, Reveal, Pour, Done, Stopped, Error };
const char *sceneName(Scene s) {
  switch (s) {
    case Scene::Idle: return "idle";
    case Scene::Reveal: return "reveal";
    case Scene::Pour: return "pour";
    case Scene::Done: return "done";
    case Scene::Stopped: return "stopped";
    case Scene::Error: return "error";
  }
  return "?";
}

struct BlendInfo {
  uint32_t job = 0;
  char name[40] = "";
  char strength[12] = "";
};

struct Frame {
  uint32_t now = 0, t = 0;   // t = ms since this scene started
  DispStatus s{};
  BlendInfo info;            // matches the job on screen (empty name if unknown)
  int stations = 0;          // phones on the WiFi
  float etaS = 0;            // seconds of pouring left
  float share[3] = {1.f / 3, 1.f / 3, 1.f / 3};  // scent split (orange, lavender, cedar)
};

Dispenser *disp = nullptr;
Calibration *calib = nullptr;
Adafruit_ILI9341 *tft = nullptr;
GFXcanvas16 *strip = nullptr;

std::mutex infoMu;
BlendInfo latestInfo;              // written by the web task
volatile bool demoRequested = false;

// QR code that joins the WiFi when scanned with a phone camera.
QRCode qr;
uint8_t qrData[137];               // qrcode_getBufferSize(4): 33×33 modules
bool qrOk = false;

// Director state (screen task only)
Scene scene = Scene::Idle;
uint32_t sceneStart = 0;
uint32_t shownJob = 0, doneJob = 0;
bool errorShown = false;
bool demo = false;
uint32_t demoStart = 0;
uint32_t framesInScene = 0;

void go(Scene s, uint32_t now) {
  uint32_t ms = now - sceneStart;
  if (framesInScene > 2 && ms > 0) {
    Serial.printf("[screen] %s → %s (%.1f fps)\n", sceneName(scene), sceneName(s), framesInScene * 1000.0f / ms);
  } else {
    Serial.printf("[screen] %s → %s\n", sceneName(scene), sceneName(s));
  }
  scene = s;
  sceneStart = now;
  framesInScene = 0;
}

// ---- scene: idle (waiting for guests) ---------------------------------------------------------------
void drawIdle(Painter &p, const Frame &f) {
  const float w[3] = {1, 1, 1};
  ambient(p, f.now, w);
  float s = f.now / 1000.0f;
  float in = easeOut(phase(f.t, 0, 600));

  // wordmark + tagline
  p.text(&FreeSansBold18pt7b, "ScenTTy", 18, 58, mix(BG, INK, in));
  p.text(&FreeSans9pt7b, "A perfume made", 18, 86, mix(BG, INK2, in));
  p.text(&FreeSans9pt7b, "from your personality.", 18, 105, mix(BG, INK2, in));

  // the trio: orange on top, lavender and cedar below, bobbing and morphing
  wobblyBlob(p, 84, 146 + 3 * sinf(s * 1.6f), 21, SCENT[0], s, 0);
  wobblyBlob(p, 59, 186 + 3 * sinf(s * 1.6f + 2), 21, SCENT[1], s, 2);
  wobblyBlob(p, 109, 186 + 3 * sinf(s * 1.6f + 4), 21, SCENT[2], s, 4);

  // who's around
  if (f.s.state == DispState::Busy && f.s.step >= 0) {
    p.circle(24, 223, 4 + 1.5f * sinf(s * 6), SCENT[f.s.step]);
    char buf[40];
    snprintf(buf, sizeof buf, "Operator: running %s", PUMP_IDS[f.s.step]);
    p.text(&FreeSans9pt7b, buf, 34, 228, INK2);
  } else if (f.stations > 0) {
    p.circle(24, 223, 4 + 1.5f * sinf(s * 4), SCENT[0]);
    char buf[40];
    snprintf(buf, sizeof buf, f.stations == 1 ? "1 person taking the quiz" : "%d people taking the quiz", f.stations);
    p.text(&FreeSans9pt7b, buf, 34, 228, INK);
  } else {
    p.text(&FreeSans9pt7b, "Waiting for you", 18, 228, INK3);
  }

  // QR card
  constexpr int CX = 196, CY = 24, CW = 112, CH = 156;
  p.rrect(CX, CY + 4, CW, CH, 16, mix(BG, INK, 0.07f));  // shadow
  p.rrect(CX, CY, CW, CH, 16, SURFACE);
  if (qrOk) {
    int scale = 100 / qr.size;
    int qx = CX + (CW - qr.size * scale) / 2, qy = CY + 11 + (100 - qr.size * scale) / 2;
    if (p.visible(qy, qr.size * scale)) {
      for (int y = 0; y < qr.size; y++) {
        if (!p.visible(qy + y * scale, scale)) continue;
        for (int x = 0; x < qr.size; x++) {
          if (qrcode_getModule(&qr, x, y)) p.rect(qx + x * scale, qy + y * scale, scale, scale, INK);
        }
      }
    }
  }
  p.text(&FreeSansBold9pt7b, "Scan to start", CX + CW / 2, CY + 140, INK, Align::Center);
  char wifi[40];
  snprintf(wifi, sizeof wifi, "Wi-Fi: %s", AP_SSID);
  p.text(&FreeSans9pt7b, wifi, CX + CW / 2, 203, INK2, Align::Center);
}

// ---- scene: reveal (profile name) ------------------------------------------------------------------
void drawReveal(Painter &p, const Frame &f) {
  uint32_t t = f.t;
  // rank the scents, biggest share first
  int order[3] = {0, 1, 2};
  for (int i = 0; i < 3; i++)
    for (int j = i + 1; j < 3; j++)
      if (f.share[order[j]] > f.share[order[i]]) { int tmp = order[i]; order[i] = order[j]; order[j] = tmp; }

  // burst: circles in the guest's colors bloom from the center, then soften
  float settle = phase(t, 1100, 900);
  for (int k = 0; k < 3; k++) {
    int i = order[k];
    float e = easeOut(phase(t, k * 140, 1000));
    float r = e * (90 + 260 * f.share[i]) * (1 - 0.18f * k);
    RGB col = mix(mix(SCENT[i], SOFT[i], e * 0.85f), BG, settle * 0.7f);
    p.circle(160, 120, r, col);
  }

  const char *name = f.info.name[0] ? f.info.name : "Your blend";
  float k1 = easeOut(phase(t, 700, 450));
  if (k1 > 0) kicker(p, "YOUR BLEND", 160, 70 + int((1 - k1) * 10), mix(BG, INK2, k1 * (0.4f + 0.6f * settle)), Align::Center);

  float n = easeOut(phase(t, 850, 600));
  const GFXfont *nf = fitFont(name, 300);
  if (n > 0) p.text(nf, name, 160, 112 + int((1 - n) * 16), mix(mix(SURFACE, SCENT[order[0]], 0.2f), INK, n), Align::Center);

  // accent bar under the name: the split, growing out from the middle
  float bar = easeOut(phase(t, 1250, 600));
  int bw = int(150 * bar), bx = 160 - bw / 2;
  for (int k = 0; k < 3; k++) {
    int i = k;  // pump order left → right
    int segW = int(bw * f.share[i] + 0.5f);
    if (k == 2) segW = 160 + bw / 2 - bx;
    p.rect(bx, 124, segW, 5, SCENT[i]);
    bx += segW;
  }

  // three share cards rising in, like the web's result shares
  int pct[3];
  int sum = 0, big = 0;
  for (int i = 0; i < 3; i++) {
    pct[i] = lroundf(f.share[i] * 100);
    sum += pct[i];
    if (f.share[i] > f.share[big]) big = i;
  }
  pct[big] += 100 - sum;
  static const char *const LABEL[3] = {"Orange", "Lavender", "Cedar"};
  for (int i = 0; i < 3; i++) {
    float ph = phase(t, 1450 + i * 130, 550);
    if (ph <= 0) continue;  // not started yet: draw nothing (fading from paper would show over the burst)
    float e = easeSpring(ph);
    float a = clamp01(e);
    int cx = 62 + i * 98, y = 144 + int((1 - e) * 18);
    p.rrect(cx - 44, y, 88, 58, 14, mix(BG, SURFACE, a));
    char buf[8];
    snprintf(buf, sizeof buf, "%d%%", pct[i]);
    p.text(&FreeSansBold18pt7b, buf, cx, y + 34, mix(BG, SCENT[i], a), Align::Center);
    p.text(&FreeSans9pt7b, LABEL[i], cx, y + 51, mix(BG, INK2, a), Align::Center);
  }

  char line[48];
  const char *st = f.info.strength[0] ? f.info.strength : "custom";
  snprintf(line, sizeof line, "%c%s strength, %.0f mL", toupper(st[0]), st + 1, BLEND_ML);
  if (phase(t, 2000, 500) > 0) p.text(&FreeSans9pt7b, line, 160, 226, mix(BG, INK2, phase(t, 2000, 500)), Align::Center);
}

// ---- scene: pour (bottles filling) -------------------------------------------------------------------
void drawPour(Painter &p, const Frame &f) {
  const float w[3] = {0.85f, 0.85f, 0.85f};
  ambient(p, f.now, w);
  float s = f.now / 1000.0f;
  uint32_t t = f.t;
  int step = f.s.step;
  float in = easeOut(phase(t, 0, 400));

  // header
  kicker(p, "NOW POURING", 16, 24, mix(BG, INK2, in));
  const char *name = f.info.name[0] ? f.info.name : "Your blend";
  p.text(advance(&FreeSansBold18pt7b, name) <= 200 ? &FreeSansBold18pt7b : &FreeSansBold12pt7b, name, 16, 54, mix(BG, INK, in));
  kicker(p, "LEFT", 304, 24, mix(BG, INK2, in), Align::Right);
  char eta[12];
  int secs = int(ceilf(f.etaS));
  if (step < 0 && secs <= 0) snprintf(eta, sizeof eta, "0s");
  else if (secs <= 0) snprintf(eta, sizeof eta, "<1s");
  else snprintf(eta, sizeof eta, "%ds", secs);
  p.text(&FreeSansBold18pt7b, eta, 304, 54, mix(BG, INK, in), Align::Right);

  // tube from the running reservoir to the nozzle, with liquid flowing along it
  constexpr int NOZ_X = 262, TUBE_Y = 68;
  if (step >= 0) {
    float bx = 14 + step * 50 + 19;
    RGB tube = mix(SCENT[step], BG, 0.55f);
    p.thick(bx, 86, bx, TUBE_Y, 4, tube);
    p.thick(bx, TUBE_Y, NOZ_X, TUBE_Y, 4, tube);
    float legA = 86 - TUBE_Y, legB = NOZ_X - bx, len = legA + legB;
    for (int k = 0; k < 8; k++) {
      float d = fmodf(t * 0.11f + k * len / 8, len);
      float x = d < legA ? bx : bx + (d - legA);
      float y = d < legA ? 86 - d : TUBE_Y;
      p.circle(x, y, 1.6f, SCENT[step]);
    }
  }

  // reservoirs, in the machine's real order (left → right). Glow first, so it sits behind every bottle.
  if (step >= 0) p.circle(14 + step * 50 + 19, 122, 33 + 3 * sinf(s * 7), mix(SOFT[step], BG, 0.15f));
  for (int i = 0; i < NUM_PUMPS; i++) {
    const PumpProgress &pp = f.s.pumps[i];
    bool skipped = pp.target_ml <= 0.001f;
    bool active = step == i;
    float frac = skipped ? 0 : clamp01(pp.done_ml / pp.target_ml);
    float dim = skipped ? 0.6f : 0;
    int x = 14 + i * 50;

    RGB outline = mix(mix(BG, INK, 0.2f), BG, dim);
    p.rrect(x + 11, 79, 16, 9, 3, outline);                         // neck
    p.rrect(x, 86, 38, 74, 9, outline);                             // body outline
    p.rrect(x + 2, 88, 34, 70, 7, mix(SURFACE, BG, dim));
    float level = 0.66f - 0.22f * frac;                             // drains a little as it pours
    int lh = int(66 * level), ly = 156 - lh;
    p.rrect(x + 4, ly, 30, lh, 6, mix(SCENT[i], BG, dim));
    if (active) {
      for (int k = 0; k < 3; k++) {  // bubbles
        float rise = fmodf(t * 0.035f + k * 17, float(lh - 6));
        p.circle(x + 11 + k * 8 + 2 * sinf(s * 5 + k), 153 - rise, 2, mix(SCENT[i], SURFACE, 0.55f));
      }
    }

    char buf[8] = "-";
    if (!skipped) snprintf(buf, sizeof buf, "%.1f", pp.target_ml);
    RGB label = skipped ? INK3 : (active || frac >= 0.999f ? INK : INK2);
    p.text(&FreeSansBold9pt7b, buf, x + 19, 180, label, Align::Center);
    p.rrect(x + 2, 187, 34, 5, 2, mix(BG, INK, 0.1f));
    if (!skipped) p.rrect(x + 2, 187, std::max(4, int(34 * frac)), 5, 2, SCENT[i]);
  }

  // what's being added right now (and which perfume note it is)
  if (step >= 0) {
    p.circle(20, 216, 4, SCENT[step]);
    p.text(&FreeSans9pt7b, NOTE[step], 30, 221, INK);
  } else {
    p.text(&FreeSans9pt7b, "Settling...", 16, 221, INK2);
  }

  // output bottle on the pad
  constexpr int OX = 224, OY = 82, OW = 76, OH = 120, R = 16;
  p.rect(NOZ_X - 8, 60, 16, 8, INK2);                               // nozzle
  p.tri(NOZ_X - 8, 68, NOZ_X + 8, 68, NOZ_X, 74, INK2);
  p.rrect(OX + 24, OY - 10, 28, 12, 4, mix(BG, INK, 0.22f));        // neck
  p.rrect(OX, OY, OW, OH, R, mix(BG, INK, 0.22f));
  p.rrect(OX + 2, OY + 2, OW - 4, OH - 4, R - 2, SURFACE);

  // liquid: layers in pump order, bottom up, height ∝ mL poured
  constexpr int LX = OX + 4, LW = OW - 8, LBOT = OY + OH - 4, LMAX = 100;
  float total = 0;
  for (int i = 0; i < NUM_PUMPS; i++) total += f.s.pumps[i].done_ml;
  int topY = LBOT - int(LMAX * clamp01(total / BLEND_ML));
  int topLayer = -1;
  float acc = 0;
  for (int i = 0; i < NUM_PUMPS; i++) {
    float d = f.s.pumps[i].done_ml;
    if (d <= 0.001f) continue;
    int y1 = LBOT - int(LMAX * clamp01(acc / BLEND_ML));
    acc += d;
    int y0 = LBOT - int(LMAX * clamp01(acc / BLEND_ML));
    p.rect(LX, y0, LW, y1 - y0, SCENT[i]);
    topLayer = i;
  }
  if (topLayer >= 0) {
    // a gentle wave on the surface while pouring
    float amp = step >= 0 ? 2.0f : 0.6f;
    for (int x = 0; x < LW; x++) {
      int wv = lroundf(amp * sinf(x * 0.16f + s * 5.5f));
      if (wv < 0) p.rect(LX + x, topY + wv, 1, -wv, SCENT[topLayer]);
      else if (wv > 0) p.rect(LX + x, topY, 1, wv, SURFACE);
    }
    // round off the bottom corners of the liquid to match the bottle
    constexpr int CR = R - 4;
    for (int dy = 0; dy < CR; dy++) {
      float yy = dy + 0.5f;  // rows below the corner circle's center
      int inset = int(CR - sqrtf(fmaxf(0, CR * CR - yy * yy)) + 0.5f);
      int y = LBOT - CR + dy;
      if (inset > 0) {
        p.rect(LX, y, inset, 1, SURFACE);
        p.rect(LX + LW - inset, y, inset, 1, SURFACE);
      }
    }
  }
  // drops falling from the nozzle
  if (step >= 0) {
    float fall = topY - 78;
    if (fall > 6) {
      for (int k = 0; k < 2; k++) {
        float d = fmodf(t * 0.22f + k * fall / 2, fall);
        float y = 78 + d;
        p.circle(NOZ_X, y, 3, SCENT[step]);
        p.tri(NOZ_X - 3, y - 1, NOZ_X + 3, y - 1, NOZ_X, y - 7, SCENT[step]);
      }
    }
  }
  char vol[24];
  snprintf(vol, sizeof vol, "%.1f / %.0f mL", total, BLEND_ML);
  p.text(&FreeSansBold9pt7b, vol, OX + OW / 2, 221, INK, Align::Center);
}

// ---- scene: done (confetti) ---------------------------------------------------------------------------
uint32_t hash32(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352d; x ^= x >> 15; x *= 0x846ca68b; x ^= x >> 16;
  return x;
}
float rnd(uint32_t i, uint32_t salt) { return (hash32(i * 2654435761u + salt) & 0xFFFF) / 65535.0f; }

void drawDone(Painter &p, const Frame &f) {
  uint32_t t = f.t;
  float wgt[3];
  for (int i = 0; i < 3; i++) wgt[i] = 0.75f + 0.75f * f.share[i];
  ambient(p, f.now, wgt);

  // confetti in the guest's colors: a burst, then a gentle fall for a few seconds
  constexpr int N = 54;
  for (int i = 0; i < N; i++) {
    float start = rnd(i, 1) * 3500;                      // ms
    float vy = 70 + 90 * rnd(i, 2);                      // px/s
    float life = (H + 30) / vy * 1000;
    float age = float(t) - start;
    if (age < 0) continue;
    int cycle = int(age / life);
    if (start + cycle * life > 9000) continue;          // stop spawning after 9 s
    age = fmodf(age, life) / 1000;
    float x = rnd(i, 3) * W + 10 * sinf(age * (2 + 2 * rnd(i, 4)) + i);
    float y = -12 + vy * age + 12 * age * age;
    float pick = rnd(i + cycle * 97, 5);
    int col = pick < f.share[0] ? 0 : (pick < f.share[0] + f.share[1] ? 1 : 2);
    bool flip = sinf(age * (6 + 6 * rnd(i, 6)) + i) > 0;
    if (i % 4 == 0) p.circle(x, y, 2.5f, SCENT[col]);
    else p.rect(int(x), int(y), flip ? 7 : 3, flip ? 3 : 7, SCENT[col]);
  }

  int top = 0;
  for (int i = 1; i < 3; i++) if (f.share[i] > f.share[top]) top = i;

  // check badge popping in, with a ring rippling out
  float e = easeSpring(phase(t, 100, 650));
  float rip = phase(t, 250, 900);
  if (rip > 0 && rip < 1) p.ring(160, 72, 30 + 34 * easeOut(rip), mix(SCENT[top], BG, rip));
  float r = 30 * e;
  p.circle(160, 72, r, SCENT[top]);
  if (e > 0.3f) {
    float k = e;
    p.thick(160 - 13 * k, 72 + 1 * k, 160 - 4 * k, 72 + 10 * k, 5.5f * k, SURFACE);
    p.thick(160 - 4 * k, 72 + 10 * k, 160 + 13 * k, 72 - 9 * k, 5.5f * k, SURFACE);
  }

  float a = easeOut(phase(t, 450, 500));
  kicker(p, "ALL DONE", 160, 132 + int((1 - a) * 8), mix(BG, INK2, a), Align::Center);
  const char *name = f.info.name[0] ? f.info.name : "Your blend";
  float b = easeOut(phase(t, 600, 550));
  p.text(fitFont(name, 300), name, 160, 170 + int((1 - b) * 12), mix(BG, INK, b), Align::Center);
  float c = easeOut(phase(t, 900, 500));
  p.text(&FreeSans9pt7b, "is ready. Grab your bottle!", 160, 198, mix(BG, INK2, c), Align::Center);
  p.text(&FreeSans9pt7b, "Spray on, let it settle 10 min.", 160, 222, mix(BG, INK3, phase(t, 1300, 500)), Align::Center);
}

// ---- scene: stopped (blend cancelled) -------------------------------------------------------------------
void drawStopped(Painter &p, const Frame &f) {
  const float w[3] = {0.8f, 0.8f, 0.8f};
  ambient(p, f.now, w);
  float e = easeSpring(phase(f.t, 0, 500));
  p.circle(160, 84, 30 * e, mix(BG, INK, 0.1f));
  int sq = int(20 * e);
  p.rrect(160 - sq / 2, 84 - sq / 2, sq, sq, 4, INK2);
  float a = easeOut(phase(f.t, 200, 500));
  p.text(&FreeSansBold18pt7b, "Blend stopped", 160, 150, mix(BG, INK, a), Align::Center);
  p.text(&FreeSans9pt7b, "Nothing else will pour.", 160, 178, mix(BG, INK2, a), Align::Center);
  p.text(&FreeSans9pt7b, "Ask the operator to tidy up.", 160, 200, mix(BG, INK3, a), Align::Center);
}

// ---- scene: error ---------------------------------------------------------------------------------------
void drawError(Painter &p, const Frame &f) {
  const float w[3] = {0.8f, 0.8f, 0.8f};
  ambient(p, f.now, w);
  float shake = f.t < 700 ? 6 * sinf(f.t * 0.05f) * (1 - f.t / 700.0f) : 0;
  float e = easeSpring(phase(f.t, 0, 500));
  float cx = 160 + shake;
  p.circle(cx, 70, 28 * e, DANGER);
  if (e > 0.3f) {
    p.rrect(int(cx) - 3, 70 - int(15 * e), 6, int(19 * e), 3, SURFACE);
    p.circle(cx, 70 + 11 * e, 3.2f * e, SURFACE);
  }
  float a = easeOut(phase(f.t, 200, 500));
  p.text(&FreeSansBold12pt7b, "Something went wrong", 160, 132, mix(BG, INK, a), Align::Center);

  // the reason, word-wrapped onto up to 3 lines
  const char *msg = f.s.error[0] ? f.s.error : "The pumps are off.";
  char line[64];
  int y = 158, lines = 0;
  const char *q = msg;
  while (*q && lines < 3) {
    int len = 0, lastSpace = -1;
    while (q[len]) {
      if (q[len] == ' ') lastSpace = len;
      memcpy(line, q, len + 1);
      line[len + 1] = 0;
      if (advance(&FreeSans9pt7b, line) > 290 || len + 1 >= (int)sizeof line - 1) break;
      len++;
    }
    if (q[len] && lastSpace > 0) len = lastSpace;
    memcpy(line, q, len);
    line[len] = 0;
    p.text(&FreeSans9pt7b, line, 160, y, mix(BG, INK2, a), Align::Center);
    q += len;
    while (*q == ' ') q++;
    y += 19;
    lines++;
  }
  p.text(&FreeSans9pt7b, "Ask the operator for help.", 160, 226, mix(BG, INK3, a), Align::Center);
}

// ---- director: decides which scene is on, from the dispenser's state -------------------------------------
void fillShares(Frame &f) {
  float o = f.s.pumps[0].target_ml, l = f.s.pumps[1].target_ml, c = f.s.pumps[2].target_ml;
  float sum = o + l + c;
  if (sum > 0.01f) {
    f.share[0] = o / sum;
    f.share[1] = l / sum;
    f.share[2] = c / sum;
  }
}

float realEta(const DispStatus &s) {
  float sec = 0;
  int pending = 0;
  for (int i = 0; i < NUM_PUMPS; i++) {
    float left = s.pumps[i].target_ml - s.pumps[i].done_ml;
    if (left <= 0.001f) continue;
    sec += left / calib->mlPerS(i);
    if (i != s.step) pending++;
  }
  return sec + pending * GAP_BETWEEN_PUMPS_MS / 1000.0f;
}

void directReal(Frame &f) {
  uint32_t now = f.now, t = now - sceneStart;
  const DispStatus &s = f.s;
  switch (s.state) {
    case DispState::Dispensing:
      if (s.job != shownJob) { shownJob = s.job; go(Scene::Reveal, now); }
      else if (scene == Scene::Reveal && t >= REVEAL_MS) go(Scene::Pour, now);
      else if (scene != Scene::Reveal && scene != Scene::Pour) go(Scene::Pour, now);
      break;
    case DispState::Done:
      if (s.job != doneJob) { doneJob = shownJob = s.job; go(Scene::Done, now); }
      else if (scene != Scene::Idle && t >= SCREEN_DONE_MS) go(Scene::Idle, now);
      break;
    case DispState::Error:
      if (!errorShown) { errorShown = true; go(Scene::Error, now); }
      else if (scene != Scene::Idle && t >= SCREEN_ERROR_MS) go(Scene::Idle, now);
      break;
    case DispState::Idle:
    case DispState::Busy:
      if (scene == Scene::Reveal || scene == Scene::Pour) go(Scene::Stopped, now);
      else if (scene == Scene::Stopped && t >= SCREEN_STOPPED_MS) go(Scene::Idle, now);
      else if (scene == Scene::Done && t >= SCREEN_DONE_MS) go(Scene::Idle, now);
      else if (scene == Scene::Error) go(Scene::Idle, now);
      break;
  }
  if (s.state != DispState::Error) errorShown = false;
  f.etaS = realEta(s);

  std::lock_guard<std::mutex> lock(infoMu);
  if (latestInfo.job == s.job) f.info = latestInfo;
}

// Demo: plays every scene with a made-up "Golden Hour" blend. No pump runs.
constexpr uint32_t D_REVEAL = REVEAL_MS, D_POUR = 16000, D_DONE = 10000, D_STOP = 5000, D_ERR = 7000;

void directDemo(Frame &f) {
  uint32_t e = f.now - demoStart;
  static const float ML[NUM_PUMPS] = {6.5f, 2.2f, 5.7f, 9.6f};
  const uint32_t pourMs = D_POUR - 1500;
  Scene want;
  if (e < D_REVEAL) want = Scene::Reveal;
  else if (e < D_REVEAL + D_POUR) want = Scene::Pour;
  else if (e < D_REVEAL + D_POUR + D_DONE) want = Scene::Done;
  else if (e < D_REVEAL + D_POUR + D_DONE + D_STOP) want = Scene::Stopped;
  else if (e < D_REVEAL + D_POUR + D_DONE + D_STOP + D_ERR) want = Scene::Error;
  else {
    demo = false;
    Serial.println("[screen] demo finished");
    go(Scene::Idle, f.now);
    return;
  }
  if (want != scene) go(want, f.now);

  DispStatus &s = f.s;
  s.state = DispState::Dispensing;
  float poured = BLEND_ML * clamp01(e < D_REVEAL ? 0 : float(e - D_REVEAL) / pourMs);
  if (want == Scene::Done) poured = BLEND_ML;
  s.step = -1;
  float left = poured;
  for (int i = 0; i < NUM_PUMPS; i++) {
    s.pumps[i].target_ml = ML[i];
    s.pumps[i].done_ml = fminf(ML[i], left);
    left -= s.pumps[i].done_ml;
    if (s.step < 0 && s.pumps[i].done_ml < ML[i]) s.step = i;
  }
  f.etaS = (BLEND_ML - poured) / (BLEND_ML / (pourMs / 1000.0f));
  strlcpy(s.error, "orange stayed on 2.0 s past its time (this is just the demo)", sizeof s.error);
  f.info.job = 0;
  strlcpy(f.info.name, "Golden Hour", sizeof f.info.name);
  strlcpy(f.info.strength, "signature", sizeof f.info.strength);
}

void drawScene(Painter &p, const Frame &f) {
  switch (scene) {
    case Scene::Idle: drawIdle(p, f); break;
    case Scene::Reveal: drawReveal(p, f); break;
    case Scene::Pour: drawPour(p, f); break;
    case Scene::Done: drawDone(p, f); break;
    case Scene::Stopped: drawStopped(p, f); break;
    case Scene::Error: drawError(p, f); break;
  }
}

// ---- the screen task ----------------------------------------------------------------------------------
void renderFrame(const Frame &f) {
  tft->startWrite();
  for (int oy = 0; oy < H; oy += STRIP_H) {
    Painter p{strip, oy};
    strip->fillScreen(c565(BG));
    drawScene(p, f);
    tft->setAddrWindow(0, oy, W, STRIP_H);
    tft->writePixels(strip->getBuffer(), uint32_t(W) * STRIP_H, true, false);
  }
  tft->endWrite();
}

void screenTask(void *) {
  uint32_t lastStations = 0;
  int stations = 0;
  for (;;) {
    Frame f;
    f.now = millis();
    f.s = disp->status();
    if (f.now - lastStations > 1000) {
      stations = WiFi.softAPgetStationNum();
      lastStations = f.now;
    }
    f.stations = stations;

    if (demoRequested) {
      demoRequested = false;
      demo = true;
      demoStart = f.now;
      Serial.println("[screen] demo started");
    }
    if (demo && f.s.state == DispState::Dispensing) {  // a real guest always wins
      demo = false;
      Serial.println("[screen] demo stopped: a real blend started");
    }
    if (demo) directDemo(f);
    if (!demo) {
      f.s = disp->status();
      directReal(f);
    }
    fillShares(f);
    f.t = f.now - sceneStart;

    renderFrame(f);
    framesInScene++;
    vTaskDelay(pdMS_TO_TICKS(5));  // breathe: lets the idle task and WiFi run
  }
}

// Phone cameras read "WIFI:..." codes and offer to join the network.
void initQr() {
  char payload[64];
  snprintf(payload, sizeof payload, "WIFI:T:nopass;S:%s;;", AP_SSID);
  // The library doesn't check that the text fits, so pick the version ourselves:
  // byte-mode capacity at ECC_LOW for versions 1-4.
  static const uint8_t CAPACITY[4] = {17, 32, 53, 78};
  size_t len = strlen(payload);
  for (uint8_t v = 1; v <= 4 && !qrOk; v++) {
    if (len <= CAPACITY[v - 1]) qrOk = qrcode_initText(&qr, qrData, v, ECC_LOW, payload) == 0;
  }
  if (!qrOk) Serial.println("[screen] WiFi name too long for the QR code; showing the name only");
}

}  // namespace

// ---- public ---------------------------------------------------------------------------------------------
void displayBegin(Dispenser &pumps, Calibration &cal) {
  if (!SCREEN_ENABLED) {
    Serial.println("[screen] disabled in config.h");
    return;
  }
  disp = &pumps;
  calib = &cal;

  tft = new Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
  tft->begin(TFT_SPI_HZ);
  tft->setRotation(1);  // landscape, 320×240 (HARDWARE.md §5)
  tft->fillScreen(c565(BG));

  strip = new GFXcanvas16(W, STRIP_H);
  if (!strip->getBuffer()) {
    Serial.println("[screen] not enough RAM for the frame strip; screen disabled");
    return;
  }

  initQr();

  sceneStart = millis();
  xTaskCreatePinnedToCore(screenTask, "screen", 8192, nullptr, 1, nullptr, 0);
  Serial.printf("[screen] running (QR version %d, %d×%d modules)\n", qrOk ? qr.version : 0, qr.size, qr.size);
}

void displayNoteBlend(uint32_t job, const char *profileName, const char *strength) {
  std::lock_guard<std::mutex> lock(infoMu);
  latestInfo.job = job;
  strlcpy(latestInfo.name, profileName ? profileName : "", sizeof latestInfo.name);
  strlcpy(latestInfo.strength, strength ? strength : "", sizeof latestInfo.strength);
}

void displayStartDemo() {
  if (SCREEN_ENABLED) demoRequested = true;
}

#ifdef SCREEN_HOST_TEST
// Laptop preview only (firmware/test_host/screen): draw one scene at time t into a 320×240 buffer.
void displayHostRender(int sceneIndex, uint32_t t, uint32_t now, const DispStatus &s, const char *name,
                       const char *strength, int stations, float etaS, uint16_t *out) {
  if (!strip) strip = new GFXcanvas16(W, STRIP_H);
  if (!qrOk) initQr();
  scene = Scene(sceneIndex);
  Frame f;
  f.now = now;
  f.t = t;
  f.s = s;
  strlcpy(f.info.name, name, sizeof f.info.name);
  strlcpy(f.info.strength, strength, sizeof f.info.strength);
  f.stations = stations;
  f.etaS = etaS;
  fillShares(f);
  for (int oy = 0; oy < H; oy += STRIP_H) {
    Painter p{strip, oy};
    strip->fillScreen(c565(BG));
    drawScene(p, f);
    memcpy(out + oy * W, strip->getBuffer(), sizeof(uint16_t) * W * STRIP_H);
  }
}
#endif
