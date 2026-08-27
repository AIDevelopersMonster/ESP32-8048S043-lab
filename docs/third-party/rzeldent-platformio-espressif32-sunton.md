# Third-party audit: rzeldent/platformio-espressif32-sunton

Status: `REFERENCE AUDITED / ARCHITECTURE CANDIDATE / NO CODE COPY`

Repository:

```text
https://github.com/rzeldent/platformio-espressif32-sunton
```

Upstream role:

```text
PlatformIO board definitions for Sunton Smart Display / CYD boards.
The repository is used as a set of board JSON files and is designed to work together with rzeldent/esp32-smartdisplay.
```

License boundary:

```text
Upstream license: GPL-3.0.
Use as reference and for independent reimplementation only.
Do not copy code, board files or generated definitions into this repository without an explicit license decision.
```

## Why this project matters

This is the first high-priority external reference after our local LVGL experiments reached the dynamic UX boundary.

The useful part is not a single sketch. The useful part is the board-definition model:

```text
board JSON
  -> CPU / flash / PSRAM profile
  -> display controller and RGB timings
  -> RGB GPIO map
  -> touch controller type and GPIO map
  -> SD / TF SPI map
  -> macros consumed by a reusable display/LVGL library
```

The upstream README describes these JSON files as PlatformIO board definitions that contain hardware-specific defines and are used by the `esp32-smartdisplay` LVGL drivers.

## Relevant ESP32-8048S043 variants

The repository contains at least three `8048S043` variants:

```text
esp32-8048S043C.json  capacitive touch / GT911
esp32-8048S043N.json  no touch
esp32-8048S043R.json  resistive touch / XPT2046
```

For our specimen, the closest upstream variant is:

```text
esp32-8048S043C.json
```

because our board uses GT911 capacitive touch on I2C.

## esp32-8048S043C profile summary

Upstream profile:

```text
MCU                  : esp32s3
Frameworks           : arduino, espidf
CPU                  : 240 MHz
Flash                : 16 MB
Flash mode/frequency : QIO / 80 MHz
Arduino memory type  : qio_opi
Partition file       : default_16MB.csv
PSRAM macro          : BOARD_HAS_PSRAM
USB mode             : ARDUINO_USB_MODE=1
USB CDC on boot      : 0
Upload speed         : 460800
```

This largely agrees with our local Arduino board profile, except our current upload speed is usually 921600 when stable.

## Display strategy

Upstream uses a display definition based on:

```text
DISPLAY_WIDTH                         800
DISPLAY_HEIGHT                        480
DISPLAY_BCKL                          2
DISPLAY_ST7262_PAR
LVGL_BUFFER_PIXELS                    DISPLAY_WIDTH * DISPLAY_HEIGHT
LVGL_BUFFER_MALLOC_FLAGS              MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
SMARTDISPLAY_DMA_BUFFER_SIZE          131072
SMARTDISPLAY_DMA_QUEUE_SIZE           6
SMARTDISPLAY_DMA_CHUNK_THRESHOLD      1024
SMARTDISPLAY_DMA_TIMEOUT_MS           500
ST7262_PANEL_CONFIG_CLK_SRC           LCD_CLK_SRC_PLL160M
ST7262_PANEL_CONFIG_TIMINGS_PCLK_HZ   12.5 MHz
ST7262_PANEL_CONFIG_DATA_WIDTH        16
ST7262_PANEL_CONFIG_FLAGS_FB_IN_PSRAM true
```

Important observation:

```text
The upstream design is full-framebuffer-oriented and PSRAM-oriented.
It also uses explicit DMA buffering parameters.
This is a likely reason to study it before attempting more dynamic LVGL UI on our board.
```

This differs from our current Arduino_GFX examples, which use partial LVGL draw buffers and direct `draw16bitRGBBitmap()` flushes.

## RGB timing and GPIO notes

Upstream timing for `esp32-8048S043C`:

```text
PCLK              12.5 MHz
HSYNC pulse       4
HSYNC back porch  8
HSYNC front porch 8
VSYNC pulse       4
VSYNC back porch  8
VSYNC front porch 8
PCLK active neg   true
DE idle high      false
```

Our local examples currently use a higher PCLK in some LVGL sketches. The upstream 12.5 MHz clock is a concrete candidate for future timing experiments if display tearing/jump remains an issue.

GPIO overlap with our source-backed map:

```text
DE       40
HSYNC    39
VSYNC    41
PCLK     42
Backlight 2
G0..G5   5, 6, 7, 15, 16, 4
```

Important discrepancy:

```text
Upstream ST7262 data R0..R4 are 8, 3, 46, 9, 1.
Upstream ST7262 data B0..B4 are 45, 48, 47, 21, 14.
Our current Arduino_GFX pin header names 45,48,47,21,14 as RGB_R0..R4 and 8,3,46,9,1 as RGB_B0..B4.
```

Do not blindly replace our map. Our local display examples are already physically tested. This difference may be a naming/order convention difference between the upstream ST7262/esp_lcd path and our Arduino_GFX path, or a real RGB/BGR interpretation difference.

## GT911 touch notes

Upstream `esp32-8048S043C` touch profile:

```text
Touch controller          : GT911
I2C host                  : I2C_NUM_0
SDA / SCL                 : 19 / 20
I2C speed                 : 400000
I2C address               : 0x5D
RST / INT                 : 38 / 18
Touch max                 : DISPLAY_WIDTH x DISPLAY_HEIGHT
Touch swap/mirror         : false / false / false
```

This agrees with our electrical pin path and address, but differs from our current raw GT911 observation where the controller reports `480x272` resolution and we map it into the 800x480 display space. Upstream likely lets the driver report/scalе coordinates directly to display bounds.

## SD / TF notes

Upstream SD/TF pins:

```text
TF_CS     10
TF_MOSI   11
TF_SCLK   12
TF_MISO   13
```

This agrees with our read-only SD validation path.

## Variant notes

`esp32-8048S043N`:

```text
same display family;
no touch macros;
useful for separating display-only behavior from touch behavior.
```

`esp32-8048S043R`:

```text
resistive XPT2046 touch;
SPI2_HOST;
MOSI/MISO/SCLK 11/13/12;
CS 38;
INT 18;
not our specimen, but useful as a warning that the 8048S043 family has multiple touch variants.
```

## Reusable ideas for our project

Use these ideas as architecture/reference, not code copy:

```text
1. Treat the board as a named hardware profile, not as ad-hoc sketch constants.
2. Keep full display/touch/SD definitions in one board description layer.
3. Separate capacitive/no-touch/resistive variants.
4. Prefer PSRAM-backed full-framebuffer or direct framebuffer strategies for dynamic LVGL work.
5. Study explicit DMA buffer/queue parameters before trying more animations.
6. Re-check PCLK 12.5 MHz against our current Arduino_GFX 16 MHz examples.
7. Compare esp_lcd/ST7262 path with Arduino_GFX path.
8. Keep application examples small and BSP-centered.
```

## Do not use directly yet

Do not immediately port these definitions into the Arduino BSP because:

```text
license is GPL-3.0;
upstream RGB channel naming differs from our current tested Arduino_GFX pin naming;
upstream assumes a specific esp_lcd/ST7262 driver path;
our current library targets Arduino IDE local hardware profile first;
we need to inspect rzeldent/esp32-smartdisplay next to understand how these macros are consumed.
```

## Next audit step

Proceed to:

```text
rzeldent/esp32-smartdisplay
```

Target questions for the next audit:

```text
How are ST7262_PAR macros consumed?
How is the full framebuffer registered with LVGL?
How is DMA queueing implemented?
How is GT911 integrated and scaled?
Does the library avoid visible redraw artifacts on RGB panels?
Can the architecture be independently reimplemented for our MIT-style BSP?
```
