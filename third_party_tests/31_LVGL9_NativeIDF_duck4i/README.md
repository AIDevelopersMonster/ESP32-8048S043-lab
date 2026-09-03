# Test 31 — duck4i / native ESP-IDF + ST7262 + GT911 + LVGL9

## Status

**THIRD-PARTY CANDIDATE / PHYSICAL VERDICT PENDING**

Upstream repository:

```text
duck4i/esp32_8048S043-ST7262_GT911
```

Pinned upstream commit:

```text
578966c969577309b37cf9afb698852e2e81491b
2025-04-05
```

This test is intended to run the upstream project essentially unchanged and evaluate the physical result on the ESP32-8048S043 board.

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

It is especially useful after Tests 22-29 because it tests a native esp_lcd configuration where:

- the RGB framebuffer lives in PSRAM;
- LVGL partial draw memory is explicitly INTERNAL;
- `double_fb = false`;
- `bounce_buffer_size_px` is not enabled;
- PCLK is 16 MHz.

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

## Touch

GT911 pins:

```text
SCL 20
SDA 19
INT -1
RST 38
```

The upstream touch code uses its own `gt911` component and maps raw coordinates to the 800x480 LVGL screen.

## sdkconfig.defaults

The upstream project explicitly targets ESP32-S3 and enables Octal PSRAM at 80 MHz:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240=y
CONFIG_LV_USE_DEMO_WIDGETS=y
CONFIG_LV_USE_PERF_MONITOR=y
```

## Physical checklist

After successful build/flash record:

```text
Boot
Image
Periodic flicker
Touch-redraw flicker
Horizontal jump
Touch
Touch mapping
LVGL Widgets interaction
Performance monitor
Reset/crash
```

## Rules

- Do not transplant this code into our Arduino examples.
- Do not change display/touch architecture before the first physical verdict.
- Preserve the pinned upstream commit.
- Compatibility/environment fixes, if required, must be documented separately from application behavior.
- Physical board behavior is decisive.
