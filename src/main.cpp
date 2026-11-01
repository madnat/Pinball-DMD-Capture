
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "soc/gpio_struct.h"

TFT_eSPI tft;

// Williams J603 -> SN74LVC245A -> TTGO ESP32
constexpr int PIN_RD    = 26;  // J603 pin 3
constexpr int PIN_DCLK  = 33;  // J603 pin 9
constexpr int PIN_SDATA = 13;  // J603 pin 11

constexpr int WIDTH = 128;
constexpr int HEIGHT = 32;
constexpr int BYTES_PER_ROW = 16;
constexpr int FRAME_BYTES = 512;

constexpr uint32_t FRAME_TIMEOUT_MS = 40;
constexpr uint32_t ROW_TIMEOUT_US = 600;
constexpr uint32_t FRAME_CAPTURE_TIMEOUT_US = 9000;

uint8_t frame[FRAME_BYTES];

static inline bool readClock() {
    return (GPIO.in1.val & (1UL << 1)) != 0;
}

static inline bool readData() {
    return (GPIO.in & (1UL << 13)) != 0;
}

bool waitForFrameMarker() {
    uint32_t start = millis();

    while (digitalRead(PIN_RD) == HIGH) {
        if (millis() - start > FRAME_TIMEOUT_MS)
            return false;
    }

    while (digitalRead(PIN_RD) == LOW) {
        if (millis() - start > FRAME_TIMEOUT_MS)
            return false;
    }

    return true;
}

int captureRow(uint8_t *destination) {
    memset(destination, 0, BYTES_PER_ROW);

    uint32_t startCycles = ESP.getCycleCount();
    uint32_t timeoutCycles =
        ESP.getCpuFreqMHz() * ROW_TIMEOUT_US;

    int captured = 0;

    for (int bit = 0; bit < WIDTH; bit++) {
        while (readClock()) {
            if ((uint32_t)(ESP.getCycleCount() - startCycles)
                > timeoutCycles)
                return captured;
        }

        while (!readClock()) {
            if ((uint32_t)(ESP.getCycleCount() - startCycles)
                > timeoutCycles)
                return captured;
        }

        if (readData()) {
            destination[bit / 8] |=
                (0x80 >> (bit % 8));
        }

        captured++;
    }

    return captured;
}

bool captureFrame() {
    memset(frame, 0, sizeof(frame));

    if (!waitForFrameMarker())
        return false;

    uint32_t startUs = micros();

    noInterrupts();

    for (int row = 0; row < HEIGHT; row++) {
        if ((uint32_t)(micros() - startUs)
            > FRAME_CAPTURE_TIMEOUT_US) {
            interrupts();
            return false;
        }

        int bits = captureRow(&frame[row * BYTES_PER_ROW]);

        if (bits != WIDTH) {
            interrupts();
            return false;
        }
    }

    interrupts();
    return true;
}

bool validateTopRow() {
    for (int col = 0; col < BYTES_PER_ROW; col++) {
        if (frame[col] != 0xFF)
            return false;
    }

    for (int i = BYTES_PER_ROW; i < FRAME_BYTES; i++) {
        if (frame[i] != 0x00)
            return false;
    }

    return true;
}

void printFrameSummary(bool valid) {
    Serial.printf(
        "FRAME: %s | Row0: ",
        valid ? "MATCH" : "DIFFERENT"
    );

    for (int i = 0; i < BYTES_PER_ROW; i++)
        Serial.printf("%02X ", frame[i]);

    Serial.println();

    for (int row = 0; row < HEIGHT; row++) {
        int lit = 0;

        for (int col = 0; col < BYTES_PER_ROW; col++) {
            lit += __builtin_popcount(
                frame[row * BYTES_PER_ROW + col]
            );
        }

        Serial.printf("%d%s", lit,
                      row == HEIGHT - 1 ? "\n" : ",");
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(PIN_RD, INPUT);
    pinMode(PIN_DCLK, INPUT);
    pinMode(PIN_SDATA, INPUT);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    Serial.println();
    Serial.println("PINBALL DMD CAPTURE");
    Serial.println("Diagnostic V0.6");
    Serial.println("Full 128x32 frame capture");
    Serial.println("Expected: Row0=128, Rows1-31=0");
}

void loop() {
    static uint32_t attempts = 0;
    static uint32_t complete = 0;
    static uint32_t matches = 0;

    attempts++;

    if (captureFrame()) {
        complete++;

        bool valid = validateTopRow();

        if (valid)
            matches++;

        printFrameSummary(valid);
    } else {
        Serial.println("FRAME: INCOMPLETE");
    }

    if (attempts % 10 == 0) {
        Serial.printf(
            "SUMMARY: attempts=%lu complete=%lu matches=%lu\n",
            (unsigned long)attempts,
            (unsigned long)complete,
            (unsigned long)matches
        );

        tft.fillScreen(TFT_BLACK);
        tft.setTextSize(2);
        tft.setCursor(10, 10);
        tft.println("DMD V0.6");

        tft.setTextSize(1);
        tft.setCursor(10, 50);
        tft.printf("Attempts: %lu\n", (unsigned long)attempts);
        tft.printf("Complete: %lu\n", (unsigned long)complete);
        tft.printf("Matches:  %lu\n", (unsigned long)matches);
    }

    delay(100);
}
