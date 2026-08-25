# Experimental Arduino board profile layer

Status: `LOCAL SKETCHBOOK PROFILE 01-05 PASS CANDIDATE / SAMPLE A / BOARD MANAGER OPEN`.

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

## Reader-facing setup guide

The standalone local setup guide is:

```text
boards/arduino-ide/esp32-8048s043-lab/LOCAL_PLATFORM_SETUP.md
```

Use it as the primary reference for reproducing the local Arduino hardware platform.

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

Validated 01-05 result on Sample A:

```text
01_BoardInfo       PASS, ESP32-S3 / 16 MB flash / 8 MB PSRAM / 3 MB app partition
02_DisplayRGBTest  PASS, Arduino_GFX RGB display path
03_TouchGT911Test  PASS, GT911 at 0x5D / Product ID 911 / firmware 0x1060
04_BacklightTest   PASS reported, GPIO2 blink and PWM duty stepping observed
05_TestConsole     PASS, combined RGB + GT911 + backlight console and touch events
```

Important memory boundary:

```text
01_BoardInfo is the current PSRAM acceptance test. The 05_TestConsole run validated RGB/touch/backlight integration but printed PSRAM as 0 bytes, so the console memory line must be rechecked separately.
```

Commit-safe runtime records:

```text
evidence/specimens/sample-a/arduino/01-boardinfo-local-board-profile-20260825.md
evidence/specimens/sample-a/arduino/local-board-profile-01-05-validation-20260825.md
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
  external-interface map;
  platform.local.txt overrides needed by third-party libraries.

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

The observed working memory model from `01_BoardInfo` is:

```text
Flash: 16 MB / 128 Mb
PSRAM: 8 MB / OPI PSRAM
CPU:   ESP32-S3, 240 MHz
Upload: CH340 / UART workflow, 921600 preferred, 460800 fallback
Serial monitor: 115200 baud
App slot: 3 MB
```

## Arduino_GFX target macro issue

The local profile required a `platform.local.txt` override because Arduino_GFX RGB classes must see ESP32-S3 target macros not only in the sketch but also while compiling the library `.cpp` files.

Observed failures before the fix:

```text
Arduino_ESP32RGBPanel does not name a type
undefined reference to Arduino_ESP32RGBPanel::Arduino_ESP32RGBPanel(...)
undefined reference to Arduino_RGB_Display::begin(long)
```

Validated direction:

```text
compiler.cpp.extra_flags includes CONFIG_IDF_TARGET_ESP32S3=1 and board macros;
compiler.c.extra_flags includes the same target macros;
compiler.S.extra_flags includes the same target macros.
```

The exact reproduction steps are in `LOCAL_PLATFORM_SETUP.md`.

## External interface contour

For Sample A, the first-pass known contour is:

| Interface | Purpose | Pins / signals | Status |
|---|---|---|---|
| USB-UART / CH340C | upload and serial monitor | UART upload path | source-backed / used in workflow |
| RGB LCD FPC | built-in 800x480 display | RGB parallel GPIO map | own display PASS |
| GT911 capacitive touch | built-in touch panel | SDA19, SCL20, RST38, INT18 | own touch PASS |
| Backlight | display backlight | GPIO2 | blink/PWM test PASS reported |
| TF1 / microSD | external storage socket | CS10, MOSI11, CLK12, MISO13 | source-backed / physical test open |

Still open:

```text
all exposed side pads / solder pads;
optional UART/I2C/GPIO breakout pins if present;
power-domain limits for external loads;
continuity map from ESP32-S3 GPIO to every external pad;
PSRAM line in 05_TestConsole report after PSRAM=0 observation.
```

## Why not publish a Board Manager package immediately

The local sketchbook hardware profile is validated only as a local development profile. A full Arduino Boards Manager package requires a stable package index, archive, versioning and compatibility tracking across Arduino-ESP32 core versions.

Until then, the project should keep two supported paths:

```text
1. safe fallback: ESP32S3 Dev Module + documented Sample A settings;
2. local experimental: AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8.
```

## Rule

Do not claim that a public custom board package is supported until it compiles, uploads and runs the full validation chain on Sample A and at least one additional specimen. The current claim is narrower: local sketchbook board profile 01-05 PASS candidate on Sample A, with PSRAM acceptance still tied to `01_BoardInfo`.
