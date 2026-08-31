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
idf                            5.5.5
lvgl/lvgl                      9.5.0
espressif/esp_lcd_touch        1.2.1
espressif/esp_lcd_touch_gt911  1.2.1
```

Observed compiler/toolchain during configure:

```text
C compiler                  GNU 14.2.0
CXX compiler                GNU 14.2.0
Target                      esp32s3
Python                      3.14.6
```

Configure status: **PASS**.
Build status: **PASS**.
Flash/boot status: **PASS**.
Physical display status: **PASS**.
Touch status: **PASS**.

Successful build produced the standard ESP-IDF flash set:

```text
0x0000  build/bootloader/bootloader.bin
0x8000  build/partition_table/partition-table.bin
0x10000 build/lvgl9.bin
```

Build output reported flash mode `dio`, flash size `16MB`, flash frequency `80m`.

The upstream dependency declaration remains unchanged. The resolved versions above describe this specific reproduced test environment and are recorded separately so that the original project is not silently modified.

## Architecture under test

```text
LVGL 9.5.0
  -> two partial LVGL draw buffers in PSRAM
  -> esp_lcd_panel_draw_bitmap()
  -> ESP-IDF RGB panel driver
  -> two full RGB framebuffers in PSRAM
  -> 800x480 RGB panel

GT911
  -> esp_lcd_touch_gt911 1.2.1
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

## Physical result — 2026-08-31

The unchanged upstream baseline was flashed to the lab ESP32-8048S043 specimen and tested physically.

Observed result:

```text
Display image              PASS
Visible flicker            none observed
Horizontal image jumps     none observed
UI                         stable
LVGL performance monitor   about 22–25 FPS
LVGL render time           about 1–2 ms
Touch                      responsive and correctly oriented
Touch edge accuracy        approximately 2–3 px from nominal 800x480 limits
```

The displayed upstream demo included the Espressif logo and operated smoothly during interaction.

Touch mapping is effectively correct across the panel. At the corners the reported coordinates approach the 800x480 output limits within roughly 2–3 pixels, so no immediate calibration correction is required for this baseline.

### Physical verdict

**TEST 20 = PASS.**

The board is physically stable with the upstream native ESP-IDF RGB path at 18 MHz using two full framebuffers in PSRAM and no RGB bounce buffer.

This demonstrates that a bounce buffer is **not an intrinsic requirement of the ESP32-8048S043 panel itself**. The successful Test 19 and Test 20 instead establish at least two distinct stable scan-out architectures on the same specimen:

```text
Test 19
LVGL 9.1.0
-> two 20-line INTERNAL-SRAM partial draw buffers
-> Arduino_GFX partial flush
-> RGB bounce buffer = 20 lines
-> PCLK 14 MHz
-> PASS

Test 20
LVGL 9.5.0
-> PSRAM partial draw buffers
-> native esp_lcd RGB
-> two full RGB framebuffers in PSRAM
-> RGB bounce buffer = 0
-> PCLK 18 MHz
-> PASS
```

The result therefore shifts the investigation away from a simple "panel requires bounce buffering" explanation. Stability depends on the complete driver/buffering/flush architecture. Test 20 also provides a strong native ESP-IDF reference implementation for future comparisons.

## Controlled-test rule

The baseline above was obtained without changing the upstream PCLK, framebuffer policy, bounce-buffer setting, GT911 path or UI. Any later adaptation must be recorded separately as `20B` or later and must not overwrite this baseline result.

## Prepare upstream working copy

From the root of `ESP32-8048S043-lab` in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\third_party_tests\20_LVGL9_ESP-IDF_limpens\prepare-test20.ps1
```

The script creates a local ignored-style working directory under `.external-test-work/` and checks out the pinned upstream revision.

## Build and flash

Activate the standalone ESP-IDF v5.5.5 environment first, then:

```powershell
cd .external-test-work\20_LVGL9_ESP-IDF_limpens\upstream
idf.py set-target esp32s3
idf.py build
idf.py flash monitor
```

Exit the monitor with `Ctrl+]`.

## Final comparison target

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

### Test 20 — physical PASS

```text
LVGL 9.5.0
-> PSRAM partial draw buffers
-> esp_lcd
-> two full framebuffers in PSRAM
-> no bounce buffer
-> PCLK 18 MHz
-> about 22–25 FPS
-> about 1–2 ms LVGL render time
-> touch edge error about 2–3 px
-> stable physical output
```

The purpose is not to declare one framework universally better. The important result is that two materially different RGB scan-out architectures are now independently validated on the same physical specimen.