# Sample A specimen passport

Status: `OPEN`.

Fill after the first physical board is inspected.

## Visible markings

- Product name:
- PCB marking:
- Date/batch:
- Seller page:

## Measured identity

- Chip: ESP32-S3 QFN56
- Revision: v0.2
- Flash ID:
- Flash size: 16 MB dump captured
- PSRAM: 8 MB embedded PSRAM reported by esptool
- Crystal: 40 MHz
- MAC: 84:fc:e6:6c:69:3c
- USB bridge/native USB:

## Factory firmware preservation

Local evidence folder:

```text
evidence/specimens/sample-a/factory-firmware/
```

Factory firmware dump status:

```text
Timestamp   : 20260822-195722
Read size   : 16 MB / 0x01000000
Reads       : 2
SHA-256     : 3007E5A223CD70DD9E53746C899BA25AF24721C68F1CFC69AB8A8CE3D3E6EB4C
Result      : MATCH all reads are identical
```

Commit-safe hash record:

```text
evidence/specimens/sample-a/factory-firmware/factory-dump-20260822-195722.sha256.txt
```

Do not commit factory `.bin` dumps.

## Acceptance status

| Stage | Target | Status |
|---|---|---|
| HW-00 | Photos and visible identity | OPEN |
| HW-01 | Chip / flash / PSRAM | PARTIAL |
| FW-00 | Factory flash double-read dump | CAPTURED |
| FW-01 | Factory dump SHA-256 match | MATCH |
| FW-02 | Factory partition/string analysis | OPEN |
| FW-03 | Possible factory-test leads | OPEN |
| HW-02 | RGB display | OPEN |
| HW-03 | GT911 touch | OPEN |
| HW-04 | Backlight | OPEN |
| HW-05 | SD card | OPEN |
| HW-06 | Wi-Fi / BLE | OPEN |
| SW-01 | Arduino BSP BoardInfo | OPEN |
| SW-02 | LVGL basic UI | OPEN |
| SW-03 | Web setup | OPEN |
| SW-04 | Web Flasher | OPEN |
| SW-05 | GitHub OTA | OPEN |
