# Third-party audit: wegi1/ESP32-8048S043-4INCH-LCD

Status: `PHYSICAL PASS / HISTORICAL STACK REPRODUCED / REFERENCE ONLY`.

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

## Bundled historical software stack

The repository ships its own Arduino libraries rather than assuming current Library Manager versions.

Observed bundled versions:

```text
LVGL                     8.3.0-dev
GFX Library for Arduino  1.2.8
TAMC_GT911                1.0.2
```

The first attempt with current Arduino-ESP32 3.3.11 / ESP-IDF 5.x failed because Arduino_GFX 1.2.8 depends on old ESP32-S3 LCD internals, including the historical RGB callback/API and BSD LIST macros.

The physical reproduction therefore used a historical Arduino-ESP32 2.x environment compatible with the bundled 2022-era Arduino_GFX code instead of patching the upstream source.

This is important: the physical PASS below applies to the reconstructed historical stack, not to Arduino-ESP32 3.3.11.

## LVGL configuration

The supplied replacement `lv_conf.h` identifies itself as:

```text
LVGL v8.3.0-dev
```

Important configuration values include:

```text
LV_COLOR_DEPTH            16
LV_COLOR_16_SWAP          0
LV_MEM_SIZE               48 KB
LV_INDEV_DEF_READ_PERIOD  30 ms
LV_TICK_CUSTOM            1
LV_TICK_CUSTOM_SYS_TIME_EXPR millis()
LV_USE_DEMO_WIDGETS       1
```

The demo declaration was initially missing when a different LVGL configuration was active. Installing the upstream-compatible `lv_conf.h` restored `lv_demo_widgets()` without modifying the sketch.

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

This architecture gives a useful comparison point:

```text
our tests 15/16     : native esp_lcd partial update
wegi1 Widgets       : Arduino_GFX partial update
clumsyCoder00       : Arduino_GFX full-frame/direct update
```

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

This again confirms the useful raw GT911 coordinate domain near:

```text
480 x 272
```

The implementation calls `ts.read()` on every GT911 touch poll and maps the first reported point into the display dimensions.

Its release model is intentionally simple and is not copied into our BSP.

## Physical reproduction — 2026-08-29

### Operator observation

The upstream LVGL Widgets firmware was flashed to Sample A and worked well visually.

Operator assessment:

```text
works well;
visually resembles the familiar factory demo firmware;
UI is the standard/factory-like LVGL 8 widgets demonstration;
no immediate reason to modify the historical upstream build itself.
```

The visual similarity to the factory firmware is an observation, not proof that the factory image was built from this exact repository or exact source revision.

### Serial evidence

Observed boot/application log:

```text
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3808,len:0x43c
load:0x403c9700,len:0xbec
load:0x403cc700,len:0x2a3c
entry 0x403c98d8
LVGL Widgets Demo
E (...) gpio: gpio_set_level(226): GPIO output gpio_num error
E (...) gpio: gpio_set_level(226): GPIO output gpio_num error
E (...) gpio: gpio_set_level(226): GPIO output gpio_num error
Setup done
```

The repeated `gpio_num 226` diagnostics are non-fatal in this physical run: display and UI still initialize and operate correctly. They are recorded as an upstream/historical-stack anomaly and are not being patched in the reference firmware.

### Physical status

```text
BOOT             PASS
DISPLAY          PASS
LVGL WIDGETS     PASS
GT911 TOUCH      PASS by operator observation
VISUAL QUALITY   GOOD
SERIAL CLEAN     PARTIAL — non-fatal GPIO 226 diagnostics present
OVERALL          PHYSICAL PASS
```

## Interpretation

This result changes the comparison in an important way.

A stable, good-looking dynamic LVGL 8 application exists on the same board family using:

```text
historical Arduino-ESP32 2.x
LVGL 8.3.0-dev
Arduino_GFX 1.2.8
Arduino_GFX partial-area flush
quarter-screen internal LVGL buffer
GT911 polling
14 MHz RGB timing
8/4/8 porches
```

Therefore the earlier intermittent visual jitter seen in local native-`esp_lcd` tests cannot be attributed simply to:

```text
LVGL 8 itself;
GT911 itself;
partial invalidation itself;
800x480 RGB hardware itself.
```

The experiment does not yet identify the exact cause. It narrows the search toward differences in the display transport, synchronization behavior, Arduino-ESP32/ESP-IDF generation, driver implementation, buffering, or an interaction among those factors.

## Boundary of the result

This experiment does **not** prove that:

```text
Arduino_GFX partial redraw is always jitter-free;
Arduino_GFX 1.2.8 is preferable for the project;
Arduino-ESP32 2.x should become our production base;
the factory firmware is this exact upstream project;
our current board profile/core will behave the same with this UI.
```

The historical firmware is now a known-good physical reference, not a proposed production solution.

## Next experiment: port the known-good UI path forward

The useful next step is no longer to tune or repair the historical firmware.

Instead, independently reproduce the same class of test on our current stack:

```text
our ESP32-8048S043 custom board profile
current Arduino-ESP32 / ESP-IDF 5.x
current Arduino_GFX
current LVGL 8.x
our validated BSP GT911
standard LVGL Widgets demo
Arduino_GFX partial-area flush
14 MHz / 8/4/8 panel timing retained initially
```

This creates a controlled modernization step:

```text
KNOWN GOOD HISTORICAL STACK
        ->
SAME UI CLASS + SAME DISPLAY TIMING
        ->
CURRENT BOARD PROFILE + CURRENT CORE + CURRENT LIBRARIES + OUR TOUCH BSP
```

The objective is diagnostic, not corrective: determine whether the good visual behavior survives the software-generation transition.

## Current comparative matrix

```text
Path                         LVGL         Core/IDF          Rendering                    Touch              Physical
-----------------------------------------------------------------------------------------------------------------------
15 local                     8.3.11       current/5.x       esp_lcd partial              BSP GT911          jitter present
16 local                     8.3.11       current/5.x       esp_lcd minimal partial      BSP GT911          mostly stable, intermittent jitter
clumsyCoder00 external       9.1.0        historical PIO    Arduino_GFX full-frame       TAMC GT911         PASS
wegi1 Widgets external       8.3.0-dev    Arduino-ESP32 2.x Arduino_GFX partial          TAMC GT911         PASS
17 current-stack port        8.x current  current/5.x       Arduino_GFX partial          BSP GT911          pending
```

## License boundary

A root `LICENSE` file was not found during the audit.

Therefore:

```text
Do not copy source files into ESP32-8048S043-lab.
Do not vendor the demo code.
Use the repository as an external physical comparison and documentation reference.
Independently reimplement any mechanism that proves useful.
```

## Hardware documentation value

The repository also contains board specification and schematic material, including:

```text
2-Specification/ESP32-8048S043 Specifications-EN.pdf
5-Schematic/ESP32-8048S043-1.png
5-Schematic/ESP32-4827S043-MCU-V1.0.jpg
5-Schematic/ESP32-S3-WROOM-1 Pin definition.png
```

These remain hardware cross-check references and are not treated as proof that every file corresponds exactly to Sample A PCB revision.
