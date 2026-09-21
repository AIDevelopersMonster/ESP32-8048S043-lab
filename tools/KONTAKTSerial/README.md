# KONTAKTSerial

KONTAKTSerial is the small serial terminal used by the ESP32-8048S043 lab.

It is intentionally lightweight: Python + Tkinter + pyserial. It does not depend on ESP-IDF and is intended for normal day-to-day UART testing from Windows.

## Current status

Version: `0.1.0`

The first target is the board's normal USB serial path:

```text
PC -> USB -> CH340C -> UART0 -> ESP32-S3
PC <- USB <- CH340C <- UART0 <- ESP32-S3
```

The App11 firmware currently uses UART0 at `115200 8N1`.

## Features

- enumerates Windows COM ports;
- prefers CH340 / USB-serial devices when detected;
- connect/disconnect without ESP-IDF;
- baud-rate selector;
- 8N1 with flow control disabled;
- live RX window;
- TX entry with Enter-to-send;
- configurable line ending: None / LF / CR / CRLF;
- RX/TX byte counters;
- optional timestamps;
- optional HEX view for received bytes;
- optional TX display;
- clear and save-log actions;
- quick `HELLO123` test button.

## Install

Python 3 is required.

```powershell
python -m pip install -r requirements.txt
```

On a standard Windows Python installation Tkinter is normally included.

## Run

From this directory:

```powershell
python .\KONTAKTSerial.py
```

Or double-click:

```text
run.cmd
```

Optional direct port selection:

```powershell
python .\KONTAKTSerial.py --port COM4 --baud 115200
```

## ESP32-8048S043 App11 quick test

1. Close every other program that has the COM port open.
2. Start KONTAKTSerial.
3. Select the board COM port (for Sample A this was COM4 during the 2026-09-22 test session).
4. Select `115200`.
5. Click **Connect**.
6. Use **HELLO123** or type text and press Enter.
7. With the App11 `usb-serial-terminal.monitor` widget active, the received text should appear on the TFT.
8. The TFT **SEND TEST** button is intended to send `KONTAKTS USB SERIAL TEST` back to KONTAKTSerial.

Do not treat the reverse ESP32 -> PC direction as accepted until it has been physically exercised and observed.

## Evidence note

On 2026-09-22 the PC -> USB -> CH340C -> UART0 -> ESP32-S3 -> App11 widget direction was physically exercised on Sample A using a Python one-shot serial write of `HELLO123\r\n`; the string appeared on the TFT.

This tool does not change the board's P1 classification. P1 remains a separate service/UART0 investigation.
