# Third-party audit: limpens/esp32-8048S043-lvgl9

Status: `REFERENCE AUDITED / ESP-IDF LVGL9 COMPARISON / NO DIRECT CODE COPY`

Repository:

```text
https://github.com/limpens/esp32-8048S043-lvgl9
```

Upstream target:

```text
Sunton ESP32-S3 800x480 capacitive touch display
ESP32-8048S043C family
```

## Scope

This is the fourth project in the LVGL third-party audit sequence.

It is best read as the LVGL 9 successor/variant of the same author's LVGL 8.3.11 ESP-IDF demo:

```text
limpens/esp32-8048S043        -> ESP-IDF + LVGL 8.3.11
limpens/esp32-8048S043-lvgl9  -> ESP-IDF + LVGL 9.x
```

The value for our repository is comparative rather than immediate-port:

```text
what changes between LVGL 8 and LVGL 9;
whether the display/touch transport stays the same;
whether the esp_lcd RGB panel path remains the preferred transport;
whether GT911 normalization remains below LVGL;
whether LVGL 9 simplifies or complicates the future BSP path.
```

## License boundary

No root `LICENSE` file was visible during this audit.

One demo UI file carries an Espressif `CC0-1.0` SPDX header, but that does not license the whole repository.

Therefore:

```text
Treat this project as reference-only.
Do not copy code directly.
Reimplement useful ideas independently.
```

## Project layout

Root layout:

```text
CMakeLists.txt
README.md
sdkconfig.defaults
sdkconfig.defaults.esp32s3
main/
```

Main component layout:

```text
hardware.h
lvgl9.c
lv_conf.h
lvgl_demo_ui.c
esp_logo.c
esp_text.c
idf_component.yml
CMakeLists.txt
```

Compared with the LVGL 8 project, the hardware and LVGL driver code is consolidated into `lvgl9.c` plus `hardware.h` rather than `lcd.cpp` / `lcd.h` / `main.cpp`.

## Dependencies and build target

README states this is a basic example using ESP-IDF 5.1 with `esp_lcd_touch_gt911` and LVGL 9.x from the component registry.

`idf_component.yml` declares:

```text
espressif/esp_lcd_touch_gt911 >= 1.1.0
idf >= 5.1
lvgl/lvgl ^9.4.0
```

`sdkconfig.defaults` fixes the target as ESP32-S3:

```text
CONFIG_IDF_TARGET="esp32s3"
CONFIG_IDF_TARGET_ESP32S3=y
```

`sdkconfig.defaults.esp32s3` enables the same key PSRAM path as the LVGL 8 project:

```text
16 MB flash;
SPIRAM enabled;
OPI/OCT PSRAM mode;
SPIRAM 80 MHz;
fetch instructions from SPIRAM;
RODATA in SPIRAM;
LVGL config is not skipped;
LVGL examples are not built.
```

## LVGL 9 configuration notes

`lv_conf.h` identifies itself as configuration for LVGL 9.0.0 while the component dependency currently allows LVGL `^9.4.0`.

Important settings visible in the inspected top section:

```text
LV_COLOR_DEPTH 16
LV_DEF_REFR_PERIOD 33 ms
LV_USE_OS LV_OS_NONE
LV_USE_DRAW_SW 1
LV_DRAW_SW_DRAW_UNIT_CNT 1
LV_DRAW_SW_COMPLEX 1
LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
LV_MEM_SIZE 64 KB
```

The README also states that the LVGL performance monitor is enabled and shown in the bottom-right corner.

## Hardware pin map

`hardware.h` preserves the same board model as the LVGL 8 project:

```text
LCD_H_RES 800
LCD_V_RES 480
Backlight GPIO2
HSYNC GPIO39
VSYNC GPIO41
DE GPIO40
PCLK GPIO42
```

RGB data bit order:

```text
DATA0..DATA4    GPIO8, GPIO3, GPIO46, GPIO9, GPIO1       // B3..B7
DATA5..DATA10   GPIO5, GPIO6, GPIO7, GPIO15, GPIO16, GPIO4 // G2..G7
DATA11..DATA15  GPIO45, GPIO48, GPIO47, GPIO21, GPIO14   // R3..R7
```

Touch pins:

```text
GT911 RESET GPIO38
GT911 SCL   GPIO20
GT911 SDA   GPIO19
GT911 INT   GPIO18
I2C speed   400000
```

This reinforces the same esp_lcd bit-order interpretation seen in `rzeldent` and in the LVGL 8 `limpens` project.

## Pixel clock and timing

`hardware.h` keeps the same pixel-clock note:

```text
LCD_PIXEL_CLOCK_HZ = 18 MHz
comment: serious distortion going above 18 MHz
```

`lvgl9.c` keeps the same porch/timing set:

```text
pclk_hz             18 MHz
h_res / v_res        800 / 480
hsync pulse          4
hsync back porch     8
hsync front porch    8
vsync pulse          4
vsync back porch     8
vsync front porch    8
hsync_idle_low       false
vsync_idle_low       false
de_idle_high         false
pclk_active_neg      true
pclk_idle_high       false
```

This makes the timing conclusion stronger:

```text
8/4/8 porches appear consistently across multiple references;
18 MHz is a plausible upper working PCLK candidate;
12.5 MHz remains the safer candidate from rzeldent.
```

## ESP-IDF RGB panel strategy

The LVGL 9 project keeps the same transport class:

```text
esp_lcd_new_rgb_panel();
esp_lcd_panel_reset();
esp_lcd_panel_init();
esp_lcd_panel_draw_bitmap();
```

The RGB panel config again uses:

```text
data_width        16
num_fbs           2
psram_trans_align 64
fb_in_psram       true
double_fb         true
```

So LVGL 9 does not change the important lower-level conclusion:

```text
For this board family, serious dynamic UI work should go through esp_lcd RGB panel with PSRAM-backed panel framebuffers, not through continued Arduino_GFX patching.
```

## LVGL 9 buffer strategy

`lvgl9.c` allocates two LVGL buffers from PSRAM:

```text
buf1 = heap_caps_malloc(LCD_H_RES * LCD_V_RES / 10, MALLOC_CAP_SPIRAM)
buf2 = heap_caps_malloc(LCD_H_RES * LCD_V_RES / 10, MALLOC_CAP_SPIRAM)
lv_display_set_buffers(..., LCD_H_RES * LCD_V_RES / 10, LV_DISPLAY_RENDER_MODE_PARTIAL)
```

This is different from the LVGL 8 project, which used two 100-line buffers sized as `LCD_H_RES * 100 * sizeof(lv_color_t)`.

For our audit, the safe interpretation is:

```text
LVGL 9 path still uses partial rendering with PSRAM draw buffers;
it does not move to a full LVGL frame buffer;
panel-level double framebuffer remains the more important dynamic-display feature.
```

## LVGL 9 flush path

The flush callback is the LVGL 9 API version of the same idea:

```text
lv_display_get_user_data(display) -> esp_lcd panel handle;
esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, color_map);
lv_disp_flush_ready(display);
```

There is still no custom DMA queue layer as in `rzeldent/esp32-smartdisplay`.

The important continuity is:

```text
LVGL 8 limpens path : LVGL -> esp_lcd RGB panel -> PSRAM double framebuffer
LVGL 9 limpens path : LVGL -> esp_lcd RGB panel -> PSRAM double framebuffer
```

## GT911 touch path

The project moves to the newer ESP-IDF I2C master API:

```text
driver/i2c_master.h
i2c_new_master_bus()
esp_lcd_new_panel_io_i2c(i2c_bus, ...)
```

Touch remains through:

```text
esp_lcd_touch_new_i2c_gt911();
esp_lcd_touch_read_data();
esp_lcd_touch_get_coordinates();
LVGL pointer indev;
lv_indev_set_user_data();
lv_indev_set_read_cb();
lv_indev_set_display();
```

## Touch coordinate normalization

`hardware.h` keeps the same measured raw coordinate bounds:

```text
TOUCH_H_RES_MIN 0
TOUCH_H_RES_MAX 477
TOUCH_V_RES_MIN 0
TOUCH_V_RES_MAX 269
```

`gt911_process_coordinates()` maps them to display coordinates:

```text
x = map(x, 0..477, 0..800)
y = map(y, 0..269, 0..480)
```

This is the fourth confirmation in the audit track that our GT911 issue is not incidental:

```text
GT911 raw space is about 480x272;
display space is 800x480;
normalization belongs inside the touch/BSP layer, below LVGL widgets.
```

The LVGL 9 project currently logs normalized coordinates in this process callback. That is useful during development but should not be copied into a production path without throttling or disabling.

## LVGL tick and task model

The LVGL 9 project uses a single FreeRTOS task pinned to core 1:

```text
xTaskCreatePinnedToCore(lcd_init, "lcd_init", 8192, NULL, 1, NULL, 1)
```

Inside `lcd_init()` it:

```text
creates a periodic esp_timer every 2000 us;
tick callback calls lv_tick_inc(2);
updates a label;
delays 20 ms;
calls lv_timer_handler();
```

Compared with the LVGL 8 project, this is simpler:

```text
no explicit GUI semaphore wrapper;
all LVGL work is concentrated in the pinned task;
LVGL 9 APIs replace LVGL 8 display/indev driver structs.
```

This is a useful pattern for an isolated demo, but our future BSP should still be careful about thread ownership if web/server/tasks update LVGL objects.

## Demo behavior

README says the project uses an adapted Espressif LVGL demo UI and processed images because of LVGL changes.

The code:

```text
turns backlight off during panel setup;
initializes esp_lcd RGB panel;
initializes LVGL;
allocates PSRAM draw buffers;
registers display and touch;
starts LVGL tick timer;
turns backlight on;
creates a label;
adds screen click handler;
runs an animated demo UI;
updates label text every 20 ms;
runs lv_timer_handler().
```

This is a dynamic test, not merely a static bring-up.

## What changed from the LVGL 8 project

Important changes:

```text
LVGL 8.3.11 -> LVGL ^9.4.0;
legacy lv_disp_drv_t/lv_indev_drv_t APIs -> lv_display_t/lv_indev_t APIs;
old I2C driver -> new i2c_master API;
LVGL tick is explicit esp_timer every 2 ms rather than LV_TICK_CUSTOM in lv_conf.h;
main app and display driver are consolidated into one lvgl9.c;
LVGL draw buffer allocation is smaller/different;
GUI semaphore wrapper is removed;
performance monitor is intentionally enabled.
```

Important things that did not change:

```text
esp_lcd RGB panel remains the display transport;
PSRAM panel framebuffer remains enabled;
double framebuffer remains enabled;
18 MHz / 8/4/8 timing remains;
GT911 raw coordinate mapping remains;
backlight is still turned on only after setup;
demo still exercises animation and repeated label updates.
```

## Relevance to our project

This audit does not change our immediate base from LVGL 8 to LVGL 9.

It does, however, confirm that a future LVGL 9 branch should be straightforward only after the lower-level transport questions are settled:

```text
esp_lcd RGB probe;
RGB data bit order;
12.5 MHz vs 18 MHz timing;
PSRAM framebuffer behavior;
GT911 normalized touch BSP.
```

## Recommended path for our repository

Do not jump to LVGL 9 yet.

Use this project as a future migration reference after the LVGL 8 / esp_lcd transport path is proven.

The current experiment sequence remains:

```text
12_DisplayEspLcdRgbPanel_Probe
13_LVGL_EspLcdStatic
14_GT911_NormalizedTouch
15_LVGL_EspLcdBasicUI
```

Then later, if needed:

```text
16_LVGL9_EspLcdProbe
```

## Audit result

`limpens/esp32-8048S043-lvgl9` confirms that the author did not solve the board by changing the hardware path for LVGL 9.

The stable idea stayed the same:

```text
ESP-IDF esp_lcd RGB panel + PSRAM double framebuffer + GT911 process_coordinates normalization.
```

Therefore, the next high-value work in our repository is still transport-level validation, not UI polishing.

## Next audit step

Proceed to:

```text
pixelwave/Sunton-ESP32-8048S043
```

Target questions:

```text
How does a SquareLine/LVGL/Arduino_GFX style project organize UI assets?
Does it provide useful application structure despite likely weaker display transport?
Does it contain any practical workaround for dynamic UI artifacts?
Can its UI organization be reused independently while keeping our BSP/display path separate?
```
