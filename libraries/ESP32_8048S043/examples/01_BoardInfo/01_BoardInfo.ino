/*
  ESP32-8048S043 Lab / 01_BoardInfo

  Author:
    Alex Malachevsky

  Project GitHub:
    https://github.com/AIDevelopersMonster/ESP32-8048S043-lab

  Confirming video / visual board check:
    https://youtube.com/shorts/XVaWqrtXHE4

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

  Arduino IDE settings currently used for Sample A:
    Board                                  : ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)
    FQBN                                   : AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8
    Port                                   : COM12 / CH340 USB-SERIAL port
    USB CDC On Boot                        : Disabled
    CPU Frequency                          : 240MHz (WiFi)
    Core Debug Level                       : None
    USB DFU On Boot                        : Disabled
    Erase All Flash Before Sketch Upload   : Disabled
    Events Run On                          : Core 1
    Flash Mode                             : QIO 80MHz
    Flash Size                             : 16MB (128Mb)
    JTAG Adapter                           : Disabled
    Arduino Runs On                        : Core 1
    USB Firmware MSC On Boot               : Disabled
    Partition Scheme                       : 16M Flash (3MB APP/9.9MB FATFS)
    PSRAM                                  : OPI PSRAM
    Upload Mode                            : UART0 / Hardware CDC
    Upload Speed                           : 921600
    USB Mode                               : Hardware CDC and JTAG
    Zigbee Mode                            : Disabled
    Serial Monitor                         : 115200 baud

  Notes:
    - COM12 is the local port observed on Sample A; choose your actual CH340 port.
    - If upload is unstable at 921600, retry at 460800 before changing other settings.
    - For factory flash readback/dump workflows, 460800 was more reliable than 921600.
    - The printed settings are documentation strings; runtime evidence comes from ESP.* values.
    - The generic ESP32S3 Dev Module remains the safe fallback profile.

  Expected serial result:
    - Chip model should be ESP32-S3;
    - Flash should be about 16 MB;
    - PSRAM should be about 8 MB;
    - app0 partition should be about 3 MB;
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
    Serial.println("Author        : Alex Malachevsky");
    Serial.println("GitHub        : https://github.com/AIDevelopersMonster/ESP32-8048S043-lab");
    Serial.println("Evidence video: https://youtube.com/shorts/XVaWqrtXHE4");
    Serial.println("----------------------------------------------------------------");
    Serial.println("Arduino IDE settings currently used for Sample A:");
    Serial.println("  Board                                : ESP32-8048S043 Lab N16R8 FIXED (ESP32-S3 RGB800x480 GT911)");
    Serial.println("  FQBN                                 : AIDevelopersMonster:esp32:esp32_8048s043_lab_n16r8");
    Serial.println("  Port                                 : COM12 / CH340 USB-SERIAL port");
    Serial.println("  USB CDC On Boot                      : Disabled");
    Serial.println("  CPU Frequency                        : 240MHz (WiFi)");
    Serial.println("  Core Debug Level                     : None");
    Serial.println("  USB DFU On Boot                      : Disabled");
    Serial.println("  Erase All Flash Before Sketch Upload : Disabled");
    Serial.println("  Events Run On                        : Core 1");
    Serial.println("  Flash Mode                           : QIO 80MHz");
    Serial.println("  Flash Size                           : 16MB (128Mb)");
    Serial.println("  JTAG Adapter                         : Disabled");
    Serial.println("  Arduino Runs On                      : Core 1");
    Serial.println("  USB Firmware MSC On Boot             : Disabled");
    Serial.println("  Partition Scheme                     : 16M Flash (3MB APP/9.9MB FATFS)");
    Serial.println("  PSRAM                                : OPI PSRAM");
    Serial.println("  Upload Mode                          : UART0 / Hardware CDC");
    Serial.println("  Upload Speed                         : 921600");
    Serial.println("  USB Mode                             : Hardware CDC and JTAG");
    Serial.println("  Zigbee Mode                          : Disabled");
    Serial.println("  Serial Monitor                       : 115200 baud");
    Serial.println("----------------------------------------------------------------");

    board.begin();
    board.printBoardInfo(Serial);

    Serial.println("----------------------------------------------------------------");
    Serial.println("PASS candidate if chip=ESP32-S3, flash~16MB, PSRAM~8MB, app0~3MB, ALIVE continues.");
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
