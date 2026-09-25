# App14 — Modbus RTU service / first field device

Status: **CODE READY FOR BUILD + PHYSICAL TEST**.

This stage extends the existing KONTAKTS Platform rather than creating a standalone firmware application.

Architecture:

```text
SYS | SD | WIDGET
        |
        +-- SD package: modbus-controller
                |
                v
          Widget Runtime
                |
                v
          modbus_service
                |
          UART1 GPIO17/18
                |
        automatic-direction
        TTL <-> RS485 adapter
                |
              RS485
                |
             EID041
```

## First acceptance target

- UART1 TX = GPIO17
- UART1 RX = GPIO18
- 9600 baud, 8N1
- Modbus RTU master
- slave address 1
- FC04 Read Input Registers
- start 0x0000
- two registers
- register 0 interpreted as signed temperature in 0.1 C
- register 1 interpreted as humidity in 0.1 %RH

The register map above is the **current lab hypothesis** for the first physical test. It must not be promoted to PHYSICAL PASS until the real EID041 responds and the displayed values agree with the device/environment.

The generic service already includes CRC16 validation, timeout/protocol counters and a reusable FC03/FC04 register-read primitive. The first SD widget displays bus state, temperature, humidity and diagnostics.

## Hardware boundary

The platform uses only the two primary user GPIOs on P4/P3:

```text
GPIO17 -> UART1 TX -> RS485 adapter TX/RX input
GPIO18 <- UART1 RX <- RS485 adapter RX/TX output
GND    <----------> adapter GND
3.3 V  ----------> adapter logic supply only if the selected module is 3.3 V compatible
```

The first lab assumes an RS485 module with automatic direction control. A module requiring DE/RE would need another control resource and is outside this two-GPIO acceptance.

## Acceptance

1. Build the platform branch.
2. Boot Sample A with normal display, touch and SD.
3. Connect the automatic-direction RS485 adapter to GPIO17/18.
4. Connect EID041 to A/B and power it according to its own requirements.
5. Put `widgets/modbus-controller` on the SD card and REFRESH.
6. Launch **Modbus T/RH Monitor**.
7. Confirm TX increases.
8. Confirm RX increases and Bus becomes ONLINE.
9. Confirm temperature/humidity are plausible.
10. Leave touch and SD active during the test to prove coexistence.

Until steps 7-10 are observed on hardware this remains **BUILD/TEST CANDIDATE**, not PHYSICAL PASS.
