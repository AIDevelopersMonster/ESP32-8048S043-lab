/*
  ESP32-8048S043 Lab / 01_BoardInfo

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Purpose:
    First Arduino IDE smoke test and profile diagnostic for a physical
    ESP32-8048S043 / ESP32-8048S043C-I board.

  What this example checks:
    - sketch upload path through the board USB-UART interface;
    - serial monitor output at 115200 baud;
    - compile-time Arduino board/variant/build macros;
    - ESP32-S3 chip model and revision;
    - flash size reported by the Arduino runtime;
    - PSRAM size reported by Arduino and IDF heap APIs;
    - running and boot partitions;
    - basic runtime stability through periodic ALIVE messages.

  What this example does NOT check:
    - RGB display output;
    - GT911 touch;
    - SD card;
    - Wi-Fi/BLE;
    - Web server;
    - LVGL;
    - final BSP pinout.

  Important:
    Arduino IDE Tools menu values are not directly readable as menu text from a sketch.
    This example therefore prints the compile-time macros and runtime values that
    actually reached the firmware.

  Known-good reference profile for Sample A:
    - Board: ESP32S3 Dev Module or local ESP32-8048S043 Lab profile;
    - Flash Size: 16MB (128Mb);
    - Flash Mode: QIO 80MHz;
    - Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS);
    - PSRAM: OPI PSRAM;
    - Upload through CH340 / UART0 / Hardware CDC;
    - Serial Monitor: 115200 baud.

  Expected serial result:
    - Chip model should be ESP32-S3;
    - Flash should be about 16 MB;
    - PSRAM should be about 8 MB;
    - app0 partition should be about 3 MB;
    - ALIVE lines should continue every 5 seconds.

  Evidence boundary:
    PASS for this test means BoardInfo runtime/serial PASS only.
    It does not replace the dedicated display, touch, SD, Wi-Fi or Web tests.
*/

#include <ESP32_8048S043.h>

ESP32_8048S043 board;

void setup() {
    Serial.begin(115200);
    delay(800);

    Serial.println();
    Serial.println("================================================================");
    Serial.println(" ESP32-8048S043 Lab / 01_BoardInfo");
    Serial.println(" First Arduino IDE smoke test + board/profile diagnostic");
    Serial.println("================================================================");
    Serial.println("Author : Alex Malachevsky");
    Serial.println("GitHub : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
    Serial.println("----------------------------------------------------------------");
    Serial.println("This sketch does not trust a hard-coded Tools menu table.");
    Serial.println("It prints compile-time macros and runtime ESP/IDF values below.");
    Serial.println("----------------------------------------------------------------");
    Serial.println("Known-good Sample A reference profile:");
    Serial.println("  Board             : ESP32S3 Dev Module or ESP32-8048S043 Lab local profile");
    Serial.println("  Flash Size        : 16MB (128Mb)");
    Serial.println("  Flash Mode        : QIO 80MHz");
    Serial.println("  Partition Scheme  : 16M Flash (3MB APP/9.9MB FATFS)");
    Serial.println("  PSRAM             : OPI PSRAM");
    Serial.println("  USB CDC On Boot   : Disabled when using CH340 USB-UART");
    Serial.println("  Serial Monitor    : 115200 baud");
    Serial.println("----------------------------------------------------------------");

    board.begin();
    board.printBoardInfo(Serial);

    Serial.println("----------------------------------------------------------------");
    Serial.println("PASS if chip=ESP32-S3, flash~16MB, PSRAM~8MB, app0~3MB, ALIVE continues.");
    Serial.println("If PSRAM is 0 B, compare [ARDUINO BUILD PROFILE] with the Tools menu.");
    Serial.println("================================================================");
}

void loop() {
    static uint32_t last = 0;
    if (millis() - last >= 5000) {
        last = millis();
        Serial.printf("ALIVE uptime=%lu ms freeHeap=%lu psramSize=%lu freePsram=%lu\n",
                      static_cast<unsigned long>(millis()),
                      static_cast<unsigned long>(ESP.getFreeHeap()),
                      static_cast<unsigned long>(ESP.getPsramSize()),
                      static_cast<unsigned long>(ESP.getFreePsram()));
    }
}
