# 13_LVGL_EspLcdStatic

Status: `PHYSICAL STATIC PASS CANDIDATE / TOUCH NOT TESTED`.

Firmware ID:

```text
13LVGL-ELCDS2-240827B
```

Previous firmware ID:

```text
13LVGL-ELCDS1-240827A
```

## Physical result

After the default-font patch, the operator reported:

```text
Все работает.
```

Evidence record:

```text
evidence/specimens/sample-a/arduino/13-lvgl-esp-lcd-static-20260827.md
```

## Default-font patch

The first compile attempt failed because the local Arduino LVGL configuration exposed `lv_font_montserrat_14`, but did not expose larger optional fonts:

```text
lv_font_montserrat_18
lv_font_montserrat_20
lv_font_montserrat_22
```

Revision `13LVGL-ELCDS2-240827B` removes all explicit references to those optional fonts and uses LVGL default font behavior only.

This keeps the test focused on the display transport path rather than optional LVGL font configuration.

## Purpose

This example is the first LVGL 8 static UI test over the native ESP-IDF `esp_lcd` RGB panel path for the ESP32-8048S043 board.

It follows the 12th probe result:

```text
12_DisplayEspLcdRgbPanel_Probe
STATIC TRANSPORT PASS CANDIDATE / RAW DYNAMIC DRAW NOT ACCEPTABLE
```

The 12th probe showed that native `esp_lcd` can initialize the panel and render correct static colors, but naive raw dynamic `draw_bitmap()` motion is not acceptable.

Therefore this example does not animate a moving block. It tests a calmer and more realistic path:

```text
LVGL 8.3.x
native esp_lcd RGB panel
static UI
rare small label updates
no touch
```

## What it uses

```text
Arduino sketch
LVGL 8.x
ESP-IDF esp_lcd RGB panel API from Arduino-ESP32 core
ESP32_8048S043_Pins.h
RGB panel 800x480
Backlight GPIO2
PSRAM panel framebuffer
2x LVGL PSRAM draw buffers
LV_FONT_DEFAULT only
```

## What it intentionally does not use

```text
Arduino_GFX
GT911 touch
moving block animation
slider/button interaction
SD
Wi-Fi
BLE
```

## Default settings

```text
PCLK                       : 12.5 MHz
HSYNC porch / pulse / back : 8 / 4 / 8
VSYNC porch / pulse / back : 8 / 4 / 8
PCLK active negative       : true
Panel framebuffer in PSRAM : true
Panel double framebuffer   : true
LVGL buffers               : 2x 100 lines in PSRAM
Data order                 : ESP-IDF RGB565 bus-bit order
Label update interval      : 5 seconds
Font mode                  : LV_FONT_DEFAULT only
```

The ESP-IDF RGB565 bus-bit order is:

```text
DATA0..DATA4    = B0..B4
DATA5..DATA10   = G0..G5
DATA11..DATA15  = R0..R4
```

## Expected Serial output

```text
ESP32-8048S043 Lab / 13_LVGL_EspLcdStatic
LVGL 8 static UI over native esp_lcd RGB panel
Firmware ID              : 13LVGL-ELCDS2-240827B
Mode                     : LVGL static UI over esp_lcd RGB panel
Arduino_GFX              : not used
GT911 touch              : not used
Moving animation         : not used
Font mode                : LV_FONT_DEFAULT only
PCLK                     : 12500000 Hz
[PASS] esp_lcd_new_rgb_panel()
[PASS] esp_lcd_panel_reset()
[PASS] esp_lcd_panel_init()
[PASS] lvBuf1 allocated in PSRAM
[PASS] lvBuf2 allocated in PSRAM
[PASS] LVGL display driver registered
[PASS] LVGL static UI objects created
[PASS] Backlight ON after LVGL first draw
[READY] Watch idle screen and 5-second label updates. No touch and no animation are active.
```

## Expected visual output

The screen should show a static LVGL diagnostic dashboard:

```text
title line;
three static information cards;
two static bars;
counter label updated every 5 seconds;
flush/heap labels updated every 5 seconds;
white border frame.
```

There should be:

```text
no touch response;
no moving block;
no slider;
no fast animation;
no intentional full-screen periodic redraw.
```

## Acceptance boundary

A pass here means:

```text
LVGL 8 can run over the native esp_lcd RGB panel path with a static UI and rare small label updates.
```

It does not prove:

```text
touch quality;
interactive widgets;
fast animation;
user-facing dynamic UX;
full dashboard replacement;
LVGL 9 migration;
Widget Runtime.
```

## Next step

After this pass candidate, the next isolated subsystem test is:

```text
14_GT911_NormalizedTouch
```

That test validates BSP touch normalization without LVGL widgets and without display redraw.
