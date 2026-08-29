# 17_LVGL_ArduinoGFXWidgets_CurrentStack

Status: `PHYSICAL FUNCTIONAL PASS / VISIBLE REDRAW FLICKER / USABLE WITH LIMITATION`.

## Purpose

This is a controlled forward-port of the physically successful historical `wegi1` LVGL Widgets reference onto the current ESP32-8048S043-lab software stack.

It does not copy the third-party application source. It uses the standard LVGL 8 widgets demo distributed with LVGL and independently wires it to the current project drivers.

The experiment is diagnostic: keep the UI workload and RGB timing class close to the historical reference while moving to the current board profile, current Arduino-ESP32/ESP-IDF generation, current Arduino_GFX and the project GT911 BSP.

## Physical result — 2026-08-29

Sample A successfully booted and displayed the standard LVGL Widgets interface.

Operator assessment:

```text
looks like the familiar factory demo;
pictures and interfaces work;
UI behavior is analogous to the original/reference demo;
touch and interaction are usable;
the old visible defect class remains;
with many active elements, redraw flicker is clearly visible;
the eye perceives very short transitions through a black background/frame;
even millisecond-scale black transitions are noticeable;
usable, but not visually clean enough to call production-quality animation.
```

Important interpretation boundary:

The operator visually perceives a black transition during redraw. This does **not** by itself prove that the software intentionally draws a full black frame or that the exact cause is already identified. It is recorded as a physical visual symptom.

Current status:

```text
BOOT                  PASS
DISPLAY               PASS
LVGL WIDGETS          PASS
IMAGES                 PASS
INTERACTION            PASS by operator observation
FACTORY-LIKE UI        YES by operator observation
IDLE/STATIC CONTENT    GOOD
ACTIVE REDRAW          VISIBLE FLICKER
BLACK TRANSITION       VISUALLY PERCEIVED
USABILITY              YES
VISUAL QUALITY         LIMITED UNDER ACTIVE UI
OVERALL                FUNCTIONAL PASS WITH VISUAL LIMITATION
```

Serial output for this exact physical run was not supplied, so the result is intentionally classified from physical observation rather than a synthetic serial-only PASS.

## What stays intentionally close to the known-good historical reference

```text
resolution            800 x 480
PCLK                  14 MHz
HSYNC                 8 / 4 / 8
VSYNC                 8 / 4 / 8
hsync polarity        0
vsync polarity        0
pclk active neg       1
rendering             Arduino_GFX partial-area flush
LVGL draw buffer      1/4 screen = 96000 RGB565 pixels
loop cadence          5 ms
UI                    standard LVGL Widgets demo
```

## What is deliberately modernized

```text
Board profile          ESP32-8048S043 Lab N16R8 FIXED
Arduino-ESP32          3.3.11 in the reproduced current-stack run
ESP-IDF                current 5.x generation shipped by that core
Arduino_GFX            current project-installed version
LVGL                   8.3.11 in the reproduced current-stack run
Touch                  ESP32_8048S043_Touch BSP
```

The exact Arduino_GFX version should be recorded from the local `library.properties` when reproducing the experiment, because it is not vendored or pinned by this example.

## Architecture

```text
LVGL 8 standard Widgets demo
        |
        v
single 1/4-screen LVGL draw buffer
        |
        v
LVGL partial invalidation areas
        |
        v
Arduino_GFX draw16bitRGBBitmap(area)
        |
        v
ESP32-S3 RGB panel 800x480

GT911
  |
  v
ESP32_8048S043_Touch BSP
  |
  v
LVGL pointer input
```

## Prerequisites

Use the normal project environment, not the historical `wegi1` stack.

Required components:

```text
Arduino IDE 2.x
esp32 by Espressif Systems 3.3.11
local AIDevelopersMonster ESP32 platform/profile
LVGL 8.3.11
current Arduino_GFX Library for Arduino
ESP32_8048S043 project BSP library
```

Expected custom FQBN:

```text
AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
```

Expected board name:

```text
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

Recommended board settings used by the project:

```text
USB CDC On Boot   Disabled
CPU Frequency     240 MHz
Debug Level       None
Erase All Flash   Disabled
Events Run On     Core 1
Flash Mode        QIO 80MHz
Flash Size        16MB
Arduino Runs On   Core 1
PSRAM             OPI PSRAM
Upload Mode       UART0 / Hardware CDC
Upload Speed      921600
USB Mode          Hardware CDC and JTAG
Partition Scheme  16M Flash (3MB APP/9.9MB FATFS)
```

## 1. Get the current project

PowerShell:

```powershell
cd C:\Users\CHUWI\Documents\GitHub\ESP32-8048S043-lab
git pull
```

The example is located at:

```text
libraries/ESP32_8048S043/examples/
17_LVGL_ArduinoGFXWidgets_CurrentStack/
17_LVGL_ArduinoGFXWidgets_CurrentStack.ino
```

## 2. Restore the normal project libraries

If the historical `wegi1` reproduction was run first, remove its temporary bundled libraries and restore the normal Arduino libraries before building Test 17.

Typical project BSP sync:

```powershell
$Src = "$HOME\Documents\GitHub\ESP32-8048S043-lab\libraries\ESP32_8048S043"
$Dst = "$HOME\Documents\Arduino\libraries\ESP32_8048S043"

Remove-Item $Dst -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $Dst -Force | Out-Null
Copy-Item "$Src\*" $Dst -Recurse -Force
```

Do not use the third-party bundled `LVGL 8.3.0-dev` or `Arduino_GFX 1.2.8` for Test 17.

## 3. Confirm the current library versions

PowerShell:

```powershell
$ArduinoLib = "$HOME\Documents\Arduino\libraries"

Get-ChildItem $ArduinoLib -Directory | ForEach-Object {
    $Prop = Join-Path $_.FullName "library.properties"
    if (Test-Path $Prop) {
        $Name = (Select-String $Prop '^name=' | Select-Object -First 1).Line
        $Version = (Select-String $Prop '^version=' | Select-Object -First 1).Line
        if ($Name -match 'lvgl|GFX Library for Arduino') {
            [PSCustomObject]@{
                Folder  = $_.Name
                Name    = $Name
                Version = $Version
            }
        }
    }
}
```

For the reproduced run, LVGL was:

```text
name=lvgl
version=8.3.11
```

Record the Arduino_GFX version shown on the machine used for the reproduction.

## 4. Enable the LVGL Widgets demo

The active LVGL configuration must include:

```c
#define LV_COLOR_DEPTH 16
#define LV_USE_DEMO_WIDGETS 1
```

Typical active config path:

```text
C:\Users\CHUWI\Documents\Arduino\libraries\lv_conf.h
```

Check it:

```powershell
$Conf = "$HOME\Documents\Arduino\libraries\lv_conf.h"
Select-String $Conf -Pattern 'LV_COLOR_DEPTH|LV_USE_DEMO_WIDGETS'
```

If `LV_USE_DEMO_WIDGETS` is `0`, change only that setting to `1`.

## 5. Make the official LVGL 8.3.11 demo visible to Arduino Builder

The Arduino Library Manager packaging of LVGL 8.3.11 keeps the standard demos in the library-root `demos/` directory, while Arduino Builder compiles the library `src/` tree. Test 17 therefore uses a temporary packaging shim.

First locate the installed LVGL library:

```powershell
$ArduinoLib = "$HOME\Documents\Arduino\libraries"

$Lvgl = Get-ChildItem $ArduinoLib -Directory | Where-Object {
    $p = Join-Path $_.FullName "library.properties"
    (Test-Path $p) -and (Select-String -Path $p -Pattern '^name=lvgl$' -Quiet)
} | Select-Object -First 1

$Lvgl.FullName
Get-Content (Join-Path $Lvgl.FullName "library.properties") |
    Select-String '^name=|^version='
```

Check the demo layout:

```powershell
Test-Path (Join-Path $Lvgl.FullName "demos\lv_demos.h")
Test-Path (Join-Path $Lvgl.FullName "src\demos\lv_demos.h")
```

The normal LVGL 8.3.11 Library Manager layout commonly gives:

```text
True
False
```

Create the temporary shim using the official demo files from that same LVGL installation:

```powershell
$DemoSrc = Join-Path $Lvgl.FullName "demos"
$DemoDst = Join-Path $Lvgl.FullName "src\demos"

New-Item -ItemType Directory `
    -Path "$DemoDst\widgets\assets" `
    -Force | Out-Null

Copy-Item "$DemoSrc\lv_demos.h" "$DemoDst\lv_demos.h" -Force
Copy-Item "$DemoSrc\widgets\lv_demo_widgets.c" "$DemoDst\widgets\lv_demo_widgets.c" -Force
Copy-Item "$DemoSrc\widgets\lv_demo_widgets.h" "$DemoDst\widgets\lv_demo_widgets.h" -Force
Copy-Item "$DemoSrc\widgets\assets\*.c" "$DemoDst\widgets\assets\" -Force
```

Verify:

```powershell
Test-Path "$DemoDst\lv_demos.h"
Test-Path "$DemoDst\widgets\lv_demo_widgets.c"
Test-Path "$DemoDst\widgets\lv_demo_widgets.h"
```

Expected:

```text
True
True
True
```

This does not import any `wegi1` application source. The copied files are the official standard Widgets demo from the currently installed LVGL 8.3.11 package.

## 6. Open the example

Restart Arduino IDE after changing libraries/configuration.

Open:

```text
File
-> Examples
-> ESP32_8048S043
-> 17_LVGL_ArduinoGFXWidgets_CurrentStack
```

Select:

```text
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

## 7. Compile

Press:

```text
Verify
```

The sketch intentionally fails compilation if either requirement is missing:

```text
LV_COLOR_DEPTH == 16
LV_USE_DEMO_WIDGETS == 1
```

Do not patch the demo to bypass these guards.

## 8. Flash

Connect the board by USB/UART and select the correct COM port.

Press:

```text
Upload
```

Project upload speed:

```text
921600
```

After flashing, reset/power-cycle if necessary.

## 9. Serial monitor

Use:

```text
115200 baud
```

Expected high-level markers from Test 17 include:

```text
ESP32-8048S043 Lab / Test 17
[PASS] gfx->begin()
[PASS] GT911 BSP ...
[PASS] LVGL buffer ...
[PASS] LVGL partial display driver registered
[PASS] LVGL pointer registered through BSP GT911
[UI INIT] lv_demo_widgets()
[PASS] Backlight ON after initial LVGL render
[READY] Judge this visually against the historical wegi1 Widgets PASS.
```

Runtime touch/alive counters are diagnostic only; a clean serial log does not override visible display defects.

## 10. Physical evaluation protocol

Do not tune timings, buffers or touch filtering during the first comparison run.

Check:

```text
boot and first render;
all Widgets tabs/screens;
images and icons;
buttons and touch interaction;
animations;
charts/meters;
scrolling;
fast and normal taps;
idle/static stability;
active-element-heavy screens;
screen/tab transitions;
any perceived black flash between redraw states.
```

For this reproduced run the important result was:

```text
static pictures/interfaces look correct and factory-like;
functional behavior matches the reference class;
redraw-heavy scenes visibly flicker;
short black transitions are visually obvious despite being very brief.
```

## 11. Optional cleanup of the LVGL packaging shim

After the experiment, the temporary `src/demos` copy can be removed:

```powershell
Remove-Item "$($Lvgl.FullName)\src\demos" -Recurse -Force
```

Restart Arduino IDE afterward.

Do not remove the original root-level:

```text
lvgl/demos/
```

## Memory strategy

To keep one variable close to the historical reference, Test 17 first requests the approximately 192 KB LVGL buffer from internal RAM:

```text
96000 pixels x 2 bytes = ~192000 bytes
```

If the current stack cannot provide a contiguous internal block of that size, the sketch falls back to PSRAM and prints a warning. That fallback changes one experimental variable and should be recorded in the evidence.

## Interpretation

The physical result is stronger than a simple compile/boot PASS.

The current software generation can reproduce the same standard LVGL Widgets application class on the project custom board profile:

```text
current board profile
+ current Arduino-ESP32 / ESP-IDF 5.x
+ current Arduino_GFX
+ LVGL 8.3.11
+ project GT911 BSP
+ Arduino_GFX partial-area redraw
= functional factory-like UI
```

But the visible redraw limitation remains under dynamic load.

This means Test 17 does **not** support the simple conclusion that moving from native `esp_lcd` to Arduino_GFX partial redraw automatically eliminates the defect.

At the same time, the successful images, widgets and touch path show that the modern stack, board profile, LVGL 8.3.11 and BSP are functionally viable.

The remaining visual issue should be treated as a redraw/display-presentation problem rather than evidence that the UI assets or input system are fundamentally broken.

## Current comparison

```text
Path                         Rendering                    Physical result
--------------------------------------------------------------------------------------
wegi1 historical Widgets     Arduino_GFX partial          PASS / visually good reference
15 local                     native esp_lcd partial       functional / visible jitter
16 local                     native esp_lcd minimal       mostly stable / intermittent jitter
15B local                    Arduino_GFX full-frame       separate comparison path
17 current Widgets           Arduino_GFX partial          PASS / visible redraw flicker
```

The exact mechanism behind the perceived black redraw transition is still open.

## Related evidence

```text
docs/third-party/wegi1-esp32-8048S043-4INCH-LCD.md
evidence/specimens/sample-a/arduino/17-lvgl-arduinogfx-widgets-currentstack-20260829.md
15_LVGL_EspLcdBasicUI
16_LVGL_EspLcdMinimalInvalidation
15B_LVGL_ArduinoGFXFullFrameUI
```
