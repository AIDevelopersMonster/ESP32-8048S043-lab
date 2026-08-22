# Sample A factory firmware evidence

This folder is the local evidence area for the factory firmware shipped on Sample A.

## Status

```text
FACTORY DUMP        CAPTURED
DOUBLE-READ HASH    MATCH
PARTITION ANALYSIS  FIRST-PASS DONE
FACTORY TEST LEADS  OPEN
```

## Captured dump evidence

```text
Timestamp   : 20260822-195722
Port        : COM12
Baud        : 460800
Read size   : 16 MB / 0x01000000
Reads       : 2
SHA-256     : 3007E5A223CD70DD9E53746C899BA25AF24721C68F1CFC69AB8A8CE3D3E6EB4C
Result      : MATCH all reads are identical
```

Commit-safe hash record:

```text
evidence/specimens/sample-a/factory-firmware/factory-dump-20260822-195722.sha256.txt
```

## First-pass partition analysis

```text
Size        : 16777216 bytes / 0x01000000
SHA-256     : 3007e5a223cd70dd9e53746c899ba25af24721c68f1cfc69ab8a8ce3d3e6eb4c
Image heads : 0x00000000, 0x00010000
Table       : 0x00008000
Entries     : 5
Strings     : 2689 printable strings >=5 chars
```

Partition map:

| # | Type | Subtype | Offset | Size | Label |
|---:|---|---|---:|---:|---|
| 00 | data | nvs | 0x00009000 | 0x5000 | `nvs` |
| 01 | data | ota | 0x0000E000 | 0x2000 | `otadata` |
| 02 | app | ota_0 | 0x00010000 | 0x140000 | `app0` |
| 03 | app | ota_1 | 0x00150000 | 0x140000 | `app1` |
| 04 | data | spiffs | 0x00290000 | 0x170000 | `spiffs` |

Commit-safe summary:

```text
evidence/specimens/sample-a/factory-firmware/analysis/factory-firmware-analysis-summary.md
```

## Local-only files

The following files are expected locally but must not be committed:

```text
factory-flash-read1-16mb.bin
factory-flash-read2-16mb.bin
factory-flash-16mb.bin
factory-dump-*.log
```

Firmware binaries can be proprietary and may contain unique device data. Keep them outside Git history.

## Commit-safe files

The following outputs are normally safe to commit after review:

```text
factory-dump-*.sha256.txt
analysis/factory-firmware-analysis-summary.md
analysis/factory-firmware-analysis.md   # after review
analysis/strings.txt                    # only after sensitive-data review
```

Review `strings.txt` before publishing in case it contains credentials, MAC addresses, tokens, local Wi-Fi SSIDs or other device-specific information.

## Dump command

From repository root:

```powershell
.\tools\windows\dump_factory_firmware.ps1 -Port COM12 -SizeMB 16 -Reads 2 -Baud 460800
```

## Analysis command

```powershell
py tools\analysis\firmware_scan.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis
```

## PASS boundary

This folder can support later claims, but it does not itself create a hardware PASS. It preserves the original factory firmware before the board is overwritten and creates the baseline for partition/string analysis.
