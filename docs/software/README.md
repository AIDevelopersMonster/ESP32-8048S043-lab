# Software tools

Planned toolchains:

- Arduino IDE / Arduino CLI for approachable examples;
- PlatformIO for repeatable local builds;
- ESP-IDF for lower-level RGB panel and GT911 references;
- esptool for passive chip identity, flash ID and factory firmware preservation;
- ESP Web Tools for browser flashing after safe firmware targets exist.

Do not require a local COM number in committed documentation.

## First esptool workflow

The first firmware-related workflow is deliberately read-only:

```powershell
.\tools\windows\dump_factory_firmware.ps1 -Port COM7 -SizeMB 16 -Reads 2
```

It reads the complete factory flash twice and compares SHA-256 before any lab firmware is flashed.

Then run:

```powershell
py tools\analysis\firmware_scan.py `
  evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin `
  --out evidence\specimens\sample-a\factory-firmware\analysis
```

See [`../firmware/README.md`](../firmware/README.md) for binary handling, third-party firmware and factory-firmware rules.
