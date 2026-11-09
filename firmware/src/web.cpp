// How the captive portal works:
//  1. The ESP32 is a WiFi access point ("ScenTTy", open) at 192.168.4.1.
//  2. A DNS server answers EVERY domain name with 192.168.4.1, so whatever
//     address a phone asks for, it ends up talking to us.
//  3. Right after joining, phones fetch a known "am I online?" URL and check for
//     an exact reply (Apple expects a page saying "Success", Android an empty 204).
//     We answer with a redirect to our page instead, so the phone decides this is
//     a login network and pops up its mini-browser showing the quiz.

#include "web.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

static const IPAddress AP_IP(192, 168, 4, 1);
static const IPAddress AP_MASK(255, 255, 255, 0);
static const char *PORTAL_URL = "http://192.168.4.1/";

static AsyncWebServer server(80);
static DNSServer dns;
static Dispenser *pumps = nullptr;
static Calibration *cal = nullptr;

// ---- small helpers -------------------------------------------------------------------
static void sendJson(AsyncWebServerRequest *req, int code, const JsonDocument &doc) {
  String body;
  serializeJson(doc, body);
  AsyncWebServerResponse *res = req->beginResponse(code, "application/json", body);
  res->addHeader("Cache-Control", "no-store");
  req->send(res);
}

static void sendError(AsyncWebServerRequest *req, int code, const String &msg) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["error"] = msg;
  sendJson(req, code, doc);
}

static void sendOk(AsyncWebServerRequest *req) {
  JsonDocument doc;
  doc["ok"] = true;
  sendJson(req, 200, doc);
}

static float round2(float v) { return roundf(v * 100.0f) / 100.0f; }

static void writeStatus(JsonObject out, const DispStatus &s) {
  out["state"] = stateName(s.state);
  out["job"] = s.job;
  out["step"] = s.step;
  JsonArray arr = out["pumps"].to<JsonArray>();
  for (int i = 0; i < NUM_PUMPS; i++) {
    JsonObject p = arr.add<JsonObject>();
    p["id"] = PUMP_IDS[i];
    p["target_ml"] = round2(s.pumps[i].target_ml);
    p["done_ml"] = round2(s.pumps[i].done_ml);
  }
  if (s.error[0]) out["error"] = s.error;
  else out["error"] = nullptr;
}

static void redirectToPortal(AsyncWebServerRequest *req) {
  req->redirect(PORTAL_URL);
}

// JSON POST route: body arrives parsed as a JsonVariant.
static void onJsonPost(const char *uri, ArJsonRequestHandlerFunction fn) {
  auto *h = new AsyncCallbackJsonWebHandler(uri, fn);
  h->setMethod(HTTP_POST);
  server.addHandler(h);
}

// ---- API -------------------------------------------------------------------------------
static void setupApi() {
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *req) {
    JsonDocument doc;
    writeStatus(doc.to<JsonObject>(), pumps->status());
    sendJson(req, 200, doc);
  });

  onJsonPost("/api/blend", [](AsyncWebServerRequest *req, JsonVariant &json) {
    float ml[NUM_PUMPS];
    for (int i = 0; i < NUM_PUMPS; i++) {
      String key = String(PUMP_IDS[i]) + "_ml";
      ml[i] = json[key].is<float>() ? json[key].as<float>() : NAN;  // missing → invalid
    }
    uint32_t job = 0;
    String why;
    switch (pumps->startBlend(ml, job, why)) {
      case StartResult::Accepted: {
        Serial.printf("[blend] profile: %s, strength: %s\n", json["profile"] | "?", json["strength"] | "?");
        JsonDocument doc;
        doc["accepted"] = true;
        doc["job"] = job;
        sendJson(req, 200, doc);
        break;
      }
      case StartResult::Busy: {
        JsonDocument doc;
        doc["accepted"] = false;
        doc["busy"] = true;
        writeStatus(doc["status"].to<JsonObject>(), pumps->status());
        sendJson(req, 409, doc);
        break;
      }
      case StartResult::Invalid:
        Serial.printf("[blend] rejected: %s\n", why.c_str());
        sendError(req, 400, why);
        break;
    }
  });

  // No body needed; registered as plain POST routes so an empty body is fine.
  server.on("/api/cancel", HTTP_POST, [](AsyncWebServerRequest *req) {
    pumps->cancel();
    sendOk(req);
  });

  // --- admin ---
  onJsonPost("/api/admin/run", [](AsyncWebServerRequest *req, JsonVariant &json) {
    int pump = json["pump"] | -1;
    long ms = json["ms"] | 0L;
    if (pump < 0 || pump >= NUM_PUMPS || ms <= 0) return sendError(req, 400, "need pump 0-3 and ms > 0");
    if (!pumps->manualRun(pump, (uint32_t)ms)) return sendError(req, 409, "busy: a blend is dispensing");
    sendOk(req);
  });

  server.on("/api/admin/stop", HTTP_POST, [](AsyncWebServerRequest *req) {
    pumps->manualStop();
    sendOk(req);
  });

  server.on("/api/admin/calibration", HTTP_GET, [](AsyncWebServerRequest *req) {
    JsonDocument doc;
    JsonArray rates = doc["ml_per_s"].to<JsonArray>();
    JsonArray calibrated = doc["calibrated"].to<JsonArray>();
    for (int i = 0; i < NUM_PUMPS; i++) {
      rates.add(cal->mlPerS(i));
      calibrated.add(cal->isCalibrated(i));
    }
    sendJson(req, 200, doc);
  });

  onJsonPost("/api/admin/calibration", [](AsyncWebServerRequest *req, JsonVariant &json) {
    int pump = json["pump"] | -1;
    float rate = json["ml_per_s"] | -1.0f;
    if (!cal->set(pump, rate)) {
      return sendError(req, 400, "need pump 0-3 and ml_per_s between " + String(MIN_ML_PER_S, 2) + " and " +
                                     String(MAX_ML_PER_S, 1));
    }
    sendOk(req);
  });

  // LEDs arrive in Phase 4; accept the request so the operator page works now.
  onJsonPost("/api/admin/led", [](AsyncWebServerRequest *req, JsonVariant &json) {
    Serial.printf("[led] pattern '%s' requested (LEDs not wired yet)\n", json["pattern"] | "?");
    sendOk(req);
  });
}

// ---- captive portal + files ---------------------------------------------------------------
static void setupPortal() {
  // "Am I online?" checks. Redirecting them makes the phone show its login sheet.
  const char *probes[] = {
    "/hotspot-detect.html", "/library/test/success.html",   // Apple
    "/generate_204", "/gen_204",                              // Android / Chrome
    "/connecttest.txt", "/ncsi.txt", "/redirect",             // Windows
    "/canonical.html", "/success.txt",                        // Firefox
  };
  for (const char *p : probes) server.on(p, HTTP_ANY, redirectToPortal);

  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send(LittleFS, "/admin.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html").setCacheControl("no-cache");

  server.onNotFound([](AsyncWebServerRequest *req) {
    // Some other website's address (thanks to our DNS): send them to the quiz.
    if (req->host() != AP_IP.toString()) return redirectToPortal(req);
    if (req->url().startsWith("/api/")) return sendError(req, 404, "no such endpoint");
    redirectToPortal(req);
  });
}

void webBegin(Dispenser &p, Calibration &c) {
  pumps = &p;
  cal = &c;

  if (!LittleFS.begin(/*formatOnFail=*/false)) {
    Serial.println("[web] LittleFS mount failed. Did you run 'pio run -d firmware -t uploadfs'?");
  } else if (!LittleFS.exists("/index.html")) {
    Serial.println("[web] /index.html missing. Run 'pio run -d firmware -t uploadfs'.");
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, AP_MASK);
  // channel 1, not hidden, up to 10 phones at once (the ESP32's maximum)
  WiFi.softAP(AP_SSID, nullptr, 1, 0, 10);
  WiFi.setSleep(false);  // no WiFi power saving: snappier replies
  Serial.printf("[web] WiFi \"%s\" up at %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", AP_IP);

  setupApi();
  setupPortal();
  server.begin();
  Serial.println("[web] server listening on port 80");
}

void webLoop() { dns.processNextRequest(); }
