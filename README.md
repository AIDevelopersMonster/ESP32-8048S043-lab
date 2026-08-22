# ESP32-8048S043 Lab

Practical hardware/software laboratory for the **ESP32-8048S043 / ESP32-8048S043C-I** family of ESP32-S3 4.3-inch 800x480 display modules.

This repository is intentionally evidence-first. It is **not** a generic pinout dump and it does **not** promote vendor/community claims to PASS until they are reproduced on a named physical specimen.

## Current status

```text
REPOSITORY STRUCTURE        CREATED
REFERENCE BOARD PASSPORT    OPEN
CHIP / FLASH / PSRAM        PARTIAL
FACTORY FIRMWARE DUMP       DOUBLE-READ MATCH
FACTORY FIRMWARE ANALYSIS   FIRST-PASS DONE
DISPLAY RGB PANEL           OPEN
GT911 TOUCH                 OPEN
BSP LIBRARY                 SKELETON
WEB FLASHER                 SKELETON
PHYSICAL PASS CLAIMS        NONE YET
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
- [`docs/firmware/README.md`](docs/firmware/README.md) — factory and third-party firmware preservation/analysis rules.
- [`docs/firmware/reproducible-factory-dump-and-analysis.md`](docs/firmware/reproducible-factory-dump-and-analysis.md) — reader-facing reproduction guide for factory dump and analysis.
- [`docs/pinout.md`](docs/pinout.md) — reported and future verified pin map.
- [`docs/videos.md`](docs/videos.md) — shooting plan and future evidence links.
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
