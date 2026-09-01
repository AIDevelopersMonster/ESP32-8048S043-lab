# Test 29 — PSRAM + one-line RGB bounce lower-bound check

## Purpose

Test whether the visible transition observed in Tests 23/24/27/28 is primarily the transition between the two ESP-IDF RGB transport modes:

```text
bounce_buffer_size_px == 0
versus
bounce_buffer_size_px > 0
```

rather than a gradual dependence on bounce depth.

## Controlled delta from Test 23

```text
RGB bounce buffer: 0 -> 1 display line = 800 pixels
```

All other runtime controls remain at the original failing Test 23 operating point:

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

## Existing physical matrix

```text
bounce0  -> FAIL / clearly visible touch redraw flicker
bounce5  -> PASS / clean
bounce10 -> PASS / clean
bounce20 -> PASS / clean
```

No visible difference was reported between 5, 10 and 20 lines.

## Driver-level reason this test matters

In ESP-IDF 5.5.5, `bounce_buffer_size_px` is not merely a tuning value attached to an otherwise identical DMA path.

When the resulting bounce-buffer byte size is zero, RGB GDMA is linked directly to the framebuffer path.

When it is nonzero, the driver:

1. Allocates **two** bounce buffers from `MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT` memory.
2. Builds a dedicated DMA link list over those bounce buffers.
3. Pre-fills both bounce buffers before starting RGB transmission.
4. Uses DMA EOF handling to refill successive chunks from the framebuffer.
5. On ESP32-S3, preloads the next framebuffer chunk into cache.
6. Uses the bounce-buffer DMA/restart path instead of the direct framebuffer DMA path.

Therefore the transition `0 -> nonzero` changes the execution topology of RGB scanout.

## Test 29 decision

```text
If bounce1 is clean:
    Strong evidence that the observed stability boundary is principally
    direct-PSRAM-DMA versus INTERNAL-bounce-DMA mode, not bounce depth.

If bounce1 flickers:
    A real minimum useful bounce size exists somewhere between 1 and 5 lines
    for this exact firmware/board configuration.
```

## Scope

This result characterizes this exact path only:

```text
ESP32-S3
Arduino-ESP32 / ESP-IDF 5.5.5
Arduino_GFX RGB panel
LVGL 9.1.0
800x480 RGB565
PSRAM framebuffer/draw-buffer environment
PARTIAL LVGL rendering
PCLK 14 MHz
```

It must not be generalized into a universal ESP32-S3 or LVGL rule without separate validation.
