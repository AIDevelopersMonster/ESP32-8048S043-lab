# 03_TouchGT911Test

Status: `PHYSICAL VISUAL PASS / SAMPLE A`.

Author: **Alex Malachevsky**

Project GitHub:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
```

Video evidence:

```text
https://youtube.com/shorts/_zhtl-AWcCE
```

## Sample A result

`03_TouchGT911Test` has now passed on Sample A as the third incremental Arduino-library test.

Confirmed result:

```text
gfx->begin()                 PASS
GT911 I2C address             PASS, 0x5D
GT911 Product ID              PASS, 911
GT911 FW version              PASS, 0x1060
Touch polling                 PASS
Raw touch coordinates          PASS
Mapped screen coordinates      PASS candidate
Visible red touch marker       PASS by video evidence
Overall 03_TouchGT911Test     PHYSICAL VISUAL PASS / SAMPLE A
```

Boundary:

```text
This PASS confirms the low-level GT911/I2C path and basic on-screen touch visualization.
It does not yet prove final LVGL calibration, rotation handling, gestures or full GUI event integration.
```

## Purpose

`03_TouchGT911Test` is the third incremental test from the local `ESP32_8048S043` Arduino library.

It comes after:

```text
01_BoardInfo       PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
02_DisplayRGBTest  PASS: own Arduino_GFX RGB display path
```

The purpose of this test is to validate the capacitive touchscreen path with our own code and a visible on-screen marker.

## Why this version was rewritten

The first visual version was too different from working ESP32-8048S043C examples and produced a black-screen/brownout loop on Sample A.

The current version is rewritten in a closer known-good style:

```text
static screen first;
backlight GPIO2 HIGH, same simple path as 02_DisplayRGBTest;
GT911 polling, no dependency on INT interrupt;
GT911 status register 0x814E;
GT911 point data starts at 0x814F, not 0x8150;
point data is decoded as track_id + little-endian x/y/size;
status 0x814E is cleared after each point read;
visual marker update is throttled, no fast full-screen animation.
```

## What this test checks

```text
RGB display starts through Arduino_GFX;
static 800x480 test screen is drawn;
backlight GPIO2 works in full ON mode;
I2C starts on SDA=19 / SCL=20;
GT911 is found at 0x5D or 0x14;
GT911 Product ID can be read from 0x8140;
firmware/config/resolution registers are read where available;
touch status register 0x814E is readable;
touch point data is read from 0x814F;
touching the panel prints raw x/y coordinates;
raw coordinates are mapped to screen coordinates with a calibration seed;
if display mode works, touches move a red marker on the screen.
```

## What this test does not check

```text
final LVGL touch integration;
final production calibration;
gestures;
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

The touch side itself uses Arduino `Wire` directly.

## How to run

Open from Arduino IDE:

```text
File -> Examples -> ESP32_8048S043 -> 03_TouchGT911Test
```

Upload the sketch and open Serial Monitor:

```text
115200 baud
```

## Expected serial output

The test should print a header similar to:

```text
================================================================
 ESP32-8048S043 Lab / 03_TouchGT911Test
 Known-good-style Arduino_GFX + GT911 polling test
================================================================
Point register: 0x814F, status register: 0x814E
```

Then display and I2C/touch setup:

```text
gfx->begin() start
gfx->begin(): OK
Wire.begin(SDA=19, SCL=20, speed=400000)
GT911 reset: RST38 toggle, INT18 passive pull-up, polling mode
GT911 at 0x5D, product id raw: ...
GT911 product id text: ...
I2C scan: 0x5D
Active GT911 address: 0x5D
```

The active address can also be:

```text
0x14
```

Both are acceptable for this board family.

## Expected visual output

The screen should show:

```text
ESP32-8048S043 GT911 Display + Touch Test
color bars;
grid;
four corner target circles;
a center instruction panel;
I2C / GT911 status line.
```

When touching the panel, a red marker/cross should appear and move with the finger. The lower status box should show both mapped screen coordinates and raw coordinates.

## Expected touch output

When touching the panel, Serial Monitor should show lines similar to:

```text
Touch raw packet: status=0x81 points=1 data=...
Touch #1: track=... raw_x=... raw_y=... screen_x=... screen_y=... size=...
```

Important register layout used here:

```text
0x814E = status
0x814F = point data start
point[0] = track id
point[1..2] = x, little-endian
point[3..4] = y, little-endian
point[5..6] = touch size, little-endian
```

## PASS condition

`03_TouchGT911Test` can be marked PASS on Sample A because the following evidence exists:

```text
sketch uploads successfully;
serial monitor opens at 115200;
static display screen is visible;
I2C starts on SDA=19 / SCL=20;
GT911 is detected at 0x5D;
Product ID is readable as 911;
GT911 firmware/version data is readable;
touching the panel prints point packets;
raw x/y coordinates change with finger movement;
red marker movement is shown on video;
no brownout loop or crash during the observed successful test.
```

Evidence:

```text
https://youtube.com/shorts/_zhtl-AWcCE
```

## FAIL / investigate cases

Investigate before marking PASS on any other specimen if:

```text
brownout detector is triggered;
black screen with repeated resets;
static display screen does not appear;
no I2C devices are found;
0x5D and 0x14 both fail;
Product ID read fails on the detected address;
touch status never changes while touching;
coordinates are always zero or frozen;
marker moves in the wrong direction or only in a small area.
```

Possible causes:

```text
wrong point register offset;
wrong coordinate decoding;
wrong or missing GT911 reset sequence;
wrong board variant;
GT911 INT line assumption, use polling instead;
calibration constants need adjustment;
RGB framebuffer update conflict if drawing too aggressively;
USB power/cable issue if real brownout is still present.
```

## Boundary

A PASS here confirms the low-level GT911/I2C touch path and basic on-screen visualization only.

It does not prove final GUI touch behavior. LVGL coordinate mapping, rotation, calibration and UI event handling must be validated later in LVGL examples.
