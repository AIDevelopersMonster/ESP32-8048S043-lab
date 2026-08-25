# Sample A / Arduino / 07_WebServerTest / STA web server evidence

Status: `WEB SERVER PHYSICAL PASS CANDIDATE / SAMPLE A / PSRAM REPORT CAVEAT`.

Date: `2026-08-25`

Evidence source:

```text
Serial Monitor runtime log and browser screenshot supplied by operator.
```

## Test target

```text
Example      : 07_WebServerTest
Purpose      : minimal browser-accessible HTTP server validation
Board target : ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
Profile      : local Arduino sketchbook hardware profile
Mode         : STA mode, local wifi_secrets.h present
```

## Observed STA / DHCP evidence

```text
[INFO] Local secrets header: wifi_secrets.h loaded
[STA] Connecting to SSID: TECNO CAMON 50
[PASS] STA connected and DHCP acquired
[INFO] SSID    : TECNO CAMON 50
[INFO] RSSI    : -24 dBm
[INFO] IPv4    : 10.113.29.119
[INFO] Gateway : 10.113.29.215
[INFO] DNS 0   : 10.113.29.215
```

## Observed HTTP server evidence

```text
[PASS] HTTP server started on port 80
[INFO] Mode : STA
[INFO] URL  : http://10.113.29.119/
[INFO] Open the URL in a browser on the same network.
[INFO] Check also: /status.json and /ping

WEB SERVER TEST READY
Browser reachability validation: WAITING FOR OPERATOR
```

## Observed request evidence

The browser/client reached the board and requested JSON and ping endpoints:

```text
[HTTP] #1 /status.json from 10.113.29.1
[HTTP] #2 /ping from 10.113.29.1
```

The board remained alive while handling requests:

```text
[ALIVE] uptime=77s mode=STA url=http://10.113.29.119/ requests=1 freeHeap=265856
[ALIVE] uptime=102s mode=STA url=http://10.113.29.119/ requests=2 freeHeap=265496
[ALIVE] uptime=187s mode=STA url=http://10.113.29.119/ requests=2 freeHeap=265916
```

## Browser page evidence

The browser page showed the board status table, including:

```text
Chip          : ESP32-S3 rev 2
CPU           : 240 MHz
Flash         : 16.00 MB
PSRAM         : 0 B
Free heap     : about 258 KB
Free PSRAM    : 0 B
HTTP requests : 1
```

## Result

```text
STA association             : PASS
DHCP                        : PASS
HTTP server start           : PASS
Browser reachability        : PASS, browser opened board endpoint(s)
/status.json endpoint       : PASS, request logged
/ping endpoint              : PASS, request logged
Runtime stability           : PASS candidate, ALIVE lines continue past 180 seconds
Overall 07_WebServerTest    : WEB SERVER PHYSICAL PASS CANDIDATE / SAMPLE A
```

## PSRAM caveat

This run reported:

```text
PSRAM      : 0 B
Free PSRAM : 0 B
```

This is a real runtime report from `ESP.getPsramSize()` / `ESP.getFreePsram()` in the current `07_WebServerTest` firmware, not a browser formatting issue.

However, `01_BoardInfo` remains the current PSRAM acceptance test and previously reported 8 MB OPI PSRAM under the local board profile. Therefore this evidence does not change the Sample A hardware identity. It marks a profile/build/runtime configuration issue to re-check before LVGL/Web/OTA work that depends on PSRAM.

Recommended follow-up:

```text
1. Re-run 01_BoardInfo without changing Arduino IDE Tools settings.
2. If 01_BoardInfo also reports PSRAM 0 B, the current selected board/profile/menu is not enabling PSRAM.
3. If 01_BoardInfo reports 8 MB but 07_WebServerTest reports 0 B, instrument 07_WebServerTest with explicit psramFound/build-flag diagnostics.
```

## Boundary

This evidence supports browser-accessible HTTP server PASS candidate only.

It does not yet prove:

```text
LVGL Web UI;
file upload;
LittleFS persistence;
HTTPS/TLS;
GitHub OTA manifest/download/SHA;
Widget Runtime;
long-duration Wi-Fi + RGB/LVGL stability;
PSRAM availability in this specific web-server build.
```
