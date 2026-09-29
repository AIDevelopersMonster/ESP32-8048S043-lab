# App 06 - GitHub OTA, Rollback and Recovery

**Project:** KONTAKTS / ESP32-8048S043 Lab  
**Status:** PLATFORM CORE / PHYSICAL PASS LINE — OTA, rollback and Platform 0.3.9 integration validated; historical TLS-memory issue retained as hardening evidence

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

## Platform 0.3.9 / App16 physical acceptance — 2026-09-29

Platform 0.3.9 was physically validated on Sample A as the first integrated build with BLE control transport and the existing HMI, Web, SD, OTA and UART services active together.

```text
RGB display + GT911                PASS
SD library / application launcher PASS
MA01 UART1 GPIO17/18 / RS485      PASS
HMI <-> Web <-> relay state       PASS
Wi-Fi AP + STA/router             PASS
UART0/P1 + KONTAKTSerial 0.3.1    PASS
GitHub OTA CHECK                  PASS
BLE KONTAKTS-8048 advertising     PASS
BLE GATT command/response         PASS
```

BLE contract:

```text
Service FFF0
Command FFF1 WRITE
Response FFF2 READ
```

The physical acceptance also closed the runtime memory issue found during App16 bring-up. LVGL draw buffering, NimBLE host allocation and the OTA worker stack were moved away from scarce internal RAM where appropriate, while the RGB bounce buffer remains internal/DMA-capable.

Physically tested CI full-image SHA-256:

```text
20D3CD2675E49FA84E1BE6FF9DBF0C01A4D3786234138CF4511014B9B4FDBC76
```

See `release-notes-0.3.9.md` and `../16_BLEControl/README.md`.

## Platform 0.3.6 regression acceptance — 2026-09-22

The current release line was re-tested physically on Sample A by updating Platform 0.3.5 to Platform 0.3.6 through the on-device GitHub OTA flow.

```text
GitHub manifest check              PASS
0.3.6 release detection            PASS
OTA application install           PASS
reboot into OTA candidate          PASS
PENDING_VERIFY handling            PASS
saved Wi-Fi / NVS persistence      PASS
automatic STA reconnect            PASS
on-device OTA controls             PASS
rollback / confirmation workflow   PASS
post-update Platform 0.3.6         PASS
```

Classification: **PLATFORM 0.3.6 GITHUB OTA REGRESSION PHYSICAL PASS**.

Evidence record:

- `evidence/platform-v0.3.6-github-ota-regression-physical-pass.md`

Video evidence:

- https://youtube.com/shorts/70_Huyt8igQ

The 0.3.6 release is deliberately narrow and is used to exercise the complete OTA path without mixing the regression with unrelated platform changes. The older TLS allocation-headroom observation remains preserved below as historical hardening evidence rather than being deleted.

## App08 YouTube dashboard extension

The `agent/app08-youtube-dashboard` branch extends this platform runtime with YouTube Data API channel statistics, a local `/youtube` configuration/status page, NVS-stored API credentials and local daily history.

Current App08 v0.2.6 API + local web-dashboard stage is physically validated. Published video evidence:

- https://youtube.com/shorts/cjgx2RB0l_A

This video supports the YouTube API/service and local dashboard physical pass. The later App08 v0.2.8 TFT widget stage was also physically validated and is preserved in the video/evidence registry.

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

## Historical hardening evidence: TLS allocation headroom

One `CHECK GITHUB` while v0.1.2 + LVGL was already active failed with:

```text
esp-tls-mbedtls: mbedtls_ssl_setup returned -0x7F00
esp-tls: create_ssl_handle failed
HTTP_CLIENT: Connection failed, sock < 0
```

`-0x7F00` corresponds to `MBEDTLS_ERR_SSL_ALLOC_FAILED`, so this is a real internal-RAM / largest-free-block headroom issue, not a cosmetic UI problem.

It did not invalidate the successful OTA and rollback cycles. In the later Platform 0.3.9 acceptance, GitHub OTA CHECK passed while BLE, Wi-Fi, display/LVGL, SD and MA01 services were active after the memory-placement fixes. The old failure remains useful evidence for why internal largest-free-block monitoring and explicit PSRAM ownership matter. Long-duration/repeated TLS stress remains a hardening test, not an unresolved blocker for the accepted 0.3.9 platform.

## Evidence

Key App06 physical evidence files:

- `evidence/app06-v0.1.0-baseline-physical-boot.md`
- `evidence/app06-v0.1.1-github-check-physical.md`
- `evidence/app06-v0.1.1-github-ota-physical-pass.md`
- `evidence/app06-v0.1.2-display-ota-physical-pass.md`
- `../../evidence/app08-youtube-dashboard-v0.2.6-api-physical-pass.md`

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
TLS ALLOCATION HEADROOM                  HISTORICAL ISSUE / 0.3.9 CHECK PASS
APP08 YOUTUBE API + LOCAL WEB DASHBOARD  PHYSICAL PASS
APP08 TFT YOUTUBE WIDGET                  PHYSICAL PASS
PLATFORM 0.3.6 OTA REGRESSION             PHYSICAL PASS
```
