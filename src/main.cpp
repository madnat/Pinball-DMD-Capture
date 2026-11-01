
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "soc/gpio_struct.h"

TFT_eSPI tft;

// Williams DMD input wiring
constexpr int PIN_RD    = 26;
constexpr int PIN_DCLK  = 33;
constexpr int PIN_SDATA = 13;

// DMD geometry
constexpr int DMD_WIDTH = 128;
constexpr int DMD_HEIGHT = 32;
constexpr int BYTES_PER_ROW = 16;
constexpr int FRAME_BYTES = 512;

// Capture timing
constexpr uint32_t FRAME_TIMEOUT_MS = 40;
constexpr uint32_t ROW_TIMEOUT_US = 600;
constexpr uint32_t FRAME_CAPTURE_TIMEOUT_US = 9000;

// LCD layout
constexpr int DISPLAY_X = 56;
constexpr int DISPLAY_Y = 35;
constexpr uint16_t DMD_ORANGE = 0xFD20;

uint8_t frame[FRAME_BYTES];

uint32_t attempts = 0;
uint32_t complete = 0;
uint32_t failures = 0;

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

    for (int bit = 0; bit < DMD_WIDTH; bit++) {
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

    for (int row = 0; row < DMD_HEIGHT; row++) {
        if ((uint32_t)(micros() - startUs)
            > FRAME_CAPTURE_TIMEOUT_US) {
            interrupts();
            return false;
        }

        int bits = captureRow(
            &frame[row * BYTES_PER_ROW]
        );

        if (bits != DMD_WIDTH) {
            interrupts();
            return false;
        }
    }

    interrupts();
    return true;
}

void drawFrame() {
    tft.fillRect(
        DISPLAY_X,
        DISPLAY_Y,
        DMD_WIDTH,
        DMD_HEIGHT,
        TFT_BLACK
    );

    for (int y = 0; y < DMD_HEIGHT; y++) {
        for (int x = 0; x < DMD_WIDTH; x++) {
            int index = y * BYTES_PER_ROW + (x / 8);
            uint8_t mask = 0x80 >> (x % 8);

            if (frame[index] & mask) {
                tft.drawPixel(
                    DISPLAY_X + x,
                    DISPLAY_Y + y,
                    DMD_ORANGE
                );
            }
        }
    }
}

void drawStatus() {
    tft.fillRect(0, 85, 240, 50, TFT_BLACK);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.setCursor(10, 90);
    tft.printf(
        "Frames: %lu",
        (unsigned long)complete
    );

    tft.setCursor(10, 105);
    tft.printf(
        "Failures: %lu",
        (unsigned long)failures
    );

    tft.setCursor(10, 120);
    tft.printf(
        "Attempts: %lu",
        (unsigned long)attempts
    );
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
    tft.setTextSize(2);
    tft.setCursor(10, 8);
    tft.println("DMD CAPTURE V0.7");

    Serial.println();
    Serial.println("PINBALL DMD CAPTURE");
    Serial.println("Diagnostic V0.7");
    Serial.println("Full frame capture + LCD display");
}

void loop() {
    attempts++;

    if (captureFrame()) {
        complete++;
        drawFrame();
    } else {
        failures++;
    }

    if (attempts % 10 == 0) {
        drawStatus();

        Serial.printf(
            "SUMMARY: attempts=%lu complete=%lu failures=%lu\n",
            (unsigned long)attempts,
            (unsigned long)complete,
            (unsigned long)failures
        );
    }

    delay(50);
}
