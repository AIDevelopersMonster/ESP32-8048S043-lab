# Third-party audit: Albert-Benavent-Cabrera/Robot-Core-Display

Status: `PHYSICAL FUNCTIONAL PASS / SLOW VISIBLE REDRAW / NO JITTER OBSERVED / ONLINE ESP-NOW PATH NOT YET TESTED / GPL-3.0 REFERENCE`.

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

This differs from several other references that use 14 MHz and 8/4/8.

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

The ESP-NOW startup path also disables WiFi power saving with `esp_wifi_set_ps(WIFI_PS_NONE)` and comments that this is intended to prevent display flicker.

## Important upstream anti-flicker claim

Upstream README describes its display design as anti-flicker stabilization and says the bounce-buffer/SRAM architecture eliminates flickering caused by PSRAM concurrency.

Before the physical run this was treated only as an upstream claim. The Sample A run now provides partial supporting evidence: the display is visibly slower to redraw, but the operator did not observe the previous jitter/chatter behavior.

This does **not** isolate bounce buffering as the sole cause because LVGL generation, buffer placement, RGB timings and application workload also differ from Test 17.

## External reproduction protocol

The project was built and flashed from a private/external folder outside ESP32-8048S043-lab.

The display path was intentionally preserved:

```text
RGB timings unchanged;
PCLK unchanged;
bounce-buffer size unchanged;
LVGL draw-buffer design unchanged;
GT911 path unchanged;
UI/application logic unchanged.
```

Only compatibility/configuration changes required to build the current upstream against the floated 2026 platform were made.

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

The physical result therefore does not validate operation against a real target WiFi network/Robot-Core receiver.

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

Build classification:

```text
HOST ENVIRONMENT      PASS after repair
DEPENDENCY RESOLUTION PASS after intermittent network/TLS issue
SOURCE COMPILE        PASS with narrow ESP-NOW API shim
LINK                   PASS
FIRMWARE IMAGE         PASS
DISPLAY/RUNTIME        PHYSICAL FUNCTIONAL PASS
ONLINE ESP-NOW         NOT YET TESTED
```

## Physical observation — Sample A

Operator assessment after flashing the built firmware:

```text
works well overall;
UI is functional;
redrawing is visibly slow;
slow redraw is distinguishable from jitter/chatter;
no display chatter/jitter was observed during this run;
visual behavior is substantially cleaner than the problematic redraw seen in previous current-stack partial-render tests;
overall usability assessed as excellent despite the slow redraw.
```

The important qualitative distinction is:

```text
previous problematic behavior: unstable-looking flicker/jitter during redraw
this project:                visibly slow redraw, but stable redraw
```

No claim is made that redraw is fast or animation quality is production-perfect. The significant result is the absence of the previously observed jitter/chatter failure mode in this physical run.

## Online behavior not yet reproduced

The display repository is the physical-panel/UI side of the wider Robot-Core ecosystem. Upstream describes three primary display screens:

```text
Drink Selection
Recipe Configuration
Pump Configuration
```

When integrated with the matching Robot-Core drinks-machine side, the intended flow is:

```text
display scans for TARGET_WIFI_SSID
-> learns the WiFi channel
-> switches ESP-NOW radio to that channel
-> initializes broadcast peer
-> requests pump/recipe synchronization
-> receives pump settings and recipe data
-> updates local DataManager/UI
-> sends drink orders, pump calibration and recipe updates back by ESP-NOW
```

The upstream README states that offline operation uses mock cocktail data. A real online integration would therefore be a materially different and useful stress test: it would exercise WiFi scanning and ESP-NOW traffic while the 800x480 RGB panel continues to redraw.

That is especially relevant to the anti-flicker claim because the project deliberately puts LVGL buffers in internal SRAM, uses an RGB bounce buffer, and disables WiFi power saving in the ESP-NOW startup path.

## Interpretation

This is now one of the strongest third-party references in the project.

The result changes the working picture from:

```text
partial redraw on modern stack => necessarily bad
```

to:

```text
partial redraw on modern stack can be physically stable,
provided the surrounding RGB/buffer architecture is different.
```

Comparison boundary:

```text
Test 17 current stack
LVGL 8 + Arduino_GFX partial
-> functional, visible redraw flicker/black-like transition

Robot-Core-Display current reproduction
LVGL 9 + Arduino_GFX partial + double SRAM LVGL buffers + RGB bounce buffer
-> functional, visibly slow redraw, no jitter observed
```

This does not yet prove which variable is decisive, but it sharply raises the priority of:

```text
internal SRAM draw buffers;
RGB bounce buffer;
WiFi power-save policy;
LVGL 9 display path;
RGB timing/presentation synchronization.
```

The next controlled in-repo experiment should independently reproduce these mechanisms in a minimal UI rather than copy the GPL application.

## Follow-up modernization rule

The useful mechanisms should be reproduced independently in ESP32-8048S043-lab, likely beginning with:

```text
LVGL 9 minimal one-button UI;
double internal-SRAM draw buffers;
RGB bounce buffer;
partial redraw;
current project board profile;
our GT911 BSP;
controlled A/B against Test 17.
```

After that, a second run with real WiFi/ESP-NOW activity would test whether the stabilization remains effective under radio/bus contention.

## License boundary

The upstream repository declares GPL v3. Treat the external project as a physical/reference implementation. Any mechanism adopted into ESP32-8048S043-lab should be independently implemented with license compatibility reviewed separately.
