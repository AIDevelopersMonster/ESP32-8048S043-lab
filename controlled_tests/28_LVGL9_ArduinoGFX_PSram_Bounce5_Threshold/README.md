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

## Interpretation

The absence of a visible quality gradient across 5, 10 and 20 lines suggests that, for this exact firmware/board configuration, the important transition may be enabling the driver-level bounce path itself rather than progressively increasing bounce depth.

This is not a universal ESP32-S3 or LVGL rule. It is a controlled observation for this exact Arduino_GFX + ESP-IDF RGB + PSRAM + LVGL9.1 partial-render architecture.
