# Test 21 — LVGL9 / Arduino_GFX / EEZ / clumsyCoder00

Independent third-party validation of `clumsyCoder00/Sunton-ESP32-8048S043` on the lab ESP32-8048S043 specimen.

## Goal

Run the upstream project as close to its published configuration as possible and compare it with the two already validated display paths:

```text
Test 19: Arduino_GFX + partial LVGL buffers + RGB bounce buffer = PASS
Test 20: native ESP-IDF esp_lcd + double PSRAM framebuffer + no bounce = PASS
Test 21: Arduino_GFX + LVGL DIRECT_MODE + full-screen copy every loop = pending
```

## Upstream snapshot

- Repository: https://github.com/clumsyCoder00/Sunton-ESP32-8048S043
- Pinned revision: `e89a5cd6f50c54f0d0c49dcdfde4f7dfd909c5c7`
- Revision date: 2024-07-04
- License: GPL-2.0
- Build system: PlatformIO
- Framework: Arduino
- Board profile: `esp32-s3-devkitc-1`

Published library versions:

```text
Arduino_GFX        1.4.7
eez-framework      0.0.1
TAMC_GT911         1.0.2
LVGL               9.1.0
```

The upstream `platformio.ini` does not pin these via `lib_deps`; they are documented in the upstream README instead. Test 21 therefore treats those versions as part of the baseline reproduction target.

## PlatformIO board configuration

Upstream configures:

```text
CPU                  240 MHz
Flash                80 MHz
Flash size           16 MB
Memory type          qio_opi
PSRAM macro          BOARD_HAS_PSRAM
Monitor              115200 baud
```

## RGB configuration

Upstream uses Arduino_GFX with the known ESP32-8048S043 RGB pin map:

```text
DE      40
VSYNC   41
HSYNC   39
PCLK    42
BL      2
```

Timing:

```text
HSYNC polarity/front/pulse/back = 0 / 8 / 4 / 8
VSYNC polarity/front/pulse/back = 0 / 8 / 4 / 8
```

Display object:

```text
800 x 480
rotation = 0
auto_flush = true
```

## LVGL architecture under test

The upstream source enables:

```cpp
#define DIRECT_MODE
```

and disables `RGB_PANEL`.

This is an important architecture to test because LVGL renders into one full 800x480 draw buffer, while the Arduino loop then performs:

```cpp
gfx->draw16bitRGBBitmap(0, 0, (uint16_t *)disp_draw_buf, screenWidth, screenHeight);
```

on every loop iteration.

Therefore Test 21 intentionally stresses continuous full-screen transfers instead of the event-driven partial flush used in Test 19 or the native double-framebuffer path used in Test 20.

## LVGL configuration

The upstream `lv_conf.h` identifies itself as LVGL 9.1.0 and already uses the ESP32/Xtensa-safe renderer settings that were independently required by Test 19:

```text
LV_COLOR_DEPTH              16
LV_USE_DRAW_ARM2D_SYNC      0
LV_USE_NATIVE_HELIUM_ASM    0
LV_USE_DRAW_SW_ASM          LV_DRAW_SW_ASM_NONE
LV_USE_OS                   LV_OS_NONE
LV_DEF_REFR_PERIOD          33 ms
```

## Touch baseline

Upstream uses `TAMC_GT911` with:

```text
SCL       GPIO20
SDA       GPIO19
INT       GPIO0
RST       GPIO38
rotation  ROTATION_NORMAL
raw X     480 -> 0
raw Y     272 -> 0
output    scaled to 800 x 480
```

This differs from Test 20's `esp_lcd_touch_gt911` path and gives us another independent GT911 implementation to validate.

## Controlled-test rule

For the first baseline run:

1. Do not alter RGB timing.
2. Do not add a bounce buffer.
3. Do not replace DIRECT_MODE.
4. Do not remove the full-screen copy from `loop()`.
5. Do not replace TAMC_GT911 with the lab BSP.
6. Do not regenerate the EEZ UI.
7. Record build and physical behavior before making any compatibility patch.

If an environmental compatibility patch is required merely to build on the current toolchain, record it separately before any display-performance change.

## Prepare the external working copy

From the root of `ESP32-8048S043-lab`:

```powershell
powershell -ExecutionPolicy Bypass -File .\third_party_tests\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\prepare-test21.ps1
```

The script checks out the pinned upstream revision under:

```text
.external-test-work/21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00/upstream
```

and reports whether PlatformIO CLI is available.

## Baseline PASS criteria

Display-path PASS requires:

- firmware boots repeatedly;
- EEZ UI renders correctly;
- no periodic full-screen flicker;
- no recurring horizontal image jump;
- no crash/reset;
- continuous full-screen copies remain physically stable for several minutes.

Touch is scored separately for orientation and edge accuracy.

## Why this test matters

If Test 21 also runs stably, the same physical specimen will have three independently validated architectures:

```text
A. Arduino_GFX + partial/bounce
B. native esp_lcd + double framebuffer/no bounce
C. Arduino_GFX + LVGL direct full-frame copy
```

That would narrow the old flicker/jump failure even further: it would no longer be attributable simply to Arduino_GFX, PSRAM, LVGL 9, full-frame rendering, or the panel itself in isolation.