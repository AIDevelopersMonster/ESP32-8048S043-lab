# Test 25 — PSRAM LVGL buffers with explicit INTERNAL flush staging

Controlled follow-up to the Test 23 visual FAIL.

## Starting point

Test 23 reproduced visible flicker during touch-driven redraw with:

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

Determine whether the essential stabilizing factor is specifically the RGB panel bounce implementation, or the more general act of staging PSRAM-backed partial draw data through INTERNAL SRAM before handing it to Arduino_GFX.

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

## Physical result

**VISUAL FAIL — SAME PRACTICAL FLICKER AS TEST 23.**

Operator correction on the physical ESP32-8048S043 specimen:

```text
Flicker appears during touch/redraw.
If there is any difference from Test 23, it is not distinguishable by eye without special preparation or measurement.
```

Therefore Test 25 must not be classified as a separate jitter/drift failure mode. For engineering purposes its visual behavior is the same as Test 23.

## Decision matrix

```text
Test 22: INTERNAL LVGL buffers + bounce0 + direct draw       -> PASS
Test 23: PSRAM LVGL buffers    + bounce0 + direct draw       -> FAIL / touch flicker
Test 24: PSRAM LVGL buffers    + bounce20 + direct draw      -> PASS
Test 25: PSRAM LVGL buffers    + bounce0 + INTERNAL staging  -> FAIL / same touch flicker
```

## Interpretation

Test 25 rejects the simple hypothesis that the RGB bounce buffer is equivalent to copying each LVGL dirty chunk through INTERNAL SRAM before `draw16bitRGBBitmap()`.

Manual source staging produces no practically observable improvement. Therefore the stabilizing action of the real RGB bounce buffer must occur deeper in the RGB transport/scanout path than the LVGL flush-source pointer alone.

The current evidence supports this working model:

```text
PSRAM-backed LVGL partial updates
+
RGB scanout without driver-level bounce buffering
=> visible flicker during touch/redraw on this specimen
```

while:

```text
driver-level RGB bounce buffering
=> visual stability restored
```

The next controlled experiments should target RGB scanout bandwidth/timing rather than additional LVGL source-buffer copies.
