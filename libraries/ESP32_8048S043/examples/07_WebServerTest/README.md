# 07_WebServerTest

Status: `WEB SERVER PHYSICAL PASS CANDIDATE / SAMPLE A / PSRAM REPORT CAVEAT`.

This example validates the first browser-accessible HTTP server for ESP32-8048S043 after `06_WiFiTest` has proven the Wi-Fi infrastructure path.

It is intentionally network-only. It does not initialize RGB display, GT911 touch, backlight, LVGL, SD, HTTPS, OTA or Widget Runtime.

## Why this test exists

`06_WiFiTest` proves:

```text
scan + association + DHCP + DNS + TCP/HTTP + reconnect
```

`07_WebServerTest` is the next layer:

```text
Wi-Fi connection or SoftAP fallback -> local HTTP server -> browser opens page -> JSON endpoint responds
```

This is the bridge toward:

```text
Web setup;
file upload;
browser diagnostics;
GitHub OTA dashboard;
Widget Runtime upload/control.
```

## Sample A evidence

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/07-webserver-sta-20260825.md
```

Observed STA/network result:

```text
[INFO] Local secrets header: wifi_secrets.h loaded
[STA] Connecting to SSID: TECNO CAMON 50
[PASS] STA connected and DHCP acquired
[INFO] IPv4    : 10.113.29.119
[INFO] Gateway : 10.113.29.215
[INFO] DNS 0   : 10.113.29.215
```

Observed HTTP server result:

```text
[PASS] HTTP server started on port 80
[INFO] Mode : STA
[INFO] URL  : http://10.113.29.119/
WEB SERVER TEST READY
```

Observed browser/request result:

```text
[HTTP] #1 /status.json from 10.113.29.1
[HTTP] #2 /ping from 10.113.29.1
```

Long enough ALIVE observation was also present past 180 seconds with no reboot/brownout/crash reported.

## PSRAM caveat from Sample A web-server run

The browser page and runtime JSON reported:

```text
PSRAM      : 0 B
Free PSRAM : 0 B
```

This is not treated as a browser formatting issue. It reflects what the current `07_WebServerTest` firmware reported through:

```cpp
ESP.getPsramSize()
ESP.getFreePsram()
```

At the same time, `01_BoardInfo` remains the current PSRAM acceptance test and previously reported 8 MB OPI PSRAM under the local board profile. Therefore the current `07_WebServerTest` claim is limited to WebServer reachability and does not promote PSRAM availability for this build.

Follow-up rule:

```text
Re-run 01_BoardInfo without changing Arduino IDE Tools settings.
If 01_BoardInfo also reports PSRAM 0 B, the current selected profile/menu/build is not enabling PSRAM.
If 01_BoardInfo reports 8 MB but 07_WebServerTest reports 0 B, instrument 07_WebServerTest with explicit psramFound/build-flag diagnostics.
```

## Modes

### STA mode with local credentials

If `wifi_secrets.h` is present, the example tries to join the configured Wi-Fi network:

```text
wifi_secrets.h loaded
STA connected and DHCP acquired
HTTP server started
URL: http://<board-ip>/
```

Use the same local secrets style as `06_WiFiTest`:

```cpp
#pragma once

static const char *WIFI_TEST_SSID = "YOUR_WIFI_SSID";
static const char *WIFI_TEST_PASSWORD = "YOUR_WIFI_PASSWORD";
```

`wifi_secrets.h` must not be committed.

### SoftAP fallback mode

If no credentials are present, or STA is skipped, the sketch starts its own access point:

```text
SSID: ESP32-8048S043-XXXX
PASS: 8048S043
URL : http://192.168.4.1/
```

This validates the phone/browser setup path even without an external router.

## Endpoints

```text
/            human-readable HTML status page
/status.json machine-readable board/network status
/ping        minimal plain-text ping endpoint
```

## Expected serial output

STA mode:

```text
ESP32-8048S043 Lab / 07_WebServerTest
Local secrets header: wifi_secrets.h loaded
[STA] Connecting to SSID: ...
[PASS] STA connected and DHCP acquired
[PASS] HTTP server started on port 80
[INFO] URL  : http://<board-ip>/
WEB SERVER TEST READY
```

SoftAP fallback:

```text
Local secrets header: not present (SoftAP fallback mode)
[INFO] WIFI_TEST_SSID is blank: STA connection skipped.
[PASS] SoftAP started
[INFO] AP SSID : ESP32-8048S043-XXXX
[INFO] AP PASS : 8048S043
[INFO] AP IPv4 : 192.168.4.1
[PASS] HTTP server started on port 80
```

When the browser opens the page, Serial should show:

```text
[HTTP] #1 / from <client-ip>
[HTTP] #2 /status.json from <client-ip>
[HTTP] #3 /ping from <client-ip>
```

## PASS boundary

```text
WEB SERVER PASS CANDIDATE:
  serial log shows HTTP server started;
  browser opens the root page;
  /status.json returns JSON;
  /ping returns pong;
  serial log records browser requests;
  no reboot, brownout or crash during observation.
```

## Boundary

This test does not prove:

```text
LVGL Web UI;
file upload;
LittleFS persistence;
HTTPS/TLS;
GitHub OTA manifest/download/SHA;
Widget Runtime;
long-duration Wi-Fi + display rendering stability;
PSRAM availability in this specific web-server build.
```

Those remain separate tests.
