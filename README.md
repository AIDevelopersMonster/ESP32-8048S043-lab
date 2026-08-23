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
PIN MAP                     SOURCE-BACKED / OWN BSP TESTS OPEN
FACTORY FIRMWARE DUMP       DOUBLE-READ MATCH
FACTORY FIRMWARE ANALYSIS   FIRST-PASS DONE
FACTORY SERIAL BOOT         PASS
FACTORY LVGL DISPLAY        PASS
TOUCHSCREEN VISUAL CHECK    FACTORY DEMO PASS
DISPLAY RGB PANEL           FACTORY RUNTIME PASS
GT911 / GOODIX IDENTITY     SOURCE-BACKED / I2C SCAN OPEN
BSP LIBRARY                 SKELETON
WEB FLASHER                 SKELETON
PHYSICAL PASS CLAIMS        SAMPLE A FACTORY LVGL DISPLAY + TOUCH VISUAL + BOARDINFO
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

## Source-backed hardware baseline

Manufacturer/distributor documentation and a same-layout board reference now provide a source-backed reconstruction for the main hardware map:

```text
USB-UART bridge : CH340C
Touch           : GT911 capacitive touch, visual runtime PASS, dedicated I2C scan still open
RGB LCD         : 800x480 RGB parallel panel, factory LVGL display runtime PASS
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

This map is **source-backed**, not yet a complete BSP PASS. The next step is to validate it with our own minimal RGB, touch and SD examples.

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
3. **Keep examples incremental.** Factory dump -> BoardInfo -> display -> touch -> backlight -> LVGL -> Web/OTA.
4. **No hidden negative branches.** Failed experiments may be kept as history, but current README status must reflect the current validated result.
5. **No PASS without evidence.** A working video/log/photo must name the specimen and firmware.

## Start here

- [`docs/HARDWARE-ACCEPTANCE-START.md`](docs/HARDWARE-ACCEPTANCE-START.md) — first-board acceptance workflow.
- [`hardware/SCHEMATIC_BOM_RESEARCH.md`](hardware/SCHEMATIC_BOM_RESEARCH.md) — source-backed schematic/BOM/pinout reconstruction.
- [`docs/firmware/README.md`](docs/firmware/README.md) — factory and third-party firmware preservation/analysis rules.
- [`docs/firmware/reproducible-factory-dump-and-analysis.md`](docs/firmware/reproducible-factory-dump-and-analysis.md) — reader-facing reproduction guide for factory dump and analysis.
- [`docs/pinout.md`](docs/pinout.md) — source-backed and future verified pin map.
- [`docs/videos.md`](docs/videos.md) — shooting plan and evidence links.
- [`docs/third-party/README.md`](docs/third-party/README.md) — reference projects and firmware to study without copying blindly.
- [`evidence/specimens/sample-a/README.md`](evidence/specimens/sample-a/README.md) — first specimen evidence folder.
- [`libraries/ESP32_8048S043/README.md`](libraries/ESP32_8048S043/README.md) — Arduino BSP skeleton and example plan.

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
