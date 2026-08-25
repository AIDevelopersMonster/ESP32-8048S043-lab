# 06_WiFiTest

Status: `FULL WIFI PHYSICAL PASS CANDIDATE / SAMPLE A`.

This example validates the ESP32-8048S043 Wi-Fi radio and basic infrastructure network path before porting the Web, OTA and Widget Runtime work from WT32-SC01-PLUS-Lab.

It is intentionally serial-only. It does not initialize RGB display, GT911 touch, backlight, LVGL or SD.

## Lineage

The test pattern is adapted from:

```text
WT32-SC01-PLUS-Lab / libraries/WT32_SC01_PLUS/examples/08_WiFiTest
```

The logic is portable because it uses the standard Arduino `WiFi.h` API and does not depend on WT32 display or touch pins.

## What it checks

```text
Wi-Fi STA mode starts;
STA MAC is readable;
active scan completes;
association to a configured access point;
DHCP network configuration;
DNS resolution;
TCP/HTTP HEAD request;
disconnect/reconnect cycles.
```

## Sample A scan-only evidence

Video evidence:

```text
https://youtube.com/shorts/DOus0uNBBZI
```

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/06-wifi-scan-20260825.md
```

Observed scan-only result:

```text
[INFO] Local secrets header: not present (scan-only mode)
[INFO] STA MAC: 84:FC:E6:6C:69:3C
[SCAN] Starting active Wi-Fi scan...
[PASS] Wi-Fi scan completed: 3 network(s) found
WIFI RADIO / SCAN PHYSICAL PASS CANDIDATE
Full association/DHCP/DNS/TCP/reconnect validation: PENDING
```

## Sample A full infrastructure evidence

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/06-wifi-full-infrastructure-20260825.md
```

Observed full result:

```text
[PASS] Wi-Fi scan completed: 2 network(s) found
[CONNECT] Connecting to SSID: TECNO CAMON 50
[PASS] Associated and DHCP configuration acquired
[PASS] DNS example.com -> 8.6.112.1
[PASS] TCP connection established
[PASS] HTTP response: HTTP/1.1 200 OK
[PASS] reconnect cycle 1/3  RSSI=-46 dBm  IP=10.113.29.119
[PASS] reconnect cycle 2/3  RSSI=-45 dBm  IP=10.113.29.119
[PASS] reconnect cycle 3/3  RSSI=-46 dBm  IP=10.113.29.119
WIFI TEST PHYSICAL PASS CANDIDATE
```

## Scan-only mode

By default, no real Wi-Fi credentials are committed.

Without `wifi_secrets.h`, the example runs scan-only mode:

```text
WIFI RADIO / SCAN PHYSICAL PASS CANDIDATE
Full association/DHCP/DNS/TCP/reconnect validation: PENDING
```

This is enough to prove that the Wi-Fi radio can scan nearby networks.

## Full infrastructure mode

Copy:

```text
wifi_secrets.example.h
```

to:

```text
wifi_secrets.h
```

and fill:

```cpp
static const char *WIFI_TEST_SSID = "YOUR_WIFI_SSID";
static const char *WIFI_TEST_PASSWORD = "YOUR_WIFI_PASSWORD";
```

`wifi_secrets.h` is intentionally ignored by `.gitignore` and must not be committed.

## Expected serial output

Full infrastructure success:

```text
[PASS] Wi-Fi scan completed: N network(s) found
[PASS] Associated and DHCP configuration acquired
[PASS] DNS example.com -> ...
[PASS] TCP connection established
[PASS] HTTP response: HTTP/...
[PASS] reconnect cycle 1/3 ...
[PASS] reconnect cycle 2/3 ...
[PASS] reconnect cycle 3/3 ...
WIFI TEST PHYSICAL PASS CANDIDATE
```

## PASS boundary

```text
SCAN PASS:
  serial evidence that active scan completed and one or more networks were reported.

FULL WIFI PASS:
  scan + association + DHCP + DNS + TCP/HTTP + reconnect cycles passed on a named physical specimen.
```

Current Sample A claim:

```text
FULL WIFI PHYSICAL PASS CANDIDATE / SAMPLE A
```

## Why this comes before Web/OTA

Web setup, GitHub OTA and Widget Runtime all depend on a stable Wi-Fi baseline. This example isolates the radio/network stack from RGB display, touch and LVGL so failures are easier to interpret.

## Remaining Wi-Fi-related work

```text
long-duration Wi-Fi stability;
Wi-Fi while RGB/LVGL rendering is active;
on-device Web setup UI;
HTTPS/TLS download;
GitHub OTA manifest/download/SHA validation;
Widget Runtime network workflow.
```
