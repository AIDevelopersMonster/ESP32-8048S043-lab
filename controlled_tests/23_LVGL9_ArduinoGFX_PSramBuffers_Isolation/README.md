# Test 23 — LVGL9 Arduino_GFX PSRAM Draw-Buffer Isolation

Controlled derivative of the physical-PASS Test 22 baseline.

## Controlled delta

Exactly one runtime factor is changed:

```text
Test 22: LVGL draw buffers = INTERNAL SRAM
Test 23: LVGL draw buffers = PSRAM
```

Retained controls:

```text
LVGL                    9.1.0
Display format           RGB565 / 2 bytes per pixel
LVGL draw buffers        2 x 20 lines
LVGL draw-buffer bytes   32000 bytes each
Render mode              PARTIAL
Display path             Arduino_GFX partial-area
RGB bounce buffer        0
PCLK                     14 MHz
HSYNC                     8 / 4 / 8
VSYNC                     8 / 4 / 8
pclk_active_neg           1
Touch                     ESP32_8048S043_Touch BSP / GT911
UI policy                 event-driven
```

## Physical result

**FUNCTIONAL RUN / VISUAL FAIL.**

Operator report on the physical ESP32-8048S043 specimen:

```text
Visible flicker: RETURNED
```

This is the first clean A/B result in the current LVGL9 controlled line where moving only the two LVGL draw buffers from INTERNAL SRAM to PSRAM reintroduces the visible defect.

Controlled pair:

```text
Test 22: INTERNAL SRAM + PARTIAL + Arduino_GFX + bounce0 -> VISUAL PASS
Test 23: PSRAM         + PARTIAL + Arduino_GFX + bounce0 -> VISUAL FAIL / flicker
```

## Interpretation

Within this exact Arduino_GFX partial-area architecture on the tested specimen, PSRAM placement of the LVGL draw buffers is sufficient to reintroduce visible flicker.

This result does **not** mean that PSRAM is globally unsuitable for RGB/LVGL. Other known-good architectures in this lab use PSRAM successfully. The result is specific to this controlled path and points toward an interaction among:

- LVGL partial-area source buffers in PSRAM;
- CPU/cache access during `draw16bitRGBBitmap()`;
- Arduino_GFX RGB framebuffer/update path;
- concurrent RGB scanout / memory-bandwidth pressure.

The exact mechanism is not yet proven.

## Next isolation

Keep Test 23 unchanged and restore only:

```text
RGB bounce buffer: 0 -> 20 display lines
```

If flicker disappears, the result will strongly support a bandwidth/staging interaction between PSRAM-backed draw data and RGB scanout. If flicker remains, the next experiment should stage each LVGL partial flush through an INTERNAL SRAM buffer before passing it to Arduino_GFX.
