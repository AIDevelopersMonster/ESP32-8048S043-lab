# Sample A factory firmware analysis summary

Status: `FIRST-PASS DONE`.

This file records the reproducible first-pass analysis result from the local factory dump. The original `.bin` dump is not committed.

## Input

```text
File      : evidence/specimens/sample-a/factory-firmware/factory-flash-16mb.bin
Size      : 16777216 bytes / 0x01000000
SHA-256   : 3007e5a223cd70dd9e53746c899ba25af24721c68f1cfc69ab8a8ce3d3e6eb4c
Tool      : tools/analysis/firmware_scan.py
```

## Reproduction command

From repository root, after a valid local factory dump exists:

```powershell
py tools\analysis\firmware_scan.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis
```

## Console result

```text
Input: evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin
Size: 16777216 bytes / 0x1000000
SHA-256: 3007e5a223cd70dd9e53746c899ba25af24721c68f1cfc69ab8a8ce3d3e6eb4c
ESP image header candidates: 2
  0x00000000: segments=3 spi_mode=0x02 size_freq=0x2F entry=0x403B61D8
  0x00010000: segments=6 spi_mode=0x02 size_freq=0x4F entry=0x40376810
Partition table candidates: 1
  table @ 0x00008000: 5 entries
    00 data/nvs off=0x00009000 size=0x5000 label=nvs
    01 data/ota off=0x0000E000 size=0x2000 label=otadata
    02 app/ota_0 off=0x00010000 size=0x140000 label=app0
    03 app/ota_1 off=0x00150000 size=0x140000 label=app1
    04 data/spiffs off=0x00290000 size=0x170000 label=spiffs
Printable strings >=5 chars: 2689
Wrote analysis to: evidence\specimens\sample-a\factory-firmware\analysis
```

## Partition map

| # | Type | Subtype | Offset | Size | Label | Initial interpretation |
|---:|---|---|---:|---:|---|---|
| 00 | data | nvs | 0x00009000 | 0x5000 | `nvs` | configuration / device data |
| 01 | data | ota | 0x0000E000 | 0x2000 | `otadata` | OTA boot selection metadata |
| 02 | app | ota_0 | 0x00010000 | 0x140000 | `app0` | application slot 0 |
| 03 | app | ota_1 | 0x00150000 | 0x140000 | `app1` | application slot 1 |
| 04 | data | spiffs | 0x00290000 | 0x170000 | `spiffs` | filesystem/data partition |

## Important observations

- The factory image uses an OTA-capable layout: `otadata`, `app0`, and `app1` are present.
- The app slots are each `0x140000` bytes, so future lab OTA examples must not assume larger default app slots without replacing the partition table.
- A `spiffs` partition starts at `0x00290000` and has size `0x170000`.
- Two ESP image headers were detected: one at flash start and one at `0x00010000`.
- Factory-test leads are not claimed yet. They require review of generated keyword hits and strings, plus a reproducible runtime entry path or observed behavior.

## Publication boundary

This summary is safe to publish because it contains only reproducible metadata and partition information. The original `.bin` dump and raw strings must remain local until reviewed.
