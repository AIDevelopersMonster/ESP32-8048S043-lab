#pragma once

#include <stdint.h>

// ESP32-8048S043 Lab experimental Arduino variant aliases.
// Status: SKELETON / NOT YET USED BY A SUPPORTED BOARD PACKAGE.
//
// Keep only standard Arduino-level aliases here. Complex board peripherals such
// as RGB panel timing, GT911 registers and backlight policy remain in the
// ESP32_8048S043 library.

#define EXTERNAL_NUM_INTERRUPTS 46
#define NUM_DIGITAL_PINS        49
#define NUM_ANALOG_INPUTS       20

#define LED_BUILTIN             2
#define BUILTIN_LED             LED_BUILTIN

static const uint8_t SDA = 19;
static const uint8_t SCL = 20;

static const uint8_t SS    = 10;
static const uint8_t MOSI  = 11;
static const uint8_t MISO  = 13;
static const uint8_t SCK   = 12;

static const uint8_t TX = 43;
static const uint8_t RX = 44;

#define ESP32_8048S043_HAS_RGB_PANEL 1
#define ESP32_8048S043_HAS_GT911 1
#define ESP32_8048S043_LCD_WIDTH 800
#define ESP32_8048S043_LCD_HEIGHT 480
#define ESP32_8048S043_TOUCH_GT911 1

// Display/touch/storage pins are intentionally duplicated from the runtime
// library profile for board-package experiments. The authoritative maintained
// source remains libraries/ESP32_8048S043/src/ESP32_8048S043_Pins.h.
#define ESP32_8048S043_BACKLIGHT 2

#define ESP32_8048S043_TOUCH_SDA 19
#define ESP32_8048S043_TOUCH_SCL 20
#define ESP32_8048S043_TOUCH_RST 38
#define ESP32_8048S043_TOUCH_INT 18

#define ESP32_8048S043_SD_CS   10
#define ESP32_8048S043_SD_MOSI 11
#define ESP32_8048S043_SD_CLK  12
#define ESP32_8048S043_SD_MISO 13
