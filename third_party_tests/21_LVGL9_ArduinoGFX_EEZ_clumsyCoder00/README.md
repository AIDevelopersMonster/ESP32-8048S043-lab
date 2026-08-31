# Test 21 — LVGL9 / Arduino_GFX / EEZ / clumsyCoder00

Independent third-party validation of `clumsyCoder00/Sunton-ESP32-8048S043` on the lab ESP32-8048S043 specimen.

## Goal

Run the upstream project as close to its published configuration as possible and compare it with the two already validated display paths:

```text
Test 19: Arduino_GFX + partial LVGL buffers + RGB bounce buffer = PASS
Test 20: native ESP-IDF esp_lcd + double PSRAM framebuffer + no bounce = PASS
Test 21: Arduino_GFX + LVGL DIRECT_MODE + full-screen copy every loop = PASS
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

LVGL renders into one full 800x480 draw buffer, while the Arduino loop performs:

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

This differs from Test 20's `esp_lcd_touch_gt911` path and gives another independent GT911 implementation.

## Controlled-test rule

For the first baseline run:

1. Do not alter RGB timing.
2. Do not add a bounce buffer.
3. Do not replace DIRECT_MODE.
4. Do not remove the full-screen copy from `loop()`.
5. Do not replace TAMC_GT911 with the lab BSP.
6. Do not regenerate the EEZ UI.
7. Record build and physical behavior before making any display-performance patch.

## Reproduction compatibility layers

The 2024 upstream source does not build unchanged under the 2026 PlatformIO/Arduino-ESP32 environment. The following environment-only compatibility layers were required. None changes the display, LVGL UI, touch logic or timing code.

### 1. Dependency reconstruction

`install-test21-deps.ps1` installs the published dependency versions into the upstream `lib` directory:

```text
Arduino_GFX        1.4.7
LVGL               9.1.0
TAMC_GT911         1.0.2
eez-framework      0.0.1-era commit 0f8e367bfa10e32340514530a77a1098e5e90ce2
```

The local EEZ package-manager metadata is adjusted so PlatformIO does not download a second unconstrained `lvgl >=8.3.0`; the exact local LVGL 9.1.0 copy remains authoritative.

### 2. Historical Arduino-ESP32 environment

The current 2026 `espressif32` platform supplies Arduino-ESP32 3.x and is incompatible with Arduino_GFX 1.4.7's old SPI HAL API. For the 2024 baseline, `apply-test21-2024-platform.ps1` pins:

```text
PlatformIO platform     espressif32 @ 6.7.0
Arduino-ESP32           2.0.16
ESP-IDF                 4.4.7
```

This corresponds to the toolchain generation available before the upstream snapshot date.

### 3. Windows short-path build

The full lab checkout path is long enough for the LVGL 9.1.0 nested include tree to fail on Windows with a false missing-header error for `src/lv_conf_internal.h`. The file is present. `run-test21-shortpath.ps1` temporarily maps the project to a short drive path using `subst`, builds there, then removes the mapping.

## Build result

Final clean historical-environment build: **PASS**.

```text
PlatformIO Core           6.1.19
Platform                  espressif32 @ 6.7.0
Arduino_GFX               1.4.7
LVGL                      9.1.0
eez-framework             0.0.1
TAMC_GT911                1.0.2
RAM                       100308 / 327680 bytes = 30.6%
Flash                     641761 / 6553600 bytes = 9.8%
firmware.elf              linked successfully
firmware.bin              generated successfully
esptool.py                4.5.1
Result                    SUCCESS
```

The successful build confirms that the previous failures were reproducibility/toolchain issues, not evidence against the Test 21 display architecture.

## Physical result

Physical test on the lab ESP32-8048S043 specimen: **PASS**.

User report after flashing the unmodified display/UI baseline:

```text
Everything works excellently.
```

Observed acceptance status:

```text
EEZ UI                         PASS
Display output                 PASS
Periodic full-screen flicker   NOT OBSERVED
Recurring horizontal jump      NOT OBSERVED
Crash/reset                    NOT OBSERVED
Touch                          PASS
Overall physical result        PASS
```

No RGB timing, bounce-buffer, DIRECT_MODE, full-screen-copy, touch-driver or EEZ UI changes were required to obtain the physical PASS.

## Reproduction commands

From the root of `ESP32-8048S043-lab`:

```powershell
powershell -ExecutionPolicy Bypass -File .\third_party_tests\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\prepare-test21.ps1
powershell -ExecutionPolicy Bypass -File .\third_party_tests\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\install-test21-deps.ps1
powershell -ExecutionPolicy Bypass -File .\third_party_tests\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\apply-test21-2024-platform.ps1
powershell -ExecutionPolicy Bypass -File .\third_party_tests\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\run-test21-shortpath.ps1
```

For flashing:

```powershell
powershell -ExecutionPolicy Bypass -File .\third_party_tests\21_LVGL9_ArduinoGFX_EEZ_clumsyCoder00\run-test21-shortpath.ps1 -Upload
```

## Architectural comparison

The same physical specimen now has three independently validated display architectures:

```text
Test 19
LVGL 9.1
  -> two small 20-line INTERNAL-SRAM partial draw buffers
  -> Arduino_GFX partial-area flush
  -> Arduino_GFX RGB bounce buffer, 20 lines
  -> RGB panel

Test 20
LVGL 9.5
  -> two partial draw buffers in PSRAM
  -> native ESP-IDF esp_lcd_panel_draw_bitmap()
  -> two full RGB framebuffers in PSRAM
  -> no bounce buffer
  -> RGB panel

Test 21
LVGL 9.1 DIRECT_MODE
  -> one full 800x480 LVGL draw buffer
  -> no normal LVGL partial transfer path
  -> draw16bitRGBBitmap() copies the entire screen through Arduino_GFX every loop
  -> RGB panel
```

The visual result can look similar, but the memory ownership, flush policy and transfer mechanism are different.

## Main conclusion

Test 21 materially strengthens the isolation result from Tests 19 and 20.

The old periodic horizontal jump/flicker cannot now be attributed simply to any one of the following in isolation:

- Arduino_GFX;
- PSRAM;
- LVGL 9;
- full-screen rendering;
- continuous full-screen copies;
- absence of a bounce buffer;
- presence of a bounce buffer;
- the RGB panel itself.

In particular, Test 21 proves on this specimen that **continuous full-screen copying through Arduino_GFX can be physically stable**. The earlier failing architecture must therefore depend on a more specific interaction among framebuffer ownership, flush scheduling, synchronization/timing or buffer-management details rather than on full-screen redraw alone.