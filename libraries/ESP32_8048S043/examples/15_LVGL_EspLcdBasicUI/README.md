# 15_LVGL_EspLcdBasicUI

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

Firmware ID:

```text
15LVGL-ELCDT1-240827A
```

## Purpose

This example is the first combined LVGL 8 basic UI test over the native ESP-IDF `esp_lcd` RGB panel path with GT911 normalized BSP touch input.

It follows two accepted isolated results:

```text
13_LVGL_EspLcdStatic
PHYSICAL STATIC PASS CANDIDATE / TOUCH NOT TESTED

14_GT911_NormalizedTouch
PHYSICAL PASS CANDIDATE / 9-ZONE NORMALIZATION PASS
```

The goal is to combine those two paths without returning to the previously rejected dynamic dashboard or raw moving-block update style.

## What it uses

```text
Arduino sketch
LVGL 8.x
ESP-IDF esp_lcd RGB panel API from Arduino-ESP32 core
ESP32_8048S043_Touch BSP class
ESP32_8048S043_Pins.h
RGB panel 800x480
GT911 capacitive touch
Backlight GPIO2
PSRAM panel framebuffer
2x LVGL PSRAM draw buffers
```

## What it intentionally does not use

```text
Arduino_GFX
slider
moving animation
full dashboard redraw
Wi-Fi
SD
BLE
Web
OTA
Widget Runtime
```

## Default settings

```text
PCLK                       : 12.5 MHz
HSYNC porch / pulse / back : 8 / 4 / 8
VSYNC porch / pulse / back : 8 / 4 / 8
PCLK active negative       : true
Panel framebuffer in PSRAM : true
Panel double framebuffer   : true
LVGL buffers               : 2x 80 lines in PSRAM
Font mode                  : LV_FONT_DEFAULT only
Touch input                : ESP32_8048S043_Touch BSP as LVGL pointer
UI                         : one central button + small labels
```

## Expected Serial output

```text
ESP32-8048S043 Lab / 15_LVGL_EspLcdBasicUI
LVGL 8 button UI over esp_lcd RGB + GT911 BSP touch
Firmware ID              : 15LVGL-ELCDT1-240827A
Mode                     : LVGL button UI over esp_lcd RGB panel
Arduino_GFX              : not used
GT911 touch              : ESP32_8048S043_Touch BSP
Moving animation         : not used
Slider                   : not used
PCLK                     : 12500000 Hz
[PASS] esp_lcd_new_rgb_panel()
[PASS] esp_lcd_panel_reset()
[PASS] esp_lcd_panel_init()
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272 int=...
[PASS] lvBuf1 allocated in PSRAM
[PASS] lvBuf2 allocated in PSRAM
[PASS] LVGL display driver registered
[PASS] LVGL GT911 pointer driver registered
[PASS] LVGL minimal button UI objects created
[PASS] Backlight ON after LVGL first draw
[READY] Tap the central button. Watch button reaction, labels and border stability.
```

## Expected visual output

The screen should show:

```text
title;
subtitle;
one central Tap me button;
click/press counter label;
touch coordinate/zone label;
small status line;
stable white border.
```

There should be:

```text
no slider;
no moving block;
no dashboard-style screen redraw;
no intentional animation;
no Web/SD/BLE activity.
```

## How to test

Tap only the central button first.

Then tap around the button and near the screen zones only after the button path is understood.

Record:

```text
Does it compile?
Does Serial show 15LVGL-ELCDT1-240827A?
Does display init pass?
Does GT911 init pass?
Does LVGL pointer driver register?
Does the button visibly react when pressed?
Does Pressed counter increment?
Does Clicks counter increment after release?
Does touch label show plausible coordinates and zone?
Does the border remain stable while tapping?
Does the button redraw cause tearing, flicker or geometry jumps?
Does idle screen remain stable?
Are touch readFailures or pointFailures increasing?
Any reset, brownout, panic or Guru Meditation?
```

## Acceptance boundary

A pass here means:

```text
The board can run a minimal interactive LVGL button UI using native esp_lcd display transport and GT911 BSP normalized touch input.
```

It does not prove:

```text
slider quality;
fast animation;
full dashboard UX;
long-duration HMI stability;
Widget Runtime;
Web setup;
OTA;
LVGL 9 migration;
production-ready UI quality.
```

## Interpretation

If the button works and the screen remains stable:

```text
Proceed to a controlled 16_LVGL_EspLcdControls test with two or three small widgets, still no moving animation.
```

If the button works but tearing appears only while pressing:

```text
The next step should investigate LVGL invalidation size, pressed-state redraw and possible VSYNC/panel synchronization.
```

If touch coordinates are correct in Serial but the button does not react:

```text
Inspect LVGL indev registration and coordinate delivery to LVGL.
```

If idle screen is unstable:

```text
Return to the esp_lcd/LVGL display path before adding more widgets.
```
