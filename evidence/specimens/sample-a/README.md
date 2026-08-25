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
- Flash size: 16 MB dump captured; Arduino runtime also reports 16 MB
- PSRAM: 8 MB detected by Arduino `01_BoardInfo` runtime report and local board-profile run
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
Boundary          : own GT911 test now confirms address/Product ID/polling separately
```

## Arduino runtime evidence

`01_BoardInfo` was run from Arduino IDE with the Sample A settings captured in the example README.

Current result:

```text
Upload / serial monitor : PASS
Chip                    : PASS, ESP32-S3 rev 2, 2 cores, 240 MHz
Flash size              : PASS, 16777216 bytes / 16 MB
Flash mode / speed      : QIO / 80 MHz
PSRAM                   : PASS, 8388608 bytes / 8 MB
Running partition       : app0, address 0x010000, size 3145728
Runtime stability       : PASS, ALIVE lines observed with PSRAM available
Overall 01_BoardInfo    : PASS
```

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-20260823.md
```

Local board-profile runtime record:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
```

Local board-profile result:

```text
Board target            : AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
Chip                    : PASS, ESP32-S3 rev 2
Flash                   : PASS, 16777216 bytes / 16 MB / QIO 80 MHz
PSRAM                   : PASS, 8388608 bytes / 8 MB / OPI PSRAM
Running partition       : PASS, app0 size 3145728 bytes
Runtime stability       : PASS candidate, ALIVE lines observed with freePsram available
```

Note:

```text
An earlier run reported PSRAM as 0 bytes. A later run detected 8388608 bytes / 8 MB and stayed alive with freePsram reported in the ALIVE lines. The later runtime report is the current PASS evidence.
```

## Arduino RGB display evidence

`02_DisplayRGBTest` was run after `01_BoardInfo` and visually passed on Sample A.

Current result:

```text
Display begin           : PASS, serial reports Display begin: OK
RGB control/data pins   : PASS candidate, source-backed map exercised by own sketch
Backlight               : PASS candidate, GPIO2 full ON used by own sketch
Color sequence          : PASS, RED/GREEN/BLUE/WHITE/BLACK shown visually
Orientation frame       : PASS, landscape 800x480 visual check passed
RGB color bars          : PASS, visual color-bar check passed
Stripe pattern          : PASS, data-line sanity pattern visible
Overall 02_DisplayRGBTest: PHYSICAL VISUAL PASS / SAMPLE A
```

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/02-display-rgbtest-20260823.md
```

## Arduino GT911 touch evidence

`03_TouchGT911Test` was run after `01_BoardInfo` and `02_DisplayRGBTest` and visually passed on Sample A.

Current result:

```text
Display init through Arduino_GFX : PASS, gfx->begin(): OK
GT911 I2C address                : PASS, 0x5D
GT911 Product ID                 : PASS, 911
GT911 firmware register          : PASS, 0x1060
GT911 point polling              : PASS, 0x814E status / 0x814F point data
Raw coordinate changes           : PASS
Mapped screen coordinates        : PASS candidate
Visible red marker movement      : PASS by video evidence
Overall 03_TouchGT911Test        : PHYSICAL VISUAL PASS / SAMPLE A
```

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/03-touch-gt911-20260823.md
```

Video evidence:

```text
https://youtube.com/shorts/_zhtl-AWcCE
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
Touch controller: GT911, source-backed and own I2C/Product ID test PASS on Sample A
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
evidence/specimens/sample-a/arduino/01-boardinfo-20260823.md
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
evidence/specimens/sample-a/arduino/02-display-rgbtest-20260823.md
evidence/specimens/sample-a/arduino/03-touch-gt911-20260823.md
hardware/SCHEMATIC_BOM_RESEARCH.md
docs/pinout.md
docs/videos.md
```

Do not commit factory `.bin` dumps.

## Acceptance status

| Stage | Target | Status |
|---|---|---|
| HW-00 | Photos and visible identity | PARTIAL |
| HW-01 | Chip / flash / PSRAM | PASS |
| HW-01B | CH340C bridge identity | SOURCE-BACKED / LOCAL ID OPEN |
| HW-01C | Schematic/BOM research | SOURCE-BACKED |
| HW-01D | GPIO pin map | SOURCE-BACKED / RGB OWN TEST PASS / GT911 OWN TOUCH PASS / SD OPEN |
| FW-00 | Factory flash double-read dump | CAPTURED |
| FW-01 | Factory dump SHA-256 match | MATCH |
| FW-02 | Factory partition/string analysis | FIRST-PASS DONE |
| FW-03 | Possible factory-test leads | NOT PROVEN |
| FW-04 | Factory serial boot | PASS |
| FW-05 | Factory LVGL Widgets Demo display | PASS |
| HW-02 | RGB display | OWN MINIMAL ARDUINO_GFX TEST PASS |
| HW-03 | GT911 touch | PHYSICAL VISUAL PASS / 0x5D / PRODUCT ID 911 / OWN POLLING TEST PASS |
| HW-04 | Backlight | BASIC ON PASS VIA DISPLAY TEST / DEDICATED PWM TEST OPEN |
| HW-05 | SD card | SOURCE-BACKED PIN MAP / PHYSICAL TEST OPEN |
| HW-06 | Wi-Fi / BLE | OPEN |
| SW-01 | Arduino BSP BoardInfo | PASS |
| SW-01A | Local Arduino board profile | PASS CANDIDATE / SAMPLE A / 01_BOARDINFO |
| SW-02 | LVGL basic UI | OPEN |
| SW-03 | Web setup | OPEN |
| SW-04 | Web Flasher | OPEN |
| SW-05 | GitHub OTA | OPEN |
