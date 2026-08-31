# Test 20 — LVGL9 / ESP-IDF / limpens

External reference validation for the Sunton ESP32-8048S043 display.

## Goal

Run the upstream `limpens/esp32-8048S043-lvgl9` project as close to upstream as possible and compare its physical display behaviour with our successful Test 19.

This test deliberately does **not** copy upstream source files into this repository. At the time of this audit the upstream repository does not contain a license file, so this test records the source, revision, configuration and operator evidence while leaving the third-party code in its original repository.

## Upstream

- Repository: https://github.com/limpens/esp32-8048S043-lvgl9
- Pinned revision: `eb1b8cff63e5a703631ab1638ff76eb7ba7e7a51`
- Upstream revision date: 2025-11-05
- Framework described by upstream: ESP-IDF 5.1+
- Managed components declared at the pinned revision:
  - `espressif/esp_lcd_touch_gt911 >=1.1.0`
  - `lvgl/lvgl ^9.4.0`

## Reproduced dependency resolution — 2026-08-31

On the lab Windows host, with standalone ESP-IDF v5.5.5 and the pinned upstream revision, `idf.py set-target esp32s3` completed configuration successfully and generated a new `dependencies.lock` in the ignored external working copy.

Resolved components:

```text
idf                         5.5.5
lvgl/lvgl                   9.5.0
espressif/esp_lcd_touch     1.2.1
espressif/esp_lcd_touch_gt911 1.2.1
```

Observed compiler/toolchain during configure:

```text
C compiler                  GNU 14.2.0
CXX compiler                GNU 14.2.0
Target                      esp32s3
Python                      3.14.6
```

Configure status: **PASS**.

The upstream dependency declaration remains unchanged. The resolved versions above describe this specific reproduced test environment and are recorded separately so that the original project is not silently modified.

## Architecture under test

```text
LVGL 9.x
  -> two partial LVGL draw buffers in PSRAM
  -> esp_lcd_panel_draw_bitmap()
  -> ESP-IDF RGB panel driver
  -> two full RGB framebuffers in PSRAM
  -> 800x480 RGB panel

GT911
  -> esp_lcd_touch_gt911
  -> coordinate mapping
  -> LVGL pointer input
```

## Important upstream parameters

The pinned upstream revision uses:

- LCD: `800 x 480`
- pixel clock: `18 MHz`
- HSYNC: pulse `4`, back `8`, front `8`
- VSYNC: pulse `4`, back `8`, front `8`
- `pclk_active_neg = true`
- RGB data width: `16`
- RGB panel full framebuffers: `2`
- framebuffer placement: PSRAM
- `double_fb = true`
- RGB bounce buffer: `0`
- LVGL render mode: partial
- LVGL partial buffers: PSRAM
- touch calibration domain: approximately `0..477 x 0..269`
- touch output domain: `800 x 480`

This is intentionally a strong contrast with Test 19, which achieved a physical PASS with Arduino_GFX, 14 MHz PCLK, two 20-line LVGL buffers in internal SRAM and a 20-line RGB bounce buffer.

## Controlled-test rule

For the first physical run:

1. Do not change PCLK.
2. Do not add a bounce buffer.
3. Do not move upstream framebuffers or LVGL buffers.
4. Do not substitute our GT911 BSP.
5. Do not rewrite the UI.
6. Record the original behaviour first.

Only after an upstream-baseline result is recorded may a `20B` adaptation be created.

## Prepare upstream working copy

From the root of `ESP32-8048S043-lab` in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\third_party_tests\20_LVGL9_ESP-IDF_limpens\prepare-test20.ps1
```

The script creates a local ignored-style working directory under `.external-test-work/` and checks out the pinned upstream revision.

## Build

Activate your ESP-IDF environment first, then:

```powershell
cd .external-test-work\20_LVGL9_ESP-IDF_limpens\upstream
idf.py set-target esp32s3
idf.py build
```

If build succeeds, connect the board and run:

```powershell
idf.py flash monitor
```

Exit the monitor with `Ctrl+]`.

## Evidence to record

Record all of the following before changing upstream code:

- ESP-IDF version (`idf.py --version`)
- exact upstream commit
- managed component versions resolved during build
- whether build is PASS/FAIL
- whether flash is PASS/FAIL
- boot log
- display image orientation and colors
- visible flicker/jitter/tearing
- whether the whole screen periodically shifts
- touch orientation and mapping
- touch stability
- LVGL performance monitor values if visible
- behaviour while idle for at least several minutes

## PASS criteria

The upstream baseline is a display-path PASS if:

- firmware boots repeatedly;
- image remains stable;
- no recurring full-screen flicker is visible;
- no recurring horizontal image jump is visible;
- UI continues running without crash/reset;
- memory/display path remains stable during an idle observation period.

Touch accuracy is recorded separately and does not invalidate a display-path PASS.

## Comparison target

### Test 19 — physical PASS

```text
LVGL 9.1.0
-> two 20-line INTERNAL-SRAM partial draw buffers
-> Arduino_GFX partial flush
-> RGB bounce buffer = 20 lines
-> PCLK 14 MHz
-> stable event-driven UI
-> no periodic LVGL redraw while idle
```

### Test 20 — upstream baseline

```text
LVGL 9.5.0 (resolved on 2026-08-31)
-> PSRAM partial draw buffers
-> esp_lcd
-> two full framebuffers in PSRAM
-> no bounce buffer
-> PCLK 18 MHz
-> configure: PASS
-> compile: pending
-> physical result: pending
```

The purpose is not to decide which framework is "better" from one run. The purpose is to identify which independent RGB scan-out architectures are physically stable on our specimen and which parameters are genuinely necessary.