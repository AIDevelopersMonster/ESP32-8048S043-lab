# Sample A factory runtime serial boot log

Status: `RUNTIME SERIAL EVIDENCE`.

This log was captured from the preserved factory firmware booting on the physical Sample A board. It is serial-console evidence only; it is not a display or touch PASS.

## Captured serial output

```text
05:32:02.905 -> ESP-ROM:esp32s3-20210327
05:32:18.642 -> Build:Mar 27 2021
05:32:18.642 -> rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
05:32:18.642 -> SPIWP:0xee
05:32:18.642 -> mode:DIO, clock div:1
05:32:18.642 -> load:0x3fcd0108,len:0x43c
05:32:18.642 -> load:0x403b6000,len:0xbd0
05:32:18.642 -> load:0x403ba000,len:0x29c8
05:32:18.681 -> entry 0x403b61d8
05:32:18.759 -> LVGL Widgets Demo
05:32:20.964 -> E (2170) gpio: gpio_set_level(226): GPIO output gpio_num error
05:32:20.964 -> E (2181) gpio: gpio_set_level(226): GPIO output gpio_num error
05:32:20.964 -> E (2187) gpio: gpio_set_level(226): GPIO output gpio_num error
05:32:21.243 -> Setup done
```

## Correlation with static analysis

The bootloader load lines match the bootloader candidate previously parsed from the dump:

```text
entry          : 0x403B61D8
segment 0 load : 0x3FCD0108 len 0x43C
segment 1 load : 0x403B6000 len 0xBD0
segment 2 load : 0x403BA000 len 0x29C8
```

The runtime application banner confirms the static string finding:

```text
LVGL Widgets Demo
```

## Runtime findings

Confirmed by serial boot log:

- the factory bootloader starts from SPI flash;
- the bootloader jumps to entry `0x403b61d8`;
- the factory application starts and prints `LVGL Widgets Demo`;
- setup reaches `Setup done`;
- the application reports repeated invalid GPIO output attempts on GPIO `226`.

## Interpretation

The preserved factory firmware is now identified with stronger evidence as:

```text
Factory app = runtime-booting Arduino/LVGL Widgets Demo in app0
```

The repeated error:

```text
gpio_set_level(226): GPIO output gpio_num error
```

means the firmware attempts to drive an invalid GPIO number at runtime. The exact source is not proven from this log alone. Possible areas to investigate include backlight, reset, optional panel control, touch reset/interrupt, or a library/configuration placeholder. Do not treat this as a hardware failure without additional evidence.

## Boundary

This is runtime serial evidence, not a physical display/touch validation. It does not prove:

- RGB panel output is correct;
- touch controller identity;
- touch behavior;
- exact pin mapping;
- factory diagnostic mode.
