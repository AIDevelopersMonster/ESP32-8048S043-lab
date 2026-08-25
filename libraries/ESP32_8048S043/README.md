# ESP32_8048S043 Arduino BSP

Experimental Arduino BSP skeleton for ESP32-8048S043 / ESP32-8048S043C-I boards.

## Status

```text
BSP API                 SKELETON / GROWING
01_BoardInfo            PHYSICAL PASS / SAMPLE A
02_DisplayRGBTest       PHYSICAL VISUAL PASS / SAMPLE A
03_TouchGT911Test       PHYSICAL VISUAL PASS / SAMPLE A
04_BacklightTest        PHYSICAL PASS REPORTED / SAMPLE A
05_TestConsole          PHYSICAL INTEGRATION PASS REPORTED / SAMPLE A / PSRAM REPORT CAVEAT
06_WiFiTest             SCAN PHYSICAL PASS CANDIDATE / SAMPLE A / INFRASTRUCTURE PENDING
Display driver          OWN MINIMAL ARDUINO_GFX TEST PASS
Touch driver            GT911 POLLING VISUAL TEST PASS
Backlight driver        DIGITAL/PWM TEST REPORTED PASS
Combined console        RGB + GT911 + BACKLIGHT TEST REPORTED PASS
Wi-Fi radio             SERIAL SCAN PASS CANDIDATE / INFRASTRUCTURE PENDING
LVGL port               OPEN
Physical PASS claims    SAMPLE A BOARDINFO + OWN RGB DISPLAY + OWN GT911 TOUCH + BACKLIGHT REPORTED + TEST CONSOLE REPORTED + WIFI SCAN CANDIDATE + FACTORY LVGL DISPLAY/TOUCH VISUAL
```

## Arduino IDE board setup

Recommended working profile for the examples in this library on Sample A:

```text
Board package : local sketchbook hardware profile
Board         : ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
FQBN          : AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
Port          : CH340 / USB-SERIAL port of the board
Upload Speed  : 921600 if stable; 460800 fallback
CPU Frequency : 240MHz (WiFi)
Flash Size    : 16MB / 128Mb
Flash Mode    : QIO 80MHz
Partition     : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM         : OPI PSRAM
USB CDC Boot  : Disabled when using CH340C USB-UART
Upload Mode   : UART0 / Hardware CDC, depending on Arduino menu wording
Core Debug    : None
Serial Monitor: 115200 baud
```

Local platform setup guide:

```text
boards/arduino-ide/esp32-8048s043-lab/LOCAL_PLATFORM_SETUP.md
```

Safe fallback while debugging remains `ESP32S3 Dev Module` with the same 16 MB flash / OPI PSRAM / 3 MB app profile.

## Example plan

```text
01_BoardInfo            first Arduino IDE smoke test, chip/flash/PSRAM/ALIVE
02_DisplayRGBTest       minimal Arduino_GFX RGB/backlight/color/orientation test
03_TouchGT911Test       GT911 polling visual marker + serial diagnostics
04_BacklightTest        dedicated backlight GPIO2 ON/OFF/blink/PWM test
05_TestConsole          combined RGB + GT911 + backlight diagnostic console
06_WiFiTest             Wi-Fi scan + optional association/DHCP/DNS/TCP/reconnect test
07_BLETest              future
08_SDCardTest           future
09_LVGL_BasicUI         future
10_LVGL_Dashboard       future
13_RetroClock_800x480   future
20_LVGL_GitHubOTA       future
21_LVGL_WidgetLoader    future
```

## 01_BoardInfo

Purpose:

```text
verify basic Arduino IDE upload, serial monitor, ESP32-S3 identity, 16 MB flash and 8 MB PSRAM
```

PASS boundary:

```text
PASS requires successful upload, serial output at 115200, ESP32-S3 identity, about 16 MB flash, about 8 MB PSRAM and stable ALIVE messages.
```

Current Sample A status:

```text
PHYSICAL PASS
```

## 02_DisplayRGBTest

Purpose:

```text
validate the source-backed ESP32-8048S043 RGB GPIO map with our own minimal Arduino sketch
```

What it tests:

```text
Arduino_GFX RGB panel bring-up;
backlight GPIO2 full ON;
red / green / blue / white / black screens;
landscape 800x480 orientation frame;
RGB color bars;
stripe/data-line sanity pattern.
```

Current Sample A status:

```text
PHYSICAL VISUAL PASS
```

## 03_TouchGT911Test

Purpose:

```text
validate the GT911 capacitive touch path with our own visual Arduino sketch using a known-good-style polling pattern
```

What it tests:

```text
Arduino_GFX display init;
I2C on SDA=19 / SCL=20;
GT911 address 0x5D or 0x14;
Product ID register 0x8140;
firmware/resolution registers;
status register 0x814E;
point data from 0x814F;
serial raw/screen coordinates;
visible red touch marker.
```

Current Sample A status:

```text
PHYSICAL VISUAL PASS
```

## 04_BacklightTest

Purpose:

```text
validate the ESP32-8048S043 backlight control path separately from RGB display and touch
```

What it tests:

```text
Arduino_GFX reference screen;
source-backed backlight GPIO2;
GPIO2 HIGH / LOW behavior;
visible blink sequence;
PWM / analogWrite duty steps;
serial output for every backlight stage.
```

Current Sample A status:

```text
PHYSICAL PASS REPORTED / SAMPLE A
```

## 05_TestConsole

Purpose:

```text
run RGB display, GT911 polling touch, backlight GPIO2 control and serial diagnostics together in one direct Arduino_GFX diagnostic console before LVGL
```

What it tests:

```text
Arduino_GFX diagnostic console;
GT911 detection and touch point mapping;
red marker follows touch;
BACKLIGHT button toggles GPIO2;
CLEAR button resets touch counter;
REPORT button prints serial diagnostics;
combined no-brownout/no-crash observation.
```

Current Sample A status:

```text
PHYSICAL INTEGRATION PASS REPORTED / SAMPLE A
```

Boundary note:

```text
In one observed 05_TestConsole run, the console report printed PSRAM as 0 bytes while 01_BoardInfo under the same local board profile reported 8 MB PSRAM. Treat 01_BoardInfo as the current PSRAM acceptance test and 05_TestConsole as the combined RGB + GT911 + backlight integration test until the console memory report is rechecked.
```

## 06_WiFiTest

Purpose:

```text
validate the ESP32-S3 Wi-Fi radio/network path before Web setup, GitHub OTA and Widget Runtime work
```

Lineage:

```text
adapted from WT32-SC01-PLUS-Lab / 08_WiFiTest pattern
```

What it tests:

```text
Wi-Fi STA mode;
STA MAC readout;
active scan;
optional association;
optional DHCP;
optional DNS;
optional TCP/HTTP HEAD request;
optional disconnect/reconnect cycles.
```

Open:

```text
libraries/ESP32_8048S043/examples/06_WiFiTest/06_WiFiTest.ino
```

Evidence:

```text
evidence/specimens/sample-a/arduino/06-wifi-scan-20260825.md
https://youtube.com/shorts/DOus0uNBBZI
```

Current Sample A scan-only result:

```text
STA MAC            : 84:FC:E6:6C:69:3C
Active scan        : PASS, 3 network(s) found
Infrastructure     : PENDING, no wifi_secrets.h present
```

Secrets workflow:

```text
copy wifi_secrets.example.h to wifi_secrets.h locally;
fill WIFI_TEST_SSID / WIFI_TEST_PASSWORD;
do not commit wifi_secrets.h.
```

Current Sample A status:

```text
SCAN PHYSICAL PASS CANDIDATE / SAMPLE A / FULL INFRASTRUCTURE PENDING
```

## Rule

Examples may compile before hardware validation, but README status must not say PHYSICAL PASS until the named specimen evidence exists.
