
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("Pinball DMD Capture - Baseline Test");
}

void loop() {
    Serial.println("ESP32 running");
    delay(2000);
}
