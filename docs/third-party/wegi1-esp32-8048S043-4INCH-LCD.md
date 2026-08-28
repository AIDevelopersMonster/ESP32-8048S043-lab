# Third-party audit: wegi1/ESP32-8048S043-4INCH-LCD

Status: `AUDITED FIRST-PASS / EXTERNAL LVGL WIDGETS TEST CANDIDATE / REFERENCE ONLY`.

Upstream:

```text
https://github.com/wegi1/ESP32-8048S043-4INCH-LCD
```

Repository metadata observed through GitHub:

```text
Owner          : wegi1
Repository     : ESP32-8048S043-4INCH-LCD
Default branch : main
Visibility     : public
Archived       : false
```

No root `LICENSE` file was found during this audit. Treat source as reference-only unless a clear license is established separately.

## Why this repository matters

Unlike the previous compact application repositories, this one is closer to a board-support bundle. It contains:

```text
1-Demo/
2-Specification/
3-Structure_Diagram/
4-Driver_IC_Data_Sheet/
5-Schematic/
6-User_Manual/
7-Character&Picture_Molding_Tool/
8-Burn operation/
PICTURES/
ESP32-S3.pdf
ESP-IDF manuals
```

The Arduino demo collection includes dedicated examples for:

```text
Hello world
UART
TFT HelloWorld
TFT clock
LVGL benchmark
PDQ graphics test
LVGL Widgets
Wi-Fi AP/STA/SmartConfig/TCP
and other board functions
```

For the current LVGL investigation, the two most relevant examples are:

```text
1-Demo/Demo_Arduino/3_3-3-TFT-LVGL-Benchmark/
1-Demo/Demo_Arduino/3_3-4_TFT-LVGL-Widgets/
```

## Display architecture

Both inspected LVGL examples use Arduino_GFX rather than native `esp_lcd` calls in the sketch.

Display objects:

```text
Arduino_ESP32RGBPanel
Arduino_RPi_DPI_RGBPanel
```

Observed RGB pin map:

```text
DE     40
VSYNC  41
HSYNC  39
PCLK   42
R0..R4 45, 48, 47, 21, 14
G0..G5 5, 6, 7, 15, 16, 4
B0..B4 8, 3, 46, 9, 1
Backlight GPIO2
```

This agrees with the pin map already physically validated in ESP32-8048S043-lab.

Panel timing in both LVGL examples:

```text
resolution          : 800 x 480
hsync polarity      : 0
hsync front porch   : 8
hsync pulse width   : 4
hsync back porch    : 8
vsync polarity      : 0
vsync front porch   : 8
vsync pulse width   : 4
vsync back porch    : 8
pclk active neg     : 1
prefer speed        : 14 MHz
auto_flush          : true
```

This independently repeats the 14 MHz + 8/4/8 timing combination previously observed in pixelwave/Sunton-ESP32-8048S043.

## LVGL generation

The supplied replacement `lv_conf.h` identifies itself as:

```text
LVGL v8.3.0-dev
```

Important configuration values include:

```text
LV_COLOR_DEPTH            16
LV_COLOR_16_SWAP          0
LV_MEM_SIZE               48 KB
LV_DISP_DEF_REFR_PERIOD   15 ms
LV_INDEV_DEF_READ_PERIOD  30 ms
LV_TICK_CUSTOM            1
LV_TICK_CUSTOM_SYS_TIME_EXPR millis()
```

This makes the example directly relevant to our LVGL 8.x branch rather than only as a future LVGL 9 reference.

## LVGL buffer strategy

Both benchmark and widgets examples allocate:

```text
screenWidth * screenHeight / 4
```

pixels for the LVGL draw buffer.

At 800x480 RGB565:

```text
96000 pixels
about 192000 bytes
```

Allocation request on ESP32:

```cpp
heap_caps_malloc(..., MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
```

So this is a one-quarter-screen internal-RAM partial rendering strategy, not the full-frame PSRAM strategy used by the working clumsyCoder00 EEZ experiment.

## Flush mechanism

The widgets and benchmark examples use partial LVGL invalidation areas:

```cpp
gfx->draw16bitRGBBitmap(
    area->x1,
    area->y1,
    ...,
    w,
    h
);

lv_disp_flush_ready(disp);
```

For this parallel RGB panel they explicitly keep:

```text
LV_COLOR_16_SWAP = 0
```

This architecture is useful because it gives a third comparison point:

```text
our tests 15/16     : native esp_lcd partial update
wegi1 Widgets       : Arduino_GFX partial update
clumsyCoder00       : Arduino_GFX full-frame/direct update
```

A physical run of the unmodified wegi1 Widgets demo can therefore help distinguish whether the observed intermittent jitter follows:

```text
partial update in general;
native esp_lcd specifically;
or some other timing/input/display interaction.
```

This is an experiment, not a proposed fix.

## GT911 touch configuration

The LVGL Widgets example includes `touch.h` with GT911 enabled.

Observed settings:

```text
TOUCH_GT911_SCL       20
TOUCH_GT911_SDA       19
TOUCH_GT911_INT       -1
TOUCH_GT911_RST       38
TOUCH_GT911_ROTATION  ROTATION_NORMAL
TOUCH_MAP_X1          480
TOUCH_MAP_X2          0
TOUCH_MAP_Y1          272
TOUCH_MAP_Y2          0
```

This is especially useful because it matches the same polling-style choice we considered for other third-party projects:

```text
INT = -1
```

and again confirms the observed GT911 raw coordinate domain near:

```text
480 x 272
```

The implementation calls `ts.read()` on every GT911 touch poll and maps the first reported point into the display dimensions.

Its release model is simple:

```text
touch_released() returns true for GT911
```

Do not copy this as our BSP input state model; use it only to reproduce the upstream application behavior.

## LVGL Widgets demo

The example initializes in this order:

```text
Serial
Arduino_GFX display
backlight
RED/GREEN/BLUE/BLACK startup screens
lv_init()
touch_init()
LVGL draw buffer
LVGL display driver
LVGL GT911 pointer driver
lv_demo_widgets()
```

Main loop:

```cpp
lv_timer_handler();
delay(5);
```

The UI is the standard LVGL widgets demo rather than a minimal custom button. That makes it a useful real dynamic UI stress test without us modifying the upstream firmware.

## LVGL Benchmark demo

The benchmark uses the same:

```text
RGB pin map
14 MHz timing
8/4/8 porches
Arduino_GFX partial flush
quarter-screen internal-RAM LVGL buffer
5 ms handler loop
```

but does not configure real touch input. It calls:

```cpp
lv_demo_benchmark();
```

This may be useful later for display-load comparison, but the Widgets demo is the higher-priority physical test because it combines rendering and GT911 input.

## Hardware documentation value

The repository also contains a board specification PDF and schematic material, including:

```text
2-Specification/ESP32-8048S043 Specifications-EN.pdf
5-Schematic/ESP32-8048S043-1.png
5-Schematic/ESP32-4827S043-MCU-V1.0.jpg
5-Schematic/ESP32-S3-WROOM-1 Pin definition.png
```

These are useful as hardware cross-check references but are not yet treated as proof that every file corresponds exactly to Sample A PCB revision.

## License boundary

A root `LICENSE` file was not found during the audit.

Therefore:

```text
Do not copy source files into ESP32-8048S043-lab.
Do not vendor the demo code.
Use the repository as an external physical comparison and documentation reference.
Independently reimplement any mechanism that proves useful.
```

## Recommended physical experiment

Do not modify our tests 15 or 16 for this step.

Run the upstream `LvglWidgets` example as an external firmware comparison.

The key question is only:

```text
Does Arduino_GFX partial-area LVGL rendering on this upstream stack show the same intermittent jitter behavior?
```

Record only broad observations:

```text
boot success;
UI renders;
GT911 works;
idle stability;
intermittent jitter present/absent;
whether jitter can appear on fast taps without appearing on every fast tap.
```

Do not tune timing, buffers or touch logic during the first run.

## Current comparative matrix

```text
Path                         LVGL        Rendering                    Touch
--------------------------------------------------------------------------------
15 local                     8.3.11      esp_lcd partial              BSP GT911
16 local                     8.3.11      esp_lcd minimal partial      BSP GT911
clumsyCoder00 external       9.1.0       Arduino_GFX full-frame       TAMC GT911
wegi1 Widgets external       8.3.x       Arduino_GFX partial           TAMC GT911
```

The wegi1 experiment is therefore the cleanest next external comparison without trying to fix or optimize anything.

## Next target after physical comparison

The remaining original audit-list target is:

```text
ffodGit/esp32-8048s043-getting-started-00
```
