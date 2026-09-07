# App06 v0.1.2 - display OTA physical validation

Date: 2026-09-07
Device: ESP32-8048S043 / ESP32-S3
Release: `app06-v0.1.2`
Branch: `agent/app06-ota-recovery`

## Result

The v0.1.2 on-device OTA interface was physically exercised repeatedly on the reference board. Functional operation was reported as successful across repeated runs.

Validated functional surface:

```text
800x480 RGB display                         PASS
GT911 touch                                 PASS
STATUS / OTA navigation                     PASS
installed/current version presentation      PASS
GitHub update check from device             PASS
OTA update controls from touch display      PASS
OTA status/progress presentation            PASS
PENDING_VERIFY control surface              PASS
rollback/recovery controls available        PASS
saved Wi-Fi operation after OTA             PASS
repeated physical operation                 PASS
```

The primary App06 GitHub OTA transport had already been physically proven on the same sample for factory v0.1.0 -> ota_0 v0.1.1 with verified HTTPS, manifest validation, SHA-256 PASS, image validation, reboot and NVS/Wi-Fi preservation. v0.1.2 adds the local display/touch control layer using the physically validated App03 ESP-IDF/LVGL9 RGB+GT911 stack.

## Known non-blocking UI issue

There is minor text overlap in the top line/header. It is cosmetic only; no functional control failure was observed. It is intentionally deferred rather than changing a physically working v0.1.2 during this validation step.

Classification:

`APP06 v0.1.2 DISPLAY OTA FUNCTIONAL PHYSICAL PASS`

## Video evidence

YouTube Shorts:

https://youtube.com/shorts/WaIzVkcltCk

The video records the working on-device OTA interface and functional interaction on the reference board.
