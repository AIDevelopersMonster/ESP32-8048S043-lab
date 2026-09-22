# App06 v0.1.2 - on-device OTA display design

Status: SOURCE / CI TARGET - PHYSICAL VALIDATION REQUIRED

## Why this change exists

The physically validated WT32-SC01-PLUS Example 20 established the useful UX pattern: GitHub OTA can be checked and initiated directly from the touch display, with installed/available version, state and progress visible locally.

App06 v0.1.2 ports that pattern to ESP32-8048S043 while keeping the already validated App06 ESP-IDF OTA manager and HTTPS/manifest/SHA-256 path.

The display driver itself is not ported from the Arduino WT32 implementation. It reuses the physically validated ESP32-8048S043 App03 baseline instead:

- ESP-IDF 5.5.x;
- LVGL 9.3.0;
- 800x480 RGB565;
- PCLK 16 MHz;
- one PSRAM framebuffer;
- 10-line RGB bounce buffer;
- 60-line LVGL partial buffer in internal RAM;
- modern ESP-IDF I2C master;
- GT911 touch;
- 16 KiB UI task pinned to core 1.

## Display UI

The local HMI has two top-level tabs:

```text
STATUS
  firmware version
  network state
  STA IP
  running partition
  image state
  AP provisioning hint when needed

OTA
  installed / available versions
  OTA state + image state
  progress bar
  OTA manager message
  CHECK GITHUB
  INSTALL UPDATE
  CONFIRM
  ROLLBACK
  FACTORY RECOVERY
```

Rules:

- CHECK and INSTALL are enabled only while STA is online;
- INSTALL is enabled only after a newer manifest is known;
- CONFIRM and ROLLBACK are enabled only for `PENDING_VERIFY`;
- FACTORY RECOVERY is disabled while already running the factory partition;
- on a `PENDING_VERIFY` boot the display opens the OTA tab automatically.

## v0.1.2 secondary cleanup

The network reconnect path now treats `ESP_ERR_WIFI_NOT_STARTED` / `ESP_ERR_WIFI_STOP_STATE` during the final reboot teardown as an intentional stop boundary rather than escalating into AP fallback errors.

## Physical acceptance target

Before installing v0.1.2, v0.1.1 must be explicitly confirmed `VALID`.

Then:

```text
ota_0 0.1.1 VALID
  -> CHECK 0.1.2
  -> INSTALL
  -> ota_1 0.1.2 PENDING_VERIFY
  -> display + GT911 touch remain operational
  -> saved Wi-Fi reconnects
  -> ROLLBACK from the display
  -> ota_0 0.1.1 boots again
```

This sequence is also the first controlled physical validation of App06 rollback. No PASS is claimed here until reproduced on Sample A.
