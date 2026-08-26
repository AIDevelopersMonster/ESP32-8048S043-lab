#include "ESP32_8048S043.h"

#include <math.h>

namespace {
constexpr uint16_t REG_PRODUCT_ID = 0x8140;
constexpr uint16_t REG_FW_VERSION = 0x8144;
constexpr uint16_t REG_X_RESOLUTION = 0x8146;
constexpr uint16_t REG_Y_RESOLUTION = 0x8148;
constexpr uint16_t REG_STATUS = 0x814E;
constexpr uint16_t REG_POINT1 = 0x814F;

constexpr uint32_t I2C_HZ = 400000;
constexpr uint32_t RELEASE_HOLD_MS = 180;
constexpr uint32_t FILTER_RESET_MS = 450;
constexpr float FILTER_ALPHA = 0.26f;
constexpr int DEADBAND_PX = 4;

// Physical calibration observed on Sample A during 03_TouchGT911Test and
// early 10_LVGL_BasicUI validation. It maps the GT911 raw 480x272-ish
// coordinate space to the active 800x480 RGB panel orientation.
constexpr float CAL_X_RX = 1.65867031f;
constexpr float CAL_X_RY = -0.02261823f;
constexpr float CAL_X_C = 2.12817001f;
constexpr float CAL_Y_RX = 0.02082564f;
constexpr float CAL_Y_RY = 1.79517055f;
constexpr float CAL_Y_C = 10.62223816f;

uint16_t le16(const uint8_t *data) {
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}
}

bool ESP32_8048S043_Touch::begin(TwoWire &wire) {
    if (ready_) return true;

    wire_ = &wire;
    wire_->begin(esp32_8048s043::pins::TOUCH_SDA,
                 esp32_8048s043::pins::TOUCH_SCL,
                 I2C_HZ);
    wire_->setTimeOut(50);

    resetController();

    if (!probeController()) {
        ready_ = false;
        return false;
    }

    uint8_t fw[2] = {};
    if (readReg(REG_FW_VERSION, fw, sizeof(fw))) {
        firmwareVersion_ = le16(fw);
    }

    uint8_t xres[2] = {};
    uint8_t yres[2] = {};
    if (readReg(REG_X_RESOLUTION, xres, sizeof(xres))) {
        resolutionX_ = le16(xres);
    }
    if (readReg(REG_Y_RESOLUTION, yres, sizeof(yres))) {
        resolutionY_ = le16(yres);
    }

    clearStatus();
    ready_ = true;
    return true;
}

bool ESP32_8048S043_Touch::read(ESP32_8048S043_TouchPoint &point) {
    point = ESP32_8048S043_TouchPoint{};
    if (!ready_ || address_ == 0 || !wire_) return false;

    const uint32_t nowMs = millis();

    uint8_t status = 0;
    if (!readReg(REG_STATUS, &status, 1)) {
        ++readFailures_;
        releaseIfStale(nowMs);
        return false;
    }

    ++statusReads_;
    lastStatus_ = status;
    point.status = status;

    if ((status & 0x80U) == 0) {
        releaseIfStale(nowMs);
        return true;
    }

    ++readyReads_;
    const uint8_t points = status & 0x0FU;
    if (points == 0 || points > 5) {
        if (points == 0) ++zeroPointReadyReads_;
        clearStatus();
        releaseIfStale(nowMs);
        return true;
    }

    uint8_t raw[8] = {};
    const bool ok = readReg(REG_POINT1, raw, sizeof(raw));
    clearStatus();

    if (!ok) {
        ++pointFailures_;
        releaseIfStale(nowMs);
        return false;
    }

    const uint8_t trackId = raw[0];
    const uint16_t rawX = le16(&raw[1]);
    const uint16_t rawY = le16(&raw[3]);
    const uint16_t touchSize = le16(&raw[5]);

    uint16_t mappedX = 0;
    uint16_t mappedY = 0;
    mapRawToScreen(rawX, rawY, mappedX, mappedY);

    uint16_t filteredX = mappedX;
    uint16_t filteredY = mappedY;
    acceptFiltered(mappedX, mappedY, filteredX, filteredY);

    point.touched = true;
    point.x = filteredX;
    point.y = filteredY;
    point.rawX = rawX;
    point.rawY = rawY;
    point.size = touchSize;
    point.trackId = trackId;
    point.status = status;

    ++acceptedPoints_;
    lastTouchMs_ = nowMs;
    return true;
}

int ESP32_8048S043_Touch::interruptLevel() const {
    return digitalRead(esp32_8048s043::pins::TOUCH_INT);
}

bool ESP32_8048S043_Touch::readReg(uint16_t reg, uint8_t *data, size_t len) {
    if (!wire_ || !data || len == 0 || address_ == 0) return false;

    wire_->beginTransmission(address_);
    wire_->write(static_cast<uint8_t>(reg >> 8));
    wire_->write(static_cast<uint8_t>(reg & 0xFF));
    if (wire_->endTransmission(false) != 0) return false;

    const int requested = static_cast<int>(len);
    const int received = wire_->requestFrom(static_cast<int>(address_), requested, static_cast<int>(true));
    if (received != requested) {
        while (wire_->available()) wire_->read();
        return false;
    }

    for (size_t i = 0; i < len; ++i) {
        if (!wire_->available()) return false;
        data[i] = static_cast<uint8_t>(wire_->read());
    }
    return true;
}

bool ESP32_8048S043_Touch::writeRegByte(uint16_t reg, uint8_t value) {
    if (!wire_ || address_ == 0) return false;

    wire_->beginTransmission(address_);
    wire_->write(static_cast<uint8_t>(reg >> 8));
    wire_->write(static_cast<uint8_t>(reg & 0xFF));
    wire_->write(value);
    return wire_->endTransmission() == 0;
}

void ESP32_8048S043_Touch::resetController() {
    pinMode(esp32_8048s043::pins::TOUCH_INT, INPUT_PULLUP);
    pinMode(esp32_8048s043::pins::TOUCH_RST, OUTPUT);
    digitalWrite(esp32_8048s043::pins::TOUCH_RST, LOW);
    delay(20);
    digitalWrite(esp32_8048s043::pins::TOUCH_RST, HIGH);
    delay(120);
}

bool ESP32_8048S043_Touch::probeController() {
    const uint8_t candidates[] = {
        esp32_8048s043::pins::TOUCH_GT911_ADDR,
        esp32_8048s043::pins::TOUCH_GT911_ADDR_ALT
    };

    for (uint8_t candidate : candidates) {
        address_ = candidate;
        uint8_t id[4] = {};
        if (readReg(REG_PRODUCT_ID, id, sizeof(id))) {
            return true;
        }
    }

    address_ = 0;
    return false;
}

void ESP32_8048S043_Touch::clearStatus() {
    writeRegByte(REG_STATUS, 0x00);
}

void ESP32_8048S043_Touch::mapRawToScreen(uint16_t rawX, uint16_t rawY, uint16_t &x, uint16_t &y) const {
    const float fx = CAL_X_RX * static_cast<float>(rawX) +
                     CAL_X_RY * static_cast<float>(rawY) +
                     CAL_X_C;
    const float fy = CAL_Y_RX * static_cast<float>(rawX) +
                     CAL_Y_RY * static_cast<float>(rawY) +
                     CAL_Y_C;

    x = static_cast<uint16_t>(clampInt(static_cast<int>(lroundf(fx)), 0, esp32_8048s043::pins::LCD_WIDTH - 1));
    y = static_cast<uint16_t>(clampInt(static_cast<int>(lroundf(fy)), 0, esp32_8048s043::pins::LCD_HEIGHT - 1));
}

void ESP32_8048S043_Touch::acceptFiltered(uint16_t x, uint16_t y, uint16_t &filteredX, uint16_t &filteredY) {
    if (!filterReady_) {
        filteredX_ = static_cast<float>(x);
        filteredY_ = static_cast<float>(y);
        lastX_ = x;
        lastY_ = y;
        filteredX = x;
        filteredY = y;
        filterReady_ = true;
        ++filteredUpdates_;
        return;
    }

    filteredX_ += FILTER_ALPHA * (static_cast<float>(x) - filteredX_);
    filteredY_ += FILTER_ALPHA * (static_cast<float>(y) - filteredY_);

    uint16_t nextX = static_cast<uint16_t>(clampInt(static_cast<int>(lroundf(filteredX_)), 0, esp32_8048s043::pins::LCD_WIDTH - 1));
    uint16_t nextY = static_cast<uint16_t>(clampInt(static_cast<int>(lroundf(filteredY_)), 0, esp32_8048s043::pins::LCD_HEIGHT - 1));

    if (abs(static_cast<int>(nextX) - static_cast<int>(lastX_)) < DEADBAND_PX) {
        nextX = lastX_;
    }
    if (abs(static_cast<int>(nextY) - static_cast<int>(lastY_)) < DEADBAND_PX) {
        nextY = lastY_;
    }

    if (nextX != lastX_ || nextY != lastY_) {
        ++filteredUpdates_;
    }

    lastX_ = nextX;
    lastY_ = nextY;
    filteredX = nextX;
    filteredY = nextY;
}

void ESP32_8048S043_Touch::releaseIfStale(uint32_t nowMs) {
    if (lastTouchMs_ != 0 && nowMs - lastTouchMs_ > FILTER_RESET_MS) {
        filterReady_ = false;
    }

    (void)RELEASE_HOLD_MS;
}
