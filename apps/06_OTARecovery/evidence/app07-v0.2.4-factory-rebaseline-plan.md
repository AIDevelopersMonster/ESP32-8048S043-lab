# KONTAKTS Factory 0.2.4-r1 rebaseline plan

Date: 2026-09-08
Source branch: `agent/app07-v0.2.4-progressfix`
Internal firmware version: `0.2.4`
Factory package identity: `KONTAKTS Factory 0.2.4-r1`

## Decision

Use the physically proven KONTAKTS Platform 0.2.4 codebase, plus the isolated live OTA progress fix, as the next factory/recovery baseline.

The factory app partition is:

- offset: `0x20000`
- size: `0x300000` (3 MiB)

The built application is 1,506,816 bytes, so it fits with substantial margin.

## Why 0.2.4 is suitable as factory/recovery

It contains the firmware-resident platform shell and recovery-critical services:

- display + touch shell;
- Wi-Fi setup/connection;
- STATUS;
- verified HTTPS GitHub OTA;
- explicit OTA CONFIRM/rollback support;
- factory recovery action;
- persistent SPIFFS storage;
- browser Storage Manager;
- validated Widget Runtime;
- NTP service and the first external clock widget runtime support.

When executed from the factory partition, normal OTA installation writes the candidate into an OTA slot rather than replacing the factory image. This preserves the factory image as a recovery anchor.

## Package naming

Do not silently replace the historical 0.2.4 release binary under the same filename/hash. The progress-fix build has the same internal semantic version but different bytes.

Use explicit package identity:

- `KONTAKTS-Factory-0.2.4-r1-app-only.bin`
- `KONTAKTS-Factory-0.2.4-r1-full-bootstrap.bin`

## Built binaries

Source CI run: `34234598716`
Artifact: `app06-platform-v0.2.4`

### App-only factory image

Source artifact file: `app06-ota-v0.2.4.bin`
Renamed package: `KONTAKTS-Factory-0.2.4-r1-app-only.bin`
Size: 1,506,816 bytes
SHA-256: `c1d2ca68d57231307e9f2fe841780ee0f3a8526b0ae2767da83e61ed5cb52d71`

Recommended for rebaselining an already initialized development board.

Flash only at factory offset `0x20000`.

This preserves the existing NVS credentials, OTA slots, SPIFFS `/storage`, widgets and user files, provided the flashing tool does not perform a whole-chip erase.

Example:

```text
esptool.py --chip esp32s3 --port COMx --baud 921600 write_flash 0x20000 KONTAKTS-Factory-0.2.4-r1-app-only.bin
```

### Full bootstrap image

Source artifact file: `app06-ota-recovery-v0.2.4-full.bin`
Renamed package: `KONTAKTS-Factory-0.2.4-r1-full-bootstrap.bin`
Size: 1,637,888 bytes
SHA-256: `728a7a78792ee1663c97aab353141d0eece5f2f20d8804ffea32aa47d65a0044`

The size equals `0x20000 + application_size`, confirming that the application is placed at the factory offset in the merged image.

Use this only for a blank board / deliberate clean bootstrap. Because the merged file spans from address 0 through the end of the factory application, padding in regions between the bootloader/partition table/application can overwrite or blank early data regions such as NVS and OTA metadata. It is therefore not the preferred rebaseline path for a configured board.

## Required physical validation before replacing historical factory 0.1.0

1. Flash `KONTAKTS-Factory-0.2.4-r1-app-only.bin` at `0x20000` only.
2. Boot explicitly into factory/recovery.
3. Confirm display and GT911 touch.
4. Confirm existing Wi-Fi credentials survive.
5. Confirm existing `/storage` and installed widget survive.
6. Confirm factory reports internal version `0.2.4` and factory partition state.
7. `CHECK GITHUB` must discover a newer stable OTA version when one exists.
8. Install OTA into an OTA slot.
9. Boot candidate as PENDING_VERIFY, then CONFIRM it VALID.
10. Invoke RECOVERY and verify return to factory 0.2.4-r1.
11. Repeat OTA after recovery to prove the factory anchor can always bootstrap the current platform.

Only after these physical gates pass should the historical factory 0.1.0 package be retired as the preferred recovery baseline.
