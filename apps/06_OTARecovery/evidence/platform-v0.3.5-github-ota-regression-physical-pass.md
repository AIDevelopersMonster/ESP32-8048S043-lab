# Platform 0.3.5 GitHub OTA regression physical pass

Date: 2026-09-22  
Device: ESP32-8048S043 / Sample A  
Target platform: KONTAKTS Platform 0.3.5  
Release: `app06-v0.3.5`

## Test purpose

Re-check the current GitHub OTA path after the Web Flasher and USB Serial Terminal work, using the released 0.3.5 application image rather than a full-flash image.

The important persistence boundary remains:

```text
full Web Flasher install -> clean/full image path; Wi-Fi credentials may need setup again
GitHub OTA app image     -> writes an OTA application slot; NVS and saved Wi-Fi remain intact
```

## Physical result

Operator report on Sample A:

```text
GitHub manifest check              PASS
newer release detection            PASS
OTA application download           PASS
verified application install       PASS
reboot into OTA candidate          PASS
PENDING_VERIFY handling            PASS
on-device OTA controls             PASS
saved Wi-Fi / NVS persistence      PASS
automatic Wi-Fi reconnect          PASS
post-update platform operation     PASS
rollback / confirmation workflow   PASS
```

Classification:

`PLATFORM 0.3.5 GITHUB OTA REGRESSION PHYSICAL PASS`

## Evidence boundary

This record is based on the operator's physical acceptance report in the project session on 2026-09-22. A raw serial transcript for this particular regression run was not archived in this record.

Earlier App06 evidence files contain detailed serial transcripts for the same OTA mechanisms: manifest verification, HTTPS download, SHA-256 verification, PENDING_VERIFY, Wi-Fi/NVS persistence, rollback and factory recovery behavior.

The current run confirms that the OTA mechanism still operates correctly on the Platform 0.3.5 line after the later Widget Runtime, SD library, Help Center and serial-service additions.
