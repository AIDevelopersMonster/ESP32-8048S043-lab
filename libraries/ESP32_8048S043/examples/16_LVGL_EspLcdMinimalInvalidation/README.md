# 16_LVGL_EspLcdMinimalInvalidation

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

Firmware ID:

```text
16LVGL-MINV1-240828A
```

## Purpose

This example is the minimal-invalidation LVGL 8 probe after the 15th combined display/touch test reached a clear visual boundary.

Accepted functional result before this test:

```text
15_LVGL_EspLcdBasicUI
FUNCTIONAL PASS CANDIDATE / DYNAMIC REDRAW NOT ACCEPTABLE
```

The 15th test proved the combined path:

```text
esp_lcd RGB panel;
LVGL 8.3.11;
GT911 BSP touch;
LVGL pointer driver;
central button pressed/clicked events.
```

But the physical result still had:

```text
periodic redraw every 1-3 seconds;
screen jitter/flicker during button press;
visual behavior not acceptable for user-facing HMI.
```

The 16th test removes nearly all intentional UI invalidation to isolate the remaining cause.

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
periodic UI label update
constantly changing touch label
visible button pressed-state style
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
UI                         : static screen + visible button plate + transparent click hitbox
Counter update             : only on LV_EVENT_CLICKED
Periodic UI update          : none
```

## Expected Serial output

```text
ESP32-8048S043 Lab / 16_LVGL_EspLcdMinimalInvalidation
Minimal LVGL invalidation probe over esp_lcd RGB + GT911 BSP
Firmware ID              : 16LVGL-MINV1-240828A
Mode                     : minimal LVGL invalidation probe
Arduino_GFX              : not used
GT911 touch              : ESP32_8048S043_Touch BSP
Moving animation         : not used
Slider                   : not used
Periodic UI update       : disabled
Visible press style      : transparent click target
PCLK                     : 12500000 Hz
[PASS] esp_lcd_new_rgb_panel()
[PASS] esp_lcd_panel_reset()
[PASS] esp_lcd_panel_init()
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272 int=...
[PASS] lvBuf1 allocated in PSRAM
[PASS] lvBuf2 allocated in PSRAM
[PASS] LVGL display driver registered
[PASS] LVGL GT911 pointer driver registered
[PASS] LVGL minimal-invalidation UI objects created
[PASS] Backlight ON after LVGL first draw
[READY] Tap the quiet central target. Watch idle stability, press jitter and click-only counter update.
```

## Expected visual output

The screen should show:

```text
title;
subtitle;
static blue button-like plate;
text: Tap quiet target;
small click counter;
static status line;
stable white border.
```

There should be:

```text
no slider;
no moving block;
no dashboard-style screen redraw;
no periodic status refresh;
no constantly changing touch-coordinate label;
no visible pressed-state color change.
```

## How to test

Let the screen idle for at least 15 seconds before touching it.

Then tap the central target several times, slowly:

```text
press -> release -> wait 1 second -> press -> release
```

Do not drag.

## What to report

For the first physical run, record:

```text
Does it compile?
Does Serial show 16LVGL-MINV1-240828A?
Does esp_lcd_new_rgb_panel() pass?
Does GT911 BSP init pass?
Do both LVGL PSRAM buffers allocate?
Does the first screen appear cleanly after backlight-on?
Does idle screen still redraw/flicker every 1-3 seconds?
Does touching the transparent target still cause screen jitter?
Does the click counter update on release?
Does the white border stay stable during press?
Does Serial show HITBOX pressed/clicked events?
Do readFail or pointFail remain 0?
How many flush calls are reported before and after each tap?
```

## Interpretation

If idle screen is stable but press still jitters:

```text
The remaining problem is tied to touch-time LVGL invalidation / RGB scanout synchronization.
Proceed to 16B_DisplayEspLcdVsyncProbe or an event-synchronized flush strategy.
```

If idle screen still redraws/flickers every 1-3 seconds:

```text
The redraw is not caused by our intentional label updates.
Investigate LVGL display driver timing, RGB panel refresh behavior, PSRAM framebuffer interaction and flush synchronization.
```

If counter update alone jitters:

```text
Even tiny LVGL invalidations are visually unsafe with the current flush strategy.
Do not continue UI-level polishing before VSYNC/frame-event work.
```

If this test is visually stable:

```text
Use this as the baseline for future user-facing widgets.
Add interactivity back one feature at a time.
```

## PASS boundary

A positive result here would mean:

```text
LVGL 8 over native esp_lcd can run a quiet interactive UI with GT911 BSP touch and click-only invalidation on Sample A.
```

It would not prove:

```text
slider/drag stability;
fast animation;
full dashboard UI;
Widget Runtime;
Web setup or OTA;
LVGL 9 migration.
```
