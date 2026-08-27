# 12_DisplayEspLcdRgbPanel_Probe

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN / BULK-DRAW RETEST`.

Firmware ID:

```text
12ELCD-BULK1-240827B
```

Previous first-run firmware:

```text
12ELCD-PROBE1-240827A
```

## Current finding

The first physical run showed an important boundary:

```text
esp_lcd RGB panel initialization worked;
colors were excellent;
static/quadrant/grid output was usable;
the original moving-block probe flickered and temporarily broke geometry.
```

Operator observation:

```text
the blue block with cyan outline moved, but not smoothly;
there were flickers and temporary geometry violations;
sometimes the upper outline visually collapsed with the lower outline;
left/right outline collapse was not observed.
```

Interpretation:

```text
Do not treat this as a color-order or pin-map failure.
The first dynamic probe used row-by-row draw_bitmap calls for rectangles.
That is a deliberately harsh and probably unrealistic update path for an RGB panel.
```

Revision `12ELCD-BULK1-240827B` changes the update method:

```text
full-screen patterns are built in one PSRAM frame buffer and sent with one draw_bitmap call;
moving band is built in one full-width PSRAM band buffer and sent with one draw_bitmap call per step;
a second moving probe sends one full frame per step;
no row-by-row rectangle drawing remains in the visible tests.
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
Update mode                 : bulk full-frame / bulk band draw_bitmap calls
```

The ESP-IDF RGB565 bus-bit order is:

```text
DATA0..DATA4    = B0..B4
DATA5..DATA10   = G0..G5
DATA11..DATA15  = R0..R4
```

This is intentionally different from the Arduino_GFX constructor order used in `02_DisplayRGBTest`.

## Expected Serial output

```text
ESP32-8048S043 Lab / 12_DisplayEspLcdRgbPanel_Probe
Native esp_lcd RGB panel transport probe
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
[READY] Watch screen: compare moving band vs moving full-frame behavior.
```

## Expected visual output

The screen should cycle through:

```text
RGB color bars;
orientation quadrants;
stripe/grid pattern;
moving block as one band draw per step;
moving block as one full-frame draw per step;
solid red/green/blue/white screens.
```

Acceptance for this retest:

```text
static images remain correct;
colors remain correct;
moving band is better than the previous row-by-row version;
full-frame moving block is compared separately;
Serial ALIVE lines continue;
no reset/brownout/panic.
```

## What to report now

For the `12ELCD-BULK1-240827B` run, record:

```text
Does it compile?
Does Serial show 12ELCD-BULK1-240827B?
Do both PSRAM buffers allocate?
Do colors remain excellent?
Is the moving band better than before?
Is the full-frame moving block better or worse than the moving band?
Does either mode still show horizontal tearing/flicker?
Any reset, brownout, panic, Guru Meditation, or PSRAM allocation failure?
```

## Next timing test

Only after the default 12.5 MHz bulk-draw run is understood, test the second upstream timing candidate by changing:

```cpp
#define ESP_LCD_PROBE_PCLK_HZ 12500000
```

to:

```cpp
#define ESP_LCD_PROBE_PCLK_HZ 18000000
```

Do not move to 18 MHz until the 12.5 MHz bulk-draw behavior is recorded.

## PASS boundary

A PASS here means only:

```text
Native esp_lcd RGB panel transport works on Sample A with the selected timing, data order and update granularity.
```

It does not prove:

```text
LVGL integration;
GT911 touch;
user-facing UI behavior;
Arduino_GFX equivalence;
Widget Runtime;
OTA;
Web upload/control.
```

## Failure interpretation

If compile fails:

```text
The Arduino-ESP32 core may expose a slightly different esp_lcd API than expected.
Patch the sketch before doing hardware interpretation.
```

If panel init fails:

```text
Check custom board profile, PSRAM, Arduino-ESP32 version and esp_lcd availability.
```

If the display initializes but colors are swapped:

```text
Do not change the global BSP pin names yet.
Only compare the esp_lcd data order in this probe.
```

If moving band is good but full-frame moving block is worse:

```text
Prefer limited-area bulk updates for LVGL8 esp_lcd experiments.
```

If both moving modes still show horizontal tearing:

```text
The next experiment must add VSYNC/panel-event synchronization or use a more formal LVGL/esp_lcd buffering strategy.
```

If 12.5 MHz is stable but 18 MHz is unstable:

```text
Keep 12.5 MHz as the conservative candidate for the next LVGL esp_lcd path.
```
