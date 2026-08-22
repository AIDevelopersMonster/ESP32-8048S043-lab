#pragma once

#include <Arduino.h>
#include "ESP32_8048S043_Pins.h"

class ESP32_8048S043 {
public:
    bool begin();
    void printBoardInfo(Stream &out) const;
    const char *profileId() const { return "esp32-8048s043c-i-reference"; }
};
