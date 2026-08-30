# Third-party audit: Albert-Benavent-Cabrera/Robot-Core-Display

Status: `CURRENT-UPSTREAM BUILDS WITH NARROW IDF5.5 ESP-NOW COMPAT SHIM / EXTERNAL PHYSICAL TEST PENDING / GPL-3.0 REFERENCE`.

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

The platform URL is not pinned to a commit or release. A clone/build performed on 2026-08-30 resolved that URL to the then-current Jason2866/pioarduino platform generation:

```text
espressif32     2026.08.50+sha.4f64b35
Arduino/IDF     current IDF 5.5.x generation
```

This is newer than the January 2026 application commits and creates a reproducibility boundary: `git clone` of the application does not reconstruct a fixed toolchain.

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

## Reproduction build notes — 2026-08-30

External working clone:

```text
C:\Users\CHUWI\Documents\GitHub\third-party\Robot-Core-Display
```

The host PlatformIO environment had first been left in an inconsistent state after interrupted setup/power/network events. It was repaired before judging the upstream application. Confirmed working host state:

```text
PlatformIO Core  6.1.19
pioarduino        6.1.19
Python            3.14.6
pip               26.2.1
uv                 0.12.7
```

There was also an intermittent PyPI TLS `UNEXPECTED_EOF_WHILE_READING` during dependency retrieval. That was treated as host/network evidence, not an application failure.

### Required local secrets file

The repository excludes `display/secrets.h` and ships:

```text
display/secrets_example.h
```

The external reproduction therefore created the expected local configuration by copying the template without adding a real SSID:

```text
TARGET_WIFI_SSID = YOUR_SSID_HERE
```

This is a configuration prerequisite, not a display-code modification.

### Current-upstream ESP-NOW compatibility boundary

The current floated ESP-IDF 5.5.x changed the send callback from the legacy form:

```text
onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
```

to:

```text
onDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
```

where the destination address is available through `tx_info->des_addr`.

A narrow external-only compatibility shim was applied to `display/src/core/ESPNowManager.hpp`. It conditionally uses the IDF 5.5 callback and preserves the legacy callback for older stacks. No RGB timing, LVGL, buffer, touch or UI code was changed.

### Build result

After the compatibility shim, the current upstream application builds successfully:

```text
[SUCCESS] Took 142.75 seconds
RAM:   11.7%  (38444 / 327680 bytes)
Flash: 30.4%  (1989539 / 6553600 bytes)
firmware.bin created successfully
esptool 5.3.0
```

Observed linker/tooling warnings were non-fatal:

```text
LTO wrapper used serial compilation for 12 LTRANS jobs
GNU-stack note warning from libc_tinystdio_fwrite.c.o
PlatformIO could not show firmware metrics because Windows terminal codepage was not UTF-8/cp65001
```

The latter warning does not invalidate the explicit RAM/Flash size output or the generated firmware image.

Build classification:

```text
HOST ENVIRONMENT      PASS after repair
DEPENDENCY RESOLUTION PASS after intermittent network/TLS issue
SOURCE COMPILE        PASS with narrow ESP-NOW API shim
LINK                   PASS
FIRMWARE IMAGE         PASS
DISPLAY/RUNTIME        NOT YET TESTED
```

A January 2026 upstream commit explicitly describes restoring `BOUNCE_BUFFER_SIZE` as a flicker fix, so preserving the display code untouched remains important.

## Physical observation

Pending.

## Serial evidence

Pending.

## Interpretation

The current repository is not a fully pinned reproducible build: its unpinned platform URL allows framework/API drift. As of 2026-08-30 the application itself can be built on the current stack after a narrow ESP-NOW callback compatibility shim outside the display path.

The successful build removes the compile barrier but does not yet validate the upstream anti-flicker claim. The next evidence threshold is physical execution on Sample A with the original RGB/LVGL/bounce-buffer path unchanged.

## Follow-up modernization rule

Only after the upstream physical result is known should mechanisms be independently reproduced in ESP32-8048S043-lab.

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
