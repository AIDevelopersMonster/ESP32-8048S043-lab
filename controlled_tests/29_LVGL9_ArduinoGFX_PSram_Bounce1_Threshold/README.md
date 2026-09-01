# Test 29 — RGB bounce mode boundary — CLOSED

## Final status

**PHYSICAL VISUAL PASS — CLEAN.**

The physical ESP32-8048S043 specimen showed no visible difference between:

```text
bounce1
bounce5
bounce10
bounce20
```

All four non-zero settings are visually clean during the same touch/redraw interaction that produces clearly visible flicker with `bounce0`.

The topic is therefore closed at the current architecture level.

## Controlled matrix

```text
Test 23: PSRAM LVGL buffers + bounce0  + PCLK14 -> FAIL / visible touch-redraw flicker
Test 24: PSRAM LVGL buffers + bounce20 + PCLK14 -> PASS / clean
Test 25: PSRAM LVGL buffers + bounce0  + manual INTERNAL source staging -> FAIL / same flicker
Test 26: PSRAM LVGL buffers + bounce0  + PCLK12 -> FAIL / same clearly visible flicker
Test 27: PSRAM LVGL buffers + bounce10 + PCLK14 -> PASS / clean
Test 28: PSRAM LVGL buffers + bounce5  + PCLK14 -> PASS / clean
Test 29: PSRAM LVGL buffers + bounce1  + PCLK14 -> PASS / clean
```

No visible quality gradient was observed between 1, 5, 10 and 20 display lines.

## What the code proves

Test 29 is regenerated from the physically failing Test 23 baseline and changes exactly one runtime parameter:

```cpp
static constexpr uint32_t RGB_BOUNCE_PIXELS = LCD_WIDTH * 1;
```

The generator keeps the following controls unchanged:

```text
LVGL                    9.1.0
Display format           RGB565 / 2 bytes per pixel
LVGL draw buffers        2 x 20 lines
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

Arduino_GFX passes its `bounce_buffer_size_px` directly into the ESP-IDF RGB panel configuration.

In ESP-IDF 5.5.5, `bounce_buffer_size_px == 0` and `bounce_buffer_size_px > 0` are not the same DMA architecture with different buffer sizes.

When the resulting bounce byte size is zero, the RGB scanout DMA uses the framebuffer path directly.

When it is non-zero, the driver:

1. allocates **two** bounce buffers from `MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT` memory;
2. creates a dedicated bounce-buffer DMA link;
3. pre-fills the bounce buffers;
4. starts RGB DMA from the bounce-buffer link rather than from the framebuffer link;
5. refills subsequent chunks from the framebuffer in the bounce-buffer EOF path;
6. on ESP32-S3, preloads the next framebuffer chunk into cache.

Therefore `bounce0 -> bounce>0` is a qualitative transport-mode switch.

## Theoretical conclusion

For this exact tested architecture, the decisive factor for the observed flicker is **not the bounce depth within the tested range**.

The decisive factor is whether the driver-level RGB bounce transport mode exists at all:

```text
bounce = 0
    direct framebuffer / PSRAM scanout path
    -> visible flicker during redraw

bounce > 0
    INTERNAL double-bounce DMA transport path
    -> no visible flicker in all tested non-zero cases
```

Thus the experimentally supported statement is:

> **For this ESP32-8048S043 / Arduino_GFX / ESP-IDF 5.5.5 / LVGL 9.1 partial-render configuration, enabling a valid non-zero RGB bounce buffer is sufficient to remove the observed visible redraw flicker. Increasing the tested bounce depth from 1 to 20 display lines does not produce a visible improvement.**

This is an architecture-specific engineering result, not a universal theorem about every ESP32-S3, every RGB panel, or every LVGL application.

## Why buffer size still matters

Although 1, 5, 10 and 20 lines look the same on screen, their resource costs differ.

For 800×480 RGB565:

```text
bytes per line = 800 * 2 = 1600 bytes
```

ESP-IDF allocates two INTERNAL bounce buffers, so the approximate bounce pixel-storage cost is:

| Bounce depth | One buffer | Two buffers total | Chunks per 480-line frame | Physical result |
|---:|---:|---:|---:|---|
| 0 lines | 0 B | 0 B | direct path | FAIL / flicker |
| 1 line | 1,600 B | 3,200 B | 480 | PASS / clean |
| 5 lines | 8,000 B | 16,000 B | 96 | PASS / clean |
| 10 lines | 16,000 B | 32,000 B | 48 | PASS / clean |
| 20 lines | 32,000 B | 64,000 B | 24 | PASS / clean |

These figures count bounce pixel storage only; DMA descriptors and driver bookkeeping add a smaller additional cost.

With the configured timings:

```text
horizontal total = 800 + 8 + 4 + 8 = 820 clocks
vertical total   = 480 + 8 + 4 + 8 = 500 lines
PCLK             = 14 MHz
estimated refresh = 14,000,000 / (820 * 500) ~= 34.15 Hz
```

That implies approximately:

| Bounce depth | Approx. bounce EOF/refill events per second |
|---:|---:|
| 1 line | ~16,390 / s |
| 5 lines | ~3,278 / s |
| 10 lines | ~1,639 / s |
| 20 lines | ~820 / s |

So the smallest bounce minimizes INTERNAL SRAM use but substantially increases refill/interrupt activity. The largest tested bounce uses more INTERNAL SRAM but gives the driver much more time per refill and reduces ISR/DMA-link churn.

## Practical solution

For this project:

```text
DO NOT use bounce0
when the LVGL partial draw buffers are in PSRAM on this Arduino_GFX RGB path.
```

Use a valid non-zero driver-level RGB bounce buffer.

### Production default

Keep the already validated conservative value:

```cpp
RGB_BOUNCE_PIXELS = LCD_WIDTH * 20;
```

Reason:

- physically clean;
- already validated in the known-good Test 24 architecture;
- same visible result as 1/5/10 lines;
- much lower refill/EOF rate than very small bounce sizes;
- provides more timing margin for future heavier application load.

If INTERNAL SRAM later becomes constrained, `10 lines` is the first recommended reduction:

```text
20 lines -> ~64 KB bounce pixel storage total
10 lines -> ~32 KB bounce pixel storage total
```

and Test 27 already proves 10 lines visually clean on the same specimen.

The 1-line result is valuable as a causal experiment, not as the preferred production setting.

## Closed engineering rule

```text
PSRAM LVGL partial buffers
+
Arduino_GFX RGB framebuffer path
+
bounce0
= visible redraw flicker on this specimen/configuration

PSRAM LVGL partial buffers
+
Arduino_GFX RGB framebuffer path
+
any tested valid non-zero bounce (1/5/10/20 lines)
= visually clean
```

**Conclusion: the existence of driver-level bounce transport solves the observed flicker; bounce depth is a resource/performance tuning parameter, not a visible-quality parameter within the tested range.**

## Scope

Validated on this exact path:

```text
ESP32-S3
Arduino-ESP32 / ESP-IDF 5.5.5
Arduino_GFX RGB panel
LVGL 9.1.0
800x480 RGB565
PSRAM-backed LVGL partial draw buffers
PCLK 14 MHz
HSYNC 8/4/8
VSYNC 8/4/8
pclk_active_neg = 1
GT911 BSP touch
```

No further bounce-depth search is planned unless a future firmware version, heavier workload, different PCLK, different panel, or SRAM pressure creates a new engineering requirement.
