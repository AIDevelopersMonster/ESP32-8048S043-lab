# Test 32 — DevAnyKR / LVGL9 + Arduino_GFX + EEZ Studio

## Status

**BUILD PASS / PHYSICAL PASS / CLOSED / KNOWN-GOOD THIRD-PARTY REFERENCE**

Upstream repository:

```text
DevAnyKR/ESP32_8048S043C
```

Pinned upstream commit:

```text
bb056490f0738911618f60f98e164f36dde0f84d
2025-01-23
LVGL simple timer
```

License: MIT (declared in upstream README).

Physical verdict reported by the user on 2026-09-04:

> Работает отлично! Очень хорошо двигаются ползунки.

This is a strong physical PASS. The display/UI stack is stable on the tested ESP32-8048S043 board and the interactive slider controls are reported as especially smooth and easy to operate.

No visible flicker, horizontal jump, touch-redraw instability, reset, or crash was reported in this physical run.

## Why this candidate matters

This project returns to an Arduino_GFX + LVGL PARTIAL architecture, but differs from our isolated Arduino_GFX experiments and from Test 21 in several important ways:

```text
Arduino framework / pioarduino
LVGL 9.2.2 source-consistent build
Arduino_GFX RGB
EEZ Studio generated UI
PARTIAL LVGL rendering
single dynamically allocated LVGL draw buffer
PCLK 15 MHz
pclk_active_neg = 0 in the actual EEZ demo constructor
auto_flush = true
GT911
no explicit application-level RGB bounce setting
```

This gives an important independent reference after:

```text
Test 22  INTERNAL LVGL + Arduino_GFX + bounce0  -> PASS
Test 23  PSRAM LVGL    + Arduino_GFX + bounce0  -> FAIL
Test 24+ PSRAM LVGL    + Arduino_GFX + bounce>0 -> PASS
Test 31  INTERNAL LVGL + native esp_lcd + PSRAM FB + bounce0 -> PASS
Test 32  Arduino_GFX + PARTIAL + EEZ + MALLOC_CAP_8BIT buffer -> PASS
```

Test 32 is a whole-project architecture test, not a one-variable causal experiment. Its PASS shows that Arduino_GFX + LVGL PARTIAL can be physically excellent in another real application architecture, but it does not by itself identify a single causal variable for Test 23.

## Upstream build manifest

The upstream `platformio.ini` declares:

```text
platform = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip
framework = arduino
board = esp32-8048S043C
src_dir = EEZ_DEMO

lvgl/lvgl@^9.1.0
paulstoffregen/Time@^1.6.1
https://github.com/eez-open/eez-framework.git
https://github.com/TAMCTec/gt911-arduino.git
moononournation/GFX Library for Arduino@^1.5.2
```

These declarations are not fully reproducible because `stable`, caret ranges and Git dependencies move over time.

The checked-in upstream `include/lv_conf.h`, however, explicitly identifies itself as:

```text
Configuration file for v9.2.2
```

Therefore the successful Test 32 reconstruction pins LVGL 9.2.2 rather than 9.1.0. This is source-consistent with the author's own configuration and v9.2.2 was already available before the 2025-01-23 upstream commit.

## Successful build reconstruction

The successful Test 32 environment is:

```text
pioarduino platform-espressif32 53.03.11
Arduino-ESP32                  3.1.1 line
ESP-IDF                        5.3.2.241224 line
LVGL                           9.2.2 exact
Arduino_GFX                    1.5.2 exact
Time                           1.6.1 exact
EEZ framework                  5bb6c8692d440e599469d5c52b6c3f2094dbf910
GT911 Arduino                  b3f175e65a799368be9c544e255204e1e74ad2ed
```

The first 9.1.0 reconstruction attempt failed because it was inconsistent with the checked-in LVGL 9.2.2 configuration. The corrected 9.2.2 reconstruction is the retained Test 32 build baseline.

A second build obstacle was Windows path depth in the temporary PlatformIO/LVGL dependency tree. The harness therefore uses a deliberately short work root:

```text
%USERPROFILE%\t32-devany\upstream
```

This changes only the location of the disposable build worktree; it does not modify the application/display/touch/UI source.

The harness restores the original upstream `platformio.ini`, removes disposable `.pio` build output, and verifies that the pinned upstream source tree is clean after each run.

## Actual display path used by EEZ_DEMO/main.cpp

The tested application constructs `Arduino_ESP32RGBPanel` directly:

```text
DE     40
VSYNC  41
HSYNC  39
PCLK   42

R      45,48,47,21,14
G      5,6,7,15,16,4
B      8,3,46,9,1

HSYNC  polarity 0 / front 8 / pulse 4 / back 8
VSYNC  polarity 0 / front 8 / pulse 4 / back 8
PCLK   active_neg 0
PCLK   15 MHz
```

The display wrapper is:

```cpp
Arduino_RGB_Display(..., rotation = 2, auto_flush = true)
```

Note: the repository also contains a custom PlatformIO board JSON with generic ST7262 macros, including different timing flags. For this test, the explicit constructor in `EEZ_DEMO/main.cpp` is the decisive configuration for the Arduino_GFX display object actually used by the demo.

## LVGL path

The upstream code computes:

```cpp
screenWidth  = gfx->width();
screenHeight = gfx->height();
bufSize      = (screenWidth * screenHeight) / 10;
disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize, MALLOC_CAP_8BIT);
```

For 800x480:

```text
bufSize = 38,400 bytes
```

The allocation requests `MALLOC_CAP_8BIT` only; it does not explicitly request INTERNAL or SPIRAM. Actual heap placement is therefore part of the runtime/environment and must not be silently classified as either INTERNAL or PSRAM without measurement.

LVGL uses:

```text
LV_DISPLAY_RENDER_MODE_PARTIAL
one draw buffer
```

Flush path:

```cpp
gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);
lv_disp_flush_ready(disp);
```

The UI is generated with EEZ Studio and initialized with:

```cpp
ui_init();
```

The repository also contains the EEZ Studio project source, making this candidate useful not only for transport testing but also for studying an editable generated-UI workflow.

## Touch and interactive UI verdict

The project uses TAMC_GT911 with:

```text
SCL       20
SDA       19
INT       18
RST       38
raw X     0..480
raw Y     0..272
```

Application code maps the raw GT911 coordinates to the 800x480 display dimensions.

The touch initialization includes an address fallback between GT911 addresses when the INT/address state is ambiguous.

Physical result:

```text
Touch response                PASS
Interactive sliders           PASS / very good
Slider capture/movement       very good
UI responsiveness             excellent by user report
Visible touch-redraw flicker  not reported
```

This is a useful contrast with the minor slider-capture UX limitation noted in Test 19/22. In Test 32 the user specifically reports that the sliders move very well.

## Physical verdict

```text
Date                         2026-09-04
Build                        PASS
Boot                         PASS
Display/UI                   PASS
EEZ interface                PASS
Touch                        PASS
Interactive sliders          PASS / very good
Visible display instability  not reported
Touch-redraw flicker         not reported
Horizontal jump              not reported
Reset/crash                  not reported
Physical verdict             PASS
Test state                   CLOSED / KNOWN-GOOD REFERENCE
```

## Engineering interpretation

Test 32 is another strong counterexample to any overly broad statement such as:

```text
Arduino_GFX + LVGL PARTIAL + no explicit bounce = unstable
```

That statement is false for our tested board matrix.

The stable Test 32 topology is approximately:

```text
LVGL 9.2.2
  -> EEZ-generated UI
  -> one PARTIAL MALLOC_CAP_8BIT draw buffer
  -> draw16bitRGBBitmap()
  -> Arduino_GFX RGB
  -> PCLK 15 MHz / pclk_active_neg 0
  -> GT911
  -> PHYSICAL PASS
```

The exact heap placement of the 38,400-byte draw buffer has not yet been instrumented, so it must remain an open detail rather than being inferred.

Combined with Tests 22-31, the defensible conclusion remains that the old Test 23 flicker is tied to a narrower combination of memory placement and transport/DMA topology rather than to Arduino_GFX, LVGL PARTIAL, PSRAM, or bounce0 as isolated universal causes.

## Production/UI opportunity

Test 32 is especially interesting for custom projects because the lower display/touch stack is physically good **and** the UI is authored with EEZ Studio rather than being only a fixed LVGL demo.

This gives us a second promising production direction alongside Test 31:

```text
Test 31 direction
native ESP-IDF transport
  -> replace lv_demo_widgets() with our own runtime/UI layer

Test 32 direction
Arduino_GFX transport
  -> EEZ Studio editable/generated UI
  -> strong interactive controls out of the box
```

Test 32 should now remain frozen as the known-good upstream EEZ reference. Any experiments with our own screens, widget packages, alternate memory placement or runtime loading should be derived into a new test/branch rather than modifying this baseline.

## Queued next architecture

The next particularly interesting candidate remains:

```text
xoquox/esphome-lvgl
```

That repository contains an explicit `devices/ESP32-8048S043.yaml` hardware profile and a modular 800x480 UI broken into separate YAML `widgets`, `styles`, `themes`, fonts and pages with `!include` composition. It is not runtime widget loading, but it is highly relevant to our future modular UI/widget architecture and should be studied as the next separate branch.
