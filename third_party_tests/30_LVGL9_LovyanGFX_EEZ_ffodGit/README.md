# Test 30 — ffodGit / LVGL9 + LovyanGFX + EEZ Studio

## Status

**BUILD PASS / DISPLAY + TOUCH DEMO PASS / CLOSED / KNOWN-GOOD REFERENCE**

Build PASS was obtained on 2026-09-03 using the pinned ffodGit application source and a historical PlatformIO Espressif32 6.8.1 environment reconstruction.

Physical board verdict from the user on 2026-09-03:

> Прошивка работает отлично.

A later physical observation was:

> На экране Screen2 не работают ползунки ни вертикальный ни горизонтальный.

The user additionally clarified that an interactive control on an earlier screen responds normally to touch. This is important: the observation is **local to the implementation of Screen02**, not a global GT911/LovyanGFX/LVGL touch failure.

Source inspection explains the difference. The two controls on Screen02 are `lv_bar` objects, not `lv_slider` objects. They are passive ADC indicators driven from GPIO13 by application code, whereas the earlier PWM control is an actual interactive LVGL control with an event handler and responds to touch.

Test 30 remains frozen as a known-good third-party display/touch reference architecture, with a documented Screen02 UI implementation limitation.

## Upstream

Repository:

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

## Architecture

```text
LVGL 9.1.1-dev
    -> PARTIAL draw buffer (~1/10 screen)
    -> LovyanGFX writePixels()
    -> LovyanGFX RGB bus / Panel_RGB
    -> 800x480 RGB panel
    -> GT911 touch through LovyanGFX
```

The UI is generated with EEZ Studio and contains multiple screens and interactive widgets.

This gives a third independent RGB display transport to compare with the known-good Arduino_GFX and native esp_lcd paths.

## Display configuration

From `include/lovyanGfxSetup.h`:

```text
Resolution             800 x 480
PCLK                   14 MHz
HSYNC polarity         0
HSYNC front/pulse/back 8 / 4 / 8
VSYNC polarity         0
VSYNC front/pulse/back 8 / 4 / 8
pclk_active_neg        1
de_idle_high           0
pclk_idle_high         0
Backlight              GPIO2 PWM
```

RGB pins:

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

LovyanGFX maps the configured raw touch range to the 800x480 panel coordinates.

## LVGL configuration

Upstream vendored LVGL reports:

```text
LVGL_VERSION_MAJOR 9
LVGL_VERSION_MINOR 1
LVGL_VERSION_PATCH 1
LVGL_VERSION_INFO  "dev"
```

Display setup:

```cpp
lv_display_t *disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
lv_display_set_flush_cb(disp, my_disp_flush);
lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
```

Draw buffer:

```text
DRAW_BUF_SIZE = 800 * 480 / 10 * 2 bytes
              = 76,800 bytes
```

Flush path:

```cpp
tft.startWrite();
tft.setAddrWindow(area->x1, area->y1, w, h);
tft.writePixels((lgfx::rgb565_t *)px_map, w * h);
tft.endWrite();
lv_disp_flush_ready(disp);
```

## Screen02 clarification — local UI implementation issue

The previous interactive PWM control is created as an LVGL `arc` and has an event callback. The application reads its value and applies it to PWM:

```cpp
lv_obj_t *obj = lv_arc_create(parent_obj);
lv_obj_add_event_cb(obj, event_handler_cb_screen01_screen01_arc_pwm, LV_EVENT_ALL, 0);
```

and later:

```cpp
int32_t val = lv_arc_get_value(objects.screen01_arc_pwm);
ledcWrite(PWM_CHANNEL, val);
```

This provides a working touch-interaction reference inside the same firmware.

By contrast, Screen02 creates the two visually slider-like objects as bars:

```cpp
// Screen02BarHorizontal
lv_obj_t *obj = lv_bar_create(parent_obj);
lv_bar_set_range(obj, 0, 4095);

// Screen02BarVertical
lv_obj_t *obj = lv_bar_create(parent_obj);
```

There is no `lv_slider_create()` for either Screen02 object and no drag/value-change handler attached to them.

The application defines:

```cpp
#define ANALOG_INPUT_PIN 13
```

and while Screen02 is active it reads GPIO13:

```cpp
int potValCur = analogRead(ANALOG_INPUT_PIN);
```

Only when the ADC value changes by more than 5 counts does the application update the two bars:

```cpp
int32_t mappedVal = map(potValCur, 0, 4095, 0, 100);

lv_bar_set_value(objects.screen02_bar_horizontal, potValCur, LV_ANIM_OFF);
lv_bar_set_value(objects.screen02_bar_vertical, mappedVal, LV_ANIM_OFF);
```

Therefore the correct classification is:

```text
Global touch subsystem       PASS
Earlier interactive control  PASS
Screen02 horizontal control  passive ADC bar, not touch slider
Screen02 vertical control    passive ADC bar, not touch slider
Root cause                    specific Screen02 UI/application implementation
```

If no potentiometer or varying analog signal is connected to GPIO13, the two Screen02 bars can remain static. Their non-response to dragging must not be used as evidence of a GT911, LovyanGFX or LVGL input failure.

## PlatformIO environment

Upstream `platformio.ini` originally contains an unpinned platform declaration:

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

On 2026-09-02 the unpinned platform resolved to a modern pioarduino 2026.8.50 environment that failed before application compilation. The Test 30 harness therefore applies a temporary build-environment overlay only; application, display, touch and UI source remain exact upstream.

Successful reconstruction:

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

The PlatformIO platform is pinned to the historical 6.8.1 commit. The framework resolver supplied a later package revision within the required `~3.20017.0` line, so this is recorded as a historical environment reconstruction rather than a byte-for-byte archival recreation.

## Final verdict

```text
2026-09-03
BUILD:                         PASS
PHYSICAL BOARD:                PASS
Display output:                PASS / overall clean verdict
Visible display instability:   NOT REPORTED
Touch/navigation:               PASS
Earlier interactive control:   PASS
Screen02 horizontal "slider":  local UI implementation — actually passive ADC bar
Screen02 vertical "slider":    local UI implementation — actually passive ADC bar
Reset/crash:                    NOT REPORTED
Tracked upstream restored:      PASS
LovyanGFX resolved:             1.1.16
Test state:                     CLOSED / KNOWN-GOOD REFERENCE
```

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

The old flicker/jump failures cannot be attributed generically to LVGL 9, Arduino framework use, partial rendering, the RGB panel, GT911 activity, or PSRAM availability.

The accumulated evidence continues to point to the exact display-memory/scanout transport topology as the decisive variable. In the isolated Arduino_GFX partial-render path, PSRAM LVGL draw buffers combined with driver RGB bounce disabled produced visible redraw flicker, while enabling any tested non-zero driver bounce depth restored stability. LovyanGFX now provides an independent stable transport for comparison.

Do not modify Test 30. Keep it frozen as a reference.

Next useful work is to inspect LovyanGFX `Bus_RGB` / `Panel_RGB` internals for framebuffer placement, DMA descriptors, cache handling and internal staging, then continue with another architecturally different third-party implementation such as `duck4i/esp32_8048S043-ST7262_GT911`.
