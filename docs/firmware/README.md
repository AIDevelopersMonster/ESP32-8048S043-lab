# Firmware preservation and analysis

This section tracks **factory firmware** and **third-party firmware** for the ESP32-8048S043 lab.

The repository may store scripts, hashes, partition reports, string scans and written analysis. It must not store proprietary or redistributable-unclear firmware binaries.

## Firmware classes

| Class | Meaning | Binary in Git? | Evidence allowed |
|---|---|---:|---|
| Factory firmware | Firmware shipped on the physical specimen | NO | SHA-256, dump log, partition table, strings report, screenshots/video |
| Third-party firmware | Firmware from another project/vendor/community source | NO unless license explicitly permits | Source URL, license, hash, analysis notes, compatibility notes |
| Lab firmware | Firmware built from this repository | YES through source; release binaries only through release policy | Build logs, release manifests, Web Flasher entries |

## First factory dump workflow

Run this before flashing any experimental firmware.

From repository root:

```powershell
.\tools\windows\dump_factory_firmware.ps1 -Port COM7 -SizeMB 16 -Reads 2
```

Replace `COM7` with the real port. The script reads the full flash twice, calculates SHA-256 for every read and fails if repeated reads differ.

Default local output:

```text
evidence/specimens/sample-a/factory-firmware/
```

The generated `.bin` files are intentionally ignored by Git. Keep them locally or in private backups. Commit only safe metadata and analysis outputs.

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
