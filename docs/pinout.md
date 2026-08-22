# Pinout working document

This file separates **reported** pin mappings from future **verified** mappings.

## Reported 800x480 RGB panel mapping

Status: `REPORTED ONLY` until physically validated on our specimen.

| Signal | GPIO |
|---|---:|
| DE | 40 |
| HSYNC | 39 |
| VSYNC | 41 |
| PCLK | 42 |
| Backlight | 2 |
| R0..R4 | 45, 48, 47, 21, 14 |
| G0..G5 | 5, 6, 7, 15, 16, 4 |
| B0..B4 | 8, 3, 46, 9, 1 |

## Reported touch mapping

Status: `REPORTED ONLY` until physically validated.

| Signal | GPIO |
|---|---:|
| I2C SDA | 19 |
| I2C SCL | 20 |
| GT911 address | 0x5D |

## Validation checklist

- [ ] backlight GPIO confirmed;
- [ ] RGB data order confirmed;
- [ ] PCLK polarity confirmed;
- [ ] visible full-screen colors stable;
- [ ] GT911 detected at expected address;
- [ ] touch coordinates match rendered targets;
- [ ] orientation transform documented.
