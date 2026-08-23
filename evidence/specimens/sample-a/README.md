# Sample A specimen passport

Status: `OPEN`.

Fill after the first physical board is inspected.

## Visible markings

- Product name:
- PCB marking:
- Date/batch:
- Seller page:

## Measured identity

- Chip: ESP32-S3 QFN56
- Revision: v0.2
- Flash ID:
- Flash size: 16 MB dump captured
- PSRAM: 8 MB embedded PSRAM reported by esptool
- Crystal: 40 MHz
- MAC: 84:fc:e6:6c:69:3c
- USB bridge/native USB: CH340C source-backed, local bridge identification still to be recorded

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

First-pass partition analysis:

```text
Partition table : 0x00008000
Entries         : 5
Layout          : nvs / otadata / app0 / app1 / spiffs
App slots       : app0 0x00010000 size 0x140000, app1 0x00150000 size 0x140000
SPIFFS          : 0x00290000 size 0x170000
Strings         : 2689 printable strings >=5 chars
```

Factory runtime evidence:

```text
Serial boot       : PASS, LVGL Widgets Demo banner, Setup done
Display runtime   : PASS, lv_demo_widgets visible on 800x480 panel
Touch visual      : PASS, touchscreen interaction visible in factory demo video
Observed FPS      : about 66 FPS on demo screen
Observed CPU load : about 16-18% on demo screen
Boundary          : exact touch IC scan and exact pin-map validation still open
```

## Source-backed hardware map

Source-backed map recovered from `hardware/SCHEMATIC_BOM_RESEARCH.md` and now mirrored in `docs/pinout.md`:

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

Component identity leads:

```text
USB-UART bridge : CH340C, source-backed
Touch controller: GT911, source-backed for capacitive panel
Main module     : ESP32-S3-WROOM-1-N16R8 family, source-backed
```

## Commit-safe records

```text
evidence/specimens/sample-a/factory-firmware/factory-dump-20260822-195722.sha256.txt
evidence/specimens/sample-a/factory-firmware/runtime-serial-boot-log.md
evidence/specimens/sample-a/factory-firmware/runtime-lvgl-widgets-display-pass.md
evidence/specimens/sample-a/factory-firmware/runtime-lvgl-widgets-touch-visual-pass.md
evidence/specimens/sample-a/factory-firmware/analysis/factory-firmware-analysis-summary.md
evidence/specimens/sample-a/factory-firmware/analysis/app-identity-summary.md
evidence/specimens/sample-a/factory-firmware/analysis/hardware-leads-summary.md
hardware/SCHEMATIC_BOM_RESEARCH.md
docs/pinout.md
```

Do not commit factory `.bin` dumps.

## Acceptance status

| Stage | Target | Status |
|---|---|---|
| HW-00 | Photos and visible identity | PARTIAL |
| HW-01 | Chip / flash / PSRAM | PARTIAL |
| HW-01B | CH340C bridge identity | SOURCE-BACKED / LOCAL ID OPEN |
| HW-01C | Schematic/BOM research | SOURCE-BACKED |
| HW-01D | GPIO pin map | SOURCE-BACKED / OWN TESTS OPEN |
| FW-00 | Factory flash double-read dump | CAPTURED |
| FW-01 | Factory dump SHA-256 match | MATCH |
| FW-02 | Factory partition/string analysis | FIRST-PASS DONE |
| FW-03 | Possible factory-test leads | NOT PROVEN |
| FW-04 | Factory serial boot | PASS |
| FW-05 | Factory LVGL Widgets Demo display | PASS |
| HW-02 | RGB display | FACTORY RUNTIME PASS / OWN MINIMAL TEST OPEN |
| HW-03 | GT911 touch | SOURCE-BACKED / FACTORY TOUCH VISUAL PASS / I2C SCAN OPEN |
| HW-04 | Backlight | IMPLIED BY DISPLAY, DEDICATED TEST OPEN |
| HW-05 | SD card | SOURCE-BACKED PIN MAP / PHYSICAL TEST OPEN |
| HW-06 | Wi-Fi / BLE | OPEN |
| SW-01 | Arduino BSP BoardInfo | OPEN |
| SW-02 | LVGL basic UI | OPEN |
| SW-03 | Web setup | OPEN |
| SW-04 | Web Flasher | OPEN |
| SW-05 | GitHub OTA | OPEN |
