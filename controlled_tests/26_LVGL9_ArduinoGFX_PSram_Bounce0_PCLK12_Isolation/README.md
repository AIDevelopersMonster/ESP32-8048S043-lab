# Test 26 — PSRAM + bounce0 + PCLK 12 MHz isolation

Controlled follow-up to the Test 23 visual FAIL.

## Controlled delta from Test 23

```text
PCLK: 14 MHz -> 12 MHz
```

Everything else remains in the Test 23 failing architecture:

```text
LVGL                    9.1.0
Display format           RGB565 / 2 bytes per pixel
LVGL draw buffers        2 x 20 lines
LVGL draw-buffer memory  PSRAM required
Render mode              PARTIAL
Display path             Arduino_GFX partial-area
RGB bounce buffer        0
HSYNC                    8 / 4 / 8
VSYNC                    8 / 4 / 8
pclk_active_neg          1
Touch                    ESP32_8048S043_Touch BSP / GT911
UI policy                event-driven
```

## Physical result

**VISUAL FAIL.**

Operator report on the physical ESP32-8048S043 specimen:

```text
Flicker remains clearly visible during touch/redraw.
If it became weaker, the change is small enough that it cannot be judged reliably without direct comparison.
The effect cannot be described as barely visible; it remains plainly noticeable.
The number/frequency of repeats is difficult to judge by eye.
```

Therefore no improvement is credited in the controlled result.

## Comparison

```text
Test 23: PSRAM + bounce0 + PCLK14 -> FAIL / clearly visible touch flicker
Test 26: PSRAM + bounce0 + PCLK12 -> FAIL / clearly visible touch flicker
```

## Interpretation

Reducing PCLK by about 14% does not remove the defect and does not produce a visually decisive reduction. This weakens a simple explanation in which the failure is controlled only by average RGB pixel-clock bandwidth near the 14 MHz operating point.

This does not exclude a deeper PSRAM/RGB DMA bandwidth or arbitration mechanism. It only shows that a modest PCLK reduction from 14 MHz to 12 MHz is insufficient.

The next controlled experiment should return to the original 14 MHz Test 23 baseline and vary the driver-level RGB bounce depth itself. Since 0 lines fails and 20 lines passes, the next midpoint is 10 display lines.
