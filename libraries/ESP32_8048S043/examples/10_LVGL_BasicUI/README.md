# 10_LVGL_BasicUI

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

This example is the first LVGL-based local HMI shell for the ESP32-8048S043 Arduino library.

It combines the previously validated low-level layers:

```text
RGB display;
GT911 touch;
backlight PWM;
PSRAM-backed LVGL draw buffers;
interactive LVGL widgets.
```

## Why this test exists

The verified stack now has:

```text
01 BoardInfo / profile diagnostics
02 RGB display
03 GT911 touch
04 Backlight
05 Combined test console
06 Wi-Fi infrastructure
07 HTTP WebServer
08 SDCard read-only
09 BLE scan
```

`10_LVGL_BasicUI` moves from peripheral validation to the first small UI application layer.

## What it checks

```text
Arduino_GFX RGB display under LVGL;
LVGL 8.x initialization;
LVGL draw buffers allocated in PSRAM where possible;
LVGL flush callback to the 800x480 RGB panel;
GT911 touch registered as an LVGL pointer input device;
interactive button click event;
live click counter;
slider-controlled backlight PWM;
continued ALIVE output while LVGL is running.
```

## What it does not check

```text
SD-backed assets;
Web upload/control;
Widget Runtime;
GitHub OTA;
long-duration HMI stress;
final UI framework architecture;
LVGL 9 compatibility.
```

## Dependencies

Install these Arduino libraries:

```text
Arduino_GFX_Library
lvgl
```

This example is written for LVGL 8.x and expects:

```text
LV_COLOR_DEPTH == 16
```

## Arduino IDE setup

Use the same local profile that passed `01_BoardInfo`:

```text
Board             : ESP32-8048S043 Lab N16R8 FIXED
Flash Size        : 16MB (128Mb)
Flash Mode        : QIO 80MHz
Partition Scheme  : 16M Flash (3MB APP/9.9MB FATFS)
PSRAM             : OPI PSRAM
Serial Monitor    : 115200 baud
```

## Correct Arduino library folder

The installed Arduino library must look like this:

```text
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\src
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\examples\10_LVGL_BasicUI
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\README.md
```

It must not be nested like this:

```text
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\ESP32_8048S043\examples\10_LVGL_BasicUI
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

Test-Path "$Dst\examples\10_LVGL_BasicUI\10_LVGL_BasicUI.ino"
Test-Path "$Dst\ESP32_8048S043"
```

Expected result:

```text
True
False
```

## Running the test

Open:

```text
File -> Examples -> ESP32_8048S043 -> 10_LVGL_BasicUI
```

Expected screen:

```text
ESP32-8048S043 / LVGL BasicUI
RGB + GT911 + LVGL status line
Tap me button
Button clicks counter
Backlight PWM slider
Footer note
```

Expected interaction:

```text
touching Tap me increments the counter;
moving the slider changes the backlight brightness;
serial output reports button clicks and ALIVE state.
```

## Expected serial output

A good run should include:

```text
ESP32-8048S043 Lab / 10_LVGL_BasicUI
LVGL 8 basic UI validation

[DISPLAY INIT]
[PASS] gfx->begin()

[TOUCH INIT]
[PASS] GT911 at 0x5D

[LVGL INIT]
[PASS] lvBuf1 allocated in PSRAM
[PASS] lvBuf2 allocated in PSRAM
[PASS] LVGL touch input registered
[PASS] LVGL display driver registered

[UI INIT]
[PASS] LVGL UI objects created

LVGL BASIC UI READY
```

ALIVE lines should continue:

```text
[ALIVE] uptime=... display=OK touch=OK lvgl=OK ui=OK clicks=... touchReports=... lvglLoops=... freeHeap=... psram=... freePsram=...
```

## PASS boundary

```text
LVGL BASIC UI PASS CANDIDATE:
  display initializes;
  GT911 initializes;
  LVGL draw buffers allocate;
  LVGL display driver registers;
  LVGL touch input registers;
  UI appears on screen;
  button press changes counter;
  slider changes backlight;
  ALIVE continues without reset/brownout/crash.
```

## Boundary

This is the first local LVGL HMI shell. It does not yet validate the future Web setup, SD-backed assets, Widget Runtime or GitHub OTA layers.
