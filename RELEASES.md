# Release protocol

This project will use separate release channels.

| Channel | Tag pattern | Output | Status rule |
|---|---|---|---|
| Arduino BSP | `arduino-v*` | installable ZIP | CI compile + README status sync |
| Web Flasher | `main` / Pages | ESP Web Tools site | only non-destructive validated targets |
| GitHub OTA | `ota-v*` | `.bin` + manifest | requires full physical OTA PASS before stable wording |
| Widget Runtime | future | JSON schema + firmware | schema validation + persistence evidence |

Before every release, update:

1. root README status;
2. library README status;
3. example README status;
4. release notes;
5. video/evidence links.
