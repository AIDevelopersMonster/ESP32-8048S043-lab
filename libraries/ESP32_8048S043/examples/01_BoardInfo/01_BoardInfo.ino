/*
  ESP32-8048S043 Lab / 01_BoardInfo

  Purpose:
    First Arduino IDE smoke test for a physical ESP32-8048S043 / ESP32-8048S043C-I board.

  What this example checks:
    - sketch upload path through the board USB-UART interface;
    - serial monitor output at 115200 baud;
    - ESP32-S3 chip model and revision;
    - flash size reported by the Arduino runtime;
    - PSRAM size reported by the Arduino runtime;
    - basic runtime stability through periodic ALIVE messages.

  What this example does NOT check:
    - RGB display output;
    - GT911 touch;
    - SD card;
    - Wi-Fi/BLE;
    - final BSP pinout.

  Arduino IDE recommended settings:
    Board package : esp32 by Espressif Systems
    Board         : ESP32S3 Dev Module
    Port          : CH340 / USB-SERIAL port of the board
    Upload Speed  : 460800 recommended first, 921600 only if stable
    CPU Frequency : 240MHz (WiFi)
    Flash Size    : 16MB / 128Mb
    Flash Mode    : DIO recommended first; QIO may also work on some setups
    Partition     : any 16MB-compatible scheme for this smoke test
    PSRAM         : OPI PSRAM / Enabled
    USB CDC Boot  : Disabled when using the CH340C USB-UART port
    Upload Mode   : UART0 / Hardware CDC, depending on Arduino menu wording
    Core Debug    : None

  Expected serial result:
    - Chip model should be ESP32-S3;
    - Flash should be about 16 MB;
    - PSRAM should be about 8 MB;
    - ALIVE lines should continue every 5 seconds.

  Evidence boundary:
    PASS for this test means BoardInfo runtime/serial PASS only.
    It does not replace the dedicated display, touch and SD tests.
*/

#include <ESP32_8048S043.h>

ESP32_8048S043 board;

void setup() {
    Serial.begin(115200);
    delay(800);

    Serial.println();
    Serial.println("================================================================");
    Serial.println(" ESP32-8048S043 Lab / 01_BoardInfo");
    Serial.println(" First Arduino IDE smoke test");
    Serial.println("================================================================");
    Serial.println("Arduino IDE settings expected for Sample A:");
    Serial.println("  Board         : ESP32S3 Dev Module");
    Serial.println("  Upload Speed  : 460800 recommended first");
    Serial.println("  Flash Size    : 16MB / 128Mb");
    Serial.println("  PSRAM         : OPI PSRAM / Enabled");
    Serial.println("  USB CDC Boot  : Disabled for CH340C USB-UART");
    Serial.println("  Serial Monitor: 115200 baud");
    Serial.println("----------------------------------------------------------------");

    board.begin();
    board.printBoardInfo(Serial);

    Serial.println("----------------------------------------------------------------");
    Serial.println("PASS candidate if chip=ESP32-S3, flash~16MB, PSRAM~8MB, ALIVE continues.");
    Serial.println("================================================================");
}

void loop() {
    static uint32_t last = 0;
    if (millis() - last >= 5000) {
        last = millis();
        Serial.printf("ALIVE uptime=%lu ms freeHeap=%lu freePsram=%lu\n",
                      static_cast<unsigned long>(millis()),
                      static_cast<unsigned long>(ESP.getFreeHeap()),
                      static_cast<unsigned long>(ESP.getFreePsram()));
    }
}
