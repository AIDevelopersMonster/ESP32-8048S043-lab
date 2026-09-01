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

## Physical result

**VISUAL PASS.**

Operator report on the physical ESP32-8048S043 specimen:

```text
Visible flicker: DISAPPEARED
```

## Closed decision matrix

```text
Test 22: INTERNAL SRAM + bounce0  -> VISUAL PASS
Test 23: PSRAM         + bounce0  -> VISUAL FAIL / flicker
Test 24: PSRAM         + bounce20 -> VISUAL PASS
```

## Interpretation

This closes the first clean interaction matrix in the current LVGL9 controlled line.

The observations exclude the following overly broad explanations:

- PSRAM is not globally incompatible with LVGL draw buffers, because Test 24 is stable with the same PSRAM draw buffers.
- RGB bounce buffering is not globally required, because Test 22 is stable without it when the LVGL draw buffers are in INTERNAL SRAM.
- The visible defect is not explained by LVGL9, PARTIAL mode, Arduino_GFX, panel timing, or touch alone, because those controls remain unchanged across Tests 22–24.

The controlled evidence supports a narrower conclusion:

```text
PSRAM-backed LVGL partial draw buffers
+
Arduino_GFX RGB path without internal bounce staging
=> visible flicker on this specimen
```

and

```text
adding the 20-line RGB bounce buffer
=> visible stability restored
```

This strongly implicates a memory-bandwidth / staging / scanout interaction rather than a generic rendering bug. The exact internal mechanism is not yet proven.

## Next controlled experiment

To distinguish the specific Arduino_GFX RGB bounce mechanism from the more general effect of moving PSRAM draw data through INTERNAL SRAM, the next test should start from the Test 23 failing configuration and:

```text
keep LVGL draw buffers in PSRAM
keep RGB bounce = 0
add one explicit INTERNAL SRAM staging buffer in lvglFlush()
copy the dirty region from PSRAM to INTERNAL SRAM before draw16bitRGBBitmap()
```

If that removes flicker, the essential factor is the PSRAM-to-INTERNAL staging boundary rather than the library's bounce implementation itself.
