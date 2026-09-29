# Release protocol and current channels

The repository uses separate release channels for firmware, SD content, Web Flasher and host/mobile tools.

## Current accepted platform

```text
KONTAKTS Platform 0.3.9
Status: PHYSICAL PASS / Sample A
Release tag: app06-v0.3.9
```

Accepted full-image SHA-256:

```text
20D3CD2675E49FA84E1BE6FF9DBF0C01A4D3786234138CF4511014B9B4FDBC76
```

OTA manifest:

```text
https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/latest/download/app06-ota.json
```

## Channels

| Channel | Current identifier | Output | Acceptance rule |
|---|---|---|---|
| Platform full image | `app06-v0.3.9` | full `.bin` | exact hardware-tested image/hash preferred |
| GitHub OTA | `app06-v0.3.9` | OTA `.bin` + JSON manifest | hardware OTA path must pass |
| SD library | `app09-sd-current` | `kontakts-sd-library.zip` + package ZIPs | SD mount/launcher/help must pass |
| Web Flasher | GitHub Pages from `main` | ESP Web Tools site | current platform image SHA checked in CI |
| Android App17 | `app17-v0.1.1` | permanent APK + SHA-256 | exact CI artifact + install/launch/BLE/relay physical pass |
| Arduino BSP | `arduino-v*` when promoted | installable ZIP | CI compile + physical board evidence |

## Release discipline

Before promoting a platform release:

1. CI must build the intended image.
2. Record SHA-256 of the candidate.
3. Flash/update a named physical specimen.
4. Exercise the functions claimed by that release.
5. Keep the exact accepted artifact when reproducibility/build metadata could otherwise change the binary.
6. Update root README, application README, release notes, Web Flasher catalog and SD Help as applicable.
7. Do not call an untested rebuild equivalent to a hardware-tested binary merely because the source is the same.

## Platform 0.3.9 accepted scope

```text
RGB display + GT911
SD library / launcher / Help
Wi-Fi AP + STA
Web control
UART0/P1 service
UART1 GPIO17/18 + RS485 + MA01
GitHub OTA CHECK
BLE KONTAKTS-8048
Android App17 relay control
```

## Verified current asset inventory

See:

```text
docs/RELEASE-ASSET-INVENTORY.md
```

Current SD bundle SHA-256:

```text
38C8E2F8B490EB18C7052C6F9AD878BF91561B57F94D71A5BD75A5FAAAB56EF7
```

Because `app09-sd-current` is intentionally mutable, this value must be refreshed whenever the SD bundle workflow republishes the current tag.

## Security boundary

Current BLE and local HTTP control are laboratory/trusted-network MVPs. Production authentication, BLE bonding/application authentication and public-network exposure are separate future gates.


## Android App17 permanent release

```text
Tag       app17-v0.1.1
APK       kontakts-mobile-app17-v0.1.1-debug.apk
SHA-256   5DE40BC802438A35285077241DA603B0441EAD28179928FAD0E15EA0285868D0
CI run    36507403489
```

Release:

https://github.com/AIDevelopersMonster/ESP32-8048S043-lab/releases/tag/app17-v0.1.1

The publication workflow pins the physically tested artifact. A new App17 version must receive a new physical acceptance checkpoint before the pinned release workflow is extended to that version.
