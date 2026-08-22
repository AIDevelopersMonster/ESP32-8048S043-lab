# Sample A factory firmware evidence

This folder is the local evidence area for the factory firmware shipped on Sample A.

## Status

```text
FACTORY DUMP        OPEN
DOUBLE-READ HASH    OPEN
PARTITION ANALYSIS  OPEN
FACTORY TEST LEADS  OPEN
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
.\tools\windows\dump_factory_firmware.ps1 -Port COM7 -SizeMB 16 -Reads 2
```

## Analysis command

```powershell
py tools\analysis\firmware_scan.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis
```

## PASS boundary

This folder can support later claims, but it does not itself create a hardware PASS. It only preserves evidence for analysis before the board is overwritten.
