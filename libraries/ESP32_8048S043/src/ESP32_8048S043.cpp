#include "ESP32_8048S043.h"

bool ESP32_8048S043::begin() {
    return true;
}

void ESP32_8048S043::printBoardInfo(Stream &out) const {
    out.println("============================================================");
    out.println("ESP32-8048S043 Lab / 01_BoardInfo");
    out.println("Profile: esp32-8048s043c-i-reference");
    out.println("Profile status: REPORTED ONLY until physically validated");
    out.println("------------------------------------------------------------");
    out.printf("Chip model      : %s\n", ESP.getChipModel());
    out.printf("Chip revision   : %u\n", ESP.getChipRevision());
    out.printf("CPU frequency   : %u MHz\n", ESP.getCpuFreqMHz());
    out.printf("Flash size      : %lu bytes\n", static_cast<unsigned long>(ESP.getFlashChipSize()));
    out.printf("PSRAM size      : %lu bytes\n", static_cast<unsigned long>(ESP.getPsramSize()));
    out.printf("Free heap       : %lu bytes\n", static_cast<unsigned long>(ESP.getFreeHeap()));
    out.printf("Free PSRAM      : %lu bytes\n", static_cast<unsigned long>(ESP.getFreePsram()));
    out.println("------------------------------------------------------------");
    out.println("Reported display: 800x480 RGB/DPI");
    out.printf("Reported touch  : GT911 I2C SDA=%d SCL=%d ADDR=0x%02X\n",
               esp32_8048s043::pins::TOUCH_SDA,
               esp32_8048s043::pins::TOUCH_SCL,
               esp32_8048s043::pins::TOUCH_GT911_ADDR);
    out.println("============================================================");
}
