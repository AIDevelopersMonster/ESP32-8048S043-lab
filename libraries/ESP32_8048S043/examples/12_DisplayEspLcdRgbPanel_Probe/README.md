# 12_DisplayEspLcdRgbPanel_Probe

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION OPEN`.

Firmware ID:

```text
12ELCD-PROBE1-240827A
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

## Expected Serial output

```text
ESP32-8048S043 Lab / 12_DisplayEspLcdRgbPanel_Probe
Native esp_lcd RGB panel transport probe
Firmware ID              : 12ELCD-PROBE1-240827A
Mode                     : esp_lcd RGB panel only
Arduino_GFX              : not used
LVGL                     : not used
GT911 touch              : not used
PCLK                     : 12500000 Hz
Data order               : ESP-IDF RGB565 bus bits DATA0..15 = B0..B4,G0..G5,R0..R4
[PASS] line buffer allocated
[PASS] esp_lcd_new_rgb_panel()
[PASS] esp_lcd_panel_reset()
[PASS] esp_lcd_panel_init()
[PASS] Backlight ON after panel init
[READY] Watch screen: correct colors, stable image, no random tearing/noise.
```

## Expected visual output

The screen should cycle through:

```text
RGB color bars;
orientation quadrants;
stripe/grid pattern;
small moving block probe;
solid red/green/blue/white screens.
```

Acceptance for first pass:

```text
screen initializes;
no random noise after backlight-on;
color bars show expected red/green/blue/white/yellow/cyan/magenta;
orientation quadrants are stable;
stripe/grid pattern is stable;
small moving block does not produce large jumps/tears;
Serial ALIVE lines continue;
no reset/brownout/panic.
```

## What to report

For the first physical run, record:

```text
Does it compile?
Does it upload?
Does Serial show 12ELCD-PROBE1-240827A?
Does esp_lcd_new_rgb_panel() pass?
Does the screen light only after panel init?
Are red/green/blue correct or swapped?
Is the image stable at 12.5 MHz?
Does the moving block look acceptable?
Any reset, brownout, panic, Guru Meditation, or PSRAM allocation failure?
```

## Next timing test

Only after the default 12.5 MHz run is understood, test the second upstream timing candidate by changing:

```cpp
#define ESP_LCD_PROBE_PCLK_HZ 12500000
```

to:

```cpp
#define ESP_LCD_PROBE_PCLK_HZ 18000000
```

Do not start with 18 MHz. The third-party reference says serious distortion appears above 18 MHz, so 18 MHz is the upper candidate, not the safe baseline.

## PASS boundary

A PASS here means only:

```text
Native esp_lcd RGB panel transport works on Sample A with the selected timing and data order.
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

If 12.5 MHz is stable but 18 MHz is unstable:

```text
Keep 12.5 MHz as the conservative candidate for the next LVGL esp_lcd path.
```
