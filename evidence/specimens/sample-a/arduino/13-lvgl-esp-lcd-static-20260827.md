# ESP32-8048S043 / Sample A / Arduino 13_LVGL_EspLcdStatic

Status: `PHYSICAL STATIC PASS CANDIDATE / TOUCH NOT TESTED`.

Date: 2026-08-27

Specimen:

```text
Sample A
ESP32-8048S043 / ESP32-S3 / 800x480 RGB panel / GT911 family
```

Example:

```text
libraries/ESP32_8048S043/examples/13_LVGL_EspLcdStatic
```

Firmware ID accepted:

```text
13LVGL-ELCDS2-240827B
```

Previous build attempt:

```text
13LVGL-ELCDS1-240827A
```

## Compile boundary

The first `13LVGL-ELCDS1-240827A` attempt failed because the local Arduino LVGL configuration exposed only the default/lower Montserrat font set. The missing optional symbols were:

```text
lv_font_montserrat_18
lv_font_montserrat_20
lv_font_montserrat_22
```

The sketch was patched to use `LV_FONT_DEFAULT` only.

## Physical observation

Operator report after the default-font patch:

```text
Все работает.
```

Within the current evidence level this means:

```text
example compiles;
firmware runs on Sample A;
LVGL static screen over native esp_lcd path is visually usable;
no blocking failure was reported for the static screen path.
```

## Configuration under test

```text
Display transport       : esp_lcd_new_rgb_panel()
Arduino_GFX             : not used
LVGL                    : LVGL 8.x Arduino library
GT911                   : not used
Moving animation         : not used
Resolution              : 800x480
PCLK                    : 12.5 MHz
Porches                 : HSYNC 8/4/8, VSYNC 8/4/8
Panel framebuffer       : PSRAM
Panel double framebuffer: enabled
LVGL draw buffers       : 2 x 100 lines in PSRAM
Font mode               : LV_FONT_DEFAULT only
Backlight               : enabled after first LVGL draw
```

## Interpretation

This result is materially better than the raw dynamic `draw_bitmap()` moving-block probe:

```text
12_DisplayEspLcdRgbPanel_Probe proved static esp_lcd transport and exposed raw dynamic redraw limitations.
13_LVGL_EspLcdStatic shows that LVGL 8 over native esp_lcd can run as a static UI path after the font fix.
```

## Decision

Proceed to the next isolated subsystem step:

```text
14_GT911_NormalizedTouch
```

The next test should validate the BSP GT911 normalization layer without LVGL widgets and without using raw moving display updates.

## PASS boundary

A positive result here means only:

```text
LVGL 8 static UI over native esp_lcd works on Sample A at 12.5 MHz after the default-font patch.
```

It does not prove:

```text
GT911 normalized touch;
interactive LVGL widgets;
fast animation;
polished user-facing HMI;
LVGL 9 migration;
Widget Runtime;
Web setup or OTA.
```
