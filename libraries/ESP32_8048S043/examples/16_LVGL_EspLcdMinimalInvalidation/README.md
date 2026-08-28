# 16_LVGL_EspLcdMinimalInvalidation

Status: `FUNCTIONAL PASS CANDIDATE / IDLE STABLE / HARD-TAP JITTER OPEN`.

Firmware ID:

```text
16LVGL-MINV1-240828A
```

## Physical result on Sample A

The first physical run changed the display/LVGL boundary materially.

Accepted observations:

```text
Idle for 15 seconds: no self-redraw every 1-3 seconds.
Gentle phone-like touch / careful touch: no visible jitter.
Hard tapping on the target: screen jitter can appear.
Click counter increases after release.
White border remains stable.
HITBOX pressed/clicked events are printed.
readFail and pointFail remain 0 in the uploaded log.
```

Final uploaded ALIVE line summary:

```text
uptime=85s
clicked=38
pressed=38
reports=715
releases=38
flush=82
readFail=0
pointFail=0
heap=285924
psram=8388608
freePsram=6589028
```

Evidence:

```text
evidence/specimens/sample-a/arduino/16-lvgl-esp-lcd-minimal-invalidation-20260828.md
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

## Interpretation

This result proves that the 15th test's periodic redraw defect was caused by intentional UI invalidation and not by idle panel instability.

Current boundary:

```text
Quiet idle LVGL screen: stable.
Gentle click interaction: functionally usable.
Hard-tap robustness: still open.
```

Recommended next step:

```text
17_LVGL_EspLcdManualHitbox
```

Candidate purpose:

```text
Do not register GT911 as a LVGL pointer driver.
Read GT911 manually in the sketch.
Do manual hit testing against the central target.
Do not invalidate anything on press.
Update only a small counter label after release.
```

This separates LVGL pointer-state invalidation from manual touch handling with click-only LVGL update.

Alternative lower-level path remains:

```text
16B_DisplayEspLcdVsyncProbe
```

if even click-only manual updates still produce unacceptable artifacts.

## PASS boundary

A positive result here means:

```text
LVGL 8 over native esp_lcd can run a quiet interactive screen with GT911 BSP touch and click-only invalidation on Sample A, with stable idle display and correct click delivery.
```

It does not prove:

```text
hard-tap robustness;
slider/drag stability;
fast animation;
full dashboard UI;
Widget Runtime;
Web setup or OTA;
LVGL 9 migration.
```
