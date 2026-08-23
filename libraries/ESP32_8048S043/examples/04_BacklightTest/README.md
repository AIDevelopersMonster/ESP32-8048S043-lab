# 04_BacklightTest

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

Author: **Alex Malachevsky**

Project GitHub:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
```

## Purpose

`04_BacklightTest` is the fourth incremental test from the local `ESP32_8048S043` Arduino library.

It comes after:

```text
01_BoardInfo       PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
02_DisplayRGBTest  PASS: own Arduino_GFX RGB display path
03_TouchGT911Test  PASS: own GT911 polling visual touch test
```

The purpose of this test is to validate the display backlight control path separately from RGB display and touch.

## What this test checks

```text
RGB display starts through Arduino_GFX;
static 800x480 test screen is drawn;
backlight control pin is GPIO2;
GPIO2 HIGH turns the backlight on;
GPIO2 LOW turns the backlight off;
repeated blink behavior is visible;
PWM / analogWrite duty steps are attempted;
brightness changes are observed if the board backlight circuit supports PWM;
serial log records every stage and duty value.
```

## What this test does not check

```text
final production brightness curve;
perceived brightness linearity;
gamma correction;
LVGL integration;
GT911 touch;
SD card;
Wi-Fi/BLE;
final full BSP status.
```

## Arduino IDE settings

Use the same working profile already validated for Sample A:

```text
Board                                  : ESP32S3 Dev Module
USB CDC On Boot                        : Disabled
CPU Frequency                          : 240MHz (WiFi)
Core Debug Level                       : None
USB DFU On Boot                        : Disabled
Erase All Flash Before Sketch Upload   : Disabled
Events Run On                          : Core 1
Flash Mode                             : QIO 80MHz
Flash Size                             : 16MB (128Mb)
JTAG Adapter                           : Disabled
Arduino Runs On                        : Core 1
USB Firmware MSC On Boot               : Disabled
Partition Scheme                       : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM                                  : OPI PSRAM
Upload Mode                            : UART0 / Hardware CDC
Upload Speed                           : 921600
USB Mode                               : Hardware CDC and JTAG
Zigbee Mode                            : Disabled
Serial Monitor                         : 115200 baud
```

If upload is unstable at 921600, retry upload at 460800 before changing other settings.

## Dependency

Required project library:

```text
ESP32_8048S043
```

Required display dependency:

```text
Arduino_GFX_Library by moononournation
```

## How to run

Open from Arduino IDE:

```text
File -> Examples -> ESP32_8048S043 -> 04_BacklightTest
```

Upload the sketch and open Serial Monitor:

```text
115200 baud
```

## Expected serial output

The test should print a header similar to:

```text
================================================================
 ESP32-8048S043 Lab / 04_BacklightTest
 Dedicated display backlight ON/OFF/PWM validation
================================================================
Target : BACKLIGHT GPIO2
```

Then it should initialize the display:

```text
gfx->begin() start
gfx->begin(): OK
Static screen drawn. Starting backlight sequence.
```

Then the sequence should repeat:

```text
[BACKLIGHT SEQUENCE]
Backlight pin: GPIO2
Backlight stage: DIGITAL OFF        duty=  0 note=screen should go dark
Backlight stage: DIGITAL ON         duty=255 note=full brightness
Backlight stage: BLINK 1 OFF        duty=  0 note=blink check
Backlight stage: BLINK 1 ON         duty=255 note=blink check
...
Backlight stage: PWM STEP ...       duty=... note=look for brightness change
Backlight stage: FINAL ON           duty=255 note=sequence complete
```

## Expected visual output

The screen should show:

```text
ESP32-8048S043 04_BacklightTest;
color bars;
grid;
backlight test instruction panel;
current stage and duty value.
```

During the sequence the physical screen should:

```text
turn dark at duty=0;
return to full brightness at duty=255;
blink three times;
try brightness levels 0, 16, 32, 64, 96, 128, 160, 192, 224, 255.
```

## PASS condition

Mark `04_BacklightTest` as PASS only when all of the following are true on a named specimen:

```text
sketch uploads successfully;
serial monitor opens at 115200;
static display screen is visible;
GPIO2 HIGH visibly turns the backlight on;
GPIO2 LOW visibly turns the backlight off;
blink sequence is visibly observed;
no brownout loop or crash during the observed test.
```

PWM brightness PASS is a separate stronger condition:

```text
intermediate duty steps visibly change brightness;
brightness changes are repeatable across the sequence;
no visible RGB corruption appears during PWM stages.
```

If ON/OFF works but intermediate PWM steps do not visibly change brightness, record:

```text
DIGITAL BACKLIGHT PASS / PWM DIMMING OPEN
```

not full PWM PASS.

Recommended evidence:

```text
serial log from boot through at least one complete sequence;
short video showing OFF/ON, blink and PWM/duty stages;
visual note whether intermediate duty values are actually distinguishable.
```

## FAIL / investigate cases

Investigate before marking PASS if:

```text
static display screen does not appear;
GPIO2 HIGH does not enable the backlight;
GPIO2 LOW does not turn the backlight off;
board resets during brightness changes;
intermediate PWM creates strong flicker or display corruption;
backlight remains always on regardless of GPIO2 state.
```

Possible causes:

```text
wrong backlight GPIO;
wrong board variant;
backlight transistor is active-low on another variant;
backlight circuit supports only enable/disable, not useful PWM;
Arduino core analogWrite behavior differs;
USB power/cable issue if brightness transitions trigger brownout.
```

## Boundary

A PASS here confirms only the low-level backlight control behavior on Sample A.

It does not prove final LVGL brightness settings, UI slider behavior or production brightness policy. Those belong to later LVGL/basic UI examples.
