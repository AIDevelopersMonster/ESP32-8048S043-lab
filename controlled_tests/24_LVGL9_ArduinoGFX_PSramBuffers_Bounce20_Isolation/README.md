# Test 24 — LVGL9 Arduino_GFX PSRAM Draw Buffers + Bounce20

Controlled follow-up to Test 23.

## Baseline

Test 23 physically reproduced visible flicker with:

```text
LVGL draw buffers        PSRAM
RGB bounce buffer        0
Render mode              PARTIAL
Display path             Arduino_GFX partial-area
PCLK                     14 MHz
HSYNC                    8/4/8
VSYNC                    8/4/8
```

## Test 24 controlled delta

Change exactly one runtime factor:

```text
RGB bounce buffer: 0 -> 20 display lines
```

Retain:

```text
LVGL                    9.1.0
Display format           RGB565 / 2 bytes per pixel
LVGL draw buffers        2 x 20 lines
LVGL draw-buffer bytes   32000 bytes each
LVGL draw-buffer memory  PSRAM required
Render mode              PARTIAL
Display path             Arduino_GFX partial-area
PCLK                     14 MHz
HSYNC                    8 / 4 / 8
VSYNC                    8 / 4 / 8
pclk_active_neg          1
Touch                    ESP32_8048S043_Touch BSP / GT911
UI policy                event-driven
```

## Decision matrix

```text
Test 22: INTERNAL SRAM + bounce0  -> VISUAL PASS
Test 23: PSRAM         + bounce0  -> VISUAL FAIL / flicker
Test 24: PSRAM         + bounce20 -> physical verdict required
```

If Test 24 becomes visually stable, that strongly implicates an interaction between PSRAM-backed partial draw data and the RGB scanout/update path that can be mitigated by internal bounce staging.

If flicker remains, the next controlled experiment should keep PSRAM draw buffers and stage each LVGL partial flush explicitly through an INTERNAL SRAM transfer buffer before `draw16bitRGBBitmap()`.
