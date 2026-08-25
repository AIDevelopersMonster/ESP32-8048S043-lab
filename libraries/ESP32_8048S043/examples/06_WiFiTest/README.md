# 06_WiFiTest

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

This example validates the ESP32-8048S043 Wi-Fi radio path before porting the Web, OTA and Widget Runtime work from WT32-SC01-PLUS-Lab.

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
optional association to an access point;
optional DHCP network configuration;
optional DNS resolution;
optional TCP/HTTP HEAD request;
optional reconnect cycles.
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

Scan-only success:

```text
[INFO] STA MAC: ...
[SCAN] Starting active Wi-Fi scan...
[PASS] Wi-Fi scan completed: N network(s) found
WIFI RADIO / SCAN PHYSICAL PASS CANDIDATE
```

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

## Why this comes before Web/OTA

Web setup, GitHub OTA and Widget Runtime all depend on a stable Wi-Fi baseline. This example isolates the radio/network stack from RGB display, touch and LVGL so failures are easier to interpret.
