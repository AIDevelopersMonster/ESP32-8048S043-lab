# App06 v0.1.2 - display OTA physical validation

Date: 2026-09-07
Device: ESP32-8048S043 / ESP32-S3
Release: `app06-v0.1.2`
Branch: `agent/app06-ota-recovery`

## Result

The v0.1.2 on-device OTA interface was physically exercised repeatedly on the reference board. Functional operation was reported as successful across repeated runs and is now backed by a full serial transcript covering OTA, display/touch startup, rollback and re-install cycles.

Validated functional surface:

```text
800x480 RGB display                         PASS
GT911 touch                                 PASS
STATUS / OTA navigation                     PASS
installed/current version presentation      PASS
GitHub update check from device             PASS (with one TLS allocation failure noted below)
OTA update controls from touch display      PASS
OTA status/progress presentation            PASS
PENDING_VERIFY control surface              PASS
explicit rollback                           PASS
factory return after rollback               PASS
saved Wi-Fi/NVS across OTA + rollback       PASS
intentional-reboot reconnect suppression    PASS
re-install v0.1.2 after rollback            PASS
repeated physical operation                 PASS
```

## Serial-confirmed cycle

### 0.1.1 rollback proof

The earlier `factory 0.1.0 -> ota_0 0.1.1` OTA candidate booted `PENDING_VERIFY`, retained saved Wi-Fi credentials, then explicit rollback was observed:

```text
APP06_OTA: Running partition=ota_0 version=0.1.1 state=PENDING_VERIFY
APP05_NET: STA online ip=10.113.29.119
esp_ota_ops: Rollback to previously worked partition.
boot: Defaulting to factory image
APP06_OTA: Running partition=factory version=0.1.0 state=FACTORY
APP05_NET: Saved Wi-Fi credentials found for ssid=TECNO CAMON 50 (password hidden)
APP05_NET: STA online ip=10.113.29.119
```

### Factory 0.1.0 -> v0.1.2 OTA

The release manifest offered v0.1.2, OTA selected `ota_0`, and the downloaded image passed SHA-256 verification:

```text
APP06_OTA: Update available: installed=0.1.0 available=0.1.2
APP06_OTA: OTA target=ota_0 offset=0x320000 size=3145728
APP06_OTA: SHA-256 PASS: d8356366c114b831f98cedf514da16f825c78c135e0d5e3d2a5b578399f33422
APP06_OTA: OTA verified; next boot partition=ota_0 version=0.1.2
```

The new candidate then booted from `ota_0` as `PENDING_VERIFY`:

```text
app_init: App version:      0.1.2
APP06_OTA: Running partition=ota_0 version=0.1.2 state=PENDING_VERIFY
```

### Display / touch / memory baseline on v0.1.2

Serial confirms the 8 MiB Octal PSRAM device, LVGL UI task, GT911 and local OTA HMI startup:

```text
esp_psram: Found 8MB PSRAM device
APP06_UI: Starting 800x480 LVGL OTA UI on core 1
GT911: TouchPad_ID:0x39,0x31,0x31
GT911: TouchPad_Config_Version:65
APP06_UI: Display OTA controls ready; UI stack high-water=14124 bytes
APP06: APP06:OTA:READY version=0.1.2 running=ota_0 image_state=PENDING_VERIFY display=READY
```

Saved NVS credentials again survived and the board returned to STA-only operation:

```text
APP05_NET: Saved Wi-Fi credentials found for ssid=TECNO CAMON 50 (password hidden)
APP05_NET: STA online ip=10.113.29.119
APP05_NET: Provisioning AP disabled; STA-only mode active
```

The UI task remained healthy in the observed run:

```text
APP06_UI: UI task stack high-water=11372 bytes
```

### v0.1.2 explicit rollback and reboot-cleanup fix

The v0.1.2 candidate was explicitly rolled back:

```text
esp_ota_ops: Rollback to previously worked partition.
APP05_NET: Wi-Fi is stopping for reboot; reconnect suppressed
```

This validates the v0.1.2 cleanup for the earlier `ESP_ERR_WIFI_NOT_STARTED / ESP_ERR_WIFI_STOP_STATE` reconnect noise during intentional reboot teardown.

Bootloader then returned to the previously worked factory image:

```text
boot: Defaulting to factory image
app_init: App version:      0.1.0
APP06_OTA: Running partition=factory version=0.1.0 state=FACTORY
APP05_NET: Saved Wi-Fi credentials found for ssid=TECNO CAMON 50 (password hidden)
APP05_NET: STA online ip=10.113.29.119
```

A subsequent physical cycle installed v0.1.2 again, reproduced the same v0.1.2 SHA-256, booted `ota_0 / 0.1.2 / PENDING_VERIFY`, brought up GT911/LVGL, and reconnected to the saved Wi-Fi.

## Known hardening issue: one TLS allocation failure

One GitHub CHECK performed while v0.1.2 + LVGL was running produced:

```text
esp-tls-mbedtls: mbedtls_ssl_setup returned -0x7F00
esp-tls: create_ssl_handle failed
HTTP_CLIENT: Connection failed, sock < 0
```

`-0x7F00` is `MBEDTLS_ERR_SSL_ALLOC_FAILED`, i.e. a TLS memory allocation failure. This does not invalidate the successful OTA/update/rollback cycles, but it is a real memory-headroom / largest-free-block hardening item and must not be classified as a cosmetic UI issue.

Recommended next hardening gate: log `heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL)` before TLS setup and reduce/move internal-RAM consumers if the largest block is too small.

## Known non-blocking UI issue

There is minor text overlap in the top line/header. It is cosmetic only; no functional control failure was observed. It is intentionally deferred rather than changing a physically working v0.1.2 during this validation step.

Classification:

`APP06 v0.1.2 DISPLAY OTA + EXPLICIT ROLLBACK FUNCTIONAL PHYSICAL PASS`

with one open hardening item:

`TLS memory headroom under active LVGL UI - OPEN`

## Video evidence

YouTube Shorts:

https://youtube.com/shorts/WaIzVkcltCk

The video records the working on-device OTA interface and functional interaction on the reference board.
