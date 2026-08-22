# Sample A factory firmware evidence

This folder is the local evidence area for the factory firmware shipped on Sample A.

## Status

```text
FACTORY DUMP        CAPTURED
DOUBLE-READ HASH    MATCH
PARTITION ANALYSIS  OPEN
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
analysis/factory-firmware-analysis.md
analysis/strings.txt
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
