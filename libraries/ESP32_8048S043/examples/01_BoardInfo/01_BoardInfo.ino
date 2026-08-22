#include <ESP32_8048S043.h>

ESP32_8048S043 board;

void setup() {
    Serial.begin(115200);
    delay(800);
    board.begin();
    board.printBoardInfo(Serial);
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
