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
09_BLETest              BLE SCAN PHYSICAL PASS CANDIDATE / SAMPLE A
10_LVGL_BasicUI         FUNCTIONAL PASS CANDIDATE / SAMPLE A / TOUCH QUALITY OPEN
11_LVGL_Dashboard       DIAGNOSTIC FUNCTIONAL PASS CANDIDATE / DYNAMIC UX NOT ACCEPTABLE
12_DisplayEspLcdRGB     STATIC TRANSPORT PASS CANDIDATE / RAW DYNAMIC DRAW NOT ACCEPTABLE
13_LVGL_EspLcdStatic    PHYSICAL STATIC PASS CANDIDATE / TOUCH NOT TESTED
14_GT911_NormalizedTouch PHYSICAL PASS CANDIDATE / 9-ZONE NORMALIZATION PASS
Display driver          OWN MINIMAL ARDUINO_GFX TEST PASS + NATIVE ESP_LCD STATIC TRANSPORT PASS CANDIDATE
Touch driver            GT911 BSP API IMPLEMENTED / 9-ZONE NORMALIZATION PASS CANDIDATE
Backlight driver        DIGITAL/PWM TEST REPORTED PASS / LVGL SLIDER EXERCISED
Combined console        RGB + GT911 + BACKLIGHT TEST REPORTED PASS
Wi-Fi radio/network     SCAN + ASSOCIATION + DHCP + DNS + TCP/HTTP + RECONNECT PASS CANDIDATE
HTTP server             BROWSER + JSON + PING PASS CANDIDATE
SD / TF                 READ-ONLY MOUNT + METADATA + LIST PASS CANDIDATE
BLE radio/stack         ARDUINO BLE INIT + ACTIVE SCAN + ADV RECEIVE PASS CANDIDATE
LVGL port               STATIC ESP_LCD PATH PASS CANDIDATE / POLISHED DYNAMIC UX OPEN
Physical PASS claims    SAMPLE A BOARDINFO + RGB DISPLAY + GT911 TOUCH + BACKLIGHT + CONSOLE + WIFI + WEB + READ-ONLY SD + BLE SCAN + LVGL BASIC UI FUNCTIONAL + LVGL DASHBOARD STABILITY + ESP_LCD STATIC + LVGL ESP_LCD STATIC + GT911 9-ZONE NORMALIZATION + FACTORY LVGL DISPLAY/TOUCH VISUAL
```

## LVGL decision boundary

The local LVGL examples prove that the board can render LVGL screens, allocate PSRAM draw buffers, initialize GT911 and handle simple control intent. They do not yet prove a polished user-facing HMI path.

Current decision:

```text
10_LVGL_BasicUI    : functional button/slider proof, touch quality open
11_LVGL_Dashboard  : stable diagnostic dashboard, dynamic touch UX not acceptable
12_DisplayEspLcd   : native esp_lcd static transport works; raw dynamic draw not acceptable
13_LVGL_EspLcdStatic: LVGL 8 static UI over native esp_lcd works at 12.5 MHz
14_GT911_NormalizedTouch: GT911 BSP normalization covers all 9 physical zones
```

The manual-touch dashboard experiment reduced the area of the touch-time artifact but did not remove the visual problem. The raw moving-block esp_lcd probe also showed that unsynchronized raw dynamic drawing is not a product UI path.

Further LVGL work should combine the validated native esp_lcd static display path with the validated GT911 normalized touch path in a minimal widget test before any dashboard, slider, animation or Widget Runtime work.

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

## Correct Arduino library folder

When copying the library into Arduino, the local folder must be:

```text
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\src
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\examples
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\README.md
```

It must not be nested as:

```text
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\ESP32_8048S043\examples
```

Safe update command:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\ESP32-8048S043-lab
git pull

$Src = "$HOME\Documents\GitHub\ESP32-8048S043-lab\libraries\ESP32_8048S043"
$Dst = "$HOME\Documents\Arduino\libraries\ESP32_8048S043"

Remove-Item $Dst -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $Dst -Force | Out-Null
Copy-Item "$Src\*" $Dst -Recurse -Force

Test-Path "$Dst\examples\14_GT911_NormalizedTouch\14_GT911_NormalizedTouch.ino"
Test-Path "$Dst\ESP32_8048S043"
```

Expected result:

```text
True
False
```

## Example plan

```text
01_BoardInfo                 first Arduino IDE smoke test, chip/flash/PSRAM/profile/ALIVE
02_DisplayRGBTest            minimal Arduino_GFX RGB/backlight/color/orientation test
03_TouchGT911Test            GT911 polling visual marker + serial diagnostics
04_BacklightTest             dedicated backlight GPIO2 ON/OFF/blink/PWM test
05_TestConsole               combined RGB + GT911 + backlight diagnostic console
06_WiFiTest                  Wi-Fi scan + association/DHCP/DNS/TCP/reconnect test
07_WebServerTest             Wi-Fi/SoftAP + browser HTTP server + JSON status + ping
08_SDCardTest                read-only SD mount + metadata + directory listing
09_BLETest                   Arduino BLE init + active advertisement scan
10_LVGL_BasicUI              LVGL 8 basic UI: button, counter, slider, GT911 BSP input
11_LVGL_Dashboard            diagnostic HMI/dashboard layer, dynamic UX not accepted
12_DisplayEspLcdRgbPanel_Probe native esp_lcd RGB transport probe, raw dynamic draw boundary
13_LVGL_EspLcdStatic         LVGL 8 static UI over native esp_lcd RGB path
14_GT911_NormalizedTouch     serial-only GT911 BSP 9-zone normalization diagnostic
15_LVGL_EspLcdBasicUI        next: minimal LVGL button over esp_lcd + GT911 BSP input
20_LVGL_GitHubOTA            future
21_LVGL_WidgetLoader         future
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

Evidence:

```text
evidence/specimens/sample-a/arduino/08-sdcard-readonly-20260826.md
https://youtube.com/shorts/vACvK85U0Lw
```

Current Sample A status:

```text
READ-ONLY SD PHYSICAL PASS CANDIDATE / SAMPLE A
```

## 09_BLETest

Purpose:

```text
validate the ESP32-S3 Arduino BLE initialization and advertisement receive path before LVGL/Web/OTA application work
```

Evidence:

```text
evidence/specimens/sample-a/arduino/09-ble-scan-20260826.md
```

Current Sample A status:

```text
BLE SCAN PHYSICAL PASS CANDIDATE / SAMPLE A
```

Boundary:

```text
This validates Arduino-level BLE init and advertising receive only. Pairing, connections, GATT, HID, provisioning and Wi-Fi/BLE coexistence remain separate tests.
```

## 10_LVGL_BasicUI

Purpose:

```text
validate the first LVGL local HMI shell using the already-tested RGB display, GT911 touch path, backlight control and 8 MB PSRAM profile
```

What it tests:

```text
Arduino_GFX RGB display under LVGL;
LVGL 8.x initialization;
PSRAM-backed LVGL draw buffers;
LVGL flush callback to 800x480 RGB panel;
GT911 through ESP32_8048S043_Touch BSP as LVGL pointer input;
button click event and live counter;
slider-controlled backlight PWM;
continued ALIVE output while LVGL runs.
```

Evidence:

```text
evidence/specimens/sample-a/arduino/10-lvgl-basic-ui-bsp-touch-20260826.md
```

Open:

```text
libraries/ESP32_8048S043/examples/10_LVGL_BasicUI/10_LVGL_BasicUI.ino
```

Current Sample A status:

```text
FUNCTIONAL PASS CANDIDATE / SAMPLE A / TOUCH QUALITY OPEN
```

Boundary:

```text
This is the first local LVGL UI layer. It proves functional button/slider interaction, but final touch smoothness remains open. SD-backed assets, Web upload/control, Widget Runtime and GitHub OTA remain separate stages.
```

## 11_LVGL_Dashboard

Purpose:

```text
record the first dashboard-style LVGL screen and test whether static/manual touch handling can avoid visible dynamic redraw artifacts on the RGB panel
```

Evidence:

```text
evidence/specimens/sample-a/arduino/11-lvgl-dashboard-manual-touch-20260826.md
```

Open:

```text
libraries/ESP32_8048S043/examples/11_LVGL_Dashboard/11_LVGL_Dashboard.ino
```

Current Sample A status:

```text
DIAGNOSTIC FUNCTIONAL PASS CANDIDATE / DYNAMIC UX NOT ACCEPTABLE
```

Boundary:

```text
This example proves stable static dashboard rendering and manual control intent, but the touch-time visual dynamics remain unacceptable for normal user-facing applications. It is frozen as a diagnostic record.
```

## 12_DisplayEspLcdRgbPanel_Probe

Purpose:

```text
validate native ESP-IDF esp_lcd RGB panel transport independently from Arduino_GFX, LVGL and GT911
```

Evidence:

```text
evidence/specimens/sample-a/arduino/12-esp-lcd-rgb-panel-probe-20260827.md
```

Current Sample A status:

```text
STATIC TRANSPORT PASS CANDIDATE / RAW DYNAMIC DRAW NOT ACCEPTABLE
```

Boundary:

```text
Static native esp_lcd output works with correct colors, but raw dynamic draw_bitmap motion is not acceptable as a product UI path.
```

## 13_LVGL_EspLcdStatic

Purpose:

```text
validate LVGL 8 static UI over native esp_lcd RGB panel transport, without touch and without animation
```

Evidence:

```text
evidence/specimens/sample-a/arduino/13-lvgl-esp-lcd-static-20260827.md
```

Current Sample A status:

```text
PHYSICAL STATIC PASS CANDIDATE / TOUCH NOT TESTED
```

Boundary:

```text
This proves the LVGL 8 static esp_lcd path after the default-font patch. It does not prove touch integration or interactive widget quality.
```

## 14_GT911_NormalizedTouch

Purpose:

```text
validate the ESP32_8048S043_Touch BSP normalization layer as a serial-only GT911 diagnostic, without display rendering or LVGL widgets
```

Evidence:

```text
evidence/specimens/sample-a/arduino/14-gt911-normalized-touch-20260827.md
```

Current Sample A status:

```text
PHYSICAL PASS CANDIDATE / 9-ZONE NORMALIZATION PASS
```

Boundary:

```text
This proves GT911 detection at 0x5D, raw resolution 480x272, 800x480 coordinate normalization and all 9 physical zones. It does not prove LVGL pointer integration.
```

## Rule

Examples may compile before hardware validation, but README status must not say PHYSICAL PASS until the named specimen evidence exists.
