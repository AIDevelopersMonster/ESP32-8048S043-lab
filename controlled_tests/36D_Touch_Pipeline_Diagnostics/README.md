# Test 36D — raw-to-LVGL touch pipeline diagnostics

## Status

**DIAGNOSTIC TEST PREPARED / PHYSICAL VERDICT PENDING**

Parent: Test 36C modern-I2C reproduction.

Known physical facts before this test:

```text
Test 36    legacy I2C, degraded touch
Test 36B   sleep 500 -> 16 ms, no improvement
Test 36C   modern I2C, 9 clicked callbacks / 60 physical presses
```

Test 36D is intentionally diagnostic. It does not attempt another cure. Its purpose is to locate where presses disappear.

## Diagnostic layers

The test instruments two boundaries:

```text
A. GT911 / esp_lcd_touch boundary
   - every raw PRESSED sample returned by esp_lcd_touch_get_coordinates()
   - transition to RELEASED
   - mapped x/y used by LVGL

B. LVGL object event boundary
   - LV_EVENT_PRESSED
   - LV_EVENT_RELEASED
   - LV_EVENT_PRESS_LOST
   - LV_EVENT_CLICKED
   - target button label
```

This lets us distinguish:

```text
raw press/release present, CLICKED absent
    -> loss is above GT911 acquisition, in LVGL hit/press/release semantics

raw press itself absent
    -> loss is at/below GT911 acquisition/driver boundary

PRESSED present, RELEASED/PRESS_LOST abnormal
    -> explains stuck-looking visual state and suppressed CLICKED
```

## Experimental baseline

Test 36D keeps the Test 36C runtime architecture:

```text
modern I2C master
SDA19 / SCL20
400 kHz
GT911 INT disabled
GT911 mapping unchanged
LVGL 9.3.0
esp_lvgl_port 2.6.0
LVGL_TASK_SLEEP 500
PCLK 18 MHz
2 PSRAM RGB framebuffers
bounce 10
direct mode / avoid tearing unchanged
same six-card Stream Deck UI
```

Only diagnostic logging is added.

## Physical protocol

Use the same reproducible protocol:

```text
6 cards
10 presses per card
~500 ms between presses
60 total physical press attempts
```

Capture the full COM log from the first press through the last release.

## What to count

```text
RAW PRESS transitions
RAW RELEASE transitions
LVGL PRESSED events
LVGL RELEASED events
LVGL PRESS_LOST events
LVGL CLICKED events
```

The total number of repeated raw PRESSED samples while one finger is held is not the key metric. The important values are transition counts and whether every physical press yields a complete press/release pair.
