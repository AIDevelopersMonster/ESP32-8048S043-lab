# Test 36 — halyssonJr / LVGL Editor XML Stream Deck / native ESP-IDF

## Status

**BUILD PASS / DISPLAY PHYSICAL PASS / TOUCH UX FAIL-DEGRADED / NOT A KNOWN-GOOD HMI REFERENCE**

Pinned upstream:

```text
halyssonJr/lvgl-demo-esp32s3
79e862ca332525ba8721c4691f450fb44ec08738
2025-08-05
```

The exact pinned application builds successfully after the laboratory harness was corrected to preserve the upstream tracked `sdkconfig`.

Physical-board testing on 2026-09-04 produced a mixed result:

```text
Boot                         PASS
Backlight                    PASS
Stream Deck UI appears       PASS
Six cards render             PASS
Icons/text render            PASS
Display stability            PASS
Visible flicker              not reported
Visible tearing              not reported
GT911 input                  FUNCTIONAL BUT DEGRADED
Button presses               intermittent / missed
Button release/pressed state sometimes appears to stick
Visual pressed feedback      present but very subtle
On-screen application action not implemented upstream
Overall HMI verdict          TOUCH UX FAIL-DEGRADED
```

The user first described the screen as essentially six static images, then observed a very small press animation. Repeated physical use showed that the buttons do not respond reliably: some touches are missed and some presses appear to remain held or delayed.

This means Test 36 must **not** be classified together with the responsive Test 31/32/33/34 references.

## What the six cards actually do

The generated screen attaches `LV_EVENT_CLICKED` to each `deck_btn`.

The application callback is implemented in `main/examples/examples.c`, but it only logs the button label to Serial:

```c
void button_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *deck_btn = lv_event_get_target(e);

    if (code == LV_EVENT_CLICKED)
    {
        lv_obj_t *btn_label = lv_obj_get_child(deck_btn, 1);
        ESP_LOGI(demo_tag,"Button Name : %s", lv_label_get_text(btn_label));
    }
}
```

Therefore:

```text
screen navigation       NOT IMPLEMENTED
application state       NOT CHANGED
button action           SERIAL LOG ONLY
```

The small visible response is consistent with the normal pressed state of an `lv_button`. The XML component itself does not define a dedicated press animation or transition.

## Strong touch-latency candidate found after physical test

The upstream display code configures the LVGL port as:

```c
#define LVGL_TASK_SLEEP 500
#define LVGL_TIMER_MS   5

const lvgl_port_cfg_t lvgl_cfg = {
    .task_priority = 4,
    .task_stack = 8192,
    .task_affinity = -1,
    .task_max_sleep_ms = LVGL_TASK_SLEEP,
    .timer_period_ms = LVGL_TIMER_MS
};
```

At the same time the GT911 interrupt pin is explicitly disabled:

```c
#define TOUCH_GPIO_INT GPIO_NUM_NC
```

The exact `esp_lvgl_port 2.6.0` documentation states that the LVGL task can sleep until a display/animation/input interrupt, user wake, or `task_max_sleep_ms` timeout, and specifically warns that a touch interrupt pin should be configured when a large sleep value is used.

This creates a strong mechanism consistent with the physical symptoms:

```text
GT911 interrupt disabled
        +
LVGL task allowed to sleep up to 500 ms
        ->
short touch can occur between polls
release can be observed late
        ->
missed presses / delayed or apparently stuck pressed state
```

This is currently a **mechanism hypothesis supported by source configuration and Espressif documentation**, not yet a proven causal conclusion for this board.

## Next controlled derivative — Test 36B

A one-variable derivative is justified before abandoning the project:

```text
ONLY CHANGE:
LVGL_TASK_SLEEP 500 ms -> 16 ms

UNCHANGED:
GT911 driver
GT911 INT remains disabled
I2C 400 kHz
coordinate mapping
LVGL 9.3.0
esp_lvgl_port 2.6.0
PCLK 18 MHz
timings
2 PSRAM framebuffers
bounce10
direct mode
avoid_tearing
XML/generated UI
```

If button acquisition/release becomes immediately reliable, the 500 ms no-interrupt sleep configuration is strongly implicated.

If Test 36B remains poor, the next independent candidate is a separate interrupt-enabled experiment using the board's known GT911 INT path; it must not be mixed into 36B.

## Why Test 36 is still useful

Even with the poor baseline input UX, Test 36 contributes several useful ideas:

```text
LVGL Editor XML source
reusable declarative components
generated C for embedded target
large 140x140 touch-target design
native esp_lcd RGB
2 PSRAM framebuffers
bounce10
DIRECT + avoid_tearing
```

The authoring workflow remains interesting. The exact baseline touch scheduling is not suitable as a production reference on our specimen.

## Exact dependency baseline

```text
ESP-IDF                         5.5.0 upstream lock
host reproduction              ESP-IDF 5.5.5
LVGL                            9.3.0
espressif/esp_lvgl_port         2.6.0
espressif/esp_lcd_touch_gt911   1.1.3
espressif/esp_lcd_touch         1.1.2
target                          esp32s3
```

## Board / display baseline

```text
ESP32-S3
16 MB flash
Octal PSRAM 80 MHz
800x480 RGB
PCLK 18 MHz
pclk_active_neg true
HSYNC pulse/back/front 30/16/20
VSYNC pulse/back/front 13/10/22
2 RGB framebuffers in PSRAM
10-line RGB bounce buffer
LVGL DIRECT
bb_mode true
avoid_tearing true
```

GT911:

```text
I2C bus 1
SDA19 / SCL20
400 kHz
RST38
INT disabled
raw X 0..477 -> 0..800
raw Y 0..269 -> 0..480
```

## Harness history

The first laboratory compilation attempt was invalidated because the original lab runner called `idf.py set-target esp32s3`, which replaced the tracked upstream `sdkconfig` and removed required project settings including `CONFIG_LV_USE_OBJ_NAME=y` and the custom partition map.

That was fixed by:

```text
c26e88962561b8ef5bde654d9e688eb89413a8ab
fix(test36): preserve upstream sdkconfig and build cache

24e1b8ac91e1324a72cc39f5d89d3ea2bcc0940e
fix(test36): build with pinned upstream sdkconfig
```

The corrected build passed. The first invalidated attempt is not an upstream BUILD FAIL.

## Final baseline classification

```text
Test 36 exact upstream

BUILD                      PASS
DISPLAY                    PHYSICAL PASS
GRAPHICS STABILITY         PASS by observation
TOUCH DETECTION            PARTIAL / DEGRADED
TOUCH RESPONSIVENESS       FAIL for production-quality UX
BUTTON ACTIONS             SERIAL LOG ONLY BY DESIGN
LVGL Editor/XML workflow   VALID / INTERESTING
KNOWN-GOOD HMI REFERENCE   NO
NEXT                       Test 36B sleep 500 -> 16 ms isolation
```
