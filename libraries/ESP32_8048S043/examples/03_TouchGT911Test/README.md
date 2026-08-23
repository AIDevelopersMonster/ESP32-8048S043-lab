# 03_TouchGT911Test

Status: `SOURCE IMPLEMENTED / VISUAL TEST ADDED / PHYSICAL VALIDATION OPEN`.

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

The purpose of this test is to validate the capacitive touchscreen path with our own code and make the result visible directly on the display:

```text
GT911 controller detection;
I2C pins SDA=19 and SCL=20;
GT911 candidate addresses 0x5D and 0x14;
optional reset/address strap pins RST=38 and INT=18;
Product ID register read;
raw touch coordinate reporting;
visual touch dots/trails on the 800x480 display.
```

This is now a visual test. It still prints diagnostics to Serial Monitor, but the main operator-facing result is on the LCD: touching the panel should draw dots and trails.

## Why the test was changed to visual mode

The first serial-only version proved that touch events are arriving, but it was not convenient for real validation. It also decoded touch coordinates in the wrong byte order for the observed Sample A data.

Observed serial evidence had lines like:

```text
Touch points=1 status=0x81
  P1 id=203 x=39680 y=14592 size=0 raw=CB 00 9B 00 39 00 00 00
```

The raw data is meaningful when x/y are read high-byte first:

```text
00 9B -> x = 155
00 39 -> y = 57
```

So the visual version now decodes GT911 point coordinates as high-byte-first for x/y and draws them directly on the 800x480 display.

## What this test checks

```text
RGB display initializes through Arduino_GFX;
I2C bus starts on SDA=19 / SCL=20;
I2C scan reports connected devices;
GT911 is found at 0x5D or 0x14;
GT911 Product ID can be read from 0x8140;
firmware/config/resolution registers are read where available;
touch status register 0x814E is readable;
touching the panel prints x/y coordinates;
touching the panel draws visible dots/trails;
coordinates change when the finger moves.
```

## What this test does not check

```text
LVGL touch integration;
final coordinate calibration;
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

## Dependencies

Required Arduino libraries:

```text
ESP32_8048S043
Arduino_GFX_Library by moononournation
```

The touch part uses Arduino `Wire` directly. No external GT911 library is required.

## How to run

Open from Arduino IDE:

```text
File -> Examples -> ESP32_8048S043 -> 03_TouchGT911Test
```

Upload the sketch and open Serial Monitor:

```text
115200 baud
```

## Expected display result

The screen should show:

```text
03 Touch GT911 Visual Test
Touch the panel: dots/trails should follow your finger
GT911 address 0x5D active. Touch the screen.
```

or the same with address:

```text
0x14
```

The display also shows corner/center target markers.

When touching the panel:

```text
a yellow dot appears at the touch position;
a green line/trail follows finger movement;
the bottom status line updates with touch id, raw x/y and screen x/y;
repeated touches should not reset the board.
```

## Expected serial output

The test should print a header similar to:

```text
================================================================
 ESP32-8048S043 Lab / 03_TouchGT911Test
 Visual GT911 touch validation
================================================================
Author : Alex Malachevsky
GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
Purpose: validate own visual GT911 touch path after 01/02 PASS
----------------------------------------------------------------
Display begin: OK
Wire.begin(SDA=19, SCL=20, speed=400000)
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

## Expected touch output

When touching the panel, Serial Monitor should show lines similar to:

```text
Touch points=1 status=0x81
  P1 id=... x=155 y=57 size=... raw=...
```

The exact x/y values depend on where the panel is touched. They should now be in a sane 800x480-class range, not huge swapped-byte values.

## PASS condition

Mark `03_TouchGT911Test` as PASS only when all of the following are true on a named specimen:

```text
sketch uploads successfully;
display shows the visual touch test screen;
serial monitor opens at 115200;
I2C starts on SDA=19 / SCL=20;
GT911 candidate is detected at 0x5D or 0x14;
Product ID or GT911 register block is readable;
touching the panel prints point data;
touching the panel draws visible dots/trails;
raw x/y coordinates change with finger movement;
visual touch location is plausible for the finger position;
no boot loop or crash during the observed test.
```

Recommended evidence:

```text
serial log from boot through Product ID read;
serial log showing several touch coordinate lines;
short video showing finger movement and dots/trails on the display.
```

## FAIL / investigate cases

Investigate before marking PASS if:

```text
no I2C devices are found;
only unrelated I2C addresses are found;
0x5D and 0x14 both fail;
Product ID read fails on the detected address;
touch status never changes while touching;
coordinates are always zero or frozen;
coordinates are still huge swapped-byte values;
dots appear mirrored, rotated or badly offset;
board resets during touch polling.
```

Possible causes:

```text
wrong SDA/SCL pins;
GT911 reset/address strap issue;
FPC/panel cable issue;
wrong board variant;
I2C speed too high for the specimen;
coordinate byte-order mismatch;
controller present but not GT911-compatible;
rotation/mapping still needs adjustment for LVGL.
```

## Boundary

A PASS here confirms the low-level GT911/I2C path and a first visual coordinate sanity check.

It does not prove final GUI touch behavior. LVGL coordinate mapping, rotation, calibration and UI event handling must be validated later in LVGL examples.
