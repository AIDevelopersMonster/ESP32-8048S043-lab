# Sample A / Arduino / 08_SDCardTest / read-only SD evidence

Status: `READ-ONLY SD PHYSICAL PASS CANDIDATE / SAMPLE A`.

Date: `2026-08-26`

Evidence source:

```text
Serial Monitor runtime log supplied by operator.
Video evidence supplied by operator.
```

Video evidence:

```text
https://youtube.com/shorts/vACvK85U0Lw
```

## Test target

```text
Example      : 08_SDCardTest
Purpose      : read-only microSD / TF SPI validation
Board target : ESP32-8048S043 Lab N16R8 FIXED
Mode         : read-only, no write/format/delete/rename operations
```

## Runtime baseline

```text
Runtime: chip=ESP32-S3 rev=2 flash=16777216 psram=8388608 freePsram=8384788
```

This confirms that the SD test was run after the local board profile PSRAM issue was fixed: Arduino runtime sees the 8 MB OPI PSRAM.

## Pin map under test

```text
CS   : GPIO10
MOSI : GPIO11
CLK  : GPIO12
MISO : GPIO13
```

Observed serial line:

```text
Pin map: CS=10 MOSI=11 CLK=12 MISO=13
```

## Mount evidence

```text
[MOUNT] Trying SD.begin(CS=10, CLK=12, MISO=13, MOSI=11, freq=10000000 Hz)
[PASS] SD mounted
```

The card mounted at the first attempted SPI frequency:

```text
Mounted frequency  : 10000000 Hz
```

## Card and filesystem evidence

```text
Card type          : SDHC/SDXC (3)
Card size          : 32220119040 bytes / 30.01 GB
Filesystem total   : 32211599360 bytes / 30.00 GB
Filesystem used    : 98304 bytes / 96.00 KB
Filesystem free    : 32211501056 bytes / 30.00 GB
```

## Directory listing evidence

Root directory opened and listed:

```text
[LIST] Directory: /
  DIR   System Volume Information
```

Nested directory opened and listed:

```text
[LIST] Directory: /System Volume Information
  FILE  IndexerVolumeGuid                        76 bytes / 76 B
```

The sketch did not find a regular non-empty file in the root directory for the optional preview, but this does not reduce the mount/list PASS candidate:

```text
[INFO] No regular non-empty file found in root for read preview; mount/list still valid.
```

## PASS candidate banner

```text
SD CARD READ-ONLY PHYSICAL PASS CANDIDATE
Mount + metadata + root listing completed. No writes performed.
```

## Stability evidence

ALIVE messages continued with SD status, 10 MHz mount frequency and PSRAM still available:

```text
[ALIVE] uptime=5s sd=PASS_CANDIDATE freq=10000000Hz freeHeap=343948 psram=8388608 freePsram=8355012
[ALIVE] uptime=70s sd=PASS_CANDIDATE freq=10000000Hz freeHeap=343948 psram=8388608 freePsram=8355012
[ALIVE] uptime=140s sd=PASS_CANDIDATE freq=10000000Hz freeHeap=343948 psram=8388608 freePsram=8355012
```

## Result

```text
SD SPI pin map             : PASS candidate, CS=10 MOSI=11 CLK=12 MISO=13 exercised
SD mount                   : PASS, mounted at 10 MHz
Card type readout          : PASS, SDHC/SDXC
Card size readout          : PASS, about 30.01 GB
Filesystem stats           : PASS, total/used/free reported
Root directory listing     : PASS
Nested directory listing   : PASS
Write operations           : NOT PERFORMED
Runtime stability          : PASS candidate, ALIVE lines continue to at least 140 seconds
Video evidence             : PRESENT, https://youtube.com/shorts/vACvK85U0Lw
Overall 08_SDCardTest      : READ-ONLY SD PHYSICAL PASS CANDIDATE / SAMPLE A
```

## Boundary

This evidence validates read-only SD mount, metadata and listing only.

It does not yet prove:

```text
filesystem write safety;
formatting;
long-duration SD stress;
concurrent SD + RGB/LVGL rendering;
SD-backed Web upload;
Widget Runtime storage;
serving files from SD over HTTP.
```
