# ESP32-8048S043 / Sample A / Arduino 14_GT911_NormalizedTouch

Status: `PHYSICAL PASS CANDIDATE / 9-ZONE NORMALIZATION PASS`.

Date: 2026-08-27

Specimen:

```text
Sample A
ESP32-8048S043 / ESP32-S3 / 800x480 RGB panel / GT911 family
```

Example:

```text
libraries/ESP32_8048S043/examples/14_GT911_NormalizedTouch
```

Firmware ID accepted:

```text
14TOUCH-NORM1-240827A
```

## Purpose

This test validates the GT911 touch path independently from display redraw, LVGL widgets and Arduino_GFX.

It follows:

```text
13_LVGL_EspLcdStatic
PHYSICAL STATIC PASS CANDIDATE / TOUCH NOT TESTED
```

The goal was to confirm that the BSP touch layer can detect the controller, read raw GT911 coordinates and normalize them into the board's 800x480 display coordinate space.

## Runtime identity

Observed boot/runtime profile:

```text
ARDUINO_BOARD      : ESP32_8048S043_LAB
ARDUINO_VARIANT    : esp32_8048s043_lab
ESP-IDF SDK        : v5.5.5
Chip               : ESP32-S3 rev 2
Flash              : 16777216 bytes
PSRAM              : 8388608 bytes
Mode               : GT911 BSP read only
Display            : not used
Arduino_GFX        : not used
esp_lcd display    : not used
LVGL               : not used
Mapped coordinates : 800x480
GT911 pins         : SDA=19 SCL=20 RST=38 INT=18
```

## GT911 detection

Touch initialization passed:

```text
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272 int=1
```

This confirms:

```text
GT911 I2C address     : 0x5D
GT911 firmware        : 0x1060
GT911 raw resolution  : 480x272
INT level at init     : 1
```

## Physical 3x3 zone result

The diagnostic reached all nine normalized zones:

```text
[ZONES] seen=9/9 mask=0b111111111 -> TOP_LEFT TOP_CENTER TOP_RIGHT CENTER_LEFT CENTER CENTER_RIGHT BOTTOM_LEFT BOTTOM_CENTER BOTTOM_RIGHT
```

The final ALIVE line also confirmed:

```text
zones=9/9
readFail=0
drvReadFail=0
drvPointFail=0
releases=3
accepted=280
filtered=262
statusReads=581
ready=301
zeroReady=21
lastStatus=0x00
heap=342460
psram=8388608
freePsram=8383500
```

## Representative mapped positions

Representative observed mappings:

```text
CENTER       : raw=(177, 83)  mapped=(294,163) zone=CENTER
TOP_CENTER   : raw=(178, 66)  mapped=(294,155) zone=TOP_CENTER
BOTTOM_CENTER: raw=(316,194)  mapped=(487,329) zone=BOTTOM_CENTER
BOTTOM_RIGHT : raw=(350,198)  mapped=(539,360) zone=BOTTOM_RIGHT
CENTER_RIGHT : raw=(373,134)  mapped=(598,310) zone=CENTER_RIGHT
TOP_RIGHT    : raw=(348, 53)  mapped=(595,154) zone=TOP_RIGHT
BOTTOM_LEFT  : raw=(108,235)  mapped=(263,348) zone=BOTTOM_LEFT
CENTER_LEFT  : raw=(55,122)   mapped=(120,307) zone=CENTER_LEFT
TOP_LEFT     : raw=(49, 30)   mapped=(93,152)  zone=TOP_LEFT
```

The mapped values are intentionally filtered; exact raw values vary by finger position and pressure. The acceptance criterion is correct physical zone coverage and no driver read/point failures.

## Interpretation

This test proves the isolated touch layer is usable:

```text
GT911 controller is detected through the BSP class;
raw coordinate space is the expected 480x272-ish GT911 space;
BSP normalization maps touches into the 800x480 screen coordinate system;
all 9 logical zones were observed;
release reporting works;
no read/point failures were reported in the final ALIVE line.
```

## Decision

Proceed to the combined display + touch LVGL experiment:

```text
15_LVGL_EspLcdBasicUI
```

That next test should combine:

```text
13_LVGL_EspLcdStatic display path;
14_GT911_NormalizedTouch touch path;
small LVGL button only;
no slider;
no moving animation;
minimal redraw area.
```

## PASS boundary

A positive result here means:

```text
GT911 BSP normalization produces plausible 800x480 coordinates and full 3x3 zone coverage on Sample A.
```

It does not prove:

```text
LVGL pointer integration;
interactive LVGL widget stability;
touch smoothness under display redraw;
final gesture behavior;
multi-touch;
Widget Runtime;
user-facing HMI quality.
```
