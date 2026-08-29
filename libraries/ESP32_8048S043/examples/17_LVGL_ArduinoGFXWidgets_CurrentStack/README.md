# 17_LVGL_ArduinoGFXWidgets_CurrentStack

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

## Purpose

This is a controlled forward-port of the physically successful historical `wegi1` LVGL Widgets reference onto the current ESP32-8048S043-lab stack.

It does not copy the third-party application source. It uses the standard LVGL 8 widgets demo already distributed with LVGL and independently wires it to the current project drivers.

## What stays intentionally close to the known-good reference

```text
resolution          800 x 480
PCLK                14 MHz
HSYNC               8 / 4 / 8
VSYNC               8 / 4 / 8
hsync polarity      0
vsync polarity      0
pclk active neg     1
rendering            Arduino_GFX partial-area flush
LVGL draw buffer     1/4 screen = 96000 RGB565 pixels
loop cadence         5 ms
UI                    standard LVGL Widgets demo
```

## What is deliberately modernized

```text
Board profile        ESP32-8048S043 Lab N16R8 FIXED
Arduino-ESP32        current project generation
ESP-IDF              current 5.x generation
Arduino_GFX          current installed version
LVGL                 current 8.x project version
Touch                ESP32_8048S043_Touch BSP
```

The experiment changes the software generation while keeping the visual workload and RGB timing class close to the historical physical PASS.

## Required LVGL configuration

The current LVGL configuration must include:

```c
#define LV_COLOR_DEPTH 16
#define LV_USE_DEMO_WIDGETS 1
```

The sketch intentionally stops at compile time if these are not enabled.

## Before compiling

Restore the normal project libraries after the historical `wegi1` reproduction. Do not leave the bundled third-party `LVGL 8.3.0-dev` or `Arduino_GFX 1.2.8` active for this test.

Use the project's normal local board profile:

```text
ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
```

Expected current FQBN:

```text
AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
```

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

## Memory strategy

To keep one more variable close to the historical reference, test 17 first requests the approximately 192 KB LVGL buffer from internal RAM:

```text
96000 pixels x 2 bytes = ~192000 bytes
```

If the current stack cannot provide a contiguous internal block of that size, the sketch falls back to PSRAM and prints a warning. That fallback must be recorded in the evidence because it changes one experimental variable.

## Expected serial markers

```text
ESP32-8048S043 Lab / Test 17
[PASS] gfx->begin()
[PASS] GT911 BSP ...
[PASS] LVGL buffer in internal RAM: ...
```

or, if internal allocation is unavailable:

```text
[WARN] Internal allocation unavailable; LVGL buffer in PSRAM: ...
```

Then:

```text
[PASS] LVGL partial display driver registered
[PASS] LVGL pointer registered through BSP GT911
[UI INIT] lv_demo_widgets()
[PASS] Backlight ON after initial LVGL render
[READY] Judge this visually against the historical wegi1 Widgets PASS.
```

Runtime evidence:

```text
[TOUCH PRESS]
[TOUCH RELEASE]
[ALIVE] ... flush=... indev=... press=... release=...
```

## Physical judgment

Do not tune anything during the first run.

Compare against the historical `wegi1` physical PASS and record only what is actually observed:

```text
boot/display success;
widgets rendering quality;
animations and scrolling;
normal taps;
fast taps;
intermittent jitter present/absent;
idle stability;
whether the behavior is better/same/worse than the historical reference.
```

The operator's visual judgment is intentionally part of the evidence. It should not be replaced by an artificial PASS solely because Serial reports no errors.

## Interpretation matrix

If test 17 is visually clean:

```text
historical Arduino_GFX partial path  PASS
current Arduino_GFX partial path     PASS
native esp_lcd partial path          jitter observed
```

That would strongly narrow the unresolved difference toward our native `esp_lcd` transport/synchronization path rather than LVGL 8, GT911, or partial invalidation itself.

If test 17 reproduces the intermittent jitter:

```text
historical stack                     PASS
current stack                         jitter
```

then the next diagnostic boundary becomes the software-generation transition: current Arduino-ESP32/ESP-IDF, current Arduino_GFX behavior, current memory placement, or an interaction among them.

No conclusion should be promoted before physical observation.

## Related evidence

```text
docs/third-party/wegi1-esp32-8048S043-4INCH-LCD.md
15_LVGL_EspLcdBasicUI
16_LVGL_EspLcdMinimalInvalidation
15B_LVGL_ArduinoGFXFullFrameUI
```
