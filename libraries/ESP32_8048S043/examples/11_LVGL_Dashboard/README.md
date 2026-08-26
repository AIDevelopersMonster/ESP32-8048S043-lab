# 11_LVGL_Dashboard

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

First dashboard-style LVGL 8 example for the ESP32-8048S043 Arduino library.

This example follows `10_LVGL_BasicUI`, which proved the functional LVGL path on Sample A:

```text
RGB display under LVGL;
PSRAM-backed LVGL draw buffers;
GT911 touch through ESP32_8048S043_Touch BSP;
button events;
backlight slider interaction.
```

## Purpose

`11_LVGL_Dashboard` moves from a minimal button/slider test toward a more useful local HMI page.

The screen contains:

```text
firmware ID / uptime line;
memory card with heap and PSRAM bars;
GT911 touch BSP status card;
refresh button;
backlight PWM slider;
continued ALIVE output in Serial.
```

## Firmware ID

Expected current firmware ID:

```text
11DASH-SRC1-240826A
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

Use the same local profile that passed the earlier examples:

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
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\examples\11_LVGL_Dashboard
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\README.md
```

It must not be nested like this:

```text
C:\Users\CHUWI\Documents\Arduino\libraries\ESP32_8048S043\ESP32_8048S043\examples\11_LVGL_Dashboard
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

Test-Path "$Dst\examples\11_LVGL_Dashboard\11_LVGL_Dashboard.ino"
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
File -> Examples -> ESP32_8048S043 -> 11_LVGL_Dashboard
```

Expected serial output:

```text
ESP32-8048S043 Lab / 11_LVGL_Dashboard
LVGL 8 dashboard validation
Firmware ID: 11DASH-SRC1-240826A

[DISPLAY INIT]
[PASS] gfx->begin()

[TOUCH BSP INIT]
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272

[LVGL INIT]
[PASS] lvBuf1 allocated in PSRAM
[PASS] lvBuf2 allocated in PSRAM
[PASS] LVGL display + input drivers registered

[PASS] LVGL dashboard UI objects created
LVGL DASHBOARD READY
```

ALIVE lines should continue:

```text
[ALIVE] fw=11DASH-SRC1-240826A uptime=... display=OK touch=OK lvgl=OK ui=OK refresh=... accepted=... filtered=... loops=... freeHeap=... psram=8388608 freePsram=...
```

## PASS boundary

```text
DASHBOARD PASS CANDIDATE:
  display initializes;
  GT911 BSP initializes;
  LVGL draw buffers allocate;
  dashboard appears on screen;
  heap/PSRAM values update;
  Refresh button works;
  backlight slider works;
  ALIVE continues without reset/brownout/crash.
```

## Boundary

This is still a local HMI example. It does not validate Web setup, Widget Runtime, GitHub OTA, SD-backed assets, long-duration HMI stability or LVGL 9 compatibility.
