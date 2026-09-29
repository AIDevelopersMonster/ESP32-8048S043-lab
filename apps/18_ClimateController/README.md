# App18 — Climate Controller

Status: **DESIGN / IMPLEMENTATION STARTED**

App18 combines the existing RS485 temperature/humidity sensor provider with the existing MA01 relay provider to form a thermostat and humidity regulator on the common KONTAKTS Platform command/service architecture.

## Default control ranges

The first-run defaults are intentionally non-zero and immediately usable for bench testing:

```text
Temperature MIN   25.0 °C
Temperature MAX   30.0 °C
Temperature HYST   0.5 °C

Humidity MIN      45 %RH
Humidity MAX      60 %RH
Humidity HYST      2 %RH
```

These are startup defaults only. They must be editable from the SD widget and persisted in NVS.

## Default relay allocation

```text
DO1 = HEATER
DO2 = COOLER / FAN
DO3 = HUMIDIFIER
DO4 = DEHUMIDIFIER / EXHAUST
```

Interlocks:

```text
HEATER and COOLER must never be ON together.
HUMIDIFIER and DEHUMIDIFIER must never be ON together.
```

## Initial control logic

```text
T < 25.0                 -> DO1 ON
T > 25.5                 -> DO1 OFF

T > 30.0                 -> DO2 ON
T < 29.5                 -> DO2 OFF

RH < 45                  -> DO3 ON
RH > 47                  -> DO3 OFF

RH > 60                  -> DO4 ON
RH < 58                  -> DO4 OFF
```

The hysteresis prevents relay chatter near the threshold.

## Architecture

```text
RS485 T/RH sensor
        |
        v
sensor provider
        |
        v
climate control service
        |
        +--> bindings / status --> SD widget
        |
        v
MA01 provider
        |
        v
UART1 GPIO17/18 -> RS485 -> MA01 -> DO1..DO4
```

The SD widget is the HMI/configuration layer. Automatic regulation must continue to run even when the widget screen is not open.

## Safety baseline

- sensor offline -> FAILSAFE;
- MA01 offline -> FAULT;
- no opposite outputs active at the same time;
- relay writes only when desired output state changes;
- configuration persisted in NVS;
- later physical acceptance must verify each control rule independently.
