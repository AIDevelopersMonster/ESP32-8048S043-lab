#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "ESP32_8048S043_Pins.h"

struct ESP32_8048S043_TouchPoint {
    bool touched = false;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t rawX = 0;
    uint16_t rawY = 0;
    uint16_t size = 0;
    uint8_t trackId = 0;
    uint8_t status = 0;
};

class ESP32_8048S043_Touch {
public:
    bool begin(TwoWire &wire = Wire);
    bool read(ESP32_8048S043_TouchPoint &point);

    bool ready() const { return ready_; }
    uint8_t address() const { return address_; }
    uint16_t firmwareVersion() const { return firmwareVersion_; }
    uint16_t resolutionX() const { return resolutionX_; }
    uint16_t resolutionY() const { return resolutionY_; }
    uint8_t lastStatus() const { return lastStatus_; }
    uint32_t statusReads() const { return statusReads_; }
    uint32_t readyReads() const { return readyReads_; }
    uint32_t zeroPointReadyReads() const { return zeroPointReadyReads_; }
    uint32_t readFailures() const { return readFailures_; }
    uint32_t pointFailures() const { return pointFailures_; }
    uint32_t acceptedPoints() const { return acceptedPoints_; }
    uint32_t filteredUpdates() const { return filteredUpdates_; }
    int interruptLevel() const;

private:
    bool readReg(uint16_t reg, uint8_t *data, size_t len);
    bool writeRegByte(uint16_t reg, uint8_t value);
    void resetController();
    bool probeController();
    void clearStatus();
    void mapRawToScreen(uint16_t rawX, uint16_t rawY, uint16_t &x, uint16_t &y) const;
    void acceptFiltered(uint16_t x, uint16_t y, uint16_t &filteredX, uint16_t &filteredY);
    void releaseIfStale(uint32_t nowMs);

    TwoWire *wire_ = &Wire;
    bool ready_ = false;
    uint8_t address_ = 0;
    uint16_t firmwareVersion_ = 0;
    uint16_t resolutionX_ = 0;
    uint16_t resolutionY_ = 0;
    uint8_t lastStatus_ = 0;

    uint32_t statusReads_ = 0;
    uint32_t readyReads_ = 0;
    uint32_t zeroPointReadyReads_ = 0;
    uint32_t readFailures_ = 0;
    uint32_t pointFailures_ = 0;
    uint32_t acceptedPoints_ = 0;
    uint32_t filteredUpdates_ = 0;
    uint32_t lastTouchMs_ = 0;

    bool filterReady_ = false;
    float filteredX_ = 0.0f;
    float filteredY_ = 0.0f;
    uint16_t lastX_ = 0;
    uint16_t lastY_ = 0;
};

class ESP32_8048S043 {
public:
    bool begin();
    void printBoardInfo(Stream &out) const;
    const char *profileId() const { return "esp32-8048s043c-i-reference"; }
};
