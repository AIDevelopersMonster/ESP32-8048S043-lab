# 03_TouchGT911Test

Status: `SOURCE IMPLEMENTED / SAFE VISUAL VERSION / PHYSICAL VALIDATION OPEN`.

Author: **Alex Malachevsky**

Project GitHub:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
```

## Purpose

`03_TouchGT911Test` is the third incremental test from the local `ESP32_8048S043` Arduino library.

It comes after:

```text
01_BoardInfo       PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
02_DisplayRGBTest  PASS: own Arduino_GFX RGB display path
```

The purpose of this test is to validate the capacitive touchscreen path with our own code:

```text
GT911 controller detection;
I2C pins SDA=19 and SCL=20;
GT911 candidate addresses 0x5D and 0x14;
Product ID register read;
raw touch coordinate reporting;
optional low-power on-screen touch visualization.
```

## Important Sample A note

A first combined visual version produced a black screen and repeated brownout resets on Sample A:

```text
E BOD: Brownout detector was triggered
```

The current version is therefore staged and safer:

```text
Serial starts first;
I2C starts before display;
I2C speed is reduced to 100 kHz;
GT911 reset/address strap is not forced by default;
Display is enabled only after GT911 is detected;
Backlight is started dimmed instead of full ON.
```

This keeps the already useful serial GT911 evidence while still allowing a visual touch trail when the power path is stable enough.

## What this test checks

```text
I2C bus starts on SDA=19 / SCL=20;
I2C scan reports connected devices;
GT911 is found at 0x5D or 0x14;
GT911 Product ID can be read from 0x8140;
firmware/config/resolution registers are read where available;
touch status register 0x814E is readable;
touching the panel prints x/y coordinates;
coordinates change when the finger moves;
if display init succeeds, touches draw dots/trails on screen.
```

## What this test does not check

```text
final LVGL touch integration;
final coordinate rotation/mapping;
calibration;
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

Required display dependency for visual mode:

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
 Safe low-power visual GT911 touch validation
================================================================
Author : Alex Malachevsky
GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
Purpose: validate GT911 first, then enable dim visual touch canvas
----------------------------------------------------------------
Safe mode: I2C first, 100 kHz, no forced reset strap, dim backlight
----------------------------------------------------------------
Wire.begin(SDA=19, SCL=20, speed=100000)
```

Then it should scan I2C and probe the expected GT911 addresses:

```text
[I2C SCAN]
I2C device found at 0x5D
...
[GT911 PROBE]
Candidate primary address  : 0x5D
Candidate alternate address: 0x14
Pins: SDA=19 SCL=20 RST=38 INT=18
Active GT911 address: 0x5D
```

Depending on the board strap state, the active address may be:

```text
0x5D
```

or:

```text
0x14
```

Both are acceptable for GT911 detection on this board family.

## Expected visual output

After GT911 is detected, the display should start in low-power visual mode:

```text
03 Touch GT911 Safe Visual
Touch the panel: dots/trails should follow your finger
GT911 0x5D active. Dim visual mode.
```

Touching the panel should draw:

```text
yellow touch dots;
green movement trails;
status line with raw/screen coordinates.
```

## Expected touch output

When touching the panel, Serial Monitor should show lines similar to:

```text
Touch points=1 status=0x81
  P1 id=... x=155 y=57 size=... raw=...
```

The current parser auto-selects coordinate byte order that fits the 800x480 panel. This fixes the earlier impossible values such as `x=39680 y=14592` that came from reading Sample A point bytes in the wrong order.

## PASS condition

Mark `03_TouchGT911Test` as PASS only when all of the following are true on a named specimen:

```text
sketch uploads successfully;
serial monitor opens at 115200;
I2C starts on SDA=19 / SCL=20;
GT911 candidate is detected at 0x5D or 0x14;
Product ID or GT911 register block is readable;
touching the panel prints point data;
x/y coordinates change with finger movement;
if display mode is enabled, touches draw dots/trails on the screen;
no brownout loop or crash during the observed test.
```

Recommended evidence:

```text
serial log from boot through Product ID read;
serial log showing at least several touch coordinate lines;
short video showing finger movement and dots/trails on the display.
```

## FAIL / investigate cases

Investigate before marking PASS if:

```text
brownout detector is triggered;
black screen with repeated resets;
no I2C devices are found;
only unrelated I2C addresses are found;
0x5D and 0x14 both fail;
Product ID read fails on the detected address;
touch status never changes while touching;
coordinates are always zero or frozen;
board resets during touch polling.
```

Possible causes:

```text
USB power path too weak for display + backlight + RGB + touch together;
wrong or marginal USB cable/hub;
backlight current spike;
wrong SDA/SCL pins;
GT911 reset/address strap issue;
FPC/panel cable issue;
wrong board variant;
controller present but not GT911-compatible.
```

## Boundary

A PASS here confirms the low-level GT911/I2C touch path and optional on-screen touch visualization only.

It does not prove final GUI touch behavior. LVGL coordinate mapping, rotation, calibration and UI event handling must be validated later in LVGL examples.
