# ESP32-8048S043 / Sample A / Arduino 15_LVGL_EspLcdBasicUI

Status: `FUNCTIONAL PASS CANDIDATE / DYNAMIC REDRAW NOT ACCEPTABLE`.

Date: 2026-08-27

Specimen:

```text
Sample A
ESP32-8048S043 / ESP32-S3 / 800x480 RGB panel / GT911 family
```

Example:

```text
libraries/ESP32_8048S043/examples/15_LVGL_EspLcdBasicUI
```

Firmware ID tested:

```text
15LVGL-ELCDT1-240827A
```

## Purpose

This test combined two previously accepted isolated paths:

```text
13_LVGL_EspLcdStatic
PHYSICAL STATIC PASS CANDIDATE / TOUCH NOT TESTED

14_GT911_NormalizedTouch
PHYSICAL PASS CANDIDATE / 9-ZONE NORMALIZATION PASS
```

The goal was to check a minimal interactive LVGL path over native `esp_lcd` display transport plus GT911 BSP touch input.

## Runtime identity

Observed runtime profile:

```text
Firmware ID          : 15LVGL-ELCDT1-240827A
ESP-IDF SDK          : v5.5.5
ARDUINO_BOARD        : ESP32_8048S043_LAB
ARDUINO_VARIANT      : esp32_8048s043_lab
LVGL version         : 8.3.11
LV_COLOR_DEPTH       : 16
Flash                : 16777216 bytes
PSRAM                : 8388608 bytes
Free PSRAM at boot   : 8384788 bytes
Free heap at boot    : 291800 bytes
PCLK                 : 12500000 Hz
Framebuffer in PSRAM : true
Double framebuffer   : true
Font mode            : LV_FONT_DEFAULT only
```

## Init result

All init stages passed:

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

## Functional result

The combined path worked functionally:

```text
central button was visible;
GT911 BSP touch produced mapped coordinates;
LVGL pointer input reached the button;
LV_EVENT_PRESSED fired;
LV_EVENT_CLICKED fired after release;
Pressed counter increased;
Clicked counter increased;
no touch read failures were reported;
no point read failures were reported.
```

Representative button events:

```text
[BUTTON] pressed=1 touch=(355,191) zone=CENTER flush=7
[BUTTON] clicked=1 pressed=1 touch=(359,205) zone=CENTER flush=13
[BUTTON] pressed=10 touch=(373,202) zone=CENTER flush=118
[BUTTON] clicked=10 pressed=10 touch=(373,202) zone=CENTER flush=124
```

Final relevant ALIVE line:

```text
[ALIVE] fw=15LVGL-ELCDT1-240827A uptime=30s panel=OK touch=OK lvgl=OK ui=OK clicked=10 pressed=10 reports=79 releases=10 flush=132 indev=940 loops=5354 readFail=0 pointFail=0 heap=285908 psram=8388608 freePsram=6589028
```

## Operator visual observation

Operator report:

```text
Недостатки все те же: перерисовка раз в секунду или две или три и дребезг экрана при нажатии кнопки, без их учета все работает.
```

This is the decisive boundary for the 15th test.

## Interpretation

The test separates two facts:

```text
1. The combined display + LVGL + GT911 BSP input pipeline is functional.
2. The dynamic redraw behavior is still not acceptable for a user-facing HMI.
```

This is not a GT911 detection failure:

```text
addr=0x5D
fw=0x1060
res=480x272
readFail=0
pointFail=0
pressed/clicked events reached LVGL
```

This is not an RGB pin-map or color-order failure. The remaining problem is visual redraw / tearing / update synchronization under LVGL invalidation and button-state redraw.

## Decision

Freeze `15_LVGL_EspLcdBasicUI` as:

```text
FUNCTIONAL PASS CANDIDATE / DYNAMIC REDRAW NOT ACCEPTABLE
```

Do not promote it as a polished user-application template.

The next display-layer investigation should remove unnecessary periodic label updates and/or introduce synchronization strategy before declaring any user-facing LVGL path acceptable.

Recommended next experiment:

```text
16_LVGL_EspLcdMinimalInvalidation
```

Candidate goals:

```text
no periodic status label update;
no constantly changing touch label;
button visual state minimized or disabled;
click-only counter update;
measure whether screen still tears when the smallest possible LVGL invalidation occurs.
```

Alternative lower-level path:

```text
16B_DisplayEspLcdVsyncProbe
```

Candidate goals:

```text
check whether Arduino-ESP32 / ESP-IDF v5.5.5 exposes usable RGB panel event callbacks;
use VSYNC / frame event information to reason about safe update windows;
do not involve LVGL until event behavior is known.
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
