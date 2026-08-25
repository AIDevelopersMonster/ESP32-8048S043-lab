/*
  ESP32-8048S043 Lab / 07_WebServerTest

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Purpose:
    Validate a minimal browser-accessible HTTP server after 06_WiFiTest has
    proven scan + association + DHCP + DNS + TCP/HTTP + reconnect.

  What this example checks:
    - Wi-Fi STA connection with local wifi_secrets.h, or SoftAP fallback;
    - WebServer starts on port 80;
    - root HTML page is reachable from a browser;
    - /status.json returns machine-readable board/network status;
    - /ping returns a minimal text response;
    - serial log records browser requests.

  What this example does NOT check:
    - LVGL;
    - RGB display;
    - GT911 touch;
    - HTTPS/TLS;
    - OTA firmware update;
    - file upload;
    - Widget Runtime.

  Local secrets:
    For STA mode, copy wifi_secrets.example.h to wifi_secrets.h and fill:

      WIFI_TEST_SSID
      WIFI_TEST_PASSWORD

    If no credentials are present, the sketch starts a local SoftAP:

      SSID: ESP32-8048S043-XXXX
      PASS: 8048S043
      URL : http://192.168.4.1/

  PASS boundary:
    PASS candidate requires Serial to show the server URL and a browser to open
    the root page or /status.json without crash or reboot.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#if __has_include("wifi_secrets.h")
#include "wifi_secrets.h"
#define ESP32_8048S043_WEB_LOCAL_SECRETS 1
#else
static const char *WIFI_TEST_SSID = "";
static const char *WIFI_TEST_PASSWORD = "";
#define ESP32_8048S043_WEB_LOCAL_SECRETS 0
#endif

static constexpr uint32_t CONNECT_TIMEOUT_MS = 20000;
static constexpr uint16_t HTTP_PORT = 80;
static constexpr const char *AP_PASSWORD = "8048S043";

WebServer server(HTTP_PORT);

static bool staConnected = false;
static bool apStarted = false;
static String apSsid;
static uint32_t requestCount = 0;
static uint32_t lastHeartbeatMs = 0;

static void printDivider() {
  Serial.println("------------------------------------------------------------");
}

static String htmlEscape(const String &in) {
  String out;
  out.reserve(in.length() + 8);

  for (size_t i = 0; i < in.length(); ++i) {
    const char c = in[i];
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c; break;
    }
  }

  return out;
}

static String formatBytes(uint64_t value) {
  char buf[32];

  if (value >= 1024ULL * 1024ULL) {
    snprintf(buf, sizeof(buf), "%.2f MB", static_cast<double>(value) / (1024.0 * 1024.0));
  } else if (value >= 1024ULL) {
    snprintf(buf, sizeof(buf), "%.2f KB", static_cast<double>(value) / 1024.0);
  } else {
    snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(value));
  }

  return String(buf);
}

static String currentModeText() {
  if (staConnected && apStarted) {
    return "STA+AP";
  }
  if (staConnected) {
    return "STA";
  }
  if (apStarted) {
    return "AP";
  }
  return "NONE";
}

static String currentUrlText() {
  if (staConnected) {
    return "http://" + WiFi.localIP().toString() + "/";
  }
  if (apStarted) {
    return "http://" + WiFi.softAPIP().toString() + "/";
  }
  return "not available";
}

static String currentIpText() {
  if (staConnected) {
    return WiFi.localIP().toString();
  }
  if (apStarted) {
    return WiFi.softAPIP().toString();
  }
  return "0.0.0.0";
}

static bool waitForConnection(uint32_t timeoutMs) {
  const uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
    Serial.print('.');
  }

  Serial.println();
  return WiFi.status() == WL_CONNECTED;
}

static String makeStatusJson() {
  String json;
  json.reserve(1024);

  json += "{";
  json += "\"project\":\"ESP32-8048S043 Lab\",";
  json += "\"example\":\"07_WebServerTest\",";
  json += "\"mode\":\"" + currentModeText() + "\",";
  json += "\"url\":\"" + currentUrlText() + "\",";
  json += "\"ip\":\"" + currentIpText() + "\",";
  json += "\"request_count\":" + String(requestCount) + ",";
  json += "\"uptime_ms\":" + String(millis()) + ",";
  json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\",";
  json += "\"chip_revision\":" + String(ESP.getChipRevision()) + ",";
  json += "\"cpu_mhz\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"flash_bytes\":" + String(static_cast<uint32_t>(ESP.getFlashChipSize())) + ",";
  json += "\"psram_bytes\":" + String(static_cast<uint32_t>(ESP.getPsramSize())) + ",";
  json += "\"free_heap\":" + String(static_cast<uint32_t>(ESP.getFreeHeap())) + ",";
  json += "\"free_psram\":" + String(static_cast<uint32_t>(ESP.getFreePsram())) + ",";
  json += "\"sta_connected\":" + String(staConnected ? "true" : "false") + ",";
  json += "\"sta_ssid\":\"" + htmlEscape(WiFi.SSID()) + "\",";
  json += "\"sta_rssi\":" + String(staConnected ? WiFi.RSSI() : 0) + ",";
  json += "\"sta_mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"ap_started\":" + String(apStarted ? "true" : "false") + ",";
  json += "\"ap_ssid\":\"" + htmlEscape(apSsid) + "\"";
  json += "}";

  return json;
}

static String makeHtmlPage() {
  const String mode = currentModeText();
  const String url = currentUrlText();

  String html;
  html.reserve(4096);

  html += "<!doctype html><html lang='en'><head>";
  html += "<meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP32-8048S043 Lab</title>";
  html += "<style>";
  html += "body{font-family:system-ui,Arial,sans-serif;margin:0;background:#111827;color:#e5e7eb;}";
  html += "main{max-width:860px;margin:auto;padding:24px;}";
  html += "h1{font-size:1.8rem;margin:0 0 8px;}";
  html += "p{line-height:1.5;}";
  html += ".card{background:#1f2937;border:1px solid #374151;border-radius:16px;padding:18px;margin:16px 0;}";
  html += ".ok{color:#86efac;font-weight:700;}";
  html += ".warn{color:#fde68a;font-weight:700;}";
  html += "table{border-collapse:collapse;width:100%;}td{padding:7px;border-bottom:1px solid #374151;}td:first-child{color:#9ca3af;width:38%;}";
  html += "a{color:#93c5fd;}code{background:#111827;border:1px solid #374151;border-radius:6px;padding:2px 5px;}";
  html += "</style></head><body><main>";

  html += "<h1>ESP32-8048S043 Lab / 07_WebServerTest</h1>";
  html += "<p class='ok'>WEB SERVER RUNNING</p>";
  html += "<div class='card'><p>Minimal HTTP server validation after Wi-Fi full infrastructure PASS. ";
  html += "This page proves that the board can serve a browser-accessible page over Wi-Fi.</p></div>";

  html += "<div class='card'><h2>Network</h2><table>";
  html += "<tr><td>Mode</td><td>" + htmlEscape(mode) + "</td></tr>";
  html += "<tr><td>URL</td><td><code>" + htmlEscape(url) + "</code></td></tr>";
  html += "<tr><td>STA SSID</td><td>" + htmlEscape(WiFi.SSID()) + "</td></tr>";
  html += "<tr><td>STA RSSI</td><td>" + String(staConnected ? WiFi.RSSI() : 0) + " dBm</td></tr>";
  html += "<tr><td>STA MAC</td><td>" + WiFi.macAddress() + "</td></tr>";
  html += "<tr><td>AP SSID</td><td>" + htmlEscape(apSsid) + "</td></tr>";
  html += "</table></div>";

  html += "<div class='card'><h2>Board</h2><table>";
  html += "<tr><td>Chip</td><td>" + String(ESP.getChipModel()) + " rev " + String(ESP.getChipRevision()) + "</td></tr>";
  html += "<tr><td>CPU</td><td>" + String(ESP.getCpuFreqMHz()) + " MHz</td></tr>";
  html += "<tr><td>Flash</td><td>" + formatBytes(ESP.getFlashChipSize()) + "</td></tr>";
  html += "<tr><td>PSRAM</td><td>" + formatBytes(ESP.getPsramSize()) + "</td></tr>";
  html += "<tr><td>Free heap</td><td>" + formatBytes(ESP.getFreeHeap()) + "</td></tr>";
  html += "<tr><td>Free PSRAM</td><td>" + formatBytes(ESP.getFreePsram()) + "</td></tr>";
  html += "<tr><td>Uptime</td><td>" + String(millis() / 1000) + " s</td></tr>";
  html += "<tr><td>HTTP requests</td><td>" + String(requestCount) + "</td></tr>";
  html += "</table></div>";

  html += "<div class='card'><h2>Endpoints</h2>";
  html += "<p><a href='/status.json'>/status.json</a> - machine-readable status</p>";
  html += "<p><a href='/ping'>/ping</a> - minimal ping response</p>";
  html += "</div>";

  html += "<div class='card'><p class='warn'>Boundary: this test validates HTTP server reachability only. ";
  html += "LVGL Web UI, upload, HTTPS, GitHub OTA and Widget Runtime remain separate tests.</p></div>";

  html += "</main></body></html>";
  return html;
}

static void logRequest(const char *route) {
  ++requestCount;

  Serial.printf("[HTTP] #%lu %s from %s\n",
                static_cast<unsigned long>(requestCount),
                route,
                server.client().remoteIP().toString().c_str());
}

static void handleRoot() {
  logRequest("/");
  server.send(200, "text/html; charset=utf-8", makeHtmlPage());
}

static void handleStatusJson() {
  logRequest("/status.json");
  server.send(200, "application/json", makeStatusJson());
}

static void handlePing() {
  logRequest("/ping");
  server.send(200, "text/plain; charset=utf-8", "pong ESP32-8048S043 07_WebServerTest\n");
}

static void handleNotFound() {
  logRequest("404");
  server.send(404, "text/plain; charset=utf-8", "404 not found\n");
}

static bool connectStaIfConfigured() {
  if (strlen(WIFI_TEST_SSID) == 0) {
    Serial.println("[INFO] WIFI_TEST_SSID is blank: STA connection skipped.");
    return false;
  }

  Serial.printf("[STA] Connecting to SSID: %s\n", WIFI_TEST_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_TEST_SSID, WIFI_TEST_PASSWORD);

  if (!waitForConnection(CONNECT_TIMEOUT_MS)) {
    Serial.printf("[WARN] STA connection failed, status=%d\n", static_cast<int>(WiFi.status()));
    return false;
  }

  Serial.println("[PASS] STA connected and DHCP acquired");
  Serial.printf("[INFO] SSID    : %s\n", WiFi.SSID().c_str());
  Serial.printf("[INFO] RSSI    : %d dBm\n", WiFi.RSSI());
  Serial.printf("[INFO] IPv4    : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[INFO] Gateway : %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("[INFO] DNS 0   : %s\n", WiFi.dnsIP(0).toString().c_str());

  return true;
}

static bool startSoftAp() {
  uint8_t mac[6] = {0};
  WiFi.macAddress(mac);

  char name[32];
  snprintf(name, sizeof(name), "ESP32-8048S043-%02X%02X", mac[4], mac[5]);
  apSsid = name;

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  if (!WiFi.softAP(apSsid.c_str(), AP_PASSWORD)) {
    Serial.println("[FAIL] SoftAP start failed");
    return false;
  }

  delay(300);

  Serial.println("[PASS] SoftAP started");
  Serial.printf("[INFO] AP SSID : %s\n", apSsid.c_str());
  Serial.printf("[INFO] AP PASS : %s\n", AP_PASSWORD);
  Serial.printf("[INFO] AP IPv4 : %s\n", WiFi.softAPIP().toString().c_str());

  return true;
}

static void startRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status.json", HTTP_GET, handleStatusJson);
  server.on("/ping", HTTP_GET, handlePing);
  server.onNotFound(handleNotFound);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("============================================================");
  Serial.println(" ESP32-8048S043 Lab / 07_WebServerTest");
  Serial.println(" Minimal browser-accessible HTTP server validation");
  Serial.println("============================================================");
  Serial.println("Author : Alex Malachevsky");
  Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
  Serial.println("----------------------------------------------------------------");
  Serial.printf("[INFO] Local secrets header: %s\n",
                ESP32_8048S043_WEB_LOCAL_SECRETS ? "wifi_secrets.h loaded" : "not present (SoftAP fallback mode)");

  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  delay(200);

  staConnected = connectStaIfConfigured();

  if (!staConnected) {
    apStarted = startSoftAp();
    if (!apStarted) {
      Serial.println("RESULT = FAIL (no STA, no SoftAP)");
      return;
    }
  }

  startRoutes();
  server.begin();

  printDivider();
  Serial.printf("[PASS] HTTP server started on port %u\n", HTTP_PORT);
  Serial.printf("[INFO] Mode : %s\n", currentModeText().c_str());
  Serial.printf("[INFO] URL  : %s\n", currentUrlText().c_str());
  Serial.println("[INFO] Open the URL in a browser on the same network.");
  Serial.println("[INFO] Check also: /status.json and /ping");
  Serial.println();
  Serial.println("============================================================");
  Serial.println(" WEB SERVER TEST READY");
  Serial.println(" Browser reachability validation: WAITING FOR OPERATOR");
  Serial.println("============================================================");
}

void loop() {
  server.handleClient();

  const uint32_t now = millis();
  if (now - lastHeartbeatMs >= 5000) {
    lastHeartbeatMs = now;
    Serial.printf("[ALIVE] uptime=%lus mode=%s url=%s requests=%lu freeHeap=%lu\n",
                  static_cast<unsigned long>(now / 1000),
                  currentModeText().c_str(),
                  currentUrlText().c_str(),
                  static_cast<unsigned long>(requestCount),
                  static_cast<unsigned long>(ESP.getFreeHeap()));
  }

  delay(2);
}
