# App 06 - GitHub OTA, Rollback and Recovery

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Status:** FUNCTIONAL PHYSICAL PASS — GitHub OTA + on-device LVGL9 OTA control + explicit rollback validated; TLS memory-headroom hardening remains open

## Current validated result

App06 now has a physically validated GitHub OTA path, a physically validated on-device OTA control surface, and a physically validated explicit rollback path on the ESP32-8048S043 reference board.

Validated physical path:

```text
factory 0.1.0
   -> verified HTTPS GitHub manifest check
   -> GitHub OTA download
   -> byte-count / SHA-256 / ESP image validation
   -> ota_0 0.1.1 PENDING_VERIFY
   -> saved Wi-Fi/NVS preserved
   -> explicit rollback
   -> factory 0.1.0
   -> GitHub OTA 0.1.2
   -> ota_0 0.1.2 PENDING_VERIFY
   -> LVGL9 display + GT911 touch online
   -> explicit rollback from the v0.1.2 control surface
   -> factory 0.1.0
   -> re-install 0.1.2 reproduced successfully
```

The v0.1.2 release adds a local 800x480 LVGL 9.3 / GT911 touch UI based on the physically validated App03 display stack. Repeated physical runs confirmed that the display, touch interaction, STATUS/OTA navigation and OTA controls work functionally on the board.

Video evidence for the v0.1.2 display OTA interface:

- https://youtube.com/shorts/WaIzVkcltCk

Known UI issue: minor text overlap in the top/header line. This is cosmetic and is intentionally deferred because no functional control failure was observed.

## Controlled variable

App06 does not invent a new OTA transport. It ports the GitHub Release OTA contract already physically validated in `AIDevelopersMonster/WT32-SC01-PLUS-Lab` Example 20 and adds the rollback/recovery gate plus the ESP32-8048S043 on-device touch UI.

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
      +-- explicit ROLLBACK -> previous working image
      +-- reset/WDT before confirm -> bootloader rollback
      +-- FACTORY RECOVERY -> factory partition
```

Local/offline OTA is explicitly out of the primary App06 scope. It is reserved as a future corporate/private-deployment mode.

## On-device control surface

v0.1.2 adds two local touch-display pages:

```text
STATUS
  firmware version
  network state
  STA IP
  running partition
  image state

OTA
  installed / available versions
  OTA state + message
  progress bar
  CHECK GITHUB
  INSTALL UPDATE
  CONFIRM
  ROLLBACK
  FACTORY RECOVERY
```

Rules:

- CHECK and INSTALL require STA online;
- INSTALL is enabled only when a newer update is known;
- CONFIRM and ROLLBACK are available only for `PENDING_VERIFY` candidates;
- FACTORY RECOVERY is disabled while already running factory;
- a `PENDING_VERIFY` boot opens the OTA view automatically.

## GitHub manifest

The device checks:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
```

No GitHub token is stored on the device. HTTPS certificate verification uses the ESP-IDF certificate bundle; insecure TLS is not enabled.

## Rollback contract

ESP-IDF v5.5.5 is built with:

```text
CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y
```

A newly installed OTA image therefore boots as `PENDING_VERIFY`. App06 intentionally keeps explicit confirm/rollback controls available for laboratory validation.

Explicit rollback is now physically proven. The serial transcript includes:

```text
esp_ota_ops: Rollback to previously worked partition.
```

followed by:

```text
boot: Defaulting to factory image
APP06_OTA: Running partition=factory version=0.1.0 state=FACTORY
```

The v0.1.2 reboot cleanup is also physically proven:

```text
APP05_NET: Wi-Fi is stopping for reboot; reconnect suppressed
```

This replaces the earlier false reconnect/AP-fallback noise seen during OTA reboot teardown.

## Persistence boundary

```text
full USB/Web-Flasher installation -> may erase NVS
app-only GitHub OTA              -> preserves NVS and stored Wi-Fi credentials
```

This boundary has been physically observed across OTA and rollback operation on the reference board. The same saved SSID reconnects automatically after update and after return to factory.

## Open hardening item: TLS allocation headroom

One `CHECK GITHUB` while v0.1.2 + LVGL was already active failed with:

```text
esp-tls-mbedtls: mbedtls_ssl_setup returned -0x7F00
esp-tls: create_ssl_handle failed
HTTP_CLIENT: Connection failed, sock < 0
```

`-0x7F00` corresponds to `MBEDTLS_ERR_SSL_ALLOC_FAILED`, so this is a real internal-RAM / largest-free-block headroom issue, not a cosmetic UI problem.

It does not invalidate the successful OTA and rollback cycles: subsequent update/install operations completed with the expected SHA-256 and booted v0.1.2 successfully. It remains the next technical hardening gate.

Next measurement should include the largest internal free block immediately before TLS setup, not only total free heap.

## Evidence

Key App06 physical evidence files:

- `evidence/app06-v0.1.0-baseline-physical-boot.md`
- `evidence/app06-v0.1.1-github-check-physical.md`
- `evidence/app06-v0.1.1-github-ota-physical-pass.md`
- `evidence/app06-v0.1.2-display-ota-physical-pass.md`

## Current classification

```text
BASELINE FACTORY BOOT                    PASS
GITHUB MANIFEST CHECK                    PASS
VERIFIED HTTPS OTA DOWNLOAD              PASS
SHA-256 / IMAGE VALIDATION               PASS
BOOT INTO OTA CANDIDATE                  PASS
NVS / SAVED WI-FI PRESERVATION           PASS
800x480 LVGL9 DISPLAY                    PASS
GT911 TOUCH                              PASS
ON-DEVICE OTA CONTROL                    PASS
EXPLICIT ROLLBACK                        PASS
RETURN TO FACTORY                        PASS
RE-INSTALL v0.1.2                        PASS
INTENTIONAL-REBOOT RECONNECT SUPPRESSION PASS
REPEATED FUNCTIONAL RUNS                 PASS
KNOWN HEADER TEXT OVERLAP                COSMETIC / DEFERRED
TLS ALLOCATION HEADROOM                  OPEN HARDENING ITEM
```
