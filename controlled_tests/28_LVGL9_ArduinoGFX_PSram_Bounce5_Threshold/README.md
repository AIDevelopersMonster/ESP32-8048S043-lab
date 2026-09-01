# Test 28 — PSRAM + bounce5 threshold check

Controlled follow-up to Tests 23, 24 and 27.

## Controlled delta from Test 23

```text
RGB bounce buffer: 0 -> 5 display lines
```

All other runtime controls remain at the original Test 23 operating point:

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

## Physical result

**VISUAL PASS — CLEAN.**

Operator report:

```text
No visible difference between bounce20, bounce10 and bounce5.
All three are clean by eye.
```

## Matrix

```text
bounce0  -> FAIL / touch flicker
bounce5  -> PASS / clean
bounce10 -> PASS / clean
bounce20 -> PASS / clean
```

## Driver-level finding

Inspection of ESP-IDF 5.5.5 shows that `bounce_buffer_size_px == 0` and `bounce_buffer_size_px > 0` are not merely different buffer sizes. They select different RGB DMA transport paths.

With bounce disabled, GDMA is linked to the framebuffer path directly. With any valid non-zero bounce size, the driver allocates two INTERNAL + DMA-capable bounce buffers, builds a dedicated bounce-buffer DMA link list, pre-fills both buffers, starts GDMA from that bounce link, and refills the next chunk from the framebuffer on DMA EOF events.

Therefore the transition

```text
bounce = 0 -> bounce > 0
```

is a qualitative execution-path change. The transitions

```text
1 -> 5 -> 10 -> 20 lines
```

only change chunk size inside the already-enabled bounce transport mode.

This explains why the current evidence can show a sharp difference between zero and non-zero bounce without showing a visible gradient between 5, 10 and 20 lines.

## Scope

This remains a controlled observation for this exact Arduino_GFX + ESP-IDF RGB + PSRAM + LVGL9.1 partial-render architecture. It is not a universal rule for all ESP32-S3/LVGL configurations.
