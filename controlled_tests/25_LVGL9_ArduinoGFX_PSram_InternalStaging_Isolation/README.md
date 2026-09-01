# Test 25 — PSRAM LVGL buffers with explicit INTERNAL flush staging

Controlled follow-up to the Test 23 visual FAIL.

## Starting point

Test 23 reproduced flicker with:

```text
LVGL draw buffers        PSRAM
RGB bounce buffer        0
Render mode              PARTIAL
Display path             Arduino_GFX partial-area
PCLK                     14 MHz
HSYNC                    8/4/8
VSYNC                    8/4/8
```

Test 24 then restored visual stability by changing only:

```text
RGB bounce buffer        0 -> 20 display lines
```

## Test 25 purpose

Determine whether the essential stabilizing factor is specifically the Arduino_GFX RGB bounce implementation, or the more general act of staging PSRAM-backed partial draw data through INTERNAL SRAM before handing it to the display path.

## Controlled delta from Test 23

Retain the failing Test 23 configuration:

```text
LVGL draw buffers        2 x 20 lines in PSRAM
RGB bounce buffer        0
LVGL                    9.1.0
Display format           RGB565
Render mode              PARTIAL
Arduino_GFX partial-area flush
PCLK                     14 MHz
HSYNC                     8/4/8
VSYNC                     8/4/8
pclk_active_neg           1
GT911 BSP
Event-driven UI
```

Add exactly one transfer stage:

```text
PSRAM pxMap
  -> memcpy()
  -> one 32000-byte INTERNAL SRAM staging buffer
  -> draw16bitRGBBitmap()
```

The staging buffer is the same maximum size as one LVGL 20-line partial draw buffer.

## Decision matrix

```text
Test 22: INTERNAL LVGL buffers + bounce0 + direct draw       -> PASS
Test 23: PSRAM LVGL buffers    + bounce0 + direct draw       -> FAIL / flicker
Test 24: PSRAM LVGL buffers    + bounce20 + direct draw      -> PASS
Test 25: PSRAM LVGL buffers    + bounce0 + INTERNAL staging  -> ?
```

## Interpretation

If Test 25 is visually stable, then the essential factor is not unique to the Arduino_GFX bounce implementation. The stronger engineering conclusion becomes:

```text
PSRAM-backed LVGL partial chunks must cross an INTERNAL-SRAM staging boundary before the Arduino_GFX RGB update path on this specimen/configuration.
```

If Test 25 still flickers, then the Arduino_GFX RGB bounce mechanism is doing something more specific than merely copying the dirty chunk through INTERNAL SRAM, and the next experiments should target RGB DMA/scanout synchronization and bounce-buffer timing semantics.
