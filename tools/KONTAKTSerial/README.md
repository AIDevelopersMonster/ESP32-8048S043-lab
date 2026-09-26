# KONTAKTSerial

KONTAKTSerial is the Windows service-terminal and engineering GUI for the ESP32-8048S043 platform.

It works through the board's **technological/service UART0** path:

```text
PC -> USB -> CH340C -> UART0/P1 -> ESP32-S3 command/service layer
                                      |
                                      +-> Modbus provider -> UART1 GPIO17/18 -> TTL/RS485 -> MA01
```

UART0/P1 is **not** the field Modbus port. It is the PC service/diagnostic transport into the common command layer.

## Current status

Version: `0.2.0`

UART0 service link:

```text
115200 8N1
no flow control
line ending: CRLF recommended
```

## Install

Python 3 is required.

```powershell
python -m pip install -r requirements.txt
```

Tkinter is normally included with standard Windows Python.

## Run

```powershell
python .\KONTAKTSerial.py
```

or:

```text
run.cmd
```

Optional direct port selection:

```powershell
python .\KONTAKTSerial.py --port COM4 --baud 115200
```

## CLI access without the GUI

Any normal serial terminal can be used against the service UART.

Example with pyserial miniterm:

```powershell
python -m serial.tools.miniterm COM4 115200
```

Use `CRLF` or press Enter in a terminal that sends a line ending.

### General service commands

```text
HELP
```

Print the currently supported command set.

### MA01 address

Read the configured MA01 slave address:

```text
MA01 ADDR
```

Set and persist an address:

```text
MA01 ADDR 16
```

The address is stored in NVS. It is not hard-coded into the provider.

Search slave addresses and save the detected MA01:

```text
MA01 SCAN
```

### MA01 identification

```text
MA01 INFO
```

Reads the MA01 identification registers through the ESP32 Modbus provider.

### MA01 state

Read DO1..DO8:

```text
MA01 READ
```

Alias:

```text
MA01 STATUS
```

Read relay mode configuration:

```text
MA01 CONFIG
```

This reads the configured MA01 channel modes and reports LEVEL / PULSE / FOLLOW state through the provider.

### MA01 output control

For channel 1:

```text
MA01 DO1 ACTION
MA01 DO1 ON
MA01 DO1 OFF
MA01 DO1 TOGGLE
```

The same syntax applies to DO1..DO8.

`ACTION` is the preferred mode-aware command:

- LEVEL -> toggle;
- PULSE -> trigger the pulse;
- FOLLOW -> direct action is rejected because the channel is externally followed.

### MA01 mode configuration

Set DO1 to pulse mode:

```text
MA01 DO1 MODE PULSE
```

Available modes:

```text
LEVEL
PULSE
FOLLOW
```

Examples:

```text
MA01 DO1 MODE LEVEL
MA01 DO2 MODE PULSE
MA01 DO3 MODE FOLLOW
```

### Pulse width

Set pulse time for a channel:

```text
MA01 DO1 PULSEMS 5000
```

Valid numeric range exposed by the service command is `0..65535` ms.

## GUI service panel

KONTAKTSerial v0.2.0 keeps the normal serial terminal and adds a small **ESP32 service UART0 / MA01** panel.

The panel sends the same ASCII commands listed above through UART0/P1. It does not open the field RS485 port directly.

Available GUI actions:

- HELP
- ADDR?
- SCAN
- INFO
- READ
- CONFIG
- SET ADDR
- DO1..DO8 selector
- ACTION / ON / OFF / TOGGLE
- LEVEL / PULSE / FOLLOW selector
- SET MODE
- pulse-time entry
- SET PULSE

No MA01 slave address is pre-filled in the GUI. The user can query, scan or enter the address explicitly.

## Typical engineering workflow

```text
HELP
MA01 ADDR
MA01 INFO
MA01 CONFIG
MA01 READ
MA01 DO1 ACTION
```

If the address has not yet been configured:

```text
MA01 SCAN
```

or set it explicitly:

```text
MA01 ADDR 16
```

## Transport separation

The platform intentionally separates transport from device logic:

```text
HMI ---------+
UART0/P1 ----+--> command/service layer --> MA01 provider --> Modbus RTU UART1 --> RS485
Web ---------+
Bluetooth ---+
```

Therefore the same MA01 provider logic can later be called from HMI, service UART, Web/API or Bluetooth without duplicating the Modbus implementation.
