# MA01-XXCX0080 8DO Modbus RTU GUI v0.5.0

Desktop engineering GUI for the Ebyte MA01-XXCX0080 8DO RS485 / Modbus RTU relay module.

## Package

- `MA01_XXCX0080_GUI_v0.5.0.zip`
- Python 3 + Tkinter
- pyserial
- Windows BAT launcher included

## v0.5.0

Three-tab UI:

1. **CONTROL**
   - one control per DO
   - current mode shown under each output
   - no background polling
   - LEVEL: stable toggle
   - PULSE: trigger only, followed by one delayed FC01 readback
   - FOLLOW: direct control disabled

2. **REGISTER LAB**
   - generic FC03 holding-register reader
   - FC06 single-register write
   - HEX / DEC / 16-bit binary view
   - individual bit editing
   - FC01 / FC0F coil-mask editor

3. **MODE SETUP**
   - LEVEL / PULSE / FOLLOW selection per channel
   - pulse width configuration
   - per-channel APPLY
   - APPLY ALL 8
   - READ MODE CONFIG
   - widget mapping preview for the ESP32-8048S043 platform:
     - LEVEL -> TOGGLE
     - PULSE -> PUSH
     - FOLLOW -> FOLLOW/STATUS

## Tested physical module

- Model: MA01-XXCX0080
- Firmware: V1.6
- RS485 / Modbus RTU
- Slave address is user-configurable and must not be hard-coded in clients.
- Current physically tested module was discovered at slave 16.
- Earlier lab sessions used slave 32; treat that only as historical configuration, not as a default assumption.
- 9600, 8N1

## Registers used

- Coils `0x0000..0x0007`: DO1..DO8 state
- Holding `0x0578..0x057F`: DO1..DO8 mode
- Holding `0x05DC..0x05E3`: DO1..DO8 pulse width
- `0x07D0`: module model
- `0x07DC`: firmware version

This utility is part of the ESP32-8048S043 Modbus-controller work and is intentionally event-driven rather than continuously polling the bus.
