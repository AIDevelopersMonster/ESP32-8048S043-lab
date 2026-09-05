# Test 36E — icon-wrapper hit-test isolation

## Status

**BUILD PASS / FLASH PASS / PHYSICAL PASS / ROOT CAUSE CONFIRMED**

Parent project: `halyssonJr/lvgl-demo-esp32s3`, pinned upstream commit `79e862ca332525ba8721c4691f450fb44ec08738`.

Test 36 initially looked like a bad GT911/touch implementation: button presses were highly selective, short taps were often ignored, and the visual pressed state sometimes appeared to stick. The display itself remained stable.

The investigation separated the layers instead of assuming the touch controller was at fault.

## Evidence chain

### Test 36 — original upstream behavior

```text
Display                         PASS
GT911 technically functional    YES
Button UX                       FAIL-DEGRADED
Missed application clicks       frequent
```

### Test 36B — LVGL task sleep 500 -> 16 ms

No useful improvement.

Conclusion:

```text
large task_max_sleep_ms was not the primary cause
```

### Test 36C — legacy I2C -> modern IDF I2C master

The modern I2C backend was confirmed active at boot, but only 9 application `CLICKED` callbacks were observed from 60 physical click attempts.

Conclusion:

```text
legacy I2C backend was not the primary cause
```

### Test 36D — raw-to-LVGL instrumentation

Physical protocol:

```text
6 cards x 10 presses = 60
+ one circular pass across all 6 cards
```

Diagnostic capture:

```text
RAW PRESS        65
RAW RELEASE      65
LVGL PRESSED      3
LVGL RELEASED     3
LVGL PRESS_LOST   0
LVGL CLICKED      3
```

This localized the dominant loss above GT911 acquisition and below the intended parent-button event handler.

The mapped GT911 coordinates also matched the visual card centers correctly, ruling out a gross coordinate mapping or axis-orientation problem.

## Root cause

Each 140 x 140 card contains a centered decorative icon wrapper created as a generic LVGL object:

```c
lv_obj_t * lv_obj_0 = lv_obj_create(lv_button_0);
lv_obj_set_width(lv_obj_0, image_size);
lv_obj_set_height(lv_obj_0, image_size);
```

All six cards use `image_size = 64`, so a 64 x 64 child object sits over the visual center of the 140 x 140 parent button.

In LVGL 9.3.0 a generic object created with `lv_obj_create()` is clickable by default. Therefore the decorative 64 x 64 wrapper became an independent hit target and intercepted touches intended for the parent `lv_button`.

The nested image widget itself is not the key problem; the generic wrapper around it is.

Test 36D supplied strong pixel-level evidence: most failed presses were inside the predicted 64-pixel child region, while the few parent-button clicks occurred just outside it.

## Test 36E single fix

The controlled fix changed only the interaction policy of that decorative wrapper:

```c
lv_obj_t * lv_obj_0 = lv_obj_create(lv_button_0);
lv_obj_remove_flag(lv_obj_0, LV_OBJ_FLAG_CLICKABLE);
```

No GT911, I2C, coordinate-map, display, card-size, image-size, LVGL timing or callback changes were required for the fix.

## Physical result after the fix

The user repeated the real-board button test:

```text
press each of the six physical cards 10 times
then press the cards once more in a circular pass
approximately 500 ms between presses
```

Externally the UI then behaved correctly: every intended press produced the expected COM callback, with no observed missed clicks or stuck interaction behavior.

Representative output became a continuous sequence such as:

```text
DEMO: Button Name : Power
DEMO: Button Name : Power
...
DEMO: Button Name : Media
...
DEMO: Button Name : Social
...
DEMO: Button Name : Settings
```

The upstream sample has a separate semantic defect: two physical cards are both labeled `Media`, and two are both labeled `Social`. Therefore the text log cannot uniquely identify all six physical cards even though their click handling now works. This is an upstream UI/data-model issue, not a touch issue.

## Final diagnosis

```text
GT911 hardware/acquisition       GOOD
I2C transport                    GOOD / not causal
LVGL polling/task sleep          not causal
coordinate mapping               GOOD
RGB/display path                 GOOD
parent button hit area           blocked by clickable decorative child
one-line child flag fix          PHYSICAL PASS
```

The apparent 'bad touch' was actually a **GUI object-tree / hit-testing bug**.

## Rule for our own ESP32-8048S043 software

For an interactive card, tile, list row, toolbar button or other composite control:

```text
ONE semantic control = ONE intentional hit target
```

Recommended pattern:

```c
lv_obj_t *card = lv_button_create(parent);      // interactive owner

lv_obj_t *icon_wrap = lv_obj_create(card);      // decoration only
lv_obj_remove_flag(icon_wrap, LV_OBJ_FLAG_CLICKABLE);
lv_obj_remove_flag(icon_wrap, LV_OBJ_FLAG_SCROLLABLE);

lv_obj_t *icon = lv_image_create(icon_wrap);    // decoration only
lv_obj_remove_flag(icon, LV_OBJ_FLAG_CLICKABLE);

lv_obj_t *label = lv_label_create(card);        // decoration only
lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
```

The application event callback belongs to the parent `card`, not to its decorative children.

### Additional software rules derived from this case

1. **Separate presentation from command identity.** Every card must have a unique stable command ID (`POWER`, `MEDIA`, `GAME`, `SOCIAL`, `WORK`, `SETTINGS`) independent of the visible label.
2. **Decorative children are non-interactive by default.** Images, icon wrappers, labels, badges and status indicators should not become hit targets unless explicitly intended.
3. **Use the full card as the finger target.** A 140 x 140 visual card should have approximately the same 140 x 140 semantic hit area.
4. **Pressed feedback belongs to the parent control.** Apply `LV_STATE_PRESSED` style/animation to the card so the user immediately sees acquisition.
5. **Do not diagnose touch quality from application callbacks alone.** If a future UI appears to miss input, inspect raw touch acquisition separately from LVGL hit testing before changing GT911/I2C/display parameters.
6. **Keep the known-good hardware stack frozen while designing UI.** Once GT911/display transport is physically proven, UI-tree errors should be investigated at the UI layer first.

## Programme decision

Test 36 is now closed as a useful negative/repair case. There is no reason to keep modifying the upstream Stream Deck demo.

The useful idea — six large visual command cards — should be reimplemented cleanly in our own application on a known-good LVGL9/display/touch foundation.

Next application direction:

```text
App 01 — Six-Card Serial Deck

known-good ESP32-8048S043 LVGL9 stack
        -> six unique full-card controls
        -> non-clickable decorative children
        -> immediate pressed feedback
        -> unique command ID per card
        -> COM output / later replaceable transport
```

This preserves the good UI concept without inheriting the third-party object-tree mistake or its ambiguous duplicate labels.
