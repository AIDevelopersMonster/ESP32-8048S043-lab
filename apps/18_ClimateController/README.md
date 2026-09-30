# App18 — Climate Controller

Status: **PHYSICAL PASS / WEB PASS on Sample A (2026-09-30)**

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
- Sample A physical acceptance completed for threshold control, interlocks, sensor-loss grace/FAILSAFE, recovery, SD HMI and local Web control; dedicated MA01-link-loss bench test remains deferred because it requires rewiring the test stand.

## Implemented service milestone (2026-09-29)

`climate_logic.c` and `climate_service.c` are compiled into the common App06
platform firmware. The existing EID041 polling task runs the controller after
its sensor read (nominally every 5 seconds, plus bus transaction time). When
AUTO is OFF and no installed widget uses the sensor, that task does not transmit
on RS485. A widget with T/RH bindings starts sensor polling even when AUTO is
OFF. No new FreeRTOS task is allocated. Regulation does not depend on the
displayed screen; AUTO continues to poll even if another panel is open.

The MA01 Web Control page reads relay registers once when opened, after a
manual command, and when READ ALL is pressed. Its optional cached events
endpoint does not perform a Modbus transaction. Routine FC01 frame dumps and
UI memory telemetry require DEBUG logging. UART0 remains available for
commands and fault diagnostics without printing each successful poll.

On a PENDING_VERIFY OTA boot, the SYS recovery controls appear before a
persisted widget is rendered. Select WIDGET explicitly to load that view;
confirm the OTA image only after the required screen and hardware checks.

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

### Local Web / phone control

The platform now exposes a responsive Climate Controller page at:

```text
http://<board-ip>/climate
```

The page is intended for a phone or desktop browser on the same local network.
It shows temperature, humidity, controller mode, sensor/relay state and DO1..DO4,
and provides AUTO ON/OFF, DEFAULTS and atomic editing of all six limits. The
browser polls the common climate service once per second; it does not implement a
second copy of control logic. Commands route through the same command/service
layer used by UART0 and BLE. AUTO ON reserves MA01 for climate control; AUTO OFF
releases it for external relay control.

Physical Web acceptance on Sample A was completed from CI run 36713194827,
commit `82d56cb78eb74347057bcde2cd3904cb268f49ac`. The app-only image size is
1,899,056 bytes and SHA-256 is
`F5A494AB887BA010C130C5AFB29F1478FE42BB4D8384B5F136A414842DD554B6`.
The image was written to ota_1, verified by flash digest, booted as NEW /
PENDING_VERIFY, exercised through `/climate`, and then confirmed VALID.

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
Sample A physical acceptance has checked DO1..DO4 control paths, temperature and humidity hysteresis, sensor removal, the 60-second stale-data grace window, FAILSAFE all-off with relay communication available, automatic recovery, SD HMI operation, AUTO ownership/release semantics, and local Web control. The dedicated MA01 communication-loss test is intentionally deferred because isolating that branch requires rebuilding the current bench wiring.

The climate sensor bindings display `--` and `NO SENSOR` until EID041 has
responded successfully. After a failed sensor poll, the last measured values
remain visible with `SENSOR: STALE`; they are **not** used to drive relays.
For up to 60 seconds after the last fresh sample, AUTO holds its last confirmed
relay states without making a new decision from the stale sample. At 60 seconds
it commands all four climate outputs OFF and enters FAILSAFE. There is no grace
period before the first valid sample; relay communication failures still use
the immediate fault path. Only a fresh valid sample can make new decisions.
The home screen also shows
`climate.error` for diagnosis when a relay fault is latched. Once an OFF
sequence has been confirmed, the FAULT indication remains until an explicit
OFF request, while redundant OFF writes stop.
The clock-only platform remains OFF after boot and sends no climate relay writes.

The EID041 sensor occupies Modbus slave address 1. The MA01 relay must use a
different address (16 on Sample A). MA01 SCAN skips address 1 and only saves a
candidate after the model, firmware, eight coils (FC01) and valid output modes
have been read. If an earlier scan stored address 1, set MA01 ADDR 16 again.

## Sample A AUTO startup diagnosis (2026-09-29)

The relay at address 16 accepts separate DO1 ON/OFF requests, including with
Climate Home open in OFF mode. During AUTO startup, the trace instead shows
the first FC05 OFF after an EID041 read timing out on every captured cycle;
the following FC05 requests generally receive valid echoes. Some captured
responses also have CRC errors. This identifies a sequencing-dependent failure,
but does not yet establish the physical cause of every corrupted frame.

The shared RTU transport now waits at least 10 ms before every request while
holding the bus mutex, including when switching between sensor and relay.
All FC01/03/04/05/06 responses are assembled across partial UART reads under
one existing 250 ms deadline. CRC, slave/function and write-echo checks remain
mandatory. Host tests cover fragmented frames, incomplete/absent replies,
deadline expiry, tick wrap, and the gap at 1 ms and 10 ms RTOS ticks.
The timing fix still requires a new firmware build and physical AUTO retest.

### Transient relay read errors and retained display

Physical AUTO testing of the RTU gap build reached confirmed relay states,
but still encountered CRC errors. Captured OFF commands now receive valid
echoes on all four channels; the available traces also show intermittent
sensor CRC errors with recovery. Web ON/OFF remains functional. The failed
AUTO read transaction has not yet been isolated by those traces.

Climate snapshot reads now retry timeout, CRC and response errors up to three
times. If all attempts fail during stable AUTO, keep confirmed outputs and
issue no new relay writes for up to 60 seconds since the last valid snapshot.
Show RELAY WAIT, then attempt OFF and latch FAULT if the grace period expires.
A lost or corrupt LEVEL write echo triggers fresh mode/state readback rather
than replaying the command. Only a correctly verified requested state and
interlock may complete the transition. Unresolved write results and other
faults after an attempted transition still follow the immediate shutdown path;
they do not use the read-only grace period.
Fresh readback showing all four outputs OFF completes shutdown even if a
write echo failed. An old cached OFF value never completes shutdown.

The display retains the last successfully read ON/OFF independently of control
ownership and fault state. On read/write uncertainty it appends `(STALE)`;
only fresh readback replaces that value. The home widget uses
`climate.relay_state` for NOT READ / ONLINE / STALE. Update its SD home.json
with this firmware change. Host tests cover retries, read-only grace without
writes, recovery, exact 60-second expiry and retained state after shutdown
without a valid readback. Physical testing remains necessary.

### Selecting the RTU fix from ota_0

CI run 36631496253 (commit 8d93a23) built the RTU fix application with SHA-256
`4092e66d550c56caea5269014aae49ea5a1305575527599ab5a0617f69625376`.
With SYS showing ota_1 VALID and readback entries seq=1 VALID / seq=2 VALID,
write only this application BIN to inactive ota_0 at `0x320000`.
After esptool reports successful write verification, use
`tools/prepare_ota0_selection.py` with the verified partition table, current
8192-byte otadata readback, application BIN and a new output filename.
It checks the exact image hash and expected OTA states and prepares one
4096-byte sector: seq=3, ota_0 NEW. Write that file only at `0xF000`.
Leave sector 1 at `0x10000` intact for rollback to ota_1. After first boot,
open SYS and press CONFIRM while ota_0 is PENDING_VERIFY, before any further
reset or esptool operation. Then retest Climate AUTO. NVS is not rewritten.

## Sample A flash layout read from the board (2026-09-29)

Before the App18 physical test, the board on COM4 identified as ESP32-S3
revision 0.2 with 16 MB flash. The partition table was **read from the board**
without writing flash:

```powershell
python -m esptool --chip esp32s3 --port COM4 read-flash 0x8000 0x1000 .\app18-ci\partition-table-before.bin
```

The readback yielded these entries (offset and size are hexadecimal):

| Partition | Offset | Size | Purpose |
| --- | ---: | ---: | --- |
| `nvs` | `0x009000` | `0x006000` | Wi-Fi and persisted settings |
| `otadata` | `0x00F000` | `0x002000` | OTA boot selection |
| `phy_init` | `0x011000` | `0x001000` | Radio initialization data |
| `factory` | `0x020000` | `0x300000` | Factory/recovery application |
| `ota_0` | `0x320000` | `0x300000` | Application slot 0 |
| `ota_1` | `0x620000` | `0x300000` | Application slot 1 |
| `storage` | `0x920000` | `0x600000` | Persistent internal filesystem |
| `coredump` | `0xF20000` | `0x010000` | Crash dumps |

This matches `apps/06_OTARecovery/idf/partitions.csv`. The CI run for commit
`f5919ae` produced an app-only BIN and a merged full BIN under the same
platform version `0.3.9`. The full BIN written at `0x0` is a service/initial
installation path: its contiguous merged range crosses `nvs` and `otadata`.
Do not use it as a settings-preserving update of this already configured board.
For the App18 lab, first identify the active OTA slot, then write the app-only
image to the **inactive** slot and switch boot selection using the ESP-IDF OTA
tool. A successful CI build does not constitute physical acceptance. The exact
slot and commands must follow the observed board state; the SD package is
installed separately under `/sd/widgets/climate-controller/`.

### Serial app-only activation checkpoint for Sample A

The image from the successful App18 CI run was written to inactive `ota_1`
(`0x620000`) with esptool; its data hash was verified. The pre-switch `otadata`
readback shows sector 0: `seq=1`, `state=2` (`VALID`), good CRC; sector 1 is
erased. This is a **staged candidate**, not an App18 physical pass yet.

`tools/prepare_ota1_selection.py` validates the board's saved 4 KiB partition
table, saved 8 KiB `otadata`, and the exact CI app-only BIN SHA-256. It then
creates only the 4 KiB second `otadata` sector with `seq=2`, a valid CRC and
`ESP_OTA_IMG_NEW` state, without accessing the board. This differs from the
ESP-IDF 5.5.5 `otatool.py switch_ota_partition` behavior, which updates the
sequence and CRC but leaves an erased state as `UNDEFINED`. `NEW` enables the
configured bootloader's `PENDING_VERIFY` flow on first boot.

After writing the prepared sector at `0x10000`, the original first sector
(`0xF000`) and NVS (`0x9000`) remain unchanged. The new app must be confirmed
using the SYS/OTA `CONFIRM` control after its physical smoke test, before a
second reboot; otherwise rollback may restore the previous app. Exact commands
are applied one at a time, after checking the tool output and board state.

### Second App18 trial after automatic rollback

The first trial reached `app09_ui` but the screen stayed dark and the task
watchdog repeatedly reported `IDLE1` starvation. A controlled reboot restored
the working `ota_0`; the saved selection then read `seq=1 state=2` (VALID) in
sector 0 and `seq=2 state=4` (ABORTED) in sector 1. The earlier application
remains intact. The corrected firmware from run `36560482258`, commit
`e1e8262`, has app-only SHA-256
`82f1d9c51a77c8888b4a643b67ae449386a77115e4bb620229a7ac766b0b160b`.
The helper now accepts only the verified `ota_0 VALID` with an erased sector 1
or the observed `ota_1 ABORTED` (seq 2, valid CRC), and only that corrected BIN.
Always read fresh `otadata` immediately before preparing a new selection;
never reuse the old `ota1-new-sector.bin` or overwrite `otadata` sector 0.
