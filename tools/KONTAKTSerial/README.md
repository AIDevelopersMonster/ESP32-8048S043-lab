# KONTAKTSerial

KONTAKTSerial is the Windows service-terminal, CLI companion, and engineering GUI for the ESP32-8048S043 technological/service port.

It works through the board's **UART0 / P1 / CH340C** path:

```text
PC -> USB -> CH340C -> UART0/P1 -> ESP32-S3 command/service layer
                                      |
                                      +-> MA01 provider -> UART1 GPIO17/18 -> TTL/RS485 -> MA01
```

UART0/P1 is a **service transport**, not the field Modbus port.

## Current status

Version: `0.3.1`

UART0 service link:

```text
115200 8N1
no flow control
CRLF recommended
```

## Video demonstration

Physical bench demonstration of **KONTAKTSerial 0.3.1 + ESP32-8048S043 + MA01 RS485**, including control from the PC through UART0/P1 and simultaneous display of the current relay state on the board HMI:

https://youtu.be/d2D-bZdQFHI

## Install

```powershell
python -m pip install -r requirements.txt
```

## GUI

Run:

```powershell
python .\KONTAKTSerial.py
```

or:

```text
run.cmd
```

Optional explicit port:

```powershell
python .\KONTAKTSerial.py --port COM4 --baud 115200
```

The GUI opens the technological COM port and sends the same ASCII commands documented below.

### GUI MA01 controls

The MA01 service panel provides:

- HELP
- ADDR?
- SCAN
- INFO
- READ
- CONFIG
- SET ADDR
- DO1..DO8 selector
- per-channel INFO
- ACTION / ON / OFF / TOGGLE
- LEVEL / PULSE
- pulse-time entry in milliseconds
- SET MODE
- SET PULSE

The GUI does **not** pre-fill a Modbus slave address. Use ADDR?, SCAN, or enter one explicitly.

When a channel INFO response is received, the GUI updates its mode and pulse-time fields from the device response.

Serial responses are buffered until a complete line is received before the GUI parser updates MA01 fields. This avoids partial COM-port chunks causing intermittent parsing failures.

## CLI

A dedicated CLI client is included:

```powershell
python .\KONTAKTSerial_CLI.py --list
python .\KONTAKTSerial_CLI.py --port COM4
```

One-shot command mode:

```powershell
python .\KONTAKTSerial_CLI.py --port COM4 -c "MA01 DO1 INFO"
```

Windows launcher:

```text
run_cli.cmd --port COM4
```

Any normal serial terminal can also be used.

### pyserial miniterm

```powershell
python -m serial.tools.miniterm COM4 115200
```

Then type commands and press Enter.

### Basic commands

```text
HELP
MA01 ADDR
MA01 SCAN
MA01 INFO
MA01 READ
MA01 CONFIG
```

### Address

Read configured address:

```text
MA01 ADDR
```

Set and persist address:

```text
MA01 ADDR 16
```

Search for MA01:

```text
MA01 SCAN
```

### Device information

```text
MA01 INFO
```

### Read all outputs

```text
MA01 READ
```

Alias:

```text
MA01 STATUS
```

### Read relay configuration

```text
MA01 CONFIG
```

### Read one channel

Example for DO1:

```text
MA01 DO1 INFO
```

Expected form:

```text
OK MA01 DO1 STATE=OFF MODE=PULSE PULSEMS=5000
```

This is the recommended command for engineering tools that need a compact per-channel state.

### Operate one channel

Mode-aware action:

```text
MA01 DO1 ACTION
```

Explicit output commands:

```text
MA01 DO1 ON
MA01 DO1 OFF
MA01 DO1 TOGGLE
```

For normal relay operation the supported modes are:

```text
LEVEL
PULSE
```

Set mode:

```text
MA01 DO1 MODE LEVEL
MA01 DO1 MODE PULSE
```

Set pulse width:

```text
MA01 DO1 PULSEMS 5000
```

Valid exposed range:

```text
0..65535 ms
```

The same syntax applies to DO1..DO8.

## Recommended CLI workflow

First connection:

```text
HELP
MA01 ADDR
```

If no address is configured:

```text
MA01 SCAN
```

Then:

```text
MA01 INFO
MA01 CONFIG
MA01 READ
MA01 DO1 INFO
```

Example engineering changes:

```text
MA01 DO1 MODE PULSE
MA01 DO1 PULSEMS 5000
MA01 DO1 ACTION
MA01 DO1 INFO
```

## Architecture

```text
HMI ---------+
UART0/P1 ----+--> command/service layer --> MA01 provider --> UART1/RS485
Web ---------+
Bluetooth ---+
```

The GUI and CLI use UART0 only as a transport. MA01-specific Modbus logic remains in the provider, so the same command/service layer can later be exposed through HMI, Web/API or Bluetooth.
