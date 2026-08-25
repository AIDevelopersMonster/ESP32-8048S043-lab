# Experimental Arduino board profile layer

Status: `LOCAL SKETCHBOOK PROFILE PASS CANDIDATE / SAMPLE A / BOARD MANAGER OPEN`.

This document describes the board-profile BSP layer for the ESP32-8048S043 Lab project.

The existing `ESP32_8048S043` Arduino library validates and exposes board functions:

```text
RGB display;
GT911 touch;
backlight;
SD pin map;
examples;
evidence workflow.
```

The board-profile layer describes how Arduino IDE / Arduino CLI should build for this exact board family:

```text
ESP32-S3 target;
16 MB flash;
8 MB OPI PSRAM;
QIO 80 MHz flash mode;
known-good partition profile;
CH340 / UART upload workflow;
serial monitor defaults;
compile-time macros for board-aware examples;
variant-level aliases for standard buses;
external interface contour.
```

## Current local implementation

The validated local implementation is a separate Arduino sketchbook hardware platform:

```text
Documents/Arduino/hardware/AIDevelopersMonster/esp32
```

Working board profile:

```text
AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

This approach does not modify the installed Espressif Arduino-ESP32 package under `Arduino15`.

Validated `01_BoardInfo` result on Sample A:

```text
Chip                    : ESP32-S3 rev 2
Flash                   : 16777216 bytes / 16 MB / QIO 80 MHz
PSRAM                   : 8388608 bytes / 8 MB / OPI PSRAM
Running app partition   : app0, address 0x010000, size 3145728
Upload path             : COM12 / CH340 / UART0 workflow
Runtime stability       : ALIVE lines observed with freePsram available
```

Commit-safe runtime record:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
```

## Why this layer exists

Using the generic Arduino board target is useful during bring-up:

```text
ESP32S3 Dev Module
```

But it leaves too many important properties in the operator's hands:

```text
Flash Size;
PSRAM mode;
Flash mode and speed;
partition scheme;
USB CDC behavior;
upload mode;
board-specific pin aliases;
project-specific compile macros.
```

A board profile reduces this to a project-defined target:

```text
ESP32-8048S043 Lab / ESP32-S3 N16R8 / RGB 800x480 / GT911
```

## Boundary between board profile and library

The board profile does not replace the library.

```text
Board profile:
  build target;
  memory assumptions;
  upload assumptions;
  partition assumptions;
  variant-level standard bus aliases;
  board macros;
  external-interface map.

ESP32_8048S043 library:
  RGB panel initialization;
  GT911 polling and coordinate mapping;
  backlight control;
  SD tests;
  LVGL glue;
  examples;
  runtime diagnostics.
```

## Current Sample A profile

Machine-readable profile:

```text
config/board_profiles/esp32-8048s043-lab-sample-a.json
```

This profile captures:

```text
ESP32-S3 rev 2;
16 MB flash;
8 MB OPI PSRAM;
factory dump SHA-256;
factory partition table;
Arduino IDE working menu profile;
RGB display GPIO map;
GT911 I2C/register map;
backlight GPIO2;
TF1/microSD SPI map;
external visible interface contour.
```

## Memory and build information exposed by the profile

The local board profile target is expected to expose board-aware macros such as:

```cpp
ARDUINO_ESP32_8048S043_LAB
ESP32_8048S043_HAS_RGB_PANEL=1
ESP32_8048S043_HAS_GT911=1
ESP32_8048S043_LCD_WIDTH=800
ESP32_8048S043_LCD_HEIGHT=480
ESP32_8048S043_TOUCH_GT911=1
```

The observed working memory model is:

```text
Flash: 16 MB / 128 Mb
PSRAM: 8 MB / OPI PSRAM
CPU:   ESP32-S3, 240 MHz
Upload: CH340 / UART workflow, 921600 preferred, 460800 fallback
Serial monitor: 115200 baud
App slot: 3 MB
```

## External interface contour

For Sample A, the first-pass known contour is:

| Interface | Purpose | Pins / signals | Status |
|---|---|---|---|
| USB-UART / CH340C | upload and serial monitor | UART upload path | source-backed / used in workflow |
| RGB LCD FPC | built-in 800x480 display | RGB parallel GPIO map | own display PASS |
| GT911 capacitive touch | built-in touch panel | SDA19, SCL20, RST38, INT18 | own touch PASS |
| Backlight | display backlight | GPIO2 | reported working / evidence record pending |
| TF1 / microSD | external storage socket | CS10, MOSI11, CLK12, MISO13 | source-backed / physical test open |

Still open:

```text
all exposed side pads / solder pads;
optional UART/I2C/GPIO breakout pins if present;
power-domain limits for external loads;
continuity map from ESP32-S3 GPIO to every external pad.
```

## Why not publish a Board Manager package immediately

The local sketchbook hardware profile is validated only as a local development profile. A full Arduino Boards Manager package requires a stable package index, archive, versioning and compatibility tracking across Arduino-ESP32 core versions.

Until then, the project should keep two supported paths:

```text
1. safe fallback: ESP32S3 Dev Module + documented Sample A settings;
2. local experimental: AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8.
```

## Rule

Do not claim that a public custom board package is supported until it compiles, uploads and runs the full validation chain on Sample A and at least one additional specimen. The current claim is narrower: local sketchbook board profile PASS candidate for `01_BoardInfo` on Sample A.
