# 10_LVGL_BasicUI / BSP touch functional evidence

Specimen: Sample A  
Date: 2026-08-26  
Example: `libraries/ESP32_8048S043/examples/10_LVGL_BasicUI/10_LVGL_BasicUI.ino`  
Firmware ID: `10LVGL-BSP1-240826C`  
Status: `FUNCTIONAL PASS CANDIDATE / TOUCH QUALITY OPEN`

## Scope

This record captures the first LVGL BasicUI run after moving GT911 access behind the `ESP32_8048S043_Touch` BSP API.

The result is intentionally not promoted to final smooth UI pass. The UI works, but touch quality remains visibly rough during dragging and finger movement.

## Arduino build/runtime profile

```text
ARDUINO_BOARD               : "ESP32_8048S043_LAB"
ARDUINO_VARIANT             : "esp32_8048s043_lab"
CONFIG_IDF_TARGET           : "esp32s3"
CONFIG_IDF_TARGET_ESP32S3   : 1
BOARD_HAS_PSRAM             : defined
LVGL version                : 8.3.11
LV_COLOR_DEPTH              : 16
ESP-IDF SDK                 : v5.5.5
Chip                        : ESP32-S3 rev 2
CPU frequency               : 240 MHz
Flash                       : 16777216 bytes
PSRAM                       : 8388608 bytes
Free PSRAM                  : 8384788 bytes
Free heap                   : 291012 bytes
```

## Initialization result

```text
[DISPLAY INIT]
[PASS] gfx->begin()

[TOUCH BSP INIT]
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272 int=1

[LVGL INIT]
[PASS] lvBuf1 allocated in PSRAM: 64000 bytes
[PASS] lvBuf2 allocated in PSRAM: 64000 bytes
[PASS] LVGL touch input registered through BSP
[PASS] LVGL display driver registered

[UI INIT]
[PASS] LVGL UI objects created
```

## Interaction evidence

Button events were received by LVGL:

```text
[LVGL] fw=10LVGL-BSP1-240826C Button clicked: 1
[LVGL] fw=10LVGL-BSP1-240826C Button clicked: 2
[LVGL] fw=10LVGL-BSP1-240826C Button clicked: 3
[LVGL] fw=10LVGL-BSP1-240826C Button clicked: 4
[LVGL] fw=10LVGL-BSP1-240826C Button clicked: 5
```

Backlight slider events were received by LVGL and routed to the physical backlight PWM path:

```text
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 254
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 243
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 231
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 218
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 205
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 192
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 180
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 168
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 157
[LVGL] fw=10LVGL-BSP1-240826C Backlight slider: 147
```

## ALIVE evidence

```text
[ALIVE] fw=10LVGL-BSP1-240826C uptime=5s display=OK touch=OK lvgl=OK ui=OK clicks=0 accepted=9 filtered=1 statusReads=100 ready=15 zeroReady=6 lastStatus=0x00 readFail=0 pointFail=0 lvglLoops=595 freeHeap=287048 psram=8388608 freePsram=7486068
[ALIVE] fw=10LVGL-BSP1-240826C uptime=10s display=OK touch=OK lvgl=OK ui=OK clicks=3 accepted=78 filtered=49 statusReads=264 ready=101 zeroReady=23 lastStatus=0x80 readFail=0 pointFail=0 lvglLoops=1528 freeHeap=287048 psram=8388608 freePsram=7486068
[ALIVE] fw=10LVGL-BSP1-240826C uptime=20s display=OK touch=OK lvgl=OK ui=OK clicks=5 accepted=237 filtered=158 statusReads=591 ready=290 zeroReady=53 lastStatus=0x00 readFail=0 pointFail=0 lvglLoops=3457 freeHeap=287048 psram=8388608 freePsram=7486068
[ALIVE] fw=10LVGL-BSP1-240826C uptime=40s display=OK touch=OK lvgl=OK ui=OK clicks=5 accepted=295 filtered=188 statusReads=1254 ready=354 zeroReady=59 lastStatus=0x00 readFail=0 pointFail=0 lvglLoops=7355 freeHeap=287048 psram=8388608 freePsram=7486068
```

## Result

```text
Display under LVGL              : PASS CANDIDATE
PSRAM LVGL draw buffers         : PASS CANDIDATE
GT911 BSP initialization         : PASS CANDIDATE
LVGL pointer registration        : PASS CANDIDATE
Button click events              : PASS CANDIDATE
Slider-to-backlight interaction  : PASS CANDIDATE
No reset/crash during run         : PASS CANDIDATE
Touch quality / smoothness        : OPEN
```

## Operator observation

```text
Works as a test, but touch/drag interaction is not yet clean or stable enough for a polished UI.
The result should be kept as a functional LVGL BasicUI pass candidate with touch quality open.
```

## Boundary

This record proves that the LVGL UI is functional with BSP-backed GT911 touch on Sample A. It does not prove final touch UX quality, long-duration HMI stability, SD-backed assets, Web upload/control, Widget Runtime, GitHub OTA, or LVGL 9 compatibility.
