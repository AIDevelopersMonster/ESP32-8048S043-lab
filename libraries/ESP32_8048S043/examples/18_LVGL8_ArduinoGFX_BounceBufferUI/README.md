# 18_LVGL8_ArduinoGFX_BounceBufferUI

Status: `SOURCE IMPLEMENTED / PHYSICAL VALIDATION PENDING`.

## Goal

This example independently reproduces the most useful display-stability ideas observed in the physically successful `Albert-Benavent-Cabrera/Robot-Core-Display` reference, without copying its GPL application code.

The experiment intentionally stays on **LVGL 8** so it can be compared directly with Test 17 and with the existing ESP32-8048S043 BSP examples.

Architecture:

```text
LVGL 8
  -> two INTERNAL-SRAM partial draw buffers
  -> Arduino_GFX partial-area flush
  -> Arduino_ESP32RGBPanel bounce buffer
  -> RGB 800x480 panel

ESP32_8048S043_Touch BSP
  -> LVGL pointer
```

## Why this test exists

Physical Test 17 showed that the current-stack Arduino_GFX partial-render path can work functionally but can show visible redraw flicker / black-like transitions.

The external Robot-Core-Display reproduction showed a different result on the same physical Sample A:

```text
redraw is visibly slow
but stable
no jitter/chatter observed
```

That third-party project combines several variables at once: LVGL 9, double internal-SRAM buffers, an RGB bounce buffer, different timing details, and its own application workload. Test 18 keeps the local LVGL 8/BSP context and adopts only the most promising memory/scan-out mechanisms.

## Controlled comparison against Test 17

Retained from Test 17:

```text
current Arduino-ESP32 / ESP-IDF 5.x
current Arduino_GFX
LVGL 8.x
our ESP32_8048S043 GT911 BSP
800x480 RGB
PCLK 14 MHz
HSYNC 8/4/8
VSYNC 8/4/8
pclk_active_neg = 1
partial-area flush
5 ms loop cadence
```

Changed:

```text
Test 17 : one large partial LVGL buffer, internal preferred / PSRAM fallback
Test 18 : two small partial LVGL buffers, INTERNAL SRAM mandatory

Test 17 : no explicit RGB bounce buffer
Test 18 : RGB bounce buffer = 20 display lines

Test 17 : standard LVGL widgets demo
Test 18 : small independently authored event-driven HMI
```

The smaller 20-line buffers are deliberate. Two 800x20 RGB565 buffers require about 64 KB total and leave a realistic chance of satisfying strict internal-SRAM allocation on ESP32-S3. The test stops if both buffers cannot be allocated internally; it does not silently fall back to PSRAM.

## HMI design lesson

This example is also a minimal pattern for embedded graphical interfaces:

```text
hardware/BSP
  -> display transport
  -> touch transport
  -> LVGL port
  -> reusable UI objects
  -> event callbacks
  -> application state
```

The UI contains:

```text
a state card;
a button that toggles state;
a click counter;
a slider;
a value label;
a static footer/status message.
```

Only event-driven state changes modify widgets. There is no periodic label update or artificial animation in the baseline. This makes redraw behavior easier to judge and illustrates a useful embedded-HMI rule: avoid invalidating the screen when nothing actually changed.

## Requirements

```text
LVGL 8.x
LV_COLOR_DEPTH == 16
ESP32_8048S043 library/BSP
Arduino_GFX with Arduino_ESP32RGBPanel bounce_buffer_size_px constructor support
```

Arduino_GFX 1.6.4 is a known reference for that constructor form. If the locally installed Arduino_GFX is older and compilation reports a constructor-argument mismatch, treat that as a library-version boundary rather than changing the experiment immediately.

## What to record during physical validation

```text
full serial boot log;
whether both LVGL buffers report INTERNAL SRAM;
gfx begin result;
GT911 address / firmware / raw resolution;
initial screen stability;
button presses;
slider drags;
redraw speed;
any black transition;
any jitter/chatter;
rapid repeated taps;
flush count before/after idle periods;
freeHeap/freePsram ALIVE values.
```

Visual classification should distinguish:

```text
FAST + STABLE
SLOW + STABLE
FLICKER / BLACK-LIKE TRANSITION
JITTER / CHATTER
```

Do not collapse slow but deterministic redraw into the same category as unstable flicker.

## Current status

```text
SOURCE IMPLEMENTED
BUILD NOT YET REPORTED
PHYSICAL VALIDATION PENDING
```

No physical PASS is claimed until Sample A is tested.

## License boundary

This example is independently authored for `ESP32-8048S043-lab`. The external Robot-Core-Display project remains a GPL-3.0 reference only; its application code is not copied into this MIT project.
