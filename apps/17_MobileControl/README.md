# App17 — Mobile BLE Control

Status: **PHYSICAL MVP PASS / ANDROID 7.1.2**.

App17 is a phone client for the already physically validated Platform 0.3.9 BLE transport. It must not change the MA01 provider, Modbus register map, command/service layer, or the accepted App16 BLE GATT contract.

## Fixed platform contract

```text
Device   KONTAKTS-8048
Service  FFF0
Command  FFF1  WRITE
Response FFF2  READ
```

The mobile app sends the same text commands already accepted by UART0, Web and BLE Scanner:

```text
HELP
MA01 ADDR
MA01 READ
MA01 CONFIG
MA01 DO1 INFO
MA01 DO1 ACTION
MA01 DO1 ON
MA01 DO1 OFF
MA01 DO1 TOGGLE
MA01 DO1 MODE LEVEL
MA01 DO1 MODE PULSE
MA01 DO1 PULSEMS 5000
```

No MA01 Modbus register address is allowed in the mobile client.

## Architecture

```text
Android phone
    |
    | BLE GATT
    v
KONTAKTS-8048
    |
    | FFF1 text command / FFF2 text response
    v
command/service layer
    |
    v
MA01 provider
    |
    v
UART1 GPIO17/18 -> RS485 -> MA01
```

## MVP

The first Android MVP should provide:

- scan for `KONTAKTS-8048`;
- connect / disconnect;
- visible connection state;
- read initial `FFF2` response;
- raw command entry for diagnostic use;
- MA01 READ;
- DO1..DO8 state display;
- ACTION for each channel;
- ON / OFF for each channel;
- refresh state after a command;
- log of command and response;
- one BLE connection at a time.

The app writes to FFF1 and explicitly reads FFF2 after the write completes. The current firmware does not require notifications.

## Android compatibility

LAB-01 v0.1.1 uses `minSdk 23` (Android 6.0+). This explicitly includes Android 7.1.2 / API 25 devices used for the physical bench test.

The first v0.1.0 CI APK used `minSdk 26` (Android 8.0) and therefore could not be installed on Android 7.1.2; that was an application packaging compatibility limit, not a BLE or firmware failure.

## Android implementation policy

Use a native Android application first. The initial implementation should keep dependencies small and use the platform Bluetooth LE APIs directly.

Permissions must cover both permission models:

- Android 12+ — `BLUETOOTH_SCAN`, `BLUETOOTH_CONNECT`;
- older Android versions — legacy Bluetooth/location requirements as required by the OS.

No pairing/bonding workflow is required for the current laboratory MVP.

## Non-goals for LAB-01

Do not add these until the basic control path passes physically:

- firmware changes;
- BLE protocol redesign;
- vendor-specific 128-bit UUID migration;
- notifications;
- background service;
- cloud account;
- Internet access;
- Wi-Fi/Web fallback;
- Modbus register editing;
- multi-board fleet management.

## Physical acceptance

App17 LAB-01 passes only after the real phone proves:

```text
scan
 -> connect KONTAKTS-8048
 -> write MA01 command to FFF1
 -> read FFF2
 -> real MA01 relay changes
 -> HMI/Web reflect the same state
```

App16 Platform 0.3.9 remains the firmware baseline throughout this test.


## Physical result — Android 7.1.2

Physical bench test completed on Xiaomi `vince_ru`, Android 7.1.2 / API 25.

Confirmed with App17 v0.1.1:

- APK installs and launches on Android 7.1.2;
- BLE connection to `KONTAKTS-8048` works;
- commands reach the existing Platform 0.3.9 BLE transport;
- `MA01 DO<n> ON` physically switches a real relay ON;
- `MA01 DO<n> OFF` physically switches a real relay OFF.

This proves the end-to-end path:

```text
Android App17
 -> BLE FFF1 / FFF2
 -> Platform 0.3.9 command/service layer
 -> MA01 provider
 -> UART1 GPIO17/18
 -> RS485
 -> Ebyte MA01
 -> physical relay
```

The firmware, MA01 provider and Modbus register map were not changed for App17.

Not yet claimed by this checkpoint: complete UI state parsing for every response format, long-duration reconnect behaviour, background operation, multi-device operation, or production security.
