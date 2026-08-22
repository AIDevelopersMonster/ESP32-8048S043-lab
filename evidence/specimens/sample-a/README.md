# Sample A specimen passport

Status: `OPEN`.

Fill after the first physical board is inspected.

## Visible markings

- Product name:
- PCB marking:
- Date/batch:
- Seller page:

## Measured identity

- Chip:
- Revision:
- Flash ID:
- Flash size:
- PSRAM:
- USB bridge/native USB:

## Factory firmware preservation

Local evidence folder:

```text
evidence/specimens/sample-a/factory-firmware/
```

Expected safe public artifacts after review:

- SHA-256 file for repeated reads;
- first-pass partition/string analysis;
- notes on possible factory-test strings or entry points.

Do not commit factory `.bin` dumps.

## Acceptance status

| Stage | Target | Status |
|---|---|---|
| HW-00 | Photos and visible identity | OPEN |
| HW-01 | Chip / flash / PSRAM | OPEN |
| FW-00 | Factory flash double-read dump | OPEN |
| FW-01 | Factory dump SHA-256 match | OPEN |
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
