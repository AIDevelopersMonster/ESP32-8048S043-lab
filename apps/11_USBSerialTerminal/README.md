# App 11 — USB Serial Terminal

## Goal

First stage of the serial-terminal work for ESP32-8048S043.

This experiment deliberately uses the board's normal USB serial path first:

```text
PC -> USB -> CH340C -> UART0 -> ESP32-S3 -> Widget Runtime -> TFT
TFT SEND TEST -> UART0 -> CH340C -> USB -> PC
```

P1 is not part of this first acceptance test.

## Platform change

The firmware gains one generic platform capability, `serial_service`.

It exposes these widget bindings:

```text
serial.state
serial.rx_text
serial.rx_bytes
serial.tx_bytes
```

and these widget actions:

```text
serial_send_test
serial_clear
```

The SD application remains declarative and is stored at:

```text
apps/09_SDWidgetLibrary/sd/widgets/usb-serial-terminal/
```

No application-specific launcher button is added to firmware.

## Stage 1 UI

The initial screen intentionally contains only:

- received text;
- RX byte count;
- TX byte count;
- SEND TEST;
- CLEAR.

The purpose is to prove the bidirectional USB/UART path before adding a keyboard.

## Acceptance test

1. Install Platform 0.3.5 test firmware.
2. Put the current SD library on the card and refresh the application list.
3. Run **USB Serial Monitor**.
4. Open the normal board COM port at 115200 baud.
5. Type a line on the PC. It must appear in the TFT RX area.
6. Press **SEND TEST** on the TFT.
7. The PC terminal must receive:

```text
KONTAKTS USB SERIAL TEST
```

8. Press **CLEAR** and verify the TFT counters and RX window reset.

## Evidence state

Software implementation only until exercised on Sample A.

Do not claim UART RX physical PASS before the PC-to-board path is observed on hardware.

## Stage 2

After Stage 1 passes, add generic LVGL 9 Widget Runtime capabilities:

```text
textarea
keyboard
```

Then the same SD package can become an interactive terminal without adding a terminal-specific firmware screen.

Later stages can add CR/LF selection, command history, ASCII/HEX and finally use the same platform capability to investigate P1.
