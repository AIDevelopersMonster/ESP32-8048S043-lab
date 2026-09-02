# Test 30 — ffodGit / LVGL9 + LovyanGFX + EEZ Studio

## Status

**BUILD PASS / PHYSICAL PASS / CLOSED**

Build PASS was obtained on 2026-09-03 using the pinned ffodGit application source and a historical PlatformIO Espressif32 6.8.1 environment reconstruction.

Physical board verdict from the user on 2026-09-03:

> Прошивка работает отлично.

No visible display instability, touch failure, reset or other functional defect was reported during the physical run. Test 30 is therefore frozen as a known-good third-party reference architecture.

This test studies a third independent display architecture for the ESP32-8048S043 family.

Upstream repository:

```text
ffodGit/esp32-8048s043-getting-started-00
```

Pinned upstream commit:

```text
18b6d4de509abb61feb0084c1583d41497836cfd
2024-09-22
```

License:

```text
MIT
Copyright (c) 2024 Embedded Weekends
Copyright (c) 2021 LVGL Kft
```

## Why this candidate matters

This is not another Arduino_GFX variant. It uses:

```text
LVGL 9.1.1-dev
    -> PARTIAL draw buffer (~1/10 screen)
    -> LovyanGFX writePixels()
    -> LovyanGFX RGB bus / Panel_RGB
    -> 800x480 RGB panel
    -> GT911 touch through LovyanGFX
```

The UI is generated with EEZ Studio and contains multiple screens and interactive widgets.

This makes Test 30 useful for two reasons:

1. it gives a third independent RGB display transport to compare with our known-good Arduino_GFX and native esp_lcd paths;
2. it tests a richer real-world UI architecture rather than a minimal synthetic screen.

## Display configuration

From `include/lovyanGfxSetup.h`:

```text
Resolution            800 x 480
PCLK                  14 MHz
HSYNC polarity        0
HSYNC front/pulse/back 8 / 4 / 8
VSYNC polarity        0
VSYNC front/pulse/back 8 / 4 / 8
pclk_active_neg       1
de_idle_high          0
pclk_idle_high        0
Backlight             GPIO2 PWM
```

RGB pins match the known ESP32-8048S043 mapping:

```text
B: 8,3,46,9,1
G: 5,6,7,15,16,4
R: 45,48,47,21,14
DE 40 / VSYNC 41 / HSYNC 39 / PCLK 42
```

## Touch configuration

LovyanGFX `Touch_GT911`:

```text
SDA       GPIO19
SCL       GPIO20
RST       GPIO38
INT       -1
I2C       port 1
frequency 400 kHz
address   0x5D
raw range 480 x 272
```

LovyanGFX maps this to the 800x480 panel coordinates.

## LVGL configuration

Upstream contains its own LVGL source tree reporting:

```text
LVGL_VERSION_MAJOR 9
LVGL_VERSION_MINOR 1
LVGL_VERSION_PATCH 1
LVGL_VERSION_INFO  "dev"
```

Main display setup:

```cpp
lv_display_t *disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
lv_display_set_flush_cb(disp, my_disp_flush);
lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
```

Draw-buffer size:

```text
DRAW_BUF_SIZE = 800 * 480 / 10 * 2 bytes
              = 76,800 bytes
```

The buffer is a static global array, therefore placement is controlled by the linker rather than explicit heap capability flags.

Flush path:

```cpp
tft.startWrite();
tft.setAddrWindow(area->x1, area->y1, w, h);
tft.writePixels((lgfx::rgb565_t *)px_map, w * h);
tft.endWrite();
lv_disp_flush_ready(disp);
```

## PlatformIO environment

Upstream `platformio.ini` originally contains:

```text
framework = arduino
board = esp32-s3-devkitm-1
BOARD_HAS_PSRAM
memory_type = qio_opi
flash = 16 MB
flash clock = 80 MHz
platform = espressif32
LovyanGFX ^1.1.16
```

The unpinned `platform = espressif32` is a reproducibility gap. On 2026-09-02 it resolved to a modern pioarduino 2026.8.50 environment that failed before application compilation. The Test 30 harness therefore applies a temporary build-environment overlay only; application, display, touch and UI source remain exact upstream.

Historical reconstruction used for the successful build:

```text
PlatformIO Core              6.1.19
Platform Espressif32         6.8.1+sha.3f33cce
Platform commit              3f33ccea90eb316581cdb7524d6a78c1335b9731
Arduino-ESP32 package        3.20017.241212+sha.dcc1105b
Arduino-ESP32 core line      2.0.17
ESP-IDF core line            4.4.7
LovyanGFX                    1.1.16
LVGL                         upstream vendored 9.1.1-dev
Xtensa ESP32-S3 toolchain    8.4.0+2021r2-patch5
esptoolpy                    1.40501.0
```

Note: the PlatformIO platform itself is pinned to the historical 6.8.1 commit. The framework package resolver currently supplied a later package revision within the required `~3.20017.0` line (`3.20017.241212+sha.dcc1105b`). This preserves the Arduino-ESP32 2.0.17 core line but is recorded explicitly rather than described as byte-for-byte archival reconstruction.

## Final verdict

```text
2026-09-03
BUILD:                         PASS
PHYSICAL BOARD:                PASS
Image/UI operation:            PASS
Visible display instability:   NOT REPORTED / overall clean verdict
Touch failure:                 NOT REPORTED / overall clean verdict
Reset/crash:                   NOT REPORTED / overall clean verdict
Tracked upstream restored:     PASS
LovyanGFX resolved:             1.1.16
Test state:                     CLOSED / KNOWN-GOOD
```

Physical verdict wording is intentionally conservative: the user reported that the firmware "works excellently" as an overall result, but did not separately enumerate every checklist item in this run.

## Comparison with known-good paths

```text
Test 19/24:
LVGL9 -> Arduino_GFX PARTIAL -> esp_lcd RGB -> bounce transport
PASS

Test 20:
LVGL9.5 -> native esp_lcd PARTIAL -> PSRAM framebuffers
PASS

Test 21:
LVGL9.1 -> Arduino_GFX DIRECT/full-screen continuous copy
PASS

Test 30:
LVGL9.1.1-dev -> LovyanGFX PARTIAL -> LovyanGFX RGB bus
PASS
```

Test 30 therefore establishes a third independent stable application-level RGB architecture on this board family.

## Engineering conclusion

The old flicker/jump failures cannot be attributed generically to any of the following:

- LVGL 9;
- Arduino framework use;
- partial rendering;
- rich interactive UI activity;
- the RGB panel itself;
- GT911 touch activity;
- or PSRAM availability on the board.

The accumulated evidence continues to point to the exact display-memory/scanout transport topology as the decisive variable. In the previously isolated Arduino_GFX partial-render path, PSRAM LVGL draw buffers combined with driver RGB bounce disabled produced visible redraw flicker, while enabling any tested non-zero driver bounce depth restored stability. LovyanGFX now provides an independent stable transport for comparison.

## Next engineering step

Do not modify Test 30. Keep it frozen as a reference.

Next useful work is either:

1. inspect LovyanGFX `Bus_RGB` / `Panel_RGB` internals to identify framebuffer placement, DMA descriptors, cache handling and any internal staging/bounce mechanism; or
2. run another architecturally different third-party implementation, preferably the native ESP-IDF `duck4i/esp32_8048S043-ST7262_GT911` path (one PSRAM framebuffer, LVGL partial INTERNAL draw buffer, no explicit bounce) as the next physical reference test.
