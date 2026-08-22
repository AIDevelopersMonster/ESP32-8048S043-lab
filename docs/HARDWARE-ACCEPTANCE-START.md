# Hardware acceptance start

This is the first-board workflow. Do not flash experimental display firmware before identity capture and factory firmware preservation.

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

## Stage 3 — factory firmware preservation

Before flashing any lab firmware, read the original flash at least twice and compare SHA-256:

```powershell
.\tools\windows\dump_factory_firmware.ps1 -Port COM7 -SizeMB 16 -Reads 2
```

The script writes local binaries under:

```text
evidence/specimens/sample-a/factory-firmware/
```

Firmware binaries are ignored by Git. Commit only reviewed metadata such as SHA-256 and analysis reports.

Then run the offline first-pass scanner:

```powershell
py tools\analysis\firmware_scan.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis
```

## Stage 4 — first safe firmware

Upload only `01_BoardInfo` first. It should not drive the RGB panel aggressively.

## Stage 5 — display bring-up

Only after identity capture and factory-firmware preservation, test RGB timing and backlight with a minimal display pattern.

## Stage 6 — touch bring-up

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
POSSIBLE FACTORY TEST LEAD
```
