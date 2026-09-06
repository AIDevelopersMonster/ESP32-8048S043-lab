# App 06 - GitHub OTA, Rollback and Recovery

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Status:** SOURCE / CI TARGET - PHYSICAL VALIDATION REQUIRED

## Controlled variable

App06 does not invent a new OTA transport. It ports the GitHub Release OTA contract already physically validated in `AIDevelopersMonster/WT32-SC01-PLUS-Lab` Example 20 and adds the rollback/recovery gate that was deliberately left open there.

Primary update path:

```text
ESP32-8048S043
      |
      | verified HTTPS
      v
GitHub Releases
      |
      +-- app06-ota.json
      +-- app06-ota.bin
      |
      v
inactive ota_0 / ota_1
      |
      +-- board/app/channel validation
      +-- semantic version ordering
      +-- byte-count verification
      +-- SHA-256 verification
      +-- ESP image validation
      +-- project/version descriptor validation
      v
select boot partition + reboot
      |
      v
ESP_OTA_IMG_PENDING_VERIFY
      |
      +-- CONFIRM -> VALID
      +-- explicit ROLLBACK -> previous OTA image
      +-- reset/WDT before confirm -> bootloader rollback
```

Local/offline OTA is explicitly out of the primary App06 scope. It is reserved as a future corporate/private-deployment mode.

## GitHub manifest

The device checks:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
```

Schema:

```json
{
  "schema": 1,
  "board": "esp32-8048s043-lab-n16r8",
  "app": "app06-ota-recovery",
  "version": "0.1.1",
  "channel": "stable",
  "size": 123456,
  "sha256": "64-lowercase-hex-digits",
  "firmware": "https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/download/app06-v0.1.1/app06-ota.bin"
}
```

No GitHub token is stored on the device. HTTPS certificate verification uses the ESP-IDF certificate bundle; insecure TLS is not enabled.

## Rollback contract

ESP-IDF v5.5.5 is built with:

```text
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
```

A newly installed OTA image therefore boots as `PENDING_VERIFY`. App06 intentionally does not auto-confirm it during the laboratory test.

Physical rollback validation requires two OTA generations, because bootloader rollback is between OTA images rather than an implicit return to the factory image:

```text
factory 0.1.0
   -> GitHub OTA 0.1.1
   -> confirm 0.1.1 VALID
   -> GitHub OTA 0.1.2
   -> leave PENDING_VERIFY
   -> rollback
   -> 0.1.1 runs again
```

Factory is a separate explicit recovery target.

## HTTP control surface

The App05 setup/status server is retained and extended:

```text
GET  /ota/status
POST /ota/check
POST /ota/install
POST /ota/confirm
POST /ota/rollback
POST /ota/recovery
```

TLS/download/hash/flash work executes in a dedicated 16 KiB OTA task instead of the HTTP server task.

## Persistence boundary

```text
full USB/Web-Flasher installation -> may erase NVS
app-only GitHub OTA              -> preserves NVS and stored Wi-Fi credentials
```

This boundary is part of App06 acceptance.

## Physical acceptance sequence

1. Install the App06 `0.1.0` full baseline once over cable/Web Flasher.
2. Provision Wi-Fi and verify `STA_ONLINE`.
3. Before any GitHub App06 release exists, `CHECK GITHUB` may report `404 / no release` as an expected pre-release boundary.
4. Publish `app06-v0.1.1` and check/install from the device.
5. Verify reboot into `ota_0` or `ota_1`, version `0.1.1`, state `PENDING_VERIFY`, and Wi-Fi credentials still present.
6. Confirm `0.1.1`; state must become `VALID`.
7. Publish/install `0.1.2`.
8. Verify `PENDING_VERIFY`, then trigger rollback (and separately test reset-before-confirm if desired).
9. Confirm the board returns to the previously validated OTA image.
10. Test explicit factory recovery separately.

No PHYSICAL PASS or rollback claim is made until these steps are reproduced on Sample A.
