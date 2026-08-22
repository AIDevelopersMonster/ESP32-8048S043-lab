# ESP32-8048S043 Lab

Practical hardware/software laboratory for the **ESP32-8048S043 / ESP32-8048S043C-I** family of ESP32-S3 4.3-inch 800x480 display modules.

This repository is intentionally evidence-first. It is **not** a generic pinout dump and it does **not** promote vendor/community claims to PASS until they are reproduced on a named physical specimen.

## Current status

```text
REPOSITORY STRUCTURE        CREATED
REFERENCE BOARD PASSPORT    OPEN
CHIP / FLASH / PSRAM        OPEN
DISPLAY RGB PANEL           OPEN
GT911 TOUCH                 OPEN
BSP LIBRARY                 SKELETON
WEB FLASHER                 SKELETON
PHYSICAL PASS CLAIMS        NONE YET
```

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

1. **Identify before flashing.** First capture photos, chip identity, flash size and PSRAM.
2. **Separate reported from verified.** Vendor/community pin maps live in docs until tested.
3. **Keep examples incremental.** BoardInfo -> display -> touch -> backlight -> LVGL -> Web/OTA.
4. **No hidden negative branches.** Failed experiments may be kept as history, but current README status must reflect the current validated result.
5. **No PASS without evidence.** A working video/log/photo must name the specimen and firmware.

## Start here

- [`docs/HARDWARE-ACCEPTANCE-START.md`](docs/HARDWARE-ACCEPTANCE-START.md) — first-board acceptance workflow.
- [`docs/pinout.md`](docs/pinout.md) — reported and future verified pin map.
- [`docs/videos.md`](docs/videos.md) — shooting plan and future evidence links.
- [`docs/third-party/README.md`](docs/third-party/README.md) — reference projects to study without copying blindly.
- [`evidence/specimens/sample-a/README.md`](evidence/specimens/sample-a/README.md) — first specimen evidence folder.
- [`libraries/ESP32_8048S043/README.md`](libraries/ESP32_8048S043/README.md) — Arduino BSP skeleton and example plan.

## Planned architecture

```text
USB / browser first install
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
├── docs/                    # hardware, software, variants, research, videos
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
