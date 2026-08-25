# ESP32_8048S043 Arduino BSP

Experimental Arduino BSP skeleton for ESP32-8048S043 / ESP32-8048S043C-I boards.

## Status

```text
BSP API                 SKELETON / GROWING
01_BoardInfo            PHYSICAL PASS / SAMPLE A / PROFILE DIAGNOSTIC V2
02_DisplayRGBTest       PHYSICAL VISUAL PASS / SAMPLE A
03_TouchGT911Test       PHYSICAL VISUAL PASS / SAMPLE A
04_BacklightTest        PHYSICAL PASS REPORTED / SAMPLE A
05_TestConsole          PHYSICAL INTEGRATION PASS REPORTED / SAMPLE A / PSRAM REPORT CAVEAT
06_WiFiTest             FULL WIFI PHYSICAL PASS CANDIDATE / SAMPLE A
07_WebServerTest        WEB SERVER PHYSICAL PASS CANDIDATE / SAMPLE A / PSRAM REPORT CAVEAT
08_SDCardTest           SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN
Display driver          OWN MINIMAL ARDUINO_GFX TEST PASS
Touch driver            GT911 POLLING VISUAL TEST PASS
Backlight driver        DIGITAL/PWM TEST REPORTED PASS
Combined console        RGB + GT911 + BACKLIGHT TEST REPORTED PASS
Wi-Fi radio/network     SCAN + ASSOCIATION + DHCP + DNS + TCP/HTTP + RECONNECT PASS CANDIDATE
HTTP server             BROWSER VALIDATION PASS CANDIDATE
SD card                 SOURCE-BACKED PIN MAP / READ-ONLY TEST IMPLEMENTED
LVGL port               OPEN
Physical PASS claims    SAMPLE A BOARDINFO + OWN RGB DISPLAY + OWN GT911 TOUCH + BACKLIGHT REPORTED + TEST CONSOLE REPORTED + FULL WIFI CANDIDATE + WEB SERVER CANDIDATE + FACTORY LVGL DISPLAY/TOUCH VISUAL
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
01_BoardInfo            first Arduino IDE smoke test, chip/flash/PSRAM/ALIVE/profile diagnostics
02_DisplayRGBTest       minimal Arduino_GFX RGB/backlight/color/orientation test
03_TouchGT911Test       GT911 polling visual marker + serial diagnostics
04_BacklightTest        dedicated backlight GPIO2 ON/OFF/blink/PWM test
05_TestConsole          combined RGB + GT911 + backlight diagnostic console
06_WiFiTest             Wi-Fi scan + association/DHCP/DNS/TCP/reconnect test
07_WebServerTest        Wi-Fi/SoftAP + browser HTTP server + JSON status + ping
08_SDCardTest           read-only microSD / TF SPI mount + metadata + root listing
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
verify basic Arduino IDE upload, serial monitor, ESP32-S3 identity, 16 MB flash, 8 MB PSRAM and compile-time build profile macros
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

Boundary note:

```text
One earlier 05_TestConsole run reported PSRAM as 0 bytes. Later BoardInfo and WebServer diagnostics isolated this as a board/profile configuration issue rather than a hardware failure. Use 01_BoardInfo V2 as the current profile/PSRAM acceptance test before memory-heavy examples.
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
association;
DHCP;
DNS;
TCP/HTTP HEAD request;
disconnect/reconnect cycles.
```

Open:

```text
libraries/ESP32_8048S043/examples/06_WiFiTest/06_WiFiTest.ino
```

Evidence:

```text
evidence/specimens/sample-a/arduino/06-wifi-scan-20260825.md
evidence/specimens/sample-a/arduino/06-wifi-full-infrastructure-20260825.md
https://youtube.com/shorts/DOus0uNBBZI
```

Current Sample A full infrastructure result:

```text
STA MAC            : 84:FC:E6:6C:69:3C
Active scan        : PASS, 2 network(s) found
Association        : PASS, connected to configured AP
DHCP               : PASS, IPv4/gateway/DNS received
DNS                : PASS, example.com resolved
TCP/HTTP           : PASS, HTTP/1.1 200 OK
Reconnect          : PASS, 3/3 cycles completed
```

Secrets workflow:

```text
copy wifi_secrets.example.h to wifi_secrets.h locally;
fill WIFI_TEST_SSID / WIFI_TEST_PASSWORD;
do not commit wifi_secrets.h.
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

What it tests:

```text
STA connection with local wifi_secrets.h or SoftAP fallback;
WebServer starts on port 80;
root HTML page is reachable from a browser;
/status.json returns machine-readable board/network status;
/ping returns a minimal text response;
serial log records browser requests.
```

Open:

```text
libraries/ESP32_8048S043/examples/07_WebServerTest/07_WebServerTest.ino
```

Current Sample A status:

```text
WEB SERVER PHYSICAL PASS CANDIDATE / SAMPLE A / PSRAM REPORT CAVEAT
```

## 08_SDCardTest

Purpose:

```text
validate the source-backed microSD / TF SPI pin map with a read-only Arduino SD.h mount, metadata report and root directory listing
```

Pin map:

```text
CS=10 MOSI=11 CLK=12 MISO=13
```

What it tests:

```text
SD card mount;
card type and size;
filesystem total/used/free;
root directory listing;
optional first-file read-only HEX preview;
continued ALIVE output after the test.
```

Open:

```text
libraries/ESP32_8048S043/examples/08_SDCardTest/08_SDCardTest.ino
```

Current Sample A status:

```text
SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN
```

## Rule

Examples may compile before hardware validation, but README status must not say PHYSICAL PASS until the named specimen evidence exists.
