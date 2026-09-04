# Test 35 — Robot-Core-Display duplicate baseline — CANCELLED

## Status

**CANCELLED / DUPLICATE OF EXISTING PHYSICAL ROBOT-CORE AUDIT / DO NOT RE-RUN AS A NEW THIRD-PARTY TEST**

This branch was created after Tests 33-34 while expanding the laboratory toward GUI/UX studies. A repository-wide audit then confirmed that `Albert-Benavent-Cabrera/Robot-Core-Display` had already been studied, built, flashed and physically evaluated earlier in the project.

The existing authoritative record is:

```text
docs/third-party/albert-benavent-robot-core-display.md
```

and the main repository README already records the physical result.

Existing physical verdict:

```text
Build / link / firmware image : PASS
Display / touch runtime       : PHYSICAL FUNCTIONAL PASS / SAMPLE A
Redraw                         : visibly slow, but stable
Jitter / chatter               : NOT OBSERVED
Online ESP-NOW integration     : NOT YET TESTED
```

Existing video evidence:

```text
https://youtube.com/shorts/r2-6dwP3yoE
```

Upstream:

```text
https://github.com/Albert-Benavent-Cabrera/Robot-Core-Display
GPL-3.0 reference
```

## Why this branch is not being executed

Repeating the same Robot-Core baseline would add little evidence and would incorrectly present an already completed third-party reproduction as a new application test.

The preparation/build scripts retained in this branch are historical work from the mistaken duplicate setup. They are **not part of the active third-party test sequence** and should not be used to claim a new Test 35 result.

## What is still genuinely useful to do with Robot-Core

Robot-Core remains valuable as a **derived modernization target**, not as a new baseline.

Later experiments discovered several mechanisms or UX ideas that could be evaluated against the already-known Robot-Core result:

```text
1. Transport modernization
   Current Robot-Core reference:
     Arduino_GFX + double INTERNAL LVGL buffers + RGB bounce20
     -> stable but visibly slow redraw

   Candidate derived experiment from Test 31:
     native esp_lcd RGB
     + INTERNAL LVGL partial buffer
     + PSRAM RGB framebuffer
     + bounce0
     -> ~66 FPS in the tested LVGL Widgets reference

   Goal:
     preserve Robot-Core application/UI behavior while comparing redraw speed
     on a proven native esp_lcd transport.

2. GT911 normalization
   Avoid board-specific magic calibration constants where possible.
   Prefer driver/BSP-level raw-resolution normalization to 800x480, as learned
   from rzeldent/limpens references and our later GT911 tests.

3. Slider ergonomics
   Test 32 (DevAnyKR + EEZ) produced especially good slider capture/movement.
   Robot-Core's custom slider controls can be compared against that interaction
   quality and independently improved without copying third-party source.

4. UI architecture
   Robot-Core already has pages/components/assets separation. Test 33/34 add a
   useful compile-time modularity lesson (layout/theme/styles/widgets), but this
   is an incremental organizational idea rather than a reason to rewrite the UI.

5. Bounce-buffer lesson
   Robot-Core already has a non-zero driver-level RGB bounce buffer. Tests 22-29
   therefore do NOT justify 'adding bounce' to Robot-Core; it already uses it.
   The later causal result mainly explains why that part of the old reference
   was technically sound.
```

Any such work must be recorded as a **derived Robot-Core modernization experiment**, clearly separated from the existing GPL-3.0 upstream baseline and from the MIT-licensed lab code.

## Active next-project rule

The next third-party application should be genuinely new to this repository.

Candidates confirmed not to have previous in-repo audits at the time of this correction:

```text
halyssonJr/lvgl-demo-esp32s3
  -> LVGL Editor / XML Stream Deck UI
  -> ESP32-8048S043 exact hardware target
  -> generated C + reusable XML component
  -> image/font resource workflow

CelliesProjects/flocking-esp32
  -> real-time animation / continuous redraw
  -> touch interaction during animation
  -> live sliders
  -> related 800x480 ESP32-S3 RGB panel, hardware adaptation required

clackups/draftling
  -> application-level ergonomics
  -> file browser / split screen
  -> tap / double-tap / drag / flick gestures
  -> keyboard + touch interaction
  -> hardware adaptation required
```

For the immediate physical sequence, `halyssonJr/lvgl-demo-esp32s3` is the strongest next candidate because it directly targets ESP32-8048S043 while also introducing a genuinely new LVGL Editor/XML GUI workflow.
