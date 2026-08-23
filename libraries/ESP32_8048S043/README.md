# ESP32_8048S043 Arduino BSP

Experimental Arduino BSP skeleton for ESP32-8048S043 / ESP32-8048S043C-I boards.

## Status

```text
BSP API                 SKELETON
01_BoardInfo            SOURCE IMPLEMENTED / README ADDED / PHYSICAL VALIDATION OPEN
02_DisplayRGBTest       SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN
Display driver          FIRST MINIMAL ARDUINO_GFX TEST ADDED
Touch driver            OPEN
LVGL port               OPEN
Physical PASS claims    FACTORY LVGL DISPLAY + TOUCH VISUAL ONLY
```

## Arduino IDE board setup

Recommended starting profile for the examples in this library:

```text
Board package : esp32 by Espressif Systems
Board         : ESP32S3 Dev Module
Port          : CH340 / USB-SERIAL port of the board
Upload Speed  : 460800 first; 921600 only if stable
CPU Frequency : 240MHz (WiFi)
Flash Size    : 16MB / 128Mb
Flash Mode    : DIO recommended first
Partition     : any 16MB-compatible scheme for early smoke tests
PSRAM         : OPI PSRAM / Enabled
USB CDC Boot  : Disabled when using CH340C USB-UART
Upload Mode   : UART0 / Hardware CDC, depending on Arduino menu wording
Core Debug    : None
Serial Monitor: 115200 baud
```

Menu names differ between ESP32 Arduino core versions. Keep the intent: ESP32-S3 target, 16 MB flash, 8 MB/OPI PSRAM enabled, external UART upload through CH340C, serial monitor at 115200.

## Example plan

```text
01_BoardInfo            first Arduino IDE smoke test, chip/flash/PSRAM/ALIVE
02_DisplayRGBTest       minimal Arduino_GFX RGB/backlight/color/orientation test
03_TouchGT911Test       future GT911 scan/coordinate test
04_BacklightTest
05_TestConsole
09_LVGL_BasicUI        future
10_LVGL_Dashboard      future
13_RetroClock_800x480  future
14_WidgetLoader        future
15_GitHubOTA           future
```

## 01_BoardInfo

Purpose:

```text
verify basic Arduino IDE upload, serial monitor, ESP32-S3 identity, 16 MB flash and 8 MB PSRAM
```

Open:

```text
libraries/ESP32_8048S043/examples/01_BoardInfo/01_BoardInfo.ino
```

See also:

```text
libraries/ESP32_8048S043/examples/01_BoardInfo/README.md
```

PASS boundary:

```text
PASS requires successful upload, serial output at 115200, ESP32-S3 identity, about 16 MB flash, about 8 MB PSRAM and stable ALIVE messages.
```

## 02_DisplayRGBTest

Purpose:

```text
validate the source-backed ESP32-8048S043 RGB GPIO map with our own minimal Arduino sketch
```

What it tests:

- RGB panel bring-up through `Arduino_GFX_Library`;
- backlight GPIO 2 full ON;
- full-screen red/green/blue/white/black;
- orientation frame with corner markers;
- RGB color-bar pattern;
- stripe pattern for data-line sanity.

Dependency:

```text
Arduino_GFX_Library by moononournation
```

PASS boundary:

```text
PASS requires physical photo/video evidence from a named specimen.
Until then the example is SOURCE IMPLEMENTED only.
```

## Rule

Examples may compile before hardware validation, but README status must not say PHYSICAL PASS until the named specimen evidence exists.
