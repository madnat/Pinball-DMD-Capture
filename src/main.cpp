
#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Williams J603 -> SN74LVC245A -> TTGO
constexpr int PIN_DE    = 27;  // J603 pin 1
constexpr int PIN_RD    = 26;  // J603 pin 3
constexpr int PIN_RCLK  = 25;  // J603 pin 5
constexpr int PIN_CL    = 32;  // J603 pin 7
constexpr int PIN_DCLK  = 33;  // J603 pin 9
constexpr int PIN_SDATA = 13;  // J603 pin 11

volatile uint32_t deCount = 0;
volatile uint32_t rdCount = 0;
volatile uint32_t rclkCount = 0;
volatile uint32_t clCount = 0;

void IRAM_ATTR onDE()   { deCount++; }
void IRAM_ATTR onRD()   { rdCount++; }
void IRAM_ATTR onRCLK() { rclkCount++; }
void IRAM_ATTR onCL()   { clCount++; }

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_DE, INPUT);
    pinMode(PIN_RD, INPUT);
    pinMode(PIN_RCLK, INPUT);
    pinMode(PIN_CL, INPUT);
    pinMode(PIN_DCLK, INPUT);
    pinMode(PIN_SDATA, INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_DE), onDE, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_RD), onRD, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_RCLK), onRCLK, RISING);
    attachInterrupt(digitalPinToInterrupt(PIN_CL), onCL, RISING);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);

    Serial.println();
    Serial.println("PINBALL DMD CAPTURE");
    Serial.println("Diagnostic V0.3");
    Serial.println("Counting DE, RD, RCLK and CL");
}

void loop() {
    static uint32_t lastReport = millis();

    if (millis() - lastReport >= 1000) {
        uint32_t now = millis();
        uint32_t elapsed = now - lastReport;
        lastReport = now;

        noInterrupts();
        uint32_t de = deCount;
        uint32_t rd = rdCount;
        uint32_t rclk = rclkCount;
        uint32_t cl = clCount;
        deCount = rdCount = rclkCount = clCount = 0;
        interrupts();

        Serial.printf(
            "DE=%lu RD=%lu RCLK=%lu CL=%lu (over %lu ms)\n",
            (unsigned long)de,
            (unsigned long)rd,
            (unsigned long)rclk,
            (unsigned long)cl,
            (unsigned long)elapsed
        );

        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(2);

        tft.setCursor(10, 10);
        tft.println("DMD V0.3");

        tft.setTextSize(1);
        tft.setCursor(10, 50);
        tft.printf("DE:   %lu\n", (unsigned long)de);
        tft.printf("RD:   %lu\n", (unsigned long)rd);
        tft.printf("RCLK: %lu\n", (unsigned long)rclk);
        tft.printf("CL:   %lu\n", (unsigned long)cl);
    }
}
