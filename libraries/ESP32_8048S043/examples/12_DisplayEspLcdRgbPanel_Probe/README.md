# 12_DisplayEspLcdRgbPanel_Probe

Status: `STATIC TRANSPORT PASS CANDIDATE / RAW DYNAMIC DRAW NOT ACCEPTABLE`.

Current firmware ID:

```text
12ELCD-BULK1-240827B
```

Previous firmware ID:

```text
12ELCD-PROBE1-240827A
```

Evidence:

```text
evidence/specimens/sample-a/arduino/12-esp-lcd-rgb-panel-probe-20260827.md
```

## Current physical result

The native `esp_lcd` RGB panel path initialized and rendered correct static output on Sample A:

```text
esp_lcd RGB panel initialization worked;
colors were excellent;
quadrants/static output were good;
12.5 MHz / 8/4/8 timing was usable for static screens;
ESP-IDF RGB565 data order produced correct visible colors.
```

But raw dynamic drawing was not acceptable:

```text
12ELCD-PROBE1-240827A:
  moving blue block with cyan outline was not smooth;
  flickers and temporary geometry violations appeared;
  upper outline sometimes visually collapsed with lower outline;
  left/right collapse was not observed.

12ELCD-BULK1-240827B:
  static/colors remained good;
  block motion became worse rather than better;
  surrounding walls/frame also started to jerk and visibly update.
```

## Interpretation

This is not a pin-map failure.

This is not a color-order failure.

The useful conclusion is:

```text
Native esp_lcd transport is viable for static output.
Naive raw draw_bitmap animation is not a user-facing UI path.
Dynamic work needs LVGL/esp_lcd buffering discipline and/or synchronization, not more raw moving-block drawing.
```

## Purpose

This example is the first isolated native `esp_lcd` RGB-panel probe for the ESP32-8048S043 lab board.

It exists because the local LVGL examples proved that LVGL can run on the board, but the dynamic behavior of the current Arduino_GFX-based UI path is not acceptable for user-facing applications.

This sketch tests the display transport layer before LVGL or GT911 are involved again.

## What it uses

```text
Arduino sketch
ESP-IDF esp_lcd RGB panel API from Arduino-ESP32 core
ESP32_8048S043_Pins.h
RGB panel 800x480
Backlight GPIO2
PSRAM framebuffer path
```

## What it intentionally does not use

```text
Arduino_GFX
LVGL
GT911 touch
SD
Wi-Fi
BLE
user-facing UI widgets
```

## Default probe settings

```text
PCLK                       : 12.5 MHz
HSYNC porch / pulse / back : 8 / 4 / 8
VSYNC porch / pulse / back : 8 / 4 / 8
PCLK active negative       : true
Framebuffer in PSRAM       : true
Double framebuffer          : true
Data order                  : ESP-IDF RGB565 bus-bit order
```

The ESP-IDF RGB565 bus-bit order is:

```text
DATA0..DATA4    = B0..B4
DATA5..DATA10   = G0..G5
DATA11..DATA15  = R0..R4
```

This is intentionally different from the Arduino_GFX constructor order used in `02_DisplayRGBTest`.

## Serial markers

The current firmware prints:

```text
Firmware ID              : 12ELCD-BULK1-240827B
Mode                     : esp_lcd RGB panel only
Arduino_GFX              : not used
LVGL                     : not used
GT911 touch              : not used
PCLK                     : 12500000 Hz
Update mode              : bulk full-frame / bulk band draw_bitmap calls
Data order               : ESP-IDF RGB565 bus bits DATA0..15 = B0..B4,G0..G5,R0..R4
[PASS] full frame buffer allocated in PSRAM
[PASS] band buffer allocated in PSRAM
[PASS] esp_lcd_new_rgb_panel()
[PASS] esp_lcd_panel_reset()
[PASS] esp_lcd_panel_init()
[PASS] Backlight ON after panel init
```

## PASS boundary

A PASS here means only:

```text
Native esp_lcd RGB panel transport works on Sample A for correct static output with the selected timing and data order.
```

It does not prove:

```text
raw dynamic drawing is acceptable;
LVGL integration;
GT911 touch;
user-facing UI behavior;
Arduino_GFX equivalence;
Widget Runtime;
OTA;
Web upload/control.
```

## Decision

Do not continue polishing raw moving-block draw tests.

Recommended next step:

```text
13_LVGL_EspLcdStatic
  -> LVGL 8.3.11;
  -> esp_lcd RGB panel;
  -> no touch;
  -> static UI plus controlled low-rate label updates;
  -> no moving block.
```

Alternative later step:

```text
12B_DisplayEspLcdVsyncProbe
  -> only if Arduino-ESP32 exposes a clean RGB-panel VSYNC event callback path.
```

## Failure interpretation for future variants

If static colors break:

```text
That is a regression in pin order, timing or transport setup.
```

If only raw motion tears/flickers:

```text
That confirms a synchronization/update-granularity boundary, not a basic panel failure.
```

If 12.5 MHz is stable but 18 MHz is unstable in a later test:

```text
Keep 12.5 MHz as the conservative candidate for the next LVGL esp_lcd path.
```
