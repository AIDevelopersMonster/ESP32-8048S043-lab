# 15_LVGL_EspLcdBasicUI

Status: `FUNCTIONAL PASS CANDIDATE / DYNAMIC REDRAW NOT ACCEPTABLE`.

Firmware ID:

```text
15LVGL-ELCDT1-240827A
```

## Result summary

The first physical run proved the combined pipeline is functional:

```text
native esp_lcd RGB panel initializes;
LVGL 8.3.11 display driver registers;
GT911 BSP touch initializes at 0x5D;
LVGL pointer driver registers;
central button receives press/click events;
Pressed and Clicked counters increase;
readFail and pointFail remain zero.
```

But the visual quality is not acceptable for user-facing UI:

```text
periodic redraw is visible;
screen jitter/redraw occurs when pressing the button;
the same dynamic redraw defect seen in earlier LVGL paths remains.
```

Operator observation:

```text
Недостатки все те же: перерисовка раз в секунду или две или три и дребезг экрана при нажатии кнопки, без их учета все работает.
```

Therefore this example is a functional integration proof, not a polished UI template.

## Evidence

```text
evidence/specimens/sample-a/arduino/15-lvgl-esp-lcd-basic-ui-20260827.md
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

The goal was to combine those two paths without returning to the previously rejected dynamic dashboard or raw moving-block update style.

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

## Observed Serial result

Successful initialization:

```text
[PASS] esp_lcd_new_rgb_panel()
[PASS] esp_lcd_panel_reset()
[PASS] esp_lcd_panel_init()
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272 int=1
[PASS] lvBuf1 allocated in PSRAM: 128000 bytes
[PASS] lvBuf2 allocated in PSRAM: 128000 bytes
[PASS] LVGL display driver registered
[PASS] LVGL GT911 pointer driver registered
[PASS] LVGL minimal button UI objects created
[PASS] Backlight ON after LVGL first draw
```

Functional LVGL button events:

```text
[BUTTON] pressed=1 touch=(355,191) zone=CENTER flush=7
[BUTTON] clicked=1 pressed=1 touch=(359,205) zone=CENTER flush=13
[BUTTON] pressed=10 touch=(373,202) zone=CENTER flush=118
[BUTTON] clicked=10 pressed=10 touch=(373,202) zone=CENTER flush=124
```

Final observed ALIVE line:

```text
[ALIVE] fw=15LVGL-ELCDT1-240827A uptime=30s panel=OK touch=OK lvgl=OK ui=OK clicked=10 pressed=10 reports=79 releases=10 flush=132 indev=940 loops=5354 readFail=0 pointFail=0 heap=285908 psram=8388608 freePsram=6589028
```

## Interpretation

This test separates two facts:

```text
The architecture is functionally connected:
esp_lcd display + LVGL + GT911 BSP pointer input.

The visual redraw behavior is still not acceptable:
button-state redraw and/or LVGL invalidation can still produce visible screen disturbance.
```

This is not a touch-controller failure:

```text
GT911 detected at 0x5D;
firmware 0x1060;
raw resolution 480x272;
LVGL received pointer events;
readFail=0;
pointFail=0.
```

This is not a color-order or RGB pin-map failure.

The remaining problem belongs to:

```text
LVGL invalidation granularity;
button pressed-state redraw;
periodic status label redraw;
RGB panel scan/update synchronization;
possibly missing VSYNC/frame-event discipline.
```

## Current decision

Freeze this example as:

```text
FUNCTIONAL PASS CANDIDATE / DYNAMIC REDRAW NOT ACCEPTABLE
```

Do not promote it as a user-facing HMI template.

## Recommended next experiment

```text
16_LVGL_EspLcdMinimalInvalidation
```

Goals:

```text
remove periodic status label updates;
remove constantly changing touch label updates;
minimize or neutralize button pressed-state visual redraw;
update only one small counter on click;
observe whether the screen still tears when LVGL invalidation is minimal.
```

Alternative lower-level follow-up:

```text
16B_DisplayEspLcdVsyncProbe
```

Goals:

```text
check whether Arduino-ESP32 / ESP-IDF v5.5.5 exposes usable RGB panel event callbacks;
reason about VSYNC/frame timing;
keep LVGL out until panel-event behavior is known.
```

## PASS boundary

A positive functional result here means:

```text
LVGL 8 can receive GT911 BSP pointer events over the native esp_lcd display path and activate a central button on Sample A.
```

It does not prove:

```text
acceptable dynamic visual quality;
tear-free LVGL updates;
polished HMI behavior;
slider/drag controls;
full dashboard UI;
Widget Runtime;
Web setup or OTA.
```
