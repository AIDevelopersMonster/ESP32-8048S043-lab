# Third-party audit: limpens/esp32-8048S043

Status: `REFERENCE AUDITED / ESP-IDF LVGL8 DISPLAY-TOUCH PIPELINE / NO DIRECT CODE COPY`

Repository:

```text
https://github.com/limpens/esp32-8048S043
```

Upstream target:

```text
Sunton ESP32-S3 800x480 capacitive touch display
ESP32-8048S043C
```

## Scope

This is the third project in the LVGL third-party audit sequence.

Unlike the previous `rzeldent` pair, this project is not a generic library. It is a concrete ESP-IDF demo for the same 4.3 inch 800x480 GT911 board family.

The value for us is practical:

```text
ESP-IDF 5.1 path;
LVGL 8.3.11, close to our current LVGL 8.3.11 Arduino examples;
esp_lcd RGB panel driver;
esp_lcd_touch_gt911 managed component;
real GT911 raw-to-display coordinate mapping;
double framebuffer in PSRAM;
small, direct LCDInit() hardware bring-up function.
```

## License boundary

No root `LICENSE` file was visible in the repository contents inspected during this audit.

One imported Espressif demo UI source carries a `CC0-1.0` SPDX header, but that does not license the whole repository.

Therefore:

```text
Treat the repository as reference-only.
Do not copy code directly.
Reimplement useful ideas independently.
```

## Project layout

Root layout:

```text
CMakeLists.txt
README.md
sdkconfig.defaults
main/
data/
```

Main component layout:

```text
main.cpp
lcd.cpp
lcd.h
lv_conf.h
lvgl_demo_ui.c
esp_logo.c
esp_text.c
idf_component.yml
CMakeLists.txt
```

The project is an ESP-IDF component/application, not Arduino or PlatformIO.

## Dependencies and build target

The README instructs:

```text
idf.py set-target esp32s3
idf.py build flash monitor
```

The component dependencies are explicit:

```text
lvgl/lvgl                    == 8.3.11
espressif/esp_lcd_touch_gt911 == 1.1.0
idf                          >= 5.1.0
```

This is very useful for us because our current Arduino examples also use LVGL 8.3.11.

The root CMake config defines:

```text
LV_CONF_INCLUDE_SIMPLE=1
project name 8048S043
local include path for main/lv_conf.h
```

## PSRAM / LVGL config

`sdkconfig.defaults` enables the relevant external memory path:

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

`lv_conf.h` uses:

```text
LV_COLOR_DEPTH 16
LV_COLOR_16_SWAP 0
LV_TICK_CUSTOM 1
LV_TICK_CUSTOM_SYS_TIME_EXPR esp_timer_get_time()/1000
LV_DISP_DEF_REFR_PERIOD 30 ms
LV_INDEV_DEF_READ_PERIOD 30 ms
```

The custom tick source is a meaningful difference from our Arduino sketches, where we currently drive `lv_tick_inc()` manually.

## Display pin map

`lcd.h` defines the board display as:

```text
LCD_H_RES 800
LCD_V_RES 480
Backlight GPIO2
HSYNC GPIO39
VSYNC GPIO41
DE GPIO40
PCLK GPIO42
```

The 16 RGB data lines are declared as sequential LCD data bits:

```text
DATA0..DATA4   GPIO8, GPIO3, GPIO46, GPIO9, GPIO1     // B3..B7
DATA5..DATA10  GPIO5, GPIO6, GPIO7, GPIO15, GPIO16, GPIO4 // G2..G7
DATA11..DATA15 GPIO45, GPIO48, GPIO47, GPIO21, GPIO14 // R3..R7
```

This matches the `rzeldent` ST7262/esp_lcd ordering and explains the apparent R/B mismatch against our Arduino_GFX header:

```text
esp_lcd RGB data_gpio_nums[] expects bus bit order B, G, R for RGB565 wiring;
our Arduino_GFX constructor labels may be interpreted differently;
do not rename our tested Arduino_GFX pins until an esp_lcd probe confirms color order physically.
```

## Pixel clock and timing

`lcd.h` sets:

```text
LCD_PIXEL_CLOCK_HZ = 18 MHz
```

and comments that serious distortion appears above 18 MHz.

`lcd.cpp` uses the following RGB timing:

```text
clk_src             LCD_CLK_SRC_DEFAULT
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

This gives us a second concrete timing candidate:

```text
rzeldent candidate : 12.5 MHz, 8/4/8 porches
limpens candidate  : 18 MHz,   8/4/8 porches, with warning not to exceed 18 MHz
```

## ESP-IDF RGB panel strategy

`LCDInit()` configures `esp_lcd_rgb_panel_config_t` and uses:

```text
esp_lcd_new_rgb_panel();
esp_lcd_panel_reset();
esp_lcd_panel_init();
```

The panel config uses:

```text
data_width            16
num_fbs               2
psram_trans_align     64
fb_in_psram           true
double_fb             true
bounce_buffer_size_px 0
```

This is a very important finding for our dynamic-UI problem.

The upstream project uses two framebuffers in PSRAM for the RGB panel itself, while our current Arduino_GFX examples use Arduino_GFX plus LVGL draw buffers and visible dynamic artifacts.

## LVGL draw-buffer strategy

In addition to the RGB panel double framebuffer, `LCDInit()` allocates two separate LVGL draw buffers in PSRAM:

```text
buf1 = LCD_H_RES * 100 * sizeof(lv_color_t)
buf2 = LCD_H_RES * 100 * sizeof(lv_color_t)
```

For 800x480 RGB565, this is about 160 KB per LVGL draw buffer.

So the model is:

```text
ESP-IDF RGB panel      : double framebuffer in PSRAM
LVGL rendering buffers : two 100-line PSRAM draw buffers
```

This is much closer to a serious dynamic display pipeline than our first Arduino_GFX tests.

## Flush path

The LVGL flush callback is simple:

```text
esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, color_map);
lv_disp_flush_ready(drv);
```

There is no custom DMA queue layer like `rzeldent/esp32-smartdisplay`, but the key difference from our current path remains:

```text
transport is esp_lcd RGB panel, not Arduino_GFX;
panel has PSRAM double framebuffer enabled;
LVGL draw buffers are separate and in PSRAM.
```

## GT911 touch path

`lcd.h` defines:

```text
TOUCH_RESET GPIO38
TOUCH_SCL   GPIO20
TOUCH_SDA   GPIO19
TOUCH_INT   GPIO18
TOUCH_FREQ  400000
```

`lcd.cpp` uses:

```text
I2C_NUM_0;
I2C pullups enabled;
esp_lcd_new_panel_io_i2c();
esp_lcd_touch_new_i2c_gt911();
LVGL pointer indev read callback;
esp_lcd_touch_read_data();
esp_lcd_touch_get_coordinates();
```

## Touch coordinate mapping

This project uses measured calibration bounds in `lcd.h`:

```text
TOUCH_H_RES_MIN 0
TOUCH_H_RES_MAX 477
TOUCH_V_RES_MIN 0
TOUCH_V_RES_MAX 269
```

Then it registers `process_coordinates` in the GT911 touch config:

```text
x = map(x, 0..477, 0..800)
y = map(y, 0..269, 0..480)
```

This is the same physical issue we observed:

```text
GT911 raw space is about 480x272;
display space is 800x480;
normalization belongs below LVGL, not in application widgets.
```

Compared with `rzeldent`, the difference is:

```text
rzeldent reads GT911-reported resolution and scales generically;
limpens hardcodes measured raw min/max bounds and scales through a process_coordinates callback.
```

For our BSP, the best independent design is likely:

```text
read GT911 info block when possible;
fall back to measured bounds if the info block is unreliable;
centralize normalization in ESP32_8048S043_Touch.
```

## LVGL threading/task model

The project uses a FreeRTOS task for LVGL updates:

```text
lvUpdateTask:
  delay 20 ms;
  take GUI semaphore;
  lv_task_handler();
  release GUI semaphore;
```

It also exposes:

```text
lvgl_acquire();
lvgl_release();
```

The demo UI uses these wrappers around LVGL object updates from timer callbacks.

However, `main.cpp` updates a label in its main loop after a delay without visibly taking the semaphore. This is acceptable as an example observation, not something to copy blindly.

## Demo behavior

The README says the example:

```text
prints an increasing number in the bottom-right corner;
runs the demo once;
changes the text color when tapping the screen.
```

`main.cpp` confirms:

```text
LCDInit();
create label at bottom-right;
add screen click event to randomize label color;
turn on backlight after display setup;
run Espressif logo demo;
update counter roughly every 50 ms.
```

`lvgl_demo_ui.c` is based on an Espressif display demo and creates animated arcs, images and a 20 ms LVGL timer.

This is useful because it is not only a static screen: it exercises animation and repeated label updates.

## What this project teaches us

Main lessons:

```text
1. ESP-IDF esp_lcd RGB panel is the right next transport to test.
2. For this board family, esp_lcd data bit order is B bits, then G bits, then R bits.
3. 18 MHz is reported as the upper practical pixel-clock boundary in this project.
4. The 8/4/8 porch timing appears again.
5. Dynamic LVGL is tested with a 20 ms task and 100-line LVGL PSRAM buffers.
6. RGB panel double framebuffer in PSRAM is a major difference from our Arduino_GFX path.
7. GT911 raw 480x272-ish coordinates must be mapped below LVGL.
8. Backlight is intentionally turned on after panel/UI setup to avoid displaying noise.
```

## Candidate experiments for our repo

This audit strengthens the planned experiment sequence:

```text
12_DisplayEspLcdRgbPanel_Probe
  - esp_lcd_new_rgb_panel();
  - no LVGL;
  - no touch;
  - static color bars;
  - test our current Arduino_GFX channel interpretation vs esp_lcd bit-order interpretation;
  - test 12.5 MHz and 18 MHz;
  - test 8/4/8 timing;
  - report color order and stability.

13_LVGL_EspLcdStatic
  - LVGL 8.3.11;
  - esp_lcd RGB panel;
  - two 100-line PSRAM draw buffers;
  - no touch;
  - static UI plus controlled label updates.

14_GT911_NormalizedTouch
  - no LVGL widgets;
  - BSP-level raw->display normalization;
  - compare GT911 info-block resolution vs measured 0..477 / 0..269 fallback.

15_LVGL_EspLcdBasicUI
  - merge esp_lcd display + normalized GT911 + basic LVGL controls.
```

## Compatibility notes

Useful alignment with our board:

```text
same 800x480 target;
same ESP32-S3 target;
same GT911 pins;
same RGB sync pins;
same 16 MB flash / OPI PSRAM assumptions;
same LVGL 8.3.11 version;
same GT911 raw-coordinate problem.
```

Open cautions:

```text
ESP-IDF project, not Arduino IDE;
no root license found;
code is example-style, not clean reusable BSP;
main loop label update does not obviously use the GUI semaphore;
physical behavior on our specimen is not yet validated;
RGB color order must be tested physically before changing our BSP pin names.
```

## Audit result

This is the most directly actionable project so far.

It confirms that our next serious path should be an `esp_lcd` RGB panel probe before any more user-facing LVGL work.

Recommended next repository step after completing the audit round:

```text
Create 12_DisplayEspLcdRgbPanel_Probe as an isolated ESP-IDF/Arduino-core-compatible display transport experiment, or create a dedicated ESP-IDF branch if Arduino IDE constraints block esp_lcd cleanly.
```

## Next audit step

Proceed to:

```text
limpens/esp32-8048S043-lvgl9
```

Target questions:

```text
What changed when the same author moved this board to LVGL 9?
Did the display transport stay esp_lcd RGB?
Did touch normalization change?
Are buffer/framebuffer choices improved or simplified?
Is the LVGL 9 path worth tracking later, while our current working base remains LVGL 8.3.11?
```
