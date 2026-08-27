# ESP32-8048S043 / Sample A / Arduino 12_DisplayEspLcdRgbPanel_Probe

Status: `STATIC TRANSPORT PASS CANDIDATE / RAW DYNAMIC DRAW NOT ACCEPTABLE`.

Date: 2026-08-27

Specimen:

```text
Sample A
ESP32-8048S043 / ESP32-S3 800x480 RGB panel / GT911 family
```

Example:

```text
libraries/ESP32_8048S043/examples/12_DisplayEspLcdRgbPanel_Probe
```

Firmware IDs tested:

```text
12ELCD-PROBE1-240827A
12ELCD-BULK1-240827B
```

## Purpose

This test was created after the local Arduino_GFX/LVGL path reached a dynamic UX boundary.

The goal was to isolate the native ESP-IDF `esp_lcd` RGB panel transport before involving LVGL or GT911 again.

## Configuration under test

```text
Display transport       : esp_lcd_new_rgb_panel()
Arduino_GFX             : not used
LVGL                    : not used
GT911                   : not used
Resolution              : 800x480
PCLK                    : 12.5 MHz
Porches                 : HSYNC 8/4/8, VSYNC 8/4/8
Framebuffer in PSRAM    : true
Double framebuffer      : true
RGB565 data order       : DATA0..4 B, DATA5..10 G, DATA11..15 R
Backlight               : GPIO2, enabled after panel init
```

## Physical observations

First firmware `12ELCD-PROBE1-240827A`:

```text
Overall test passed;
colors were excellent;
quadrants/static output were good;
the moving blue block with cyan outline was not smooth;
there were flickers and temporary geometry violations;
at moments the upper outline visually collapsed with the lower outline;
left/right outline collapse was not observed.
```

Second firmware `12ELCD-BULK1-240827B`:

```text
Static/color behavior remained good;
block motion became worse rather than better;
previously mostly the block jerked;
after bulk/full-frame update changes, surrounding walls/frame also started to jerk and visibly update.
```

## Interpretation

The test strongly separates three facts:

```text
1. esp_lcd panel initialization works on Sample A.
2. ESP-IDF RGB565 data order is correct for visible colors.
3. Naive raw draw_bitmap dynamic updates are not acceptable as a user-facing animation path.
```

This is not a pin-map failure and not a color-order failure.

The artifact is associated with update timing/granularity/synchronization while the RGB panel is scanning out.

## Decision

Do not keep polishing raw moving-block `draw_bitmap()` probes as a product UI path.

Freeze this example as a diagnostic transport probe with this boundary:

```text
STATIC TRANSPORT PASS CANDIDATE
RAW DYNAMIC DRAW NOT ACCEPTABLE
```

The next useful step is not another raw block animation.

The next step should be one of:

```text
13_LVGL_EspLcdStatic
  - LVGL 8.3.11;
  - esp_lcd RGB panel;
  - no touch;
  - static UI plus controlled low-rate label updates;
  - no moving block.

or

12B_DisplayEspLcdVsyncProbe
  - esp_lcd RGB panel;
  - VSYNC/panel event callback investigation;
  - only if Arduino-ESP32 exposes the required event API cleanly.
```

## PASS boundary

A positive result for this file means:

```text
Native esp_lcd RGB panel transport can initialize and render correct static output on Sample A at 12.5 MHz.
```

It does not mean:

```text
raw dynamic drawing is acceptable;
LVGL user interface is solved;
GT911 touch is solved;
future widget/dashboard examples may update the full screen freely.
```
