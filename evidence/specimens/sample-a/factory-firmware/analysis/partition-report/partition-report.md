# Firmware partition report

This report is generated from a local factory flash dump. The dump itself is not committed.

## Input

- File: `evidence\specimens\sample-a\factory-firmware\factory-flash-16mb.bin`
- Size: `16777216` bytes / `0x1000000`
- SHA-256: `3007e5a223cd70dd9e53746c899ba25af24721c68f1cfc69ab8a8ce3d3e6eb4c`
- Partition table offset: `0x00008000`

## Partition hashes

| # | Type | Subtype | Label | Offset | Size | End | SHA-256 | Entropy | Strings | Note |
|---:|---|---|---|---:|---:|---:|---|---:|---:|---|
| 0 | data | nvs | `nvs` | 0x00009000 | 0x5000 | 0x0000E000 | `1f55ffcddc1fce4d4ab43d09da1f8e58730a19bf3aadd78331c3eaaa8b9b4410` | -0.000 | 0 | erased |
| 1 | data | ota | `otadata` | 0x0000E000 | 0x2000 | 0x00010000 | `f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0` | 0.019 | 0 |  |
| 2 | app | ota_0 | `app0` | 0x00010000 | 0x140000 | 0x00150000 | `da24ca396977588442318ada1fd10e727c1886c0ebdd147a15317a3afdfbc763` | 3.744 | 2618 | ESP image, 6 segments |
| 3 | app | ota_1 | `app1` | 0x00150000 | 0x140000 | 0x00290000 | `35805909a516a528e396b6ea22dab437b6f1c703afe92e92dddfa0243bfae738` | -0.000 | 0 | erased |
| 4 | data | spiffs | `spiffs` | 0x00290000 | 0x170000 | 0x00400000 | `2a0ad65a735f963eca113e6599151b37d2daacc43813d8fd3a49ec24f8f59c96` | -0.000 | 0 | erased |

## App-slot comparison

- `app0` vs `app1`: **DIFFERENT**

## ESP image summaries

### Bootloader candidate at 0x00000000

```text
segments       : 3
spi_mode       : 0x02
spi_size_freq  : 0x2F
entry          : 0x403B61D8
wp_pin         : 0xEE
drive          : 000000
chip_id        : 0x0009
min_chip_rev   : 0
max_chip_rev   : 0
hash_appended  : 0x01
parsed_length  : 0x3A04
valid_segments : True
```

| # | File offset | Load address | Length |
|---:|---:|---:|---:|
| 0 | 0x00000020 | 0x3FCD0108 | 0x43C |
| 1 | 0x00000464 | 0x403B6000 | 0xBD0 |
| 2 | 0x0000103C | 0x403BA000 | 0x29C8 |

### Partition `app0` at 0x00010000

```text
segments       : 6
spi_mode       : 0x02
spi_size_freq  : 0x4F
entry          : 0x40376810
wp_pin         : 0xEE
drive          : 000000
chip_id        : 0x0009
min_chip_rev   : 0
max_chip_rev   : 0
hash_appended  : 0x01
parsed_length  : 0x86C28
valid_segments : True
```

| # | File offset | Load address | Length |
|---:|---:|---:|---:|
| 0 | 0x00010020 | 0x3C050020 | 0x29A8C |
| 1 | 0x00039AB4 | 0x3FC91FE0 | 0x2B2C |
| 2 | 0x0003C5E8 | 0x40374000 | 0x3A30 |
| 3 | 0x00040020 | 0x42000020 | 0x4C638 |
| 4 | 0x0008C660 | 0x40377A30 | 0xA5B0 |
| 5 | 0x00096C18 | 0x50000000 | 0x10 |


## Boundary

This report proves partition structure, per-partition hashes and ESP-image metadata. It does not prove runtime behavior or factory-test entry paths.
