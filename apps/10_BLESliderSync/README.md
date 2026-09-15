# App10 — KONTAKTS BLE Slider Sync

## Goal

Demonstrate not only the finished BLE feature, but the full engineering path used to build it.

The viewer should see the software being created from source on both sides:

- ESP32-S3 firmware: BLE GATT server + LVGL UI;
- Android app: native BLE client + slider UI;
- final live synchronization between phone and ESP32-8048S043.

No third-party BLE terminal is part of the public demo flow.

## Public demo outcome

Phone:

```text
KONTAKTS BLE Slider

Device: KONTAKTS-8048
Status: Connected

        63
0  ---------O------  100
```

Board:

```text
BLE SLIDER

     63

[#############-------]
```

Moving the phone slider updates the number/bar on the board.

Second milestone: moving the LVGL slider on the board updates the phone through BLE notifications.

## Architecture

```text
Android native app
        |
        | BLE GATT
        v
ESP32-S3 native BLE
        |
        +--> slider_value characteristic, uint8 0..100
        |
        +--> LVGL label
        +--> LVGL bar/slider
```

BLE profile:

```text
Service: KONTAKTS Slider Service
Characteristic: slider_value
Type: uint8
Range: 0..100
Properties: READ / WRITE / NOTIFY
```

The first version deliberately avoids JSON and other protocol layers. One value is enough to make the entire data path visible and testable.

## Hardware contract

No external hardware is required.

The demo must preserve the normal Sample A platform resources:

```text
GPIO19/20  system I2C, GT911 touch
GPIO17/18  remain free application GPIO candidates
microSD    remains available
Wi-Fi      may remain connected during BLE testing
```

The purpose is to prove that native BLE adds phone control without consuming the two primary external GPIOs.

## Development/demo sequence

Each stage should be kept as a separate small commit so it can be shown in a video as real development rather than a finished black box.

1. **Create App10 skeleton**
   - repository folders;
   - firmware and Android subprojects;
   - protocol UUIDs documented once.

2. **Board UI only**
   - LVGL number `0..100`;
   - LVGL bar/slider;
   - local touch movement updates the number;
   - no BLE yet.

3. **ESP32 BLE server**
   - advertise as `KONTAKTS-8048`;
   - one custom service;
   - one `slider_value` characteristic;
   - log connect/disconnect/write events;
   - incoming write updates LVGL.

4. **Android app skeleton**
   - build an installable APK from source;
   - one screen;
   - connection state;
   - value label;
   - slider.

5. **Android BLE scan/connect**
   - request Android BLE permissions;
   - scan only for the KONTAKTS service/device;
   - connect automatically or with one clear Connect button;
   - discover service and characteristic.

6. **Phone -> board synchronization**
   - slider movement writes uint8 `0..100`;
   - board displays the same value immediately;
   - verify end points 0 and 100 and several intermediate values.

7. **Board -> phone synchronization**
   - characteristic notifications enabled;
   - moving the LVGL slider sends notify;
   - Android slider and number follow the board.

8. **Coexistence test**
   - GT911 touch remains stable;
   - Wi-Fi STA remains connected;
   - SD remains mounted/usable if enabled;
   - BLE slider remains responsive.

9. **Release/demo package**
   - firmware artifact;
   - Android APK;
   - source code;
   - short installation notes;
   - video demonstration link.

## What the video should actually show

The video should not begin with an already finished APK.

Show the development chain:

```text
GitHub branch
  -> write board UI
  -> compile firmware
  -> board shows local slider
  -> add BLE service
  -> create Android project
  -> create phone slider
  -> implement scan/connect
  -> implement characteristic write
  -> first successful phone -> board movement
  -> add notify
  -> first successful board -> phone movement
  -> final clean demo
```

The useful educational moment is the first time the value moves across BLE. Keep that test visible rather than hiding it behind editing.

## Demo rule

The viewer should be able to understand the result without knowing BLE terminology:

> Move a control on the phone — the ESP32 display follows. Move the control on the ESP32 — the phone follows.

GATT UUIDs, permissions and Android BLE details belong in the engineering explanation after the visible result is established.

## Acceptance criteria

- [ ] board boots normally with GT911 working;
- [ ] Android APK installs without third-party BLE tools;
- [ ] app discovers/connects to KONTAKTS board;
- [ ] phone slider 0..100 updates board value correctly;
- [ ] board slider updates phone value correctly;
- [ ] reconnect restores a coherent value/state;
- [ ] no GPIO17/18 required by BLE;
- [ ] Wi-Fi STA + BLE coexistence passes basic runtime test;
- [ ] SD/touch regressions absent;
- [ ] source and APK are reproducible from repository instructions.

## Branch

Development branch:

```text
agent/app10-ble-slider-sync
```
