# Third-party audit: Albert-Benavent-Cabrera/Robot-Core-Display

Status: `AUDITED FIRST-PASS / EXTERNAL PHYSICAL TEST PENDING / GPL-3.0 REFERENCE`.

Upstream:

```text
https://github.com/Albert-Benavent-Cabrera/Robot-Core-Display
```

## Why this case is next

This project is directly relevant to the visual defect seen on ESP32-8048S043-lab because it explicitly targets flicker on an ESP32-S3 800x480 RGB panel and implements a different stabilization strategy rather than merely changing the LVGL UI.

Current upstream architecture observed during first-pass audit:

```text
LVGL 9.1.x
    |
    v
double PARTIAL LVGL buffers
    |
    v
internal SRAM preferred
    |
    v
Arduino_GFX partial-area flush
    |
    v
Arduino_ESP32RGBPanel with bounce buffer
    |
    v
ESP32-S3 RGB 800x480

GT911 -> LVGL pointer
```

The repository README calls this approach `Bus Isolation` and attributes its anti-flicker behavior to keeping LVGL draw buffers in internal SRAM and using an RGB bounce buffer.

## Current upstream build environment

`platformio.ini` currently defines:

```text
environment     : ESP32S3-8048S043
framework       : Arduino
board           : esp32-s3-devkitc-1
platform        : Jason2866/platform-espressif32.git
CPU             : 240 MHz
Flash           : 80 MHz
Flash size      : 16 MB
memory type     : qio_opi
monitor         : 115200
LVGL            : ^9.1.0
TAMC_GT911      : ^1.0.2
```

Arduino_GFX is present in the project tree rather than declared as a normal `lib_deps` dependency.

## RGB configuration observed in current source

Pin map agrees with Sample A:

```text
DE     40
VSYNC  41
HSYNC  39
PCLK   42
R      45,48,47,21,14
G      5,6,7,15,16,4
B      8,3,46,9,1
BL     2
GT911  SDA19 SCL20 INT18 RST38
```

Current source configuration:

```text
H front/pulse/back : 8 / 4 / 20
V front/pulse/back : 8 / 4 / 8
PCLK               : 16 MHz
PCLK invert         : 1
bounce buffer       : SCREEN_WIDTH * 20
LVGL draw lines     : 100
LVGL buffers        : double
LVGL render mode    : PARTIAL
```

This differs from several other references that use 14 MHz and 8/4/8. The first physical run must keep the current upstream values unchanged.

## Display path

`DisplayManager.hpp` constructs `Arduino_ESP32RGBPanel` with a nonzero bounce-buffer size and then registers LVGL 9 with two draw buffers.

The code prefers:

```text
MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT
```

for both LVGL buffers and falls back to PSRAM only if the SRAM allocations fail.

Flush remains partial-area:

```text
LVGL invalidated area
-> Arduino_GFX draw16bitRGBBitmap(area)
-> lv_disp_flush_ready()
```

Therefore this experiment is particularly useful against Test 17:

```text
Test 17:
LVGL 8 + current Arduino_GFX + partial redraw
-> visible redraw flicker

Robot-Core-Display:
LVGL 9 + Arduino_GFX + partial redraw + double SRAM buffers + RGB bounce buffer
-> physical result pending
```

## Important claim to test physically

Upstream README describes its display design as anti-flicker stabilization and says the bounce-buffer/SRAM architecture eliminates flickering caused by PSRAM concurrency.

That claim is treated as an upstream claim until reproduced on Sample A.

## External reproduction protocol

The project must first be built and flashed essentially unchanged in a private/external folder outside ESP32-8048S043-lab.

Do not initially:

```text
change RGB timing;
change PCLK;
change bounce-buffer size;
change LVGL draw-buffer size;
replace GT911 code;
replace board definition;
port it to our BSP;
remove application logic merely to simplify the test.
```

Record:

```text
build result;
boot log;
PSRAM/SRAM messages;
GFX initialization;
LVGL initialization;
GT911 behavior;
static visual stability;
page transitions;
animations/sliders;
visible black transitions;
rapid taps;
WiFi/ESP-NOW activity if present;
any flicker or jitter;
operator video and subjective assessment.
```

## Physical observation

Pending.

## Serial evidence

Pending.

## Interpretation

Pending physical run.

## Follow-up modernization rule

Only after the untouched upstream physical result is known should mechanisms be independently reproduced in ESP32-8048S043-lab.

Likely isolated mechanisms, if the physical result is useful:

```text
LVGL 9 minimal single-button UI;
double internal-SRAM draw buffers;
RGB bounce buffer;
partial redraw;
current project board profile;
our GT911 BSP;
controlled A/B against Test 17.
```

The goal is to reproduce mechanisms, not copy the third-party application.

## License boundary

The upstream repository declares GPL v3. Treat the external project as a physical/reference implementation. Any mechanism adopted into ESP32-8048S043-lab should be independently implemented with license compatibility reviewed separately.
