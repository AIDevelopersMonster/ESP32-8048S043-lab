# KONTAKTS Factory 0.2.4-r1 rebaseline — CLOSED / PHYSICAL PASS

Date: 2026-09-08
Source branch: `agent/app07-v0.2.4-progressfix`
Internal firmware version: `0.2.4`
Factory package identity: `KONTAKTS Factory 0.2.4-r1`
Status: **CLOSED / PHYSICAL PASS for factory app-only rebaseline and live OTA progress UI**

## Physical result

The user physically flashed the app-only factory image to the real ESP32-8048S043 board at the factory offset `0x20000` and reported that it worked correctly.

Confirmed on hardware:

- `KONTAKTS-Factory-0.2.4-r1-app-only.bin` flashes successfully at `0x20000`;
- the board boots and operates correctly after the factory rebaseline;
- OTA download progress now reports intermediate percentage values instead of only `1%` and `100%`;
- the graphical OTA progress slider advances during the download;
- the corrected progress implementation is therefore physically validated, not only CI/build validated.

User confirmation: “Отлично все сработало отлично в том числе и проценты загрузки и ползунок загрузки! Так что это можешь закрывать”.

This closes the specific factory 0.2.4-r1 + live OTA progress milestone.

## Decision

Adopt `KONTAKTS Factory 0.2.4-r1` as the preferred current factory/recovery baseline for this development line, superseding historical factory 0.1.0 for new recovery rebaseline work.

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

The progress-fix build retains internal semantic version `0.2.4` but differs byte-for-byte from the historical 0.2.4 release image, so the factory package uses an explicit revision suffix:

- `KONTAKTS-Factory-0.2.4-r1-app-only.bin`
- `KONTAKTS-Factory-0.2.4-r1-full-bootstrap.bin`

## Built binaries

Source CI run: `34234598716`
Artifact: `app06-platform-v0.2.4`
CI result: **BUILD PASS**

### App-only factory image — PHYSICAL PASS

Source artifact file: `app06-ota-v0.2.4.bin`
Renamed package: `KONTAKTS-Factory-0.2.4-r1-app-only.bin`
Size: 1,506,816 bytes
SHA-256: `c1d2ca68d57231307e9f2fe841780ee0f3a8526b0ae2767da83e61ed5cb52d71`

Recommended for rebaselining an already initialized development board.

Flash only at factory offset `0x20000`.

This preserves the existing NVS credentials, OTA slots, SPIFFS `/storage`, widgets and user files, provided the flashing tool does not perform a whole-chip erase.

Example:

```text
py -m esptool --chip esp32s3 --port COM12 --baud 921600 write_flash 0x20000 KONTAKTS-Factory-0.2.4-r1-app-only.bin
```

### Full bootstrap image — BUILD PASS, not separately physically certified here

Source artifact file: `app06-ota-recovery-v0.2.4-full.bin`
Renamed package: `KONTAKTS-Factory-0.2.4-r1-full-bootstrap.bin`
Size: 1,637,888 bytes
SHA-256: `728a7a78792ee1663c97aab353141d0eece5f2f20d8804ffea32aa47d65a0044`

The size equals `0x20000 + application_size`, confirming that the application is placed at the factory offset in the merged image.

Use this only for a blank board / deliberate clean bootstrap. Because the merged file spans from address 0 through the end of the factory application, padding in regions between the bootloader/partition table/application can overwrite or blank early data regions such as NVS and OTA metadata. It is therefore not the preferred rebaseline path for a configured board.

## Gate accounting

Closed by physical evidence:

- factory app-only image flash at `0x20000`: **PASS**;
- post-flash boot/runtime: **PASS**;
- live OTA percentage progression: **PASS**;
- graphical OTA progress slider: **PASS**;
- source build/CI: **PASS**.

Not inferred from this confirmation alone:

- destructive full-bootstrap image on a blank board;
- every rollback/recovery failure path;
- every storage failure path.

Those remain independent regression gates and are not required to keep this specific milestone closed.
