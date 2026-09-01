# Test 30 — ffodGit / LVGL9 + LovyanGFX + EEZ Studio

## Status

**THIRD-PARTY CANDIDATE / PHYSICAL VERDICT PENDING**

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

Upstream `platformio.ini`:

```text
framework = arduino
board = esp32-s3-devkitm-1
BOARD_HAS_PSRAM
memory_type = qio_opi
flash = 16 MB
flash clock = 80 MHz
LovyanGFX ^1.1.16
```

Important reproducibility limitation:

```text
platform = espressif32
```

is not pinned to an exact platform version. Therefore the exact original Arduino-ESP32/ESP-IDF version is not fully reproducible from the repository alone.

## Comparison with our known-good paths

```text
Test 19/24:
LVGL9 -> Arduino_GFX PARTIAL -> esp_lcd RGB -> bounce transport

Test 20:
LVGL9.5 -> native esp_lcd PARTIAL -> PSRAM framebuffers

Test 21:
LVGL9.1 -> Arduino_GFX DIRECT/full-screen continuous copy

Test 30:
LVGL9.1.1-dev -> LovyanGFX PARTIAL -> LovyanGFX RGB bus
```

If Test 30 is physically clean, it will establish a third independent stable architecture on the same panel family.

## Physical questions

Record separately:

```text
Boot                         : PASS / FAIL
Image                        : visible / absent
Periodic flicker             : observed / not observed
Horizontal jump              : observed / not observed
Touch                        : works / fails
Touch coordinate mapping     : correct / incorrect
Screen transitions           : clean / unstable
Widget interaction           : clean / unstable
Reset / crash                : observed / not observed
```

## Next engineering step after physical test

If physically stable:

1. freeze upstream behavior;
2. inspect LovyanGFX `Bus_RGB` / `Panel_RGB` internals;
3. identify framebuffer memory and DMA staging policy;
4. compare it directly with Arduino_GFX `bounce=0` and `bounce>0` paths;
5. decide whether LovyanGFX deserves a production BSP option or is best retained as reference architecture.

If unstable:

keep the result as evidence and do not immediately modify the upstream architecture; first identify the failing transport/memory characteristic.
