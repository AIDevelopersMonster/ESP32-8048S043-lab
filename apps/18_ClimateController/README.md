# App18 — Climate Controller

Status: **SERVICE IMPLEMENTED / HMI AND PHYSICAL ACCEPTANCE PENDING**

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

## Implemented service milestone (2026-09-29)

`climate_logic.c` and `climate_service.c` are compiled into the common App06
platform firmware. The existing EID041 polling task runs the controller after
its sensor read (nominally every 5 seconds, plus bus transaction time). No new
FreeRTOS task is allocated. Regulation does not depend on the displayed screen.

- Thresholds use integer tenths; equality preserves hysteresis state.
- Configuration is validated and saved as one versioned NVS blob. RAM changes
  only after a successful NVS commit. +/- edits are allowed while AUTO is on;
  the new limits take effect at the next controller poll.
- AUTO is deliberately not persisted: boot starts OFF and does not write relays.
- Starting AUTO reserves MA01 mutations, confirms DO1..DO4 OFF, and then checks
  that all four channels use LEVEL. Set these modes before enabling AUTO.
- All platform MA01 mutation entrypoints (including raw coil writes and address
  changes) are serialized and rejected from other tasks while reserved. For this
  first milestone this reserves the whole MA01, including DO5..DO8. Read operations
  remain available. An external bus master or hardware FOLLOW inputs are outside
  this software reservation; observed output disagreement triggers FAULT.
- Opposite outputs use break-before-make with readback. A 5-second minimum OFF
  interval applies to each output and its opposite. This is a bench default,
  **not compressor protection**. Poll cadence also limits switching frequency.
- Sensor failure/invalid measurements requests all four outputs OFF (FAILSAFE).
  Relay transaction/readback failure gives FAULT; unknown outputs display `--`.
  Errors during normal regulation latch until explicit OFF, successful OFF
  confirmation and ON. Startup/failsafe OFF attempts retry each poll if needed.
- A broken RS485 link cannot guarantee physical de-energization. FAILSAFE is an
  attempted shutdown; only a successful readback confirms OFF. Hardware watchdog
  or independent interlocks are required for that guarantee.
- Normal regulation writes only changed outputs; startup, shutdown and fault
  handling explicitly resend OFF to establish/re-establish a known state.

### Shared command API (UART0 / existing BLE command transport)

```text
CLIMATE STATUS
CLIMATE SET 250 300 5 450 600 20
CLIMATE DEFAULTS
CLIMATE ON
CLIMATE OFF
```

SET order: temperature MIN/MAX/HYST, humidity MIN/MAX/HYST, all in tenths.
Thus `250` is 25.0 degrees C and `450` is 45.0 %RH. The complete six-value
configuration is validated atomically. Ranges: -40..125 degrees C, 0..100 %RH;
hysteresis must be positive with separated recovery bands.
ON/OFF acknowledges a request: poll STATUS to observe the completed transition.
Commands use letter O in ON/OFF; relay commands remain `DO1` etc., with letter O.

### Widget bindings available now

```text
climate.state             climate.error         climate.edit_state
climate.sensor_state      climate.temperature   climate.humidity
climate.t_min             climate.t_max          climate.t_hyst
climate.rh_min            climate.rh_max         climate.rh_hyst
climate.heater            climate.cooler
climate.humidifier        climate.dehumidifier
```

The SD runtime allowlist and renderer route these bindings to the service.
Output bindings describe confirmed controller-owned states; while unowned they
show `--` (use existing `modbus.ma01.doN` bindings for manual relay status).
The SD package is in `apps/09_SDWidgetLibrary/sd/widgets/climate-controller`:
`home.json` shows readings, state and relay outputs; `settings.json` edits all six
limits using +/- buttons. Each accepted click saves to NVS and shows `SAVED`.
Limit violations show `LIMIT REACHED` without changing the saved configuration.
Temperature limits move by 0.5 C; temperature hysteresis by 0.1 C;
humidity limits and hysteresis move by 1 %RH. AUTO ON/OFF is on both pages.
The widget must be copied to the SD card along with its package manifest and
both JSON files; opening the screen is not necessary for automatic regulation.

### Verification

From the repository root on a C11 host with GCC/Clang:

```sh
bash apps/18_ClimateController/tests/run.sh
```

Tests compile the actual logic/service against small NVS/Modbus/RTOS stubs:
strict threshold equality, interlocks over the accepted measurement domain,
NVS commit failure, malformed SET commands, idle boot, output-change-only writes,
reversal delay, sensor loss, relay failures and explicit recovery.
The host test does not certify UART timing, concurrent RTOS execution or hardware.
The normal App06 GitHub workflow builds the ESP-IDF firmware for this branch.
Physical acceptance must still check DO1..DO4 with indicator loads, sensor removal,
RS485 interruption, mode rejection, manual-command rejection and reboot/NVS restore.

The climate sensor bindings display `--` and `NO SENSOR` until EID041 has
responded successfully. On a failed sensor poll they immediately hide the last
reading; sensor freshness is tracked separately from other RS485 device errors.
The clock-only platform remains OFF after boot and sends no climate relay writes.
