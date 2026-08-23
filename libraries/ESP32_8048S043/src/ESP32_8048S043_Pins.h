#pragma once

#include <Arduino.h>

namespace esp32_8048s043::pins {

// ESP32-8048S043 / Jingcai ESP32-8048S043C-I source-backed profile.
// Status: SOURCE-BACKED / OWN MINIMAL TESTS STILL REQUIRED.
// See docs/pinout.md and hardware/SCHEMATIC_BOM_RESEARCH.md.

constexpr int LCD_WIDTH = 800;
constexpr int LCD_HEIGHT = 480;

constexpr int BACKLIGHT = 2;

constexpr int RGB_DE = 40;
constexpr int RGB_HSYNC = 39;
constexpr int RGB_VSYNC = 41;
constexpr int RGB_PCLK = 42;

constexpr int RGB_R0 = 45;
constexpr int RGB_R1 = 48;
constexpr int RGB_R2 = 47;
constexpr int RGB_R3 = 21;
constexpr int RGB_R4 = 14;

constexpr int RGB_G0 = 5;
constexpr int RGB_G1 = 6;
constexpr int RGB_G2 = 7;
constexpr int RGB_G3 = 15;
constexpr int RGB_G4 = 16;
constexpr int RGB_G5 = 4;

constexpr int RGB_B0 = 8;
constexpr int RGB_B1 = 3;
constexpr int RGB_B2 = 46;
constexpr int RGB_B3 = 9;
constexpr int RGB_B4 = 1;

constexpr int TOUCH_SDA = 19;
constexpr int TOUCH_SCL = 20;
constexpr int TOUCH_RST = 38;
constexpr int TOUCH_INT = 18;  // optional / board-link dependent
constexpr uint8_t TOUCH_GT911_ADDR = 0x5D;
constexpr uint8_t TOUCH_GT911_ADDR_ALT = 0x14;

constexpr int SD_CS = 10;
constexpr int SD_MOSI = 11;
constexpr int SD_CLK = 12;
constexpr int SD_MISO = 13;

}  // namespace esp32_8048s043::pins
