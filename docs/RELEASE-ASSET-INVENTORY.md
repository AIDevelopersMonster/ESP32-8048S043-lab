# Current release asset inventory — 2026-09-29

This inventory is a checked snapshot of the public release assets used by the current KONTAKTS Platform, SD library and Android client.

The authoritative value for each binary is the digest attached to the GitHub Release asset. Documentation and the Web Flasher catalog must match these values.

## KONTAKTS Platform 0.3.9

Release tag:

```text
app06-v0.3.9
```

| Asset | Size | SHA-256 |
|---|---:|---|
| `app06-factory-recovery-v0.1.0-full.bin` | 1,126,656 | `c241682145ff84db8f930d7a80997ea9a60583793456ab3a5cef4a457b01787d` |
| `app06-ota-recovery-v0.3.9-full.bin` | 2,014,912 | `20d3cd2675e49fa84e1be6ff9dbf0c01a4d3786234138cf4511014b9b4fdbc76` |
| `app06-ota.bin` | 1,883,840 | `061630b7871e045ebcded0837893cece455db95d093ed70fdf3dfba9ed8ccb26` |
| `app06-ota.json` | 319 | `f43ee46c9e51d3ea3b2cdc60d30c458f88144e05b947643f4ef3c3ed6f6715ab` |

The full image SHA is the physically accepted Platform 0.3.9 reference used by the Web Flasher.

## Previous platform 0.3.7

Release tag:

```text
app06-v0.3.7
```

| Asset | Size | SHA-256 |
|---|---:|---|
| `app06-factory-recovery-v0.1.0-full.bin` | 1,126,656 | `c241682145ff84db8f930d7a80997ea9a60583793456ab3a5cef4a457b01787d` |
| `app06-ota-recovery-v0.3.7-full.bin` | 1,795,104 | `f12cc349dcbaa6b2f5fcf9eea0b9bb03bae57634a57ccdb7ec20fa4c2d2071a6` |
| `app06-ota.bin` | 1,664,032 | `014e04359f0e357856637331283fa6d9b47e174d197d382e3b11ef923f0eb291` |
| `app06-ota.json` | 319 | `bda9375c705629e633bf23a9d7abe493e314af5452fab9ff1354ec7808516b89` |

## KONTAKTS SD Application Library — current

Mutable release tag:

```text
app09-sd-current
```

Current tag ref at audit time:

```text
4cb8440c08daca50a57c8f3eb91238250f947797
```

| Asset | Size | SHA-256 |
|---|---:|---|
| `kontakts-sd-library.json` | 399 | `3de2a824b6392a24de162a82fa4a12462306898e07a9e875463e8681c64bcd27` |
| `kontakts-sd-library.zip` | 39,915 | `38c8e2f8b490eb18c7052c6f9ad878bf91561b57f94d71a5bd75a5faaab56ef7` |
| `widget-clock.zip` | 1,795 | `c61be5ec5a21ce6658c8301f97ce91845b9d3ec526656ce4d643e636ba0b6c67` |
| `widget-modbus-controller.zip` | 3,775 | `79d08c92200ffe049227317a479d08dc66ab247a9607718bea157b2f51429428` |
| `widget-usb-serial-terminal.zip` | 6,672 | `26dfa432445a7893cbbe361109296b63ab00b572410c211e10b981e108ee43dd` |
| `widget-weather.zip` | 3,254 | `50acfb9dce622daabf893e43df301b663619ca8fa720d1062214bf914d4cad39` |
| `widget-youtube-led.zip` | 1,875 | `d65bfb93ae62e1087d25e482b4cfb9f3dbc09aea69da3b7c64f36b76758e2b92` |
| `widget-youtube.zip` | 3,212 | `b29cc1ecaf6a6a48e5a1d0ae2513598816665f7eaaee619846e521c2f9f8ce9e` |

Important: `app09-sd-current` is intentionally mutable. Re-check this table whenever the SD release workflow republishes the tag.

## KONTAKTS Mobile App17 0.1.1

Release tag:

```text
app17-v0.1.1
```

| Asset | Size | SHA-256 |
|---|---:|---|
| `kontakts-mobile-app17-v0.1.1-debug.apk` | 832,235 | `5de40bc802438a35285077241da603b0441ead28179928fad0e15ea0285868d0` |
| `kontakts-mobile-app17-v0.1.1-debug.apk.sha256` | 119 | `99c0545a2f9cd5554a7198bac25ec208be8fb062231ce0e7b2dec2f45b658ab3` |

The APK is the exact physically tested CI artifact from workflow run `36507403489`.

## Audit result

At this checkpoint:

```text
Platform 0.3.9 full image     MATCH
Platform 0.3.9 OTA binary    MATCH
Platform 0.3.7 assets        MATCH
App17 APK                     MATCH
SD current ZIP                CATALOG CORRECTED
SD package ZIP hashes         CATALOG CORRECTED
```

The SD catalog previously carried hashes from an older mutable snapshot. It has been synchronized to the current `app09-sd-current` release assets.
