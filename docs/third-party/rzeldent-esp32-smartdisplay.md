# Third-party audit: rzeldent/esp32-smartdisplay

Status: `REFERENCE AUDITED / LVGL DRIVER ARCHITECTURE CANDIDATE / NO DIRECT PORT`

Repository:

```text
https://github.com/rzeldent/esp32-smartdisplay
```

Related board-definition repository:

```text
https://github.com/rzeldent/platformio-espressif32-sunton
```

## Scope

This audit studies how the upstream project consumes the Sunton board-definition macros and turns them into an LVGL display/touch/backlight pipeline.

The goal is not to copy upstream code. The goal is to extract architecture for an independent ESP32-8048S043 BSP direction.

## License boundary

There is an upstream license inconsistency:

```text
GitHub repository metadata / root LICENSE : GPL-3.0 text
library.json                             : license field says MIT
```

Until this is resolved, treat the upstream project as reference-only:

```text
No direct code copy.
No direct file copy.
Use concepts only after independent reimplementation.
```

## Upstream role

The library is a PlatformIO Arduino library for Sunton/CYD smart display boards. It depends on LVGL and board definitions from `platformio-espressif32-sunton`.

Important upstream claims from README/library metadata:

```text
LVGL v9-oriented display/touch driver library;
uses PlatformIO board definitions;
uses Espressif esp_lcd_panel interfaces;
initialization API is smartdisplay_init();
application loop still calls lv_tick_inc() and lv_timer_handler();
SquareLine Studio generated UI is an intended application path.
```

## Repository structure

High-level layout:

```text
include/                    public headers and panel/touch headers
src/                        display, touch, DMA, panel and helper code
boards/                     submodule to platformio-espressif32-sunton
library.json                PlatformIO library metadata
platformio.ini              development/test environments
README.md                   usage guide
```

This is a library-level project, not a one-off example. That is the main architectural value.

## Public API shape

The public header exposes a very small API surface:

```text
smartdisplay_init();
smartdisplay_lcd_set_backlight(float duty);   // duty in [0, 1]
smartdisplay_lcd_set_brightness_cb(...);
smartdisplay_compute_touch_calibration(...);
optional RGB LED and CdS helpers depending on board macros.
```

This confirms the intended application model:

```text
Application code
  -> calls smartdisplay_init()
  -> creates LVGL UI
  -> calls lv_tick_inc() / lv_timer_handler()
  -> controls backlight through one high-level API
```

The application is not expected to know RGB pins, GT911 registers or panel timing constants.

## Initialization pipeline

`smartdisplay_init()` is the central board bring-up function.

Its sequence is broadly:

```text
optional RGB LED setup;
optional CdS setup;
optional speaker setup;
LVGL log registration;
lv_init();
backlight GPIO/PWM setup;
display = lvgl_lcd_init();
clear active LVGL screen;
set backlight to 50%;
if BOARD_HAS_TOUCH:
  indev = lvgl_touch_init();
  attach indev to display;
  wrap low-level touch read callback with calibration transform;
  enable LVGL input device.
```

Key idea for our BSP:

```text
board-level init owns display/touch/backlight initialization;
application-level examples should be thin;
touch calibration is a wrapper layer above the raw touch driver.
```

## ST7262 parallel RGB panel path

The relevant display file for our family is:

```text
src/lvgl_panel_st7262_par.c
```

The upstream `DISPLAY_ST7262_PAR` path does the following:

```text
lv_display_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
heap_caps_malloc(px_size * LVGL_BUFFER_PIXELS, LVGL_BUFFER_MALLOC_FLAGS);
lv_display_set_buffers(..., LV_DISPLAY_RENDER_MODE_PARTIAL);
construct esp_lcd_rgb_panel_config_t from board macros;
esp_lcd_new_rgb_panel(...);
esp_lcd_panel_reset(...);
esp_lcd_panel_init(...);
smartdisplay_dma_init_with_logging(panel_handle, "ST7262 Parallel", false);
display->user_data = panel_handle;
display->flush_cb = direct_io_lv_flush;
```

Important detail:

```text
The board definition chooses LVGL_BUFFER_PIXELS.
For esp32-8048S043C the board definition sets this to DISPLAY_WIDTH * DISPLAY_HEIGHT.
Therefore the render mode is technically partial, but the buffer can hold a full 800x480 frame.
```

This is different from our current Arduino_GFX/LVGL examples, where we used smaller line buffers.

## Flush path

The ST7262 flush callback does not directly call `draw16bitRGBBitmap()` style Arduino_GFX transfer.

Instead:

```text
direct_io_lv_flush(...)
  -> panel_handle = display->user_data
  -> smartdisplay_dma_flush_with_rotation(display, area, px_map, panel_handle, "ST7262 Parallel")
```

The flush path is therefore:

```text
LVGL dirty area
  -> esp_lcd RGB panel handle
  -> smartdisplay DMA helper
  -> esp_lcd_panel_draw_bitmap()
  -> lv_display_flush_ready() after completion/fallback
```

This is the core architectural difference from our current experimental path.

## DMA helper strategy

The helper layer is not just a thin wrapper. It contains explicit policies:

```text
small transfers below threshold use direct transfer;
large transfers try DMA first;
DMA failure falls back to direct transfer;
direct draw_bitmap is retried before giving up;
queue-full fallback uses direct transfer;
worker task handles queued transfers;
chunking is used when data is larger than DMA buffer;
for async panels, completion can wait for panel transfer callback;
for RGB parallel panels, async_color_trans is false because draw_bitmap blocks.
```

Important for our observed problem:

```text
The upstream path explicitly thinks about when LVGL may reuse a framebuffer and when flush_ready is safe.
Our current Arduino_GFX path simply pushes dirty rectangles and calls lv_disp_flush_ready() immediately after draw16bitRGBBitmap().
```

This does not prove the upstream path eliminates all visible artifacts, but it is a much more deliberate transfer/completion model.

## Touch / GT911 path

The GT911 LVGL integration is in:

```text
src/lvgl_touch_gt911_i2c.c
src/esp_touch_gt911.c
include/esp_touch_gt911.h
```

The LVGL side is straightforward:

```text
create I2C master bus;
create esp_lcd_panel_io_i2c handle;
create esp_lcd_touch_config_t from board macros;
esp_lcd_touch_new_i2c_gt911(...);
create LVGL pointer indev;
indev->user_data = touch_handle;
indev->read_cb = gt911_lvgl_touch_cb;
```

The read callback does:

```text
esp_lcd_touch_read_data(touch_handle);
esp_lcd_touch_get_coordinates(...);
if pressed:
  data->point.x = x[0];
  data->point.y = y[0];
  data->state = LV_INDEV_STATE_PRESSED;
else:
  data->state = LV_INDEV_STATE_RELEASED;
```

## Very important GT911 scaling finding

The upstream GT911 driver reads the GT911 info block:

```text
product id;
firmware id;
controller x/y resolution;
vendor id.
```

It stores the controller resolution and, if the GT911-reported resolution does not match the configured display bounds, enables a coordinate-adjustment callback:

```text
x[i] = (x[i] * th->config.x_max) / gt911_resolution.x;
y[i] = (y[i] * th->config.y_max) / gt911_resolution.y;
```

This is directly relevant to our board because our GT911 reported raw `480x272` while the display is `800x480`.

Upstream solves that mismatch inside the touch driver layer, not in the application sketch.

This is a strong candidate for our own independent BSP cleanup:

```text
GT911 driver owns raw-resolution discovery;
GT911 driver maps raw coordinates to display coordinates;
LVGL/app receives already-normalized 800x480 points.
```

## Backlight handling

Backlight is handled as a simple normalized duty API:

```text
smartdisplay_lcd_set_backlight(float duty)  // 0.0 .. 1.0
```

Internally it uses LEDC and `DISPLAY_BCKL` from the board definition.

This is cleaner than exposing GPIO2/PWM details to every example.

## Rotation / calibration

The library has two relevant layers:

```text
display rotation handling through LVGL display callbacks or software rotation path;
touch calibration transform wrapper around the low-level read callback.
```

Touch calibration is affine, based on three screen points and three touch points.

For our current stage this is less urgent than the raw-resolution scaling, but it confirms that touch corrections should be centralized, not scattered across examples.

## What this explains about our local problem

Our local LVGL examples proved the functional chain:

```text
Arduino_GFX RGB panel;
small/medium PSRAM LVGL line buffers;
GT911 BSP read path;
LVGL button/slider events;
manual dashboard updates.
```

But visible dynamics were poor.

The upstream design suggests three concrete differences to investigate later:

```text
1. esp_lcd RGB panel path instead of Arduino_GFX draw16bitRGBBitmap path;
2. full-frame-capable PSRAM LVGL buffer even when render mode is partial;
3. explicit DMA/direct/fallback/flush_ready policy.
```

It also suggests one immediate BSP design correction:

```text
GT911 raw-resolution scaling belongs in the touch driver, not in the app example.
```

## Reusable ideas for our project

Use as independent architecture targets:

```text
1. Board-centered configuration layer.
2. One high-level board init function.
3. Separate display, touch and backlight drivers behind BSP API.
4. GT911 driver discovers raw resolution and scales internally.
5. Backlight is normalized 0.0..1.0 or 0..100 API, not raw PWM everywhere.
6. Dynamic LVGL path should study esp_lcd RGB panel + PSRAM framebuffer + DMA helper instead of patching Arduino_GFX examples endlessly.
7. Application examples should contain LVGL UI logic only.
8. SquareLine/UI-generated folders should be treated as app layer, not hardware layer.
```

## Do not port directly

Do not directly port now because:

```text
library targets LVGL 9.2.2 while our Arduino examples are LVGL 8.3.11;
implementation is PlatformIO-first;
license metadata is inconsistent between root LICENSE and library.json;
we have a physically tested Arduino_GFX pin path that differs in RGB channel naming from the upstream ST7262 board definition;
we need to decide whether the next experimental branch should be Arduino-IDE esp_lcd, PlatformIO, or dual-track.
```

## Recommended next technical experiment

After finishing the planned third-party audits, the first code experiment should not be another dashboard.

Better target:

```text
12_DisplayEspLcdRgbPanel_Probe
```

Purpose:

```text
bring up the ESP32-8048S043 display through esp_lcd_new_rgb_panel();
use our known-good pin map first;
then test upstream timing: PCLK 12.5 MHz, porches 8/4/8;
print refresh-rate calculation;
fill static color bars;
do not involve LVGL or touch yet.
```

Only after that:

```text
13_LVGL_EspLcdStatic
14_GT911_NormalizedTouch
15_LVGL_EspLcdBasicUI
```

This separates display transport validation from LVGL and from touch.

## Next audit step

Proceed to:

```text
limpens/esp32-8048S043
```

Target questions:

```text
Does the ESP-IDF demo use esp_lcd RGB directly?
How does it configure RGB pins and timing?
Does it use full framebuffer or partial LVGL buffer?
How is GT911 mapped?
Is the dynamic behavior likely better than our Arduino_GFX path?
```
