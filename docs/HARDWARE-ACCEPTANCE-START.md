# Hardware acceptance start

This is the first-board workflow. Do not flash experimental display firmware before identity capture.

## Stage 0 — photos

- Front of the complete board.
- Back of the complete board.
- Close-up of PCB/revision marking.
- Close-up of ESP32-S3 module/chip.
- Close-up of USB, SD, speaker/IO connectors.
- Display/touch markings if visible.

Store original photos under:

```text
evidence/specimens/sample-a/photos/
hardware/images/raw/
```

## Stage 1 — passive host audit

Run the Windows audit script when available:

```powershell
.\tools\windows\esp32-8048s043-audit.ps1
```

With a known COM port:

```powershell
.\tools\windows\esp32-8048s043-audit.ps1 -Port COM7
```

## Stage 2 — chip identity

Capture:

- ESP32 model and revision;
- crystal frequency;
- flash ID and flash size;
- PSRAM result;
- USB bridge / native USB path.

## Stage 3 — first safe firmware

Upload only `01_BoardInfo` first. It should not drive the RGB panel aggressively.

## Stage 4 — display bring-up

Only after identity capture, test RGB timing and backlight with a minimal display pattern.

## Stage 5 — touch bring-up

Run I2C scan, then GT911 coordinate test.

## PASS wording

Use these labels:

```text
OPEN
REPORTED ONLY
SOURCE IMPLEMENTED
CI PASS
PHYSICAL PASS
PHYSICAL FAIL
PARTIAL
PENDING EXTERNAL HARDWARE
```
