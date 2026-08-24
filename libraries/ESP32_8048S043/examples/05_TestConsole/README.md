# 05_TestConsole

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

Author: **Alex Malachevsky**

Project GitHub:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab
```

## Purpose

`05_TestConsole` is the fifth incremental test from the local `ESP32_8048S043` Arduino library.

It comes after the individual hardware blocks have already been tested separately:

```text
01_BoardInfo       PASS: ESP32-S3 / 16 MB flash / 8 MB PSRAM / serial alive
02_DisplayRGBTest  PASS: own Arduino_GFX RGB display path
03_TouchGT911Test  PASS: own GT911 polling visual touch test
04_BacklightTest   PASS candidate: GPIO2 backlight ON/OFF/PWM behavior
```

The purpose of this test is to run the already validated parts together in a single local diagnostic console before moving to LVGL.

## What this test checks

```text
RGB display starts through Arduino_GFX;
static 800x480 diagnostic console is drawn;
ESP32-S3 chip/flash/PSRAM/heap information is displayed;
I2C starts on SDA=19 / SCL=20;
GT911 is detected at 0x5D or 0x14;
GT911 Product ID can be read from 0x8140;
GT911 touch points are read by polling;
touch point data starts at 0x814F;
raw coordinates are mapped to screen coordinates;
visible red touch marker moves on the display;
BACKLIGHT button toggles GPIO2 ON/OFF;
CLEAR button resets the touch counter;
REPORT button prints a serial diagnostic report.
```

## What this test does not check

```text
LVGL;
gestures;
SD card;
Wi-Fi/BLE;
final production calibration;
final UI framework;
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

Required project library:

```text
ESP32_8048S043
```

Required display dependency:

```text
Arduino_GFX_Library by moononournation
```

The touch side uses Arduino `Wire` directly.

## How to run

Open from Arduino IDE:

```text
File -> Examples -> ESP32_8048S043 -> 05_TestConsole
```

Upload the sketch and open Serial Monitor:

```text
115200 baud
```

## Expected serial output

The test should print a header similar to:

```text
================================================================
 ESP32-8048S043 Lab / 05_TestConsole
 Combined RGB + GT911 + Backlight diagnostic console
================================================================
Serial : 115200 baud
```

Then display and touch setup:

```text
gfx->begin() start
gfx->begin(): OK
Wire.begin(SDA=19, SCL=20, speed=400000)
GT911 at 0x5D, product id raw: ...
GT911 product id text: 911.
I2C scan: 0x5D
Active GT911 address: 0x5D
GT911 FW version: ...
GT911 resolution registers: ...
```

Touching the screen should print lines similar to:

```text
Touch #1: track=0 raw_x=... raw_y=... screen_x=... screen_y=... size=...
```

Tapping the buttons should print:

```text
Button: BACKLIGHT -> OFF
Button: BACKLIGHT -> ON
Button: CLEAR -> touch counter reset
Button: REPORT
[TEST CONSOLE REPORT]
...
[/TEST CONSOLE REPORT]
```

## Expected visual output

The screen should show a simple diagnostic console:

```text
ESP32-8048S043 05_TestConsole
System panel with chip/flash/PSRAM/backlight info;
Touch panel with I2C/GT911 status;
live touch area;
BACKLIGHT button;
CLEAR button;
REPORT button;
lower status line with touch coordinates, heap and last action.
```

When touching the panel, a red marker/cross should appear and move with the finger.

## PASS condition

Mark `05_TestConsole` as PASS only when all of the following are true on a named specimen:

```text
sketch uploads successfully;
serial monitor opens at 115200;
static diagnostic console is visible;
ESP32/flash/PSRAM information is displayed;
GT911 is detected at 0x5D or 0x14;
touching the screen prints raw and mapped coordinates;
red marker moves on the display;
BACKLIGHT button toggles the display backlight;
CLEAR button resets the counter;
REPORT button prints a serial report;
no brownout loop or crash during the observed test.
```

Recommended evidence:

```text
serial log from boot through GT911 detection;
serial log showing several touch coordinate lines;
video showing finger movement, red marker movement and at least one button action.
```

## Boundary

A PASS here confirms that the previously validated low-level blocks can run together in one direct Arduino_GFX/Wire diagnostic console.

It does not prove LVGL UI behavior. LVGL rendering, LVGL input driver mapping, event handling, widgets and memory behavior must be validated later in dedicated LVGL examples.
