# Sample A specimen passport

Status: `ACTIVE / MULTI-SUBSYSTEM VALIDATED`.

This folder contains commit-safe evidence for the first physical ESP32-8048S043 specimen used in the lab.

## Measured identity

```text
Chip              : ESP32-S3, rev v0.2
Flash             : 16 MB, Arduino runtime and factory dump confirmed
PSRAM             : 8 MB OPI PSRAM, Arduino runtime confirmed
CPU               : 240 MHz in Arduino validation profile
Factory dump      : 16 MB, double-read SHA-256 match
Wi-Fi STA MAC     : 84:FC:E6:6C:69:3C
BLE local address : 84:fc:e6:6c:69:3d
```

USB bridge / native USB boundary:

```text
CH340C is source-backed; local bridge identity still needs a dedicated local record.
```

## Factory firmware preservation

Local evidence folder:

```text
evidence/specimens/sample-a/factory-firmware/
```

Factory firmware dump status:

```text
Timestamp   : 20260822-195722
Read size   : 16 MB / 0x01000000
Reads       : 2
SHA-256     : 3007E5A223CD70DD9E53746C899BA25AF24721C68F1CFC69AB8A8CE3D3E6EB4C
Result      : MATCH all reads are identical
```

Factory runtime evidence:

```text
Serial boot       : PASS, LVGL Widgets Demo banner, Setup done
Display runtime   : PASS, lv_demo_widgets visible on 800x480 panel
Touch visual      : PASS, touchscreen interaction visible in factory demo video
Observed FPS      : about 66 FPS on demo screen
Observed CPU load : about 16-18% on demo screen
```

## Arduino evidence summary

| Example | Target | Current Sample A status |
|---|---|---|
| 01_BoardInfo | board profile, chip, flash, PSRAM, partition, ALIVE | PHYSICAL PASS / PROFILE DIAGNOSTIC V2 |
| 02_DisplayRGBTest | RGB display via Arduino_GFX | PHYSICAL VISUAL PASS |
| 03_TouchGT911Test | GT911 touch polling and visual marker | PHYSICAL VISUAL PASS |
| 04_BacklightTest | GPIO2 backlight ON/OFF/PWM | PHYSICAL PASS REPORTED |
| 05_TestConsole | RGB + GT911 + backlight combined console | PHYSICAL INTEGRATION PASS REPORTED |
| 06_WiFiTest | Wi-Fi scan, association, DHCP, DNS, TCP/HTTP, reconnect | FULL WIFI PHYSICAL PASS CANDIDATE |
| 07_WebServerTest | browser HTTP server, status JSON and ping | WEB SERVER PHYSICAL PASS CANDIDATE |
| 08_SDCardTest | read-only SD mount, metadata and directory listing | READ-ONLY SD PHYSICAL PASS CANDIDATE |
| 09_BLETest | Arduino BLE init, active scan and advertisement receive | BLE SCAN PHYSICAL PASS CANDIDATE |

## Commit-safe Arduino records

```text
evidence/specimens/sample-a/arduino/01-boardinfo-20260823.md
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
evidence/specimens/sample-a/arduino/01-boardinfo-esp32s3-devmodule-opi-reference-20260825.md
evidence/specimens/sample-a/arduino/02-display-rgbtest-20260823.md
evidence/specimens/sample-a/arduino/03-touch-gt911-20260823.md
evidence/specimens/sample-a/arduino/06-wifi-scan-20260825.md
evidence/specimens/sample-a/arduino/06-wifi-full-infrastructure-20260825.md
evidence/specimens/sample-a/arduino/07-webserver-sta-20260825.md
evidence/specimens/sample-a/arduino/08-sdcard-readonly-20260826.md
evidence/specimens/sample-a/arduino/09-ble-scan-20260826.md
```

## Source-backed and tested hardware map

```text
LCD DE        40
LCD VSYNC     41
LCD HSYNC     39
LCD PCLK      42
Backlight PWM 2
GT911 SDA/SCL 19 / 20
GT911 RESET   38
GT911 INT     18, optional / link-dependent
SD CS/MOSI/CLK/MISO 10 / 11 / 12 / 13
RGB R0..R4    45, 48, 47, 21, 14
RGB G0..G5    5, 6, 7, 15, 16, 4
RGB B0..B4    8, 3, 46, 9, 1
```

Current validation boundary:

```text
RGB display      : own Arduino_GFX runtime PASS
GT911 touch      : own I2C/Product ID/polling/visual marker PASS
Backlight        : reported dedicated pass and exercised by display tests
SD / TF          : read-only mount + metadata + listing PASS candidate
Wi-Fi            : scan + infrastructure PASS candidate
BLE              : Arduino BLE active scan + advertisement receive PASS candidate
```

## Acceptance status

| Stage | Target | Status |
|---|---|---|
| HW-00 | Photos and visible identity | PARTIAL |
| HW-01 | Chip / flash / PSRAM | PASS |
| HW-01B | CH340C bridge identity | SOURCE-BACKED / LOCAL ID OPEN |
| HW-01C | Schematic/BOM research | SOURCE-BACKED |
| HW-01D | GPIO pin map | SOURCE-BACKED / RGB + GT911 + SD READ-ONLY TESTED |
| FW-00 | Factory flash double-read dump | CAPTURED |
| FW-01 | Factory dump SHA-256 match | MATCH |
| FW-02 | Factory partition/string analysis | FIRST-PASS DONE |
| FW-04 | Factory serial boot | PASS |
| FW-05 | Factory LVGL Widgets Demo display/touch | PASS |
| HW-02 | RGB display | OWN MINIMAL ARDUINO_GFX TEST PASS |
| HW-03 | GT911 touch | PHYSICAL VISUAL PASS / 0x5D / PRODUCT ID 911 |
| HW-04 | Backlight | PHYSICAL PASS REPORTED |
| HW-05 | SD card | READ-ONLY PHYSICAL PASS CANDIDATE |
| HW-06 | Wi-Fi | FULL WIFI PHYSICAL PASS CANDIDATE |
| HW-07 | BLE | BLE SCAN PHYSICAL PASS CANDIDATE |
| SW-01 | Arduino BSP BoardInfo | PASS |
| SW-01A | Local Arduino board profile | PASS CANDIDATE / SAMPLE A |
| SW-02 | LVGL basic UI | OPEN |
| SW-03 | Web setup | OPEN |
| SW-04 | Web Flasher | OPEN |
| SW-05 | GitHub OTA | OPEN |

## Boundary

Do not commit factory `.bin` dumps.

Do not promote beyond the exact evidence boundary:

```text
BLE scan evidence does not prove pairing, GATT, HID, provisioning or Wi-Fi/BLE coexistence.
SD read-only evidence does not prove write safety, formatting, SD stress or SD-backed Web/Widget storage.
WebServer evidence does not yet prove Web setup, OTA or Widget Runtime.
```
