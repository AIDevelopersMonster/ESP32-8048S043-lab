# Test 36E — icon-wrapper hit-test isolation

## Status

**CONTROLLED FIX PREPARED / PHYSICAL VERDICT PENDING**

Parent runtime baseline: Test 36C modern-I2C reproduction.

Test 36D localized the loss above GT911 acquisition:

```text
physical attempts   66
RAW PRESS           65
RAW RELEASE         65
LVGL PRESSED         3
LVGL CLICKED         3
```

The GT911 path therefore acquired about 98.5% of the touches, while only about 4.5% reached the intended parent `lv_button`.

## Root-cause candidate

Each 140 x 140 Stream Deck card contains a centered 64 x 64 generic object created with `lv_obj_create()` as an icon wrapper:

```c
lv_obj_t * lv_obj_0 = lv_obj_create(lv_button_0);
lv_obj_set_width(lv_obj_0, image_size);
lv_obj_set_height(lv_obj_0, image_size);
```

All cards use `image_size = 64`.

In LVGL 9.3.0, a generic object starts with `LV_OBJ_FLAG_CLICKABLE`. The nested image itself is non-clickable by default, but the generic wrapper remains a child hit target over the visual center of the parent button.

Test 36D showed that failed touches cluster inside that predicted central child region, while the few successful parent-button clicks landed just outside it.

## Single intended delta

Add exactly one interaction-policy line to the generated component:

```c
lv_obj_t * lv_obj_0 = lv_obj_create(lv_button_0);
lv_obj_remove_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE);
```

No geometry, style, size, event callback, touch, I2C, LVGL timing, display or transport setting changes.

## Preserved baseline

```text
upstream commit           79e862ca332525ba8721c4691f450fb44ec08738
modern I2C master         yes
SDA/SCL                   19 / 20
I2C speed                 400 kHz
GT911 INT                 disabled
GT911 mapping             unchanged
LVGL                      9.3.0
esp_lvgl_port             2.6.0
LVGL_TASK_SLEEP           500 ms
PCLK                      18 MHz
RGB framebuffers          2 PSRAM
bounce                    10 lines
direct/avoid-tearing      unchanged
button dimensions         140 x 140
icon wrapper dimensions   64 x 64
button callback           original CLICKED-only callback
```

## Prediction

If the child wrapper is the cause, center-of-card presses should now hit the parent button instead of being intercepted by the child object.

A strong confirmation would be approximately one `DEMO: Button Name` line for every physical press.

## Physical protocol

Use the same six-card protocol:

```text
press each card 10 times
~500 ms between presses
60 physical attempts
```

Optionally add one final circular pass across all six cards.

Record:

```text
physical attempts
DEMO callback count
missed presses
stuck-looking presses
subjective response
```
