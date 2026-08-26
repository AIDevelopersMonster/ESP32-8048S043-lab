# 11_LVGL_Dashboard / manual touch limitation evidence

Specimen: Sample A  
Date: 2026-08-26  
Example: `libraries/ESP32_8048S043/examples/11_LVGL_Dashboard/11_LVGL_Dashboard.ino`  
Firmware ID: `11DASH-MT1-240826C`  
Status: `FUNCTIONAL STABILITY PASS CANDIDATE / DYNAMIC UX NOT ACCEPTABLE`

## Scope

This record captures the manual-touch dashboard experiment after `10_LVGL_BasicUI` showed functional LVGL interaction but visibly poor touch dynamics.

The experiment intentionally disabled LVGL pointer input and used manual GT911 hitboxes:

```text
GT911 touch path          : ESP32_8048S043_Touch BSP
LVGL pointer device       : disabled
Refresh button            : fixed hitbox + debounce
Backlight control         : broad horizontal touch band + X-axis projection
Dashboard refresh         : static screen, no 1 Hz redraw
Telemetry                 : Serial ALIVE every 5 s
```

## Boot and initialization evidence

```text
ESP32-8048S043 Lab / 11_LVGL_Dashboard
LVGL 8 dashboard validation
Firmware ID: 11DASH-MT1-240826C

Mode   : RGB display + manual GT911 hitboxes + LVGL dashboard
Refresh: static screen, manual dashboard refresh only
Touch  : LVGL pointer disabled; button/axis handled by sketch

ARDUINO_BOARD               : "ESP32_8048S043_LAB"
ARDUINO_VARIANT             : "esp32_8048s043_lab"
ESP-IDF SDK                 : v5.5.5
Chip                        : ESP32-S3 rev 2
Flash                       : 16777216 bytes
PSRAM                       : 8388608 bytes
Static refresh              : enabled
Manual touch                : enabled

[DISPLAY INIT]
[PASS] gfx->begin()
[TOUCH BSP INIT]
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272 int=1
[LVGL INIT]
[PASS] lvBuf1 allocated in PSRAM: 64000 bytes
[PASS] lvBuf2 allocated in PSRAM: 64000 bytes
[PASS] LVGL display driver registered
[PASS] GT911 touch handled manually, LVGL pointer driver disabled
[PASS] LVGL dashboard UI objects created

LVGL DASHBOARD READY
Firmware ID: 11DASH-MT1-240826C
Static refresh mode: no 1 Hz dashboard redraw.
Manual touch mode: no LVGL pressed/drag redraw under finger.
```

## Stability evidence

The sketch remained alive for a long idle run with stable heap/PSRAM values and no reset/brownout/crash reported.

Representative ALIVE lines:

```text
[ALIVE] fw=11DASH-MT1-240826C uptime=5s display=OK touch=OK lvgl=OK ui=OK refresh=0 manualTouch=0 slider=0 accepted=0 filtered=0 loops=601 freeHeap=287016 psram=8388608 freePsram=7486068
[ALIVE] fw=11DASH-MT1-240826C uptime=600s display=OK touch=OK lvgl=OK ui=OK refresh=0 manualTouch=0 slider=0 accepted=0 filtered=0 loops=119363 freeHeap=287016 psram=8388608 freePsram=7486068
[ALIVE] fw=11DASH-MT1-240826C uptime=1000s display=OK touch=OK lvgl=OK ui=OK refresh=0 manualTouch=0 slider=0 accepted=0 filtered=0 loops=199203 freeHeap=287016 psram=8388608 freePsram=7486068
[ALIVE] fw=11DASH-MT1-240826C uptime=1400s display=OK touch=OK lvgl=OK ui=OK refresh=0 manualTouch=0 slider=0 accepted=0 filtered=0 loops=279043 freeHeap=287016 psram=8388608 freePsram=7486068
```

Earlier in the same run, manual touch events were observed:

```text
[ALIVE] fw=11DASH-MT1-240826C uptime=160s display=OK touch=OK lvgl=OK ui=OK refresh=0 manualTouch=12 slider=5 accepted=12 filtered=5 loops=31527 freeHeap=287016 psram=8388608 freePsram=7486068
```

## Operator observation

```text
The manual-touch method reduces the area where the artifact appears, but the artifact remains during touch.
The result is acceptable as a laboratory test only.
This dynamic behavior is not acceptable for user-facing applications.
```

## Result

```text
Static idle dashboard stability      : PASS CANDIDATE
Serial telemetry / long idle run     : PASS CANDIDATE
Display initialization               : PASS CANDIDATE
GT911 BSP initialization              : PASS CANDIDATE
LVGL dashboard rendering              : PASS CANDIDATE
Manual hitbox / axis touch experiment : FUNCTIONAL TEST ONLY
Dynamic touch UX                      : NOT ACCEPTABLE FOR USER APPLICATIONS
```

## Decision

Freeze `11_LVGL_Dashboard` as a diagnostic/stability example and do not spend more time trying to polish this local partial-redraw approach.

Further LVGL work should switch to studying and porting better-organized third-party / WT32-style patterns:

```text
clean BSP display/touch/backlight separation;
small LVGL application sketches;
proven LVGL display flushing strategy;
proper handling of RGB-panel redraw dynamics;
no user-facing UI based on the current manual-touch workaround.
```

## Boundary

This evidence proves that the dashboard is stable and functional as a lab example. It explicitly does not prove polished HMI behavior, acceptable dynamic touch UX, long-term application readiness, Widget Runtime, Web upload/control, GitHub OTA, or LVGL 9 compatibility.
