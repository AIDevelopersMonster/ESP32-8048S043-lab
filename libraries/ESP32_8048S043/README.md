# ESP32_8048S043 Arduino BSP

Experimental Arduino BSP skeleton for ESP32-8048S043 / ESP32-8048S043C-I boards.

## Status

```text
BSP API                 SKELETON / GROWING
01_BoardInfo            PHYSICAL PASS / SAMPLE A / PROFILE DIAGNOSTIC V2
02_DisplayRGBTest       PHYSICAL VISUAL PASS / SAMPLE A
03_TouchGT911Test       PHYSICAL VISUAL PASS / SAMPLE A
04_BacklightTest        PHYSICAL PASS REPORTED / SAMPLE A
05_TestConsole          PHYSICAL INTEGRATION PASS REPORTED / SAMPLE A
06_WiFiTest             FULL WIFI PHYSICAL PASS CANDIDATE / SAMPLE A
07_WebServerTest        WEB SERVER PHYSICAL PASS CANDIDATE / SAMPLE A
08_SDCardTest           READ-ONLY SD PHYSICAL PASS CANDIDATE / SAMPLE A
Display driver          OWN MINIMAL ARDUINO_GFX TEST PASS
Touch driver            GT911 POLLING VISUAL TEST PASS
Backlight driver        DIGITAL/PWM TEST REPORTED PASS
Combined console        RGB + GT911 + BACKLIGHT TEST REPORTED PASS
Wi-Fi radio/network     SCAN + ASSOCIATION + DHCP + DNS + TCP/HTTP + RECONNECT PASS CANDIDATE
HTTP server             BROWSER + JSON + PING PASS CANDIDATE
SD / TF                 READ-ONLY MOUNT + METADATA + LIST PASS CANDIDATE
LVGL port               OPEN
Physical PASS claims    SAMPLE A BOARDINFO + RGB DISPLAY + GT911 TOUCH + BACKLIGHT + CONSOLE + WIFI + WEB + READ-ONLY SD + FACTORY LVGL DISPLAY/TOUCH VISUAL
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
01_BoardInfo            first Arduino IDE smoke test, chip/flash/PSRAM/profile/ALIVE
02_DisplayRGBTest       minimal Arduino_GFX RGB/backlight/color/orientation test
03_TouchGT911Test       GT911 polling visual marker + serial diagnostics
04_BacklightTest        dedicated backlight GPIO2 ON/OFF/blink/PWM test
05_TestConsole          combined RGB + GT911 + backlight diagnostic console
06_WiFiTest             Wi-Fi scan + association/DHCP/DNS/TCP/reconnect test
07_WebServerTest        Wi-Fi/SoftAP + browser HTTP server + JSON status + ping
08_SDCardTest           read-only SD mount + metadata + directory listing
09_BLETest              future
10_LVGL_BasicUI         future
11_LVGL_Dashboard       future
13_RetroClock_800x480   future
20_LVGL_GitHubOTA       future
21_LVGL_WidgetLoader    future
```

## 01_BoardInfo

Purpose:

```text
verify basic Arduino IDE upload, serial monitor, ESP32-S3 identity, 16 MB flash, 8 MB PSRAM, profile macros and ALIVE stability
```

Current Sample A status:

```text
PHYSICAL PASS / PROFILE DIAGNOSTIC V2
```

## 02_DisplayRGBTest

Purpose:

```text
validate the source-backed ESP32-8048S043 RGB GPIO map with our own minimal Arduino sketch
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

Current Sample A status:

```text
PHYSICAL VISUAL PASS
```

## 04_BacklightTest

Purpose:

```text
validate the ESP32-8048S043 backlight control path separately from RGB display and touch
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

Current Sample A status:

```text
PHYSICAL INTEGRATION PASS REPORTED / SAMPLE A
```

## 06_WiFiTest

Purpose:

```text
validate the ESP32-S3 Wi-Fi radio/network path before Web setup, GitHub OTA and Widget Runtime work
```

Evidence:

```text
evidence/specimens/sample-a/arduino/06-wifi-scan-20260825.md
evidence/specimens/sample-a/arduino/06-wifi-full-infrastructure-20260825.md
https://youtube.com/shorts/DOus0uNBBZI
```

Current Sample A status:

```text
FULL WIFI PHYSICAL PASS CANDIDATE / SAMPLE A
```

## 07_WebServerTest

Purpose:

```text
validate the first browser-accessible HTTP server layer before Web setup, GitHub OTA dashboard and Widget Runtime upload/control
```

Evidence:

```text
evidence/specimens/sample-a/arduino/07-webserver-sta-20260825.md
```

Current Sample A status:

```text
WEB SERVER PHYSICAL PASS CANDIDATE / SAMPLE A
```

## 08_SDCardTest

Purpose:

```text
validate the source-backed microSD / TF SPI pin map before SD-backed logs, file upload, Widget Runtime storage or offline asset loading
```

Pin map:

```text
CS=10 MOSI=11 CLK=12 MISO=13
```

Evidence:

```text
evidence/specimens/sample-a/arduino/08-sdcard-readonly-20260826.md
```

Current Sample A result:

```text
Card type          : SDHC/SDXC
Mounted frequency  : 10000000 Hz
Card size          : 32220119040 bytes / 30.01 GB
Filesystem total   : 32211599360 bytes / 30.00 GB
Directory listing  : PASS, root and System Volume Information listed
No write operations: PASS, read-only test
```

Current Sample A status:

```text
READ-ONLY SD PHYSICAL PASS CANDIDATE / SAMPLE A
```

## Rule

Examples may compile before hardware validation, but README status must not say PHYSICAL PASS until the named specimen evidence exists.
