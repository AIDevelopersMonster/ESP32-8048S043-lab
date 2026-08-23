# Reproducible factory dump and analysis

This guide lets another reader reproduce the factory firmware preservation and first-pass analysis workflow.

The exact binary dump is not distributed from this repository. Every reader must dump their own physical specimen and compare their results.

## Requirements

- Windows PowerShell.
- Python available as `py`.
- `esptool` installed:

```powershell
py -m pip install --upgrade esptool
```

- Board connected through the correct COM port.

## Step 1 — identify the COM port

Use Device Manager or run the repository audit script if available. The Sample A run used `COM12`, but readers must use their own port.

## Step 2 — read the full factory flash twice

Recommended reliable command for a 16 MB dump:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\tools\windows\dump_factory_firmware.ps1" -Port COM12 -SizeMB 16 -Reads 2 -Baud 460800
```

Replace `COM12` with the real port.

Why `460800` baud? In the Sample A run, `921600` produced a serial read corruption during the second 16 MB read. Repeating at `460800` completed both reads and produced matching SHA-256 values.

Expected successful output shape:

```text
FACTORY DUMP  port=COM12  flash=16MB  reads=2  baud=460800
chip_id ... OK
flash_id ... OK
read_flash 1/2 ... OK
sha256 read1 <hash>
read_flash 2/2 ... OK
sha256 read2 <same hash>
MATCH all reads are identical
```

Sample A result:

```text
sha256 read1 3007E5A223CD70DD9E53746C899BA25AF24721C68F1CFC69AB8A8CE3D3E6EB4C
sha256 read2 3007E5A223CD70DD9E53746C899BA25AF24721C68F1CFC69AB8A8CE3D3E6EB4C
MATCH all reads are identical
```

## Step 3 — keep binaries local

The script writes local files under:

```text
evidence/specimens/sample-a/factory-firmware/
```

Do not commit these binary files:

```text
factory-flash-read1-16mb.bin
factory-flash-read2-16mb.bin
factory-flash-16mb.bin
factory-dump-*.log
```

The repository tracks only reviewed metadata, hashes and analysis summaries.

## Step 4 — run first-pass static analysis

```powershell
py tools\analysis\firmware_scan.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis
```

Expected Sample A result:

```text
Size: 16777216 bytes / 0x1000000
SHA-256: 3007e5a223cd70dd9e53746c899ba25af24721c68f1cfc69ab8a8ce3d3e6eb4c
ESP image header candidates: 2
Partition table candidates: 1
Printable strings >=5 chars: 2689
```

Sample A partition table:

```text
table @ 0x00008000: 5 entries
00 data/nvs    off=0x00009000 size=0x5000   label=nvs
01 data/ota    off=0x0000E000 size=0x2000   label=otadata
02 app/ota_0   off=0x00010000 size=0x140000 label=app0
03 app/ota_1   off=0x00150000 size=0x140000 label=app1
04 data/spiffs off=0x00290000 size=0x170000 label=spiffs
```

## Step 5 — run partition-level image investigation

Use the partition report tool to calculate per-partition hashes, compare app slots and summarize ESP image headers without committing binaries:

```powershell
py tools\analysis\firmware_partition_report.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis\partition-report
```

Expected outputs:

```text
evidence/specimens/sample-a/factory-firmware/analysis/partition-report/partition-report.md
evidence/specimens/sample-a/factory-firmware/analysis/partition-report/partition-table.csv
evidence/specimens/sample-a/factory-firmware/analysis/partition-report/partition-hashes.sha256.txt
```

Optional local-only binary extraction:

```powershell
py tools\analysis\firmware_partition_report.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis\partition-report `
  --extract
```

The `--extract` mode writes `.bin` files under `partitions-local-only/`. These files are for local reverse engineering only and must not be committed unless redistribution is explicitly permitted.

## Step 6 — review strings before publishing

The scanner generates `strings.txt`. Review it before committing or publishing because it may contain:

- Wi-Fi SSIDs;
- tokens or credentials;
- MAC addresses or unique identifiers;
- NVS content;
- device-specific configuration.

Do not publish raw strings until reviewed.

## Reproducibility rule

A result is reproducible when another reader can run the same commands on a named specimen and compare:

- flash size;
- SHA-256;
- image header candidates;
- partition table;
- per-partition SHA-256 values;
- app-slot comparison;
- scanner version / script commit;
- command line used.

If the SHA-256 differs, do not treat it as a failure by default. It may indicate another seller/OEM revision, different factory app, changed NVS, or a previously flashed board.
