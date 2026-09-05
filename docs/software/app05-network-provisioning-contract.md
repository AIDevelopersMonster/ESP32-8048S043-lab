# App 05 Network Provisioning Contract

Status: PLANNED AFTER APP04 PHYSICAL PASS

## Goal

Give a freshly flashed KONTAKTS device a deterministic path to become reachable on a real local network without hard-coded credentials.

## Required states

```text
BOOT
  |
  +-- valid saved Wi-Fi credentials? -- yes --> STA_CONNECTING
  |                                             |
  |                                             +-- success --> STA_ONLINE
  |                                             |
  |                                             +-- timeout/failure --> AP_SETUP
  |
  +-- no credentials --------------------------> AP_SETUP

AP_SETUP
  |
  +-- local setup page
  +-- scan/select SSID
  +-- enter credentials
  +-- persist through App04 storage service
  +-- request connection
  |
  +---------------------------------------------> STA_CONNECTING
```

## STA mode

The device must be able to:

- join an existing router/network;
- obtain an IPv4 address through DHCP;
- expose connection state to the application/UI;
- reconnect after temporary link loss;
- preserve credentials across reboot;
- avoid blocking the main application while reconnecting.

## AP fallback / first-run mode

If no usable credentials exist, or the saved network cannot be reached within a bounded interval, the device starts a configuration AP.

Initial naming direction:

`KONTAKTS-XXXXXX`

where the suffix is derived from a non-secret device identifier.

The AP exists for provisioning and recovery, not as the normal operating mode.

## Local web setup

The setup surface should initially provide only platform-critical fields:

- device name;
- scanned/selected SSID;
- Wi-Fi password;
- save/apply;
- current network state;
- IP address when online;
- clear credentials / return to AP setup.

Future project-specific settings must be added through a separate project-settings contract rather than hard-coded into the Wi-Fi service.

## Persistence contract

App05 consumes App04 storage services:

- small credentials/state -> NVS;
- larger optional web/UI resources -> filesystem;
- network service must not directly own unrelated project configuration.

Passwords must never be printed in logs or exposed back through normal status APIs.

## Controlled physical PASS boundary

App05 does not pass because an AP merely appears.

PASS requires:

1. clean first boot enters AP setup;
2. client can connect to the AP;
3. setup page is reachable;
4. a real router SSID can be selected/configured;
5. credentials persist;
6. device reboots and reconnects to the router without re-entry;
7. loss of the configured network eventually leads to a bounded recovery/setup path;
8. credentials are not leaked to serial logs;
9. network activity does not destabilize the display/touch runtime when integration is restored.

## Next controlled variable

After App05 physical pass: App06 OTA / rollback / recovery.
