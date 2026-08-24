# ESP32-8048S043 Lab

Practical hardware/software laboratory for the **ESP32-8048S043 / ESP32-8048S043C-I** family of ESP32-S3 4.3-inch 800x480 display modules.

This repository is intentionally evidence-first. It is **not** a generic pinout dump and it does **not** promote vendor/community claims to PASS until they are reproduced on a named physical specimen.

## Current status

```text
REPOSITORY STRUCTURE        CREATED
REFERENCE BOARD PASSPORT    OPEN
CHIP / FLASH / PSRAM        PASS
ARDUINO BOARDINFO           PASS
SCHEMATIC / BOM RESEARCH    SOURCE-BACKED
PIN MAP                     SOURCE-BACKED / RGB OWN TEST PASS / GT911 OWN TOUCH PASS
FACTORY FIRMWARE DUMP       DOUBLE-READ MATCH
FACTORY FIRMWARE ANALYSIS   FIRST-PASS DONE
FACTORY SERIAL BOOT         PASS
FACTORY LVGL DISPLAY        PASS
TOUCHSCREEN VISUAL CHECK    FACTORY DEMO PASS + OWN GT911 VISUAL PASS
DISPLAY RGB PANEL           OWN MINIMAL ARDUINO_GFX TEST PASS
GT911 / GOODIX IDENTITY     OWN I2C POLLING TEST PASS / 0x5D / PRODUCT ID 911
ARDUINO BOARD PROFILE       DESIGN / SAMPLE A METADATA ADDED / INSTALLER OPEN
BSP LIBRARY                 SKELETON / 01-05 IMPLEMENTED / 01-03 PHYSICAL PASS
WEB FLASHER                 SKELETON
PHYSICAL PASS CLAIMS        SAMPLE A FACTORY LVGL DISPLAY + TOUCH VISUAL + BOARDINFO + RGB DISPLAY + OWN GT911 TOUCH
```

## Factory firmware baseline

```text
Specimen    : Sample A
Dump time   : 20260822-195722
Read size   : 16 MB / 0x01000000
Reads       : 2
SHA-256     : 3007E5A223CD70DD9E53746C899BA25AF24721C68F1CFC69AB8A8CE3D3E6EB4C
Result      : MATCH all reads are identical
Analysis    : partition table found at 0x00008000, 5 entries
Runtime     : serial boot PASS, factory LVGL Widgets Demo display + touch visual PASS
```

Factory partition layout:

```text
nvs      0x00009000  size 0x5000
otadata  0x0000E000  size 0x2000
app0     0x00010000  size 0x140000
app1     0x00150000  size 0x140000
spiffs   0x00290000  size 0x170000
```

The factory `.bin` dump is intentionally not committed. Only metadata, hashes and reviewed analysis outputs are tracked.

## Arduino BoardInfo baseline

`01_BoardInfo` now runs from the Arduino IDE examples menu and records the first Arduino runtime PASS for Sample A.

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

Video evidence:

```text
https://youtube.com/shorts/wELRdRWqlnw
```

Note:

```text
An earlier run reported PSRAM as 0 bytes. A later run, after changing the Arduino PSRAM type/profile and rebuilding, detected 8388608 bytes / 8 MB and stayed alive with freePsram reported in the ALIVE lines. The later runtime report is the current PASS evidence.
```

## Arduino RGB display baseline

`02_DisplayRGBTest` is the first own minimal RGB display validation from the local `ESP32_8048S043` Arduino library.

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

## Arduino GT911 touch baseline

`03_TouchGT911Test` is the first own GT911 touch validation from the local `ESP32_8048S043` Arduino library.

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

Boundary:

```text
This PASS confirms the own low-level GT911/I2C touch path and basic visual marker movement. Final LVGL touch integration, calibration, rotation and gestures remain separate tests.
```

## Experimental Arduino board profile baseline

The project now has a separate board-profile layer in addition to the runtime Arduino library.

Machine-readable Sample A profile:

```text
config/board_profiles/esp32-8048s043-lab-sample-a.json
```

Human-readable design note:

```text
docs/arduino-board-profile.md
```

Experimental Arduino IDE kit staging area:

```text
boards/arduino-ide/esp32-8048s043-lab/
```

Current boundary:

```text
The board-profile layer documents ESP32-S3 core assumptions, 16 MB flash, 8 MB OPI PSRAM, the factory partition layout, the working Arduino IDE menu profile, compile-time macro targets and the external interface contour.

It is not yet a supported Arduino Boards Manager package. Until compile/upload evidence exists for the custom target, continue using ESP32S3 Dev Module with the documented Sample A settings.
```

## Source-backed hardware baseline

Manufacturer/distributor documentation and a same-layout board reference now provide a source-backed reconstruction for the main hardware map:

```text
USB-UART bridge : CH340C
Touch           : GT911 capacitive touch, factory visual runtime PASS and own I2C polling visual PASS
RGB LCD         : 800x480 RGB parallel panel, factory LVGL display runtime PASS and own Arduino_GFX minimal test PASS
SD / TF1        : SPI SD wiring recovered, own SD test still open
```

Recovered source-backed GPIO map:

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

This map is now **source-backed** with **own RGB display runtime PASS** and **own GT911 touch runtime PASS**. SD and the final BSP still require separate validation.

## Target family

This lab is intended for boards sold under names similar to:

```text
ESP32-8048S043
ESP32-8048S043C
ESP32-8048S043C-I
4.3 inch 800x480 ESP32-S3 capacitive touch display
```

Important: board labels, sellers and OEM revisions may differ. Treat every new board as a specimen until proven compatible.

## Repository philosophy

1. **Identify before flashing.** First capture photos, chip identity, flash size, PSRAM and the factory firmware dump.
2. **Separate reported from verified.** Vendor/community pin maps live in docs until tested.
3. **Keep examples incremental.** Factory dump -> BoardInfo -> display -> touch -> backlight -> console -> board profile -> LVGL -> Web/OTA.
4. **No hidden negative branches.** Failed experiments may be kept as history, but current README status must reflect the current validated result.
5. **No PASS without evidence.** A working video/log/photo must name the specimen and firmware.

## Start here

- [`docs/HARDWARE-ACCEPTANCE-START.md`](docs/HARDWARE-ACCEPTANCE-START.md) — first-board acceptance workflow.
- [`hardware/SCHEMATIC_BOM_RESEARCH.md`](hardware/SCHEMATIC_BOM_RESEARCH.md) — source-backed schematic/BOM/pinout reconstruction.
- [`docs/firmware/README.md`](docs/firmware/README.md) — factory and third-party firmware preservation/analysis rules.
- [`docs/firmware/reproducible-factory-dump-and-analysis.md`](docs/firmware/reproducible-factory-dump-and-analysis.md) — reader-facing reproduction guide for factory dump and analysis.
- [`docs/pinout.md`](docs/pinout.md) — source-backed and future verified pin map.
- [`docs/arduino-board-profile.md`](docs/arduino-board-profile.md) — experimental Arduino board-profile layer.
- [`docs/videos.md`](docs/videos.md) — shooting plan and evidence links.
- [`docs/third-party/README.md`](docs/third-party/README.md) — reference projects and firmware to study without copying blindly.
- [`evidence/specimens/sample-a/README.md`](evidence/specimens/sample-a/README.md) — first specimen evidence folder.
- [`libraries/ESP32_8048S043/README.md`](libraries/ESP32_8048S043/README.md) — Arduino BSP skeleton and example plan.
- [`config/board_profiles/esp32-8048s043-lab-sample-a.json`](config/board_profiles/esp32-8048s043-lab-sample-a.json) — machine-readable Sample A board profile.

## Planned architecture

```text
Factory firmware preservation
        ↓
USB / browser first lab install
        ↓
Arduino BSP for verified board profile
        ↓
RGB display + GT911 touch + backlight
        ↓
Experimental Arduino board profile
        ↓
LVGL local HMI shell
        ↓
Web setup / upload / logs
        ↓
Widget runtime and GitHub OTA only after stable partition layout
```

## Structure

```text
ESP32-8048S043-lab/
├── .github/                 # issue templates and CI workflows
├── boards/                  # experimental Arduino IDE / future board package files
├── config/board_profiles/   # machine-readable board/specimen profiles
├── docs/                    # hardware, software, firmware, variants, research, videos
├── evidence/                # named specimen evidence only
├── hardware/images/         # raw and annotated own photos
├── libraries/               # Arduino BSP and examples
├── tools/                   # host-side audit and packaging tools
├── web-flasher/             # future ESP Web Tools site
├── releases/                # release protocol notes
└── README.md
```

## License

Code and original text are intended for release under the MIT License. Third-party photos, firmware, vendor files and schematics retain their original licenses and must not be copied here unless redistribution is permitted.
