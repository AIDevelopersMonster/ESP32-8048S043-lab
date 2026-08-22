# Firmware preservation and analysis

This section tracks **factory firmware** and **third-party firmware** for the ESP32-8048S043 lab.

The repository may store scripts, hashes, partition reports, string scans and written analysis. It must not store proprietary or redistributable-unclear firmware binaries.

## Firmware classes

| Class | Meaning | Binary in Git? | Evidence allowed |
|---|---|---:|---|
| Factory firmware | Firmware shipped on the physical specimen | NO | SHA-256, dump log, partition table, strings report, screenshots/video |
| Third-party firmware | Firmware from another project/vendor/community source | NO unless license explicitly permits | Source URL, license, hash, analysis notes, compatibility notes |
| Lab firmware | Firmware built from this repository | YES through source; release binaries only through release policy | Build logs, release manifests, Web Flasher entries |

## Reproducible workflow

Reader-facing guide:

```text
docs/firmware/reproducible-factory-dump-and-analysis.md
```

This guide records the exact repeated workflow used for Sample A: double-read dump, SHA-256 comparison, local-only binary handling, and first-pass partition analysis.

## First factory dump workflow

Run this before flashing any experimental firmware.

From repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File ".\tools\windows\dump_factory_firmware.ps1" -Port COM12 -SizeMB 16 -Reads 2 -Baud 460800
```

Replace `COM12` with the real port. The script reads the full flash twice, calculates SHA-256 for every read and fails if repeated reads differ.

For Sample A, `460800` baud was used for the reproducible 16 MB dump. A previous `921600` attempt produced serial read corruption during the second read.

Default local output:

```text
evidence/specimens/sample-a/factory-firmware/
```

The generated `.bin` files are intentionally ignored by Git. Keep them locally or in private backups. Commit only safe metadata and analysis outputs.

Sample A factory SHA-256:

```text
3007E5A223CD70DD9E53746C899BA25AF24721C68F1CFC69AB8A8CE3D3E6EB4C
```

## Static first-pass analysis

After the dump succeeds:

```powershell
py tools\analysis\firmware_scan.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis
```

The scanner is conservative and offline-only. It does not modify the dump. It reports:

- full-image SHA-256;
- ESP image header candidates;
- partition table candidates;
- printable strings;
- keyword hits that may point to factory tests;
- 64 KiB entropy map.

Sample A first-pass partition layout:

```text
table @ 0x00008000: 5 entries
00 data/nvs    off=0x00009000 size=0x5000   label=nvs
01 data/ota    off=0x0000E000 size=0x2000   label=otadata
02 app/ota_0   off=0x00010000 size=0x140000 label=app0
03 app/ota_1   off=0x00150000 size=0x140000 label=app1
04 data/spiffs off=0x00290000 size=0x170000 label=spiffs
```

## Factory-test claim boundary

Static strings like `factory`, `test`, `lcd`, `touch`, `sd`, `wifi` or `gt911` are only leads. They do not prove runtime behavior.

A factory test becomes a project claim only when all are available:

1. named specimen;
2. preserved dump hash;
3. partition/string evidence;
4. reproducible entry path or observed runtime behavior;
5. log/video/photo evidence.

Until then use:

```text
POSSIBLE FACTORY TEST LEAD
```

not:

```text
PHYSICAL PASS
```

## Third-party firmware handling

For each external firmware record:

- source URL and project name;
- license or redistribution status;
- intended board variant;
- binary size and SHA-256 if obtained;
- known partition layout if visible;
- whether it is safe to flash on this specimen;
- whether it requires backup/restore preparation;
- final status: REFERENCE ONLY, ANALYZED, FLASHED, PHYSICAL PASS or PHYSICAL FAIL.

If the license is unknown, do not commit the binary and do not redistribute it through GitHub Releases.
