# Test 36E — icon-wrapper hit-test isolation

## Status

**CANCELLED / NOT REQUIRED FOR CURRENT PROGRAMME DIRECTION**

Test 36D already provided the useful engineering conclusion from the `halyssonJr` Stream Deck sample:

```text
physical attempts   66
RAW PRESS           65
RAW RELEASE         65
LVGL PRESSED         3
LVGL CLICKED         3
```

The GT911 path therefore acquired about 98.5% of touches while the intended parent `lv_button` received only about 4.5%. Source inspection and the spatial distribution of successful/failed touches strongly indicate child-object interception in the generated card hierarchy: each 140 x 140 card contains a centered 64 x 64 generic `lv_obj` icon wrapper, and generic LVGL 9.3 objects are clickable by default.

A controlled one-line verification had been prepared:

```c
lv_obj_remove_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE);
```

but the user correctly redirected the programme away from spending more board time repairing a low-value third-party demo.

## Programme decision

Do not run Test 36E unless historical root-cause confirmation is specifically wanted later.

The useful pattern to retain is simply:

> Decorative children inside an interactive card must not become independent hit targets unless that behavior is intentional.

The next work is not another Test 36 derivative. It is a clean application implementation using the already known-good fast LVGL9/native-display foundation: six large cards, full-card hit areas, explicit pressed feedback, and one COM command per press.
