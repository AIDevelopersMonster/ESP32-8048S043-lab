# ESP32-8048S043 schematic / BOM reconstruction

This note correlates the physically photographed board in this project with publicly available board-layout documentation and manufacturer/support material.

## 1. Strongest matching reference found

The closest match found is the annotated `4280S043_Layout` PDF published by macsbug for **ESP32-8048S043**, dated **2022-10-18**:

- https://macsbug.wordpress.com/2022/11/29/esp32-8048s043/
- https://macsbug.wordpress.com/wp-content/uploads/2022/10/4280s043_layout-2.pdf

The component placement, connector placement and reference designators match the photographed board extremely closely: TF1, P1-P4, USB1, LCD1, U1/U2/U3/U4/U5, Q1, T1/T2, D1/D2, JP1/JP2 and the ESP32-S3 module occupy the same locations.

This is therefore treated as a **high-confidence board-level reconstruction source**, but not as the original manufacturer's CAD schematic unless independently confirmed.

## 2. Manufacturer / distributor documentation located

HomeDing identifies this board family as a Jingcai / JCZN1688 ESP32-8048S043C and provides the manufacturer's support archive location:

- http://www.jczn1688.com/zlxz
- https://pan.jczn1688.com/pd/1/ESP32%20module/4.3inch_ESP32-8048S043.zip
- download password reported by HomeDing: `jczn1688`

TinyTronics lists the same **Jingcai ESP32-8048S043C-I**, revision **V1.2 or V1.3**, and exposes a support package named:

`006068_4.3inch_ESP32-8048S043-complete.zip`

Product page:
https://www.tinytronics.nl/en/development-boards/microcontroller-boards/with-wi-fi/jingcai-esp32-8048s043c-i-4.3-inch-tft-display-800%2A480-pixels-with-capacitive-touchscreen-esp32-s3

The distributor confirms CH340C, GT911, ESP32-S3, 16 MB flash and 8 MB PSRAM for the capacitive-touch version.

## 3. Reconstructed major-component BOM

| Ref. | Part / marking | Function | Evidence | Confidence |
|---|---|---|---|---|
| IC1 | ESP32-S3-WROOM-1-N16R8 | Main MCU module, 16 MB flash + 8 MB PSRAM | Layout PDF + photographed module family | High |
| U1 | XPT2046 | Resistive-touch controller footprint / fitted device on this PCB family | Annotated layout; package and placement match project photo | High |
| U2 | CH340C | USB-to-UART bridge | Annotated layout + distributor specification | High |
| U3 | AMS1117-3.3 | 3.3 V regulator, ESP rail | Annotated layout, SOT-223 placement | High |
| U4 | AMS1117-3.3 | 3.3 V regulator, TFT rail | Annotated layout, SOT-223 placement | High |
| U5 | marking `KDE2H`; likely LT1930-class boost converter | LCD LED backlight boost supply | Annotated layout / macsbug analysis | Medium: exact silicon vendor not confirmed |
| Touch FPC IC | GT911 | Capacitive touch controller | Layout / distributor / project macro photo | High |
| Q1 | AO3402 / AO3402-family N-MOSFET | Power switching around 5 V / 4.6 V rail | Annotated layout | Medium-high |
| T1 | `J3Y`, S9013-family NPN | CH340 auto-reset / boot circuitry | Annotated layout | High at circuit-function level |
| T2 | `J3Y`, S9013-family NPN | CH340 auto-reset / boot circuitry | Annotated layout | High at circuit-function level |
| D1 | 1N5819W | Schottky diode in 5 V / regulator path | Annotated layout | High |
| D2 | SS14 | Schottky diode in backlight boost stage | Annotated layout | High |
| L1 | 10 uH | Backlight boost inductor | Annotated layout | High |
| L2 | 0 ohm link | LCD DCLK link | Annotated layout / macsbug analysis | High |

### Important observation: both touch circuits exist on the PCB

The board layout contains the **XPT2046 resistive-touch section (U1)** and the capacitive-touch **GT911** interface. On the photographed board, the capacitive-touch controller is visible on the touch FPC, while U1 is also populated. This is consistent with public reports of ESP32-8048S043 boards where both devices are physically present even though the shipped panel uses capacitive touch.

Do not assume U1 is active in the current capacitive configuration; software/hardware testing is required.

## 4. Power architecture reconstructed from the matching layout

The matching board documentation shows approximately this structure:

```text
USB-C / P1 5 V
      |
      +--> D1 1N5819W / MOSFET switching area
      |
      +--> U3 AMS1117-3.3 --> 3.3V-ESP
      |
      +--> U4 AMS1117-3.3 --> 3.3V-TFT
      |
      +--> U5 boost converter + L1 + D2 --> LEDA ~15 V backlight rail
```

The annotated source reports approximately **15.6 V DC** at LEDA for the backlight boost stage.

JP1 is associated with the ESP 3.3 V rail and JP2 with the TFT 3.3 V rail in the reference layout.

## 5. Display / touch / SD wiring recovered

### RGB LCD

| Signal | GPIO |
|---|---:|
| DE | 40 |
| VSYNC | 41 |
| HSYNC | 39 |
| PCLK / DCLK | 42 |
| Backlight PWM | 2 |
| R0..R4 | 45, 48, 47, 21, 14 |
| G0..G5 | 5, 6, 7, 15, 16, 4 |
| B0..B4 | 8, 3, 46, 9, 1 |

The public sources disagree on the LCD-controller name: some distributor pages say **ILI9485**, while HomeDing and newer software ports identify an **ST7262-class RGB panel**. Because this is a raw RGB interface, firmware often does not communicate with a controller over SPI; the panel timing is the practically important part. Treat the exact panel-controller identity as **variant-dependent / not yet physically proven**.

### GT911 capacitive touch

| Signal | GPIO |
|---|---:|
| SDA | 19 |
| SCL | 20 |
| RESET | 38 |
| INT | 18 via optional/open link on the reference layout |
| I2C address | 0x5D or 0x14 depending on reset/address strap sequence |

### microSD / TF1

| Signal | GPIO |
|---|---:|
| CS | 10 |
| MOSI | 11 |
| CLK | 12 |
| MISO | 13 |

## 6. Passive-component values visible in the annotated layout

The 2022-10-18 reconstruction gives useful BOM-level values around the main functional blocks. These should be checked against the physical board before being promoted to a manufacturing BOM.

- R1: 10 kOhm (ESP EN timing network)
- C1: 0.1 uF (ESP EN timing network)
- R2: 10 kOhm (BOOT pull-up)
- R3, R4: 4.7 kOhm (GT911 I2C pull-ups)
- R5: 10 kOhm
- R6, R7: 100 ohm
- R8, R9: 10 kOhm
- R10: open / option link in reference layout
- R11: 100 ohm
- R12: 5.1 ohm
- R13, R14: backlight feedback / control network; verify exact values on board
- R15: 10 kOhm
- R16: 10 kOhm (DISP pull-up / option)
- R17: open on the reference capacitive-touch configuration
- C2: 0.1 uF
- C3-C6: XPT2046 touch-option capacitors; shown open for capacitive configuration
- C8/C9/C13/C14 and neighboring capacitors form USB/backlight/power decoupling; exact per-reference values are visible in the annotated PDF and should be transcribed during a dedicated BOM pass.

## 7. Exact-vs-similar board warning

`ESP32-8048S043`, `ESP32-8048S043C`, `ESP32-8048S043C_I`, and later JC/Jingcai variants are a family, not necessarily one immutable PCB revision. Public sources explicitly warn that manufacturer download packages sometimes do not exactly match newer shipped boards.

For this project the evidence priority is:

1. **our macro photographs / continuity measurements**;
2. same-layout 2022-10-18 annotated board reference;
3. Jingcai/JCZN1688 support archive for matching revision;
4. distributor listings;
5. software projects / related sibling boards.

## 8. Next actions to obtain a near-complete BOM / schematic

1. Download and archive `4.3inch_ESP32-8048S043.zip` from the JCZN1688 support server.
2. Download and archive TinyTronics `006068_4.3inch_ESP32-8048S043-complete.zip` if available.
3. Compare included schematic/layout/BOM files against the photographed board by reference designator.
4. Take straight-on macro photographs of U1, U2, U5 and the touch-FPC GT911 top markings.
5. Measure U3/U4 output rails and the U5 LEDA boost output.
6. Build `BOM.csv` only after resolving every fitted / DNP option for this exact board.

## Source links

- macsbug ESP32-8048S043 analysis: https://macsbug.wordpress.com/2022/11/29/esp32-8048s043/
- annotated board PDF: https://macsbug.wordpress.com/wp-content/uploads/2022/10/4280s043_layout-2.pdf
- HomeDing board notes and manufacturer archive pointer: https://homeding.github.io/boards/esp32s3/panel-8048S043.htm
- TinyTronics Jingcai ESP32-8048S043C-I page: https://www.tinytronics.nl/en/development-boards/microcontroller-boards/with-wi-fi/jingcai-esp32-8048s043c-i-4.3-inch-tft-display-800%2A480-pixels-with-capacitive-touchscreen-esp32-s3
- ESPHome Sunton ESP32-8048S043C device page: https://devices.esphome.io/devices/sunton-esp32-8048s043c/
