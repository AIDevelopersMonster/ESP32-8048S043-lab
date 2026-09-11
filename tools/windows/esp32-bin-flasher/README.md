# ESP32 BIN Flasher v2

Physically validated helper tool for Windows, used with the ESP32-8048S043 / ESP32-S3 lab board.

## Purpose

A small WinForms GUI wrapper around `python -m esptool` for selecting a `.bin` file and COM port and flashing from Windows without manually typing the command.

## Features

- BIN file picker;
- COM port discovery/refresh;
- ESP32-S3 default target;
- selectable baud rate and flash address;
- connection test before flashing;
- AUTO DTR/RTS reset mode plus manual fallback modes;
- non-blocking log handling so the GUI remains responsive while esptool is running;
- STOP button;
- optional esptool install/update;
- does **not** run `erase-flash` automatically, preserving NVS/Wi-Fi unless the user explicitly erases them elsewhere.

## Download

`ESP32_BIN_Flasher_v2.zip` contains:

- `ESP32_BIN_Flasher_v2.ps1`
- `START_ESP32_BIN_Flasher_v2.cmd`
- `README_ESP32_BIN_Flasher_v2.txt`

Run `START_ESP32_BIN_Flasher_v2.cmd` on Windows.

## ESP32-8048S043 defaults

Typical full-image command produced by the GUI:

```powershell
python -m esptool --chip esp32s3 --port COM12 --baud 921600 --before default-reset --after hard-reset write-flash 0x0 "firmware.bin"
```

The exact COM port varies by machine.

## Evidence status

**PHYSICAL PASS** — the GUI was used successfully on the project board on 2026-09-11. The earlier blocking stdout/stderr implementation was replaced before this version was recorded here.

Video demonstration of the validated flasher together with the newer SD application-library UI:

- https://youtube.com/shorts/T91Nbeij2r8
