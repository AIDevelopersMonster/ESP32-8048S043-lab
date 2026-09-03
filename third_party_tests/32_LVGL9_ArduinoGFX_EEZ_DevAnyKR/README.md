# Test 32 — DevAnyKR / LVGL9 + Arduino_GFX + EEZ Studio

## Status

**THIRD-PARTY CANDIDATE / BUILD + PHYSICAL VERDICT PENDING**

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

The purpose of Test 32 is to run the original third-party EEZ demo on our ESP32-8048S043 and compare its physical behavior with the established Test 19-31 matrix.

## Why this candidate was selected

This project is especially useful because it returns to an Arduino_GFX + LVGL PARTIAL architecture, but differs from our isolated Arduino_GFX experiments and from Test 21 in several important ways:

```text
Arduino framework / pioarduino
LVGL 9.x
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

This gives a valuable independent check after:

```text
Test 22  INTERNAL LVGL + Arduino_GFX + bounce0  -> PASS
Test 23  PSRAM LVGL    + Arduino_GFX + bounce0  -> FAIL
Test 24+ PSRAM LVGL    + Arduino_GFX + bounce>0 -> PASS
Test 31  INTERNAL LVGL + native esp_lcd + PSRAM FB + bounce0 -> PASS
```

Test 32 is a whole-project architecture test, not a one-variable causal experiment. We must not infer a single-variable cause from its result.

## Upstream build manifest

The upstream `platformio.ini` uses:

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

For Test 32 the application/display/touch/UI source remains exact upstream. The runner may temporarily apply a documented historical dependency overlay only, then restore `platformio.ini` and verify that the upstream tracked tree is clean.

## Historical environment reconstruction

The upstream source commit is dated 2025-01-23.

The selected reconstruction is:

```text
pioarduino platform-espressif32 53.03.11
Arduino-ESP32                  3.1.1 line
ESP-IDF                        5.3.2 line
LVGL                           9.1.0 exact
Arduino_GFX                    1.5.2 exact
Time                           1.6.1 exact
EEZ framework                  5bb6c8692d440e599469d5c52b6c3f2094dbf910
GT911 Arduino                  b3f175e65a799368be9c544e255204e1e74ad2ed
```

`53.03.11` was released before the 2025-01-23 upstream commit and corresponds to Arduino-ESP32 3.1.1 / ESP-IDF 5.3.2. The EEZ commit above is the latest repository commit before the upstream source date; the GT911 repository had not changed since its 2021 commit. This is a historical reconstruction, not a claim of byte-for-byte knowledge of the author's local machine.

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

The allocation requests `MALLOC_CAP_8BIT` only; it does not explicitly request INTERNAL or SPIRAM. Actual heap selection is therefore part of the environment/runtime architecture and should be treated carefully when comparing against our controlled tests.

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

## Touch

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

## What Test 32 can tell us

If Test 32 is physically clean, it will provide another stable Arduino_GFX PARTIAL architecture and help show that the old failure is narrower than `Arduino_GFX + PARTIAL + bounce0` in general.

If it flickers, it will be equally valuable because its memory allocation, PCLK edge and Arduino_GFX version differ from the controlled Test 22/23 pair.

Key observations to record:

```text
Boot
UI appearance
Periodic flicker
Touch-redraw flicker
Horizontal jump
Touch response
Touch mapping/corners
Animation smoothness
Reset/crash
```

## Rules

- Preserve the pinned upstream source unchanged for the first physical verdict.
- Do not transplant our Test 19-29 fixes into the upstream project before first test.
- Environment compatibility fixes must be isolated from application/display/touch source.
- Restore the original `platformio.ini` after every build attempt.
- Physical board behavior is decisive.
- If PASS, freeze Test 32 as another known-good third-party reference.

## Queued next architecture

After Test 32, a particularly interesting candidate is:

```text
xoquox/esphome-lvgl
```

That repository contains an explicit `devices/ESP32-8048S043.yaml` hardware profile and a modular 800x480 UI broken into separate YAML `widgets`, `styles`, `themes`, fonts and pages with `!include` composition. It is not runtime widget loading, but it is highly relevant to our future modular UI/widget architecture and should be studied as the next separate branch rather than mixed into Test 32.
