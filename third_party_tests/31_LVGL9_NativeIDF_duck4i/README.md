# Test 31 — duck4i / native ESP-IDF + ST7262 + GT911 + LVGL9

## Status

**PHYSICAL PASS / CLOSED / KNOWN-GOOD THIRD-PARTY REFERENCE**

Upstream repository:

```text
duck4i/esp32_8048S043-ST7262_GT911
```

Pinned upstream commit:

```text
578966c969577309b37cf9afb698852e2e81491b
2025-04-05
```

Physical board verdict reported by the user on 2026-09-03:

> Это похожая на заводскую прошивка с логотипом уже LVGL 9.2.2 виджет демо... показывает 66 fps но фон черный. Отклик тач очень хороший.

This is a strong physical PASS for the display/touch transport under the tested upstream architecture.

The black background is consistent with the upstream configuration rather than evidence of a display failure: `sdkconfig.defaults` explicitly enables:

```text
CONFIG_LV_THEME_DEFAULT_DARK=y
```

No visible flicker, horizontal jump, touch-redraw instability, reset, or crash was reported in this physical run.

## Video evidence

Physical-board video recorded by the project user:

https://youtube.com/shorts/H8bcEiqERTA

The video is retained as visual evidence for the Test 31 physical PASS and shows the LVGL 9.2.2 Widgets demo running on the tested ESP32-8048S043 board.

## Why this candidate matters

This is an architecturally distinct path from Tests 19/20/21/30:

```text
ESP-IDF 5.3.2
  -> native esp_lcd RGB panel
  -> one framebuffer in PSRAM
  -> no explicit RGB bounce buffer
  -> LVGL 9.2.2 PARTIAL
  -> one LVGL draw buffer in INTERNAL RAM (~1/4 screen)
  -> esp_lcd_panel_draw_bitmap()
  -> GT911 custom component
```

It is especially useful after Tests 22-29 because it physically confirms a stable native esp_lcd configuration where:

- the RGB framebuffer lives in PSRAM;
- LVGL partial draw memory is explicitly INTERNAL;
- `double_fb = false`;
- `bounce_buffer_size_px` is not enabled;
- PCLK is 16 MHz;
- the LVGL Widgets demo runs at an observed approximately 66 fps;
- touch response is reported as very good.

This is close to the causal boundary identified in our Arduino_GFX experiments but implemented through a different application/driver architecture.

## Upstream dependency lock

The upstream `dependencies.lock` records:

```text
ESP-IDF                  5.3.2
LVGL                     9.2.2
esp_lvgl_port            2.5.0
Target                    esp32s3
```

The default build path in `main.c` does not enable `USE_LVGL_PORT`; it uses direct LVGL integration.

## Display configuration

The board profile in `esp_lcd_st7262.h` uses:

```text
Resolution               800 x 480
PCLK                     16 MHz
HSYNC                    front 8 / pulse 4 / back 8
VSYNC                    front 8 / pulse 4 / back 8
pclk_active_neg          1
Backlight                GPIO2
DE                       GPIO40
VSYNC                    GPIO41
HSYNC                    GPIO39
PCLK                     GPIO42
```

RGB pins:

```text
R: 45,48,47,21,14
G: 5,6,7,15,16,4
B: 8,3,46,9,1
```

## Native RGB panel transport

The upstream driver constructs `esp_lcd_rgb_panel_config_t` with:

```text
data_width           = 16
bits_per_pixel       = 16
num_fbs              = 0
fb_in_psram          = true
refresh_on_demand    = false
no_fb                = false
double_fb            = false
bounce buffer        = disabled/commented
```

The driver then calls:

```cpp
esp_lcd_new_rgb_panel(&config, &display_handle);
```

LVGL flush reaches the panel through:

```cpp
esp_lcd_panel_draw_bitmap(...)
```

## LVGL draw buffer

The default path allocates:

```cpp
size_t size = width * height * sizeof(lv_color16_t) / 4;
heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
```

For 800x480 RGB565 this is one quarter screen:

```text
800 * 480 * 2 / 4 = 192,000 bytes
```

and uses:

```text
LV_DISPLAY_RENDER_MODE_PARTIAL
RGB565
```

This placement is particularly important in the accumulated test matrix: Test 31 shows that a PSRAM RGB framebuffer with bounce disabled can still be physically stable when the LVGL partial draw buffer is INTERNAL and the transport uses the native ESP-IDF RGB path.

## Touch

GT911 pins:

```text
SCL 20
SDA 19
INT -1
RST 38
```

The upstream touch code uses its own `gt911` component and maps raw coordinates to the 800x480 LVGL screen.

Physical result:

```text
Touch response: VERY GOOD
```

## sdkconfig.defaults

The upstream project explicitly targets ESP32-S3, enables Octal PSRAM at 80 MHz, the LVGL Widgets demo, performance monitor, and dark theme:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
CONFIG_LV_USE_DEMO_WIDGETS=y
CONFIG_LV_USE_PERF_MONITOR=y
CONFIG_LV_THEME_DEFAULT_DARK=y
```

Therefore the observed black LVGL Widgets background is expected from the selected upstream theme configuration.

## Physical verdict

```text
Date                         2026-09-03
Boot                         PASS
LVGL                         9.2.2 Widgets demo visible
Appearance                   similar to factory/demo firmware
Background                   black / expected dark theme
Observed performance         ~66 fps
Touch response               very good
Visible display instability  not reported
Touch-redraw flicker         not reported
Horizontal jump              not reported
Reset/crash                  not reported
Physical verdict             PASS
Test state                   CLOSED / KNOWN-GOOD REFERENCE
```

## Comparison with established references

```text
Test 19/24
LVGL9 -> Arduino_GFX PARTIAL -> RGB -> bounce transport
PASS

Test 20
LVGL9.5 -> native esp_lcd PARTIAL -> PSRAM framebuffers
PASS

Test 21
LVGL9.1 -> Arduino_GFX DIRECT/full-screen continuous copy
PASS

Test 30
LVGL9.1.1-dev -> LovyanGFX PARTIAL -> LovyanGFX RGB bus
PASS

Test 31
LVGL9.2.2 -> native ESP-IDF esp_lcd PARTIAL
INTERNAL LVGL draw buffer -> one PSRAM framebuffer -> bounce0
PASS, ~66 fps, very good touch
```

## Production opportunity — keep the transport, replace the demo/UI layer

The upstream project is useful not only as a diagnostic reference but also as a strong candidate foundation for our own ESP32-8048S043 applications.

In the original duck4i source, the visible interface is compiled into the firmware and launched directly with:

```cpp
lv_demo_widgets();
```

There is no independent widget/package loader in the upstream implementation. The demo is therefore an application-layer example sitting on top of a display/touch stack that has now been physically verified on our board.

For our own projects, the proven lower layers can be preserved:

```text
ESP-IDF
  -> native esp_lcd RGB / ST7262
  -> PSRAM RGB framebuffer
  -> INTERNAL LVGL partial draw buffer
  -> GT911
  -> LVGL 9.x
```

while replacing the fixed demo/UI layer with our own application shell, for example:

```text
proven Test 31 transport
        -> application shell
        -> UI/widget runtime
        -> separately selectable or loadable screens/widgets
        -> LittleFS / SD / network-delivered UI assets when required
```

This is a **future extension opportunity**, not a feature already implemented by duck4i.

Recommended development rule:

- keep Test 31 frozen as the exact known-good upstream reference;
- create a separate derived test/branch for our own UI or widget loader;
- initially preserve the display driver, framebuffer placement, LVGL draw-buffer placement, timings and GT911 path unchanged;
- change only the application/UI layer first;
- compare every derived implementation against the Test 31 physical baseline.

In practical terms, this project can be used as the hardware/display/touch foundation of a custom product, with the built-in `lv_demo_widgets()` replaced by our own modular UI, widget loader or application runtime.

## Engineering conclusion

Test 31 is particularly strong evidence against any generic claim that an ESP32-S3 RGB framebuffer in PSRAM requires a non-zero bounce buffer for stable display output.

The physically observed stable topology is:

```text
LVGL partial draw
  INTERNAL RAM
      -> esp_lcd_panel_draw_bitmap()
      -> single RGB framebuffer in PSRAM
      -> direct native RGB scanout
      -> bounce disabled
      -> PASS
```

Combined with Tests 22-29, the narrower conclusion remains more defensible:

- bounce is not globally required;
- PSRAM framebuffer use is not globally unstable;
- LVGL 9 is not the cause of the old flicker;
- the decisive behavior depends on the exact combination of draw-buffer placement, framebuffer placement, driver path, and scanout/DMA transport topology;
- the previously isolated failure remains specific to the Arduino_GFX partial-render configuration with PSRAM LVGL draw buffers and driver bounce disabled.

Do not modify the Test 31 upstream implementation. Keep it frozen as a known-good native ESP-IDF reference; implement any modular widget/runtime extension as a separate derived test.
