# ESP32_8048S043 Arduino BSP

Experimental Arduino BSP skeleton for ESP32-8048S043 / ESP32-8048S043C-I boards.

## Status

```text
BSP API                 SKELETON
01_BoardInfo            SOURCE IMPLEMENTED
02_DisplayRGBTest       SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN
Display driver          FIRST MINIMAL ARDUINO_GFX TEST ADDED
Touch driver            OPEN
LVGL port               OPEN
Physical PASS claims    FACTORY LVGL DISPLAY + TOUCH VISUAL ONLY
```

## Example plan

```text
01_BoardInfo
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
