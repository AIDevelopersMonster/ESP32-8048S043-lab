# 11_LVGL_Dashboard

Status: `DIAGNOSTIC FUNCTIONAL PASS CANDIDATE / DYNAMIC UX NOT ACCEPTABLE`.

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

`11_LVGL_Dashboard` tested whether a more static, manual-touch dashboard could avoid the visible redraw artifacts observed during LVGL touch interaction on the RGB panel.

The screen contains:

```text
firmware ID / uptime snapshot line;
memory card with heap and PSRAM bars;
GT911/manual touch status card;
manual refresh button;
backlight PWM axis bar;
continued ALIVE output in Serial.
```

## Firmware ID

Expected current firmware ID:

```text
11DASH-MT1-240826C
```

## Result

This example is kept as a laboratory diagnostic, not as a user-application template.

Observed result on Sample A:

```text
idle static screen                 : acceptable;
long ALIVE run                      : stable;
LVGL dashboard rendering            : functional;
manual GT911 hitbox/axis handling   : functional as a test;
touch-time visual dynamics          : still visibly poor;
user-facing application suitability : no.
```

The manual touch projection narrowed the affected area, but did not remove the visible touch-time artifact. This means the current local approach proves that LVGL can render and receive control intent, but it does not provide acceptable dynamic UX.

## Refresh and touch mode

This revision intentionally uses static/manual-touch mode.

The dashboard does not rewrite labels once per second. Runtime telemetry still goes to Serial every five seconds.

GT911 is not registered as a normal LVGL pointer device in this example. The sketch polls GT911 through `ESP32_8048S043_Touch` and interprets touches through fixed hitboxes:

```text
Refresh button       : debounced hitbox, single action per press
Backlight control    : broad horizontal touch band, X-axis projection only
Backlight update     : deadband + rate limit, not literal drawing under the finger
```

This reduced the visible redraw problem but did not make the dynamic behavior good enough for normal user-facing applications.

## Evidence

```text
evidence/specimens/sample-a/arduino/11-lvgl-dashboard-manual-touch-20260826.md
```

## Decision

Freeze this example as a diagnostic/stability record.

Do not continue trying to polish this local partial-redraw/manual-touch workaround as a product UI path. Further LVGL work should move to studying and porting better-organized third-party / WT32-style patterns:

```text
clean BSP display/touch/backlight separation;
small LVGL application sketches;
proven LVGL display flushing strategy;
proper handling of RGB-panel redraw dynamics;
no user-facing UI based on the current manual-touch workaround.
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
Firmware ID: 11DASH-MT1-240826C
Mode   : RGB display + manual GT911 hitboxes + LVGL dashboard
Touch  : LVGL pointer disabled; button/axis handled by sketch

[DISPLAY INIT]
[PASS] gfx->begin()

[TOUCH BSP INIT]
[PASS] ESP32_8048S043_Touch::begin() addr=0x5D fw=0x1060 res=480x272

[LVGL INIT]
[PASS] lvBuf1 allocated in PSRAM
[PASS] lvBuf2 allocated in PSRAM
[PASS] LVGL display driver registered
[PASS] GT911 touch handled manually, LVGL pointer driver disabled

[PASS] LVGL dashboard UI objects created
LVGL DASHBOARD READY
Manual touch mode: no LVGL pressed/drag redraw under finger.
```

ALIVE lines should continue without forcing screen updates:

```text
[ALIVE] fw=11DASH-MT1-240826C uptime=... display=OK touch=OK lvgl=OK ui=OK refresh=... manualTouch=... slider=... accepted=... filtered=... loops=... freeHeap=... psram=8388608 freePsram=...
```

## Acceptance boundary

```text
DIAGNOSTIC PASS CANDIDATE:
  display initializes;
  GT911 BSP initializes;
  LVGL draw buffers allocate;
  dashboard appears on screen;
  no visible periodic 1 Hz full-screen redraw while idle;
  Serial ALIVE continues without reset/brownout/crash;
  manual hitbox/axis touch proves control intent can be read.

NOT ACCEPTED FOR USER APPLICATIONS:
  touch-time redraw dynamics remain visibly poor;
  manual hitbox projection is a workaround, not a proper HMI input architecture;
  this example should not be copied as a final UI template.
```

## Boundary

This is still a local HMI diagnostic. It does not validate Web setup, Widget Runtime, GitHub OTA, SD-backed assets, polished HMI behavior, acceptable dynamic touch UX, long-duration application readiness or LVGL 9 compatibility.
