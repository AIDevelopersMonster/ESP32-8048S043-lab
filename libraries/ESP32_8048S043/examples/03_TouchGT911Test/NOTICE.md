# Notice for 03_TouchGT911Test

This example is an integration/adaptation for the `ESP32_8048S043` Arduino library.

It is **not** a blind full-file copy, but it intentionally follows and adapts a known-good public ESP32-8048S043C display/touch test pattern.

Reference project:

```text
ESP32-S3_Project by Tommi / tome9111991
https://github.com/tome9111991/ESP32-S3_Project
```

Reference files used for engineering comparison:

```text
ESP32-8048S043C/BOARD_CODING_NOTES.md
ESP32-8048S043C/displaytest/displaytest.ino
```

The following engineering ideas/patterns were adopted or adapted:

```text
GT911 polling mode instead of relying on the INT line;
GT911 status register 0x814E;
GT911 point data start register 0x814F;
GT911 point packet layout: track id, x, y, touch size;
little-endian x/y decoding after the track id byte;
status clear by writing 0x00 back to 0x814E;
static display screen plus throttled marker updates;
initial calibration seed for mapping compressed GT911 raw values to 800x480 screen coordinates.
```

Our integration changes include:

```text
project-specific header and PASS boundary;
use of the local ESP32_8048S043 pin header;
Sample A serial diagnostics;
repository-specific example layout;
English engineering screen labels;
status and evidence workflow aligned with ESP32-8048S043-lab;
separate boundary between low-level GT911 validation and later LVGL integration.
```

Required notice from the reference project:

```text
Required Notice: Based on ESP32-S3_Project by Tommi / tome9111991: https://github.com/tome9111991/ESP32-S3_Project
```

The referenced project states that it is licensed under the PolyForm Noncommercial License 1.0.0. Keep that in mind before reusing this example in commercial contexts.

For a future clean-room/permissive version, rederive the GT911 register handling from the controller documentation and Sample A serial evidence, and replace the calibration seed with values measured directly on Sample A.
