# 14_GT911_NormalizedTouch

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

Firmware ID:

```text
14TOUCH-NORM1-240827A
```

## Purpose

This example is the next isolated subsystem test after:

```text
13_LVGL_EspLcdStatic
PHYSICAL STATIC PASS CANDIDATE / TOUCH NOT TESTED
```

The 13th test showed that a static LVGL 8 UI can run over the native `esp_lcd` RGB panel path after the default-font patch.

The 14th test deliberately moves away from display rendering and tests only the GT911 touch normalization layer.

## What it uses

```text
Arduino sketch
ESP32_8048S043_Touch BSP class
Wire / I2C
GT911 SDA=19, SCL=20, RST=38, INT=18
Serial Monitor at 115200 baud
```

## What it intentionally does not use

```text
display initialization
Arduino_GFX
esp_lcd display drawing
LVGL
moving markers
buttons/sliders
Wi-Fi
SD
BLE
```

The backlight is forced OFF intentionally because this is a serial-only touch diagnostic.

## What it checks

```text
GT911 controller detection at 0x5D or 0x14;
firmware and raw resolution registers;
raw GT911 coordinates;
BSP-normalized 800x480 coordinates;
3x3 zone classification;
touch/release behavior;
read counters and failure counters;
BSP filter activity.
```

The 3x3 zones are:

```text
TOP_LEFT       TOP_CENTER       TOP_RIGHT
CENTER_LEFT    CENTER           CENTER_RIGHT
BOTTOM_LEFT    BOTTOM_CENTER    BOTTOM_RIGHT
```

## Expected Serial output

```text
ESP32-8048S043 Lab / 14_GT911_NormalizedTouch
GT911 BSP normalized touch diagnostic
Firmware ID              : 14TOUCH-NORM1-240827A
Mode                     : GT911 BSP read only
Display                  : not used
Arduino_GFX              : not used
esp_lcd display          : not used
LVGL                     : not used
Mapped coordinate space  : 800x480
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272 int=...
[READY] Touch glass. Serial will report raw and normalized coordinates.
```

Touch reports look like:

```text
[TOUCH] raw=(240,136) mapped=(400,240) zone=CENTER size=... id=... status=0x81 accepted=... filtered=...
[ZONES] seen=1/9 mask=...
[RELEASE] releases=1 zones=1/9
[ALIVE] fw=14TOUCH-NORM1-240827A ... zones=.../9 ...
```

Exact raw values do not need to match this example. The important check is that mapped coordinates and zones match the physical touch position.

## How to test

Open Serial Monitor at 115200 baud.

Tap the physical glass in this order:

```text
1. top-left
2. top-center
3. top-right
4. center-left
5. center
6. center-right
7. bottom-left
8. bottom-center
9. bottom-right
```

The screen may stay dark because the display is intentionally unused. Tap by physical position on the panel.

## What to report

For the first physical run, record:

```text
Does it compile?
Does Serial show 14TOUCH-NORM1-240827A?
Does ESP32_8048S043_Touch::begin() pass?
Is the detected address 0x5D or 0x14?
What firmware and resolution are printed?
Does touching center report zone=CENTER?
Does top-left report TOP_LEFT?
Does top-right report TOP_RIGHT?
Does bottom-left report BOTTOM_LEFT?
Does bottom-right report BOTTOM_RIGHT?
How many zones are seen after tapping all 9 positions?
Are readFailures or pointFailures increasing?
Does release reporting work?
```

## Acceptance boundary

A pass here means:

```text
GT911 BSP normalization produces plausible 800x480 coordinates and correct 3x3 physical zones on Sample A.
```

It does not prove:

```text
LVGL pointer integration;
interactive buttons/sliders;
touch smoothness under display redraw;
final gesture behavior;
multi-touch;
Widget Runtime;
user-facing HMI quality.
```

## Next step after pass

If this test passes, the next step is:

```text
15_LVGL_EspLcdBasicUI
```

That future test should combine:

```text
13_LVGL_EspLcdStatic display path;
14_GT911_NormalizedTouch touch path;
small LVGL button only;
no slider;
no moving animation;
minimal redraw area.
```
