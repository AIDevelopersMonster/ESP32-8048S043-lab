# Test 17 — LVGL Arduino_GFX Widgets on current stack — 2026-08-29

Status: `PHYSICAL FUNCTIONAL PASS / VISIBLE REDRAW FLICKER / USABLE WITH LIMITATION`.

## Target

```text
Board family      : ESP32-8048S043 / ESP32-S3 / 800x480 RGB / GT911
Project board     : ESP32-8048S043 Lab N16R8 FIXED
Test              : 17_LVGL_ArduinoGFXWidgets_CurrentStack
UI                : standard LVGL 8 Widgets demo
Rendering         : Arduino_GFX partial-area flush
PCLK              : 14 MHz
Porches           : HSYNC 8/4/8, VSYNC 8/4/8
Touch             : ESP32_8048S043_Touch BSP
LVGL              : 8.3.11
Arduino-ESP32     : 3.3.11 current project generation
```

## Why this test exists

A historical `wegi1` Widgets firmware using LVGL 8.3.0-dev + Arduino_GFX 1.2.8 + an Arduino-ESP32 2.x-compatible environment was physically reproduced and looked good.

Test 17 moves the same UI class and timing class onto the current project stack while preserving the Arduino_GFX partial-redraw architecture.

This is not a repair attempt. It is a controlled modernization/comparison experiment.

## Build note

The official LVGL 8.3.11 Arduino package stores its demos outside the compiled `src/` tree. To make the official standard Widgets demo visible to Arduino Builder, the reproduction used a temporary packaging shim that copies only the official LVGL 8.3.11 Widgets demo source/assets from:

```text
lvgl/demos/
```

to:

```text
lvgl/src/demos/
```

No `wegi1` application source was copied into the project.

The active LVGL configuration required:

```c
#define LV_COLOR_DEPTH 16
#define LV_USE_DEMO_WIDGETS 1
```

## Physical observation

Operator report:

```text
Looks like the factory demo.
The old defects remain.
The pictures and interfaces work and are analogous to the original/reference demo.
The firmware is usable.
With many active elements, flicker is noticeable.
Redraw is perceived as going through a black background/frame.
The black interval is only milliseconds long, but it is very visible to the eye.
```

## Physical assessment

```text
BOOT                  PASS
DISPLAY               PASS
LVGL WIDGETS          PASS
IMAGES                 PASS
INTERFACES             PASS
TOUCH / INTERACTION    PASS by operator observation
FACTORY-LIKE LOOK      YES by operator observation
STATIC CONTENT         GOOD
ACTIVE REDRAW          VISIBLE FLICKER
BLACK TRANSITION       VISUALLY PERCEIVED
USABILITY              YES
PRODUCTION VISUAL      NO — active redraw limitation remains
OVERALL                FUNCTIONAL PASS WITH VISUAL LIMITATION
```

## Interpretation

The experiment establishes that the current project stack can reproduce the standard LVGL 8 Widgets application class on the custom board profile with current Arduino_GFX and the project GT911 BSP.

Therefore the following parts are functionally viable together:

```text
current board profile
current Arduino-ESP32 / ESP-IDF 5.x generation
current Arduino_GFX
LVGL 8.3.11
ESP32_8048S043_Touch BSP
standard LVGL Widgets UI
```

However, moving to current Arduino_GFX partial-area redraw did not eliminate the visible defect class. Dynamic, redraw-heavy screens still exhibit a short but clearly perceptible flicker/black-transition symptom.

This means the previous working hypothesis must be narrowed carefully:

```text
"native esp_lcd partial redraw alone causes the defect"
```

is not supported by Test 17.

The defect follows at least one current partial-redraw path outside the native `esp_lcd` sketch implementation.

## What this does not prove

The observation of a black transition does not prove that firmware deliberately draws a full black frame.

It also does not yet isolate whether the symptom originates in:

```text
Arduino_GFX current RGB implementation;
current Arduino-ESP32 / ESP-IDF RGB behavior;
partial redraw presentation;
buffer placement/allocation;
scanout synchronization;
LVGL invalidation pattern;
or an interaction among these.
```

## Comparison snapshot

```text
Path                         Rendering                    Physical result
--------------------------------------------------------------------------------------
wegi1 historical Widgets     Arduino_GFX partial          PASS / visually good reference
15 local                     native esp_lcd partial       functional / visible jitter
16 local                     native esp_lcd minimal       mostly stable / intermittent jitter
15B local                    Arduino_GFX full-frame       separate comparison path
17 current Widgets           Arduino_GFX partial          PASS / visible redraw flicker
```

## Reproduction instructions

See:

```text
libraries/ESP32_8048S043/examples/17_LVGL_ArduinoGFXWidgets_CurrentStack/README.md
```

That README records the required current project environment, LVGL configuration, temporary official-demo packaging shim, Arduino IDE build/upload flow, serial markers, physical evaluation protocol and cleanup procedure.
