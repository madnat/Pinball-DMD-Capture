
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "soc/gpio_struct.h"
#include <string.h>

TFT_eSPI tft;

// Williams J603 -> TTGO T-Display
constexpr int PIN_RD    = 26;
constexpr int PIN_DCLK  = 33;
constexpr int PIN_SDATA = 13;

// DMD dimensions
constexpr int DMD_W = 128;
constexpr int DMD_H = 32;
constexpr int ROW_BYTES = 16;
constexpr int FRAME_BYTES = 512;

// TFT dimensions after rotation(1)
constexpr int TFT_W = 240;
constexpr int TFT_H = 135;

constexpr int SCALE_X = 1;
constexpr int SCALE_Y = 1;

constexpr int IMAGE_X = (TFT_W - DMD_W * SCALE_X) / 2;
constexpr int IMAGE_Y = (TFT_H - DMD_H * SCALE_Y) / 2;


// Capture timing
constexpr uint32_t ROW_TIMEOUT_US = 600;
constexpr uint32_t FRAME_TIMEOUT_US = 9000;
constexpr uint32_t MARKER_TIMEOUT_US = 25000;

// Three consecutive binary refreshes
uint8_t frames[3][FRAME_BYTES];

// One byte per reconstructed DMD pixel
uint8_t grey[DMD_W * DMD_H];

// Four-level greyscale palette
const uint16_t palette[4] = {
    TFT_BLACK,
    0x52AA,    // Approx. 1/3 brightness
    0xAD55,    // Approx. 2/3 brightness
    TFT_WHITE
};

uint32_t composites = 0;
uint32_t captureErrors = 0;
uint32_t lastReportMs = 0;
uint32_t lastSignature = 0;
uint32_t lastRenderUs = 0;
uint32_t lastCaptureUs = 0;

uint16_t histogram[4] = {0};

// Fast direct GPIO reads
static inline bool readClock() {
    return (GPIO.in1.val & (1UL << 1)) != 0;
}

static inline bool readData() {
    return (GPIO.in & (1UL << 13)) != 0;
}

static inline bool readRD() {
    return (GPIO.in & (1UL << 26)) != 0;
}

// Wait for the next frame marker
bool waitForFrameMarker(uint32_t timeoutUs) {
    uint32_t start = micros();

    while (readRD()) {
        if ((uint32_t)(micros() - start) > timeoutUs)
            return false;
    }

    while (!readRD()) {
        if ((uint32_t)(micros() - start) > timeoutUs)
            return false;
    }

    return true;
}

// Capture 128 serial bits
int captureRow(uint8_t *destination) {
    memset(destination, 0, ROW_BYTES);

    uint32_t startCycles = ESP.getCycleCount();
    uint32_t timeoutCycles =
        ESP.getCpuFreqMHz() * ROW_TIMEOUT_US;

    int captured = 0;

    for (int bit = 0; bit < DMD_W; bit++) {
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

// Capture one complete 128x32 refresh
bool captureFrame(uint8_t *destination) {
    memset(destination, 0, FRAME_BYTES);

    uint32_t startUs = micros();
    bool success = true;

    noInterrupts();

    for (int row = 0; row < DMD_H; row++) {
        if ((uint32_t)(micros() - startUs)
            > FRAME_TIMEOUT_US) {
            success = false;
            break;
        }

        int bits = captureRow(
            &destination[row * ROW_BYTES]
        );

        if (bits != DMD_W) {
            success = false;
            break;
        }
    }

    interrupts();

    return success;
}

// Acquire exactly three consecutive refreshes
bool captureTriple() {
    if (!waitForFrameMarker(MARKER_TIMEOUT_US))
        return false;

    uint32_t start = micros();

    for (int i = 0; i < 3; i++) {
        if (!captureFrame(frames[i]))
            return false;
    }

    lastCaptureUs = micros() - start;
    return true;
}

// Reconstruct brightness from 3 refreshes
void reconstructGreyscale() {
    memset(histogram, 0, sizeof(histogram));

    uint32_t hash = 2166136261UL;
    int pixel = 0;

    for (int i = 0; i < FRAME_BYTES; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t mask = 1U << bit;

            uint8_t level =
                ((frames[0][i] & mask) ? 1 : 0) +
                ((frames[1][i] & mask) ? 1 : 0) +
                ((frames[2][i] & mask) ? 1 : 0);

            grey[pixel++] = level;
            histogram[level]++;

            hash ^= level;
            hash *= 16777619UL;
        }
    }

    lastSignature = hash;
}

// Render horizontal runs to reduce TFT commands
void renderGreyscale() {
    uint32_t start = micros();

    for (int y = 0; y < DMD_H; y++) {
        int x = 0;

        while (x < DMD_W) {
            uint8_t level = grey[y * DMD_W + x];

            int runStart = x;
            x++;

            while (x < DMD_W &&
                   grey[y * DMD_W + x] == level) {
                x++;
            }

            tft.fillRect(
                IMAGE_X + runStart * SCALE_X,
                IMAGE_Y + y * SCALE_Y,
                (x - runStart) * SCALE_X,
                SCALE_Y,
                palette[level]
            );
        }
    }

    lastRenderUs = micros() - start;
}

void updateStatus() {
    tft.fillRect(0, 109, TFT_W, 26, TFT_BLACK);

    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.setCursor(4, 111);
    tft.printf("DMD V1.0  Frames: %lu",
               (unsigned long)(composites * 3));

    tft.setCursor(4, 122);
    tft.printf("Errors: %lu  Render: %lu ms",
               (unsigned long)captureErrors,
               (unsigned long)(lastRenderUs / 1000));
}

void printDiagnostics() {
    Serial.println();
    Serial.println("V1.0 GREYSCALE DISPLAY");

    Serial.printf(
        "Composites=%lu Errors=%lu\n",
        (unsigned long)composites,
        (unsigned long)captureErrors
    );

    Serial.printf(
        "Capture=%lu us Render=%lu us\n",
        (unsigned long)lastCaptureUs,
        (unsigned long)lastRenderUs
    );

    Serial.printf(
        "Histogram: OFF=%u L1=%u L2=%u ON=%u\n",
        histogram[0],
        histogram[1],
        histogram[2],
        histogram[3]
    );

    Serial.printf(
        "Composite signature=%08lX\n",
        (unsigned long)lastSignature
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
    tft.setCursor(10, 55);
    tft.println("DMD V1.0");

    Serial.println();
    Serial.println("PINBALL DMD CAPTURE");
    Serial.println("Version V1.0");
    Serial.println("Three-frame greyscale display");

    delay(500);
    tft.fillScreen(TFT_BLACK);
}

void loop() {
    if (captureTriple()) {
        reconstructGreyscale();
        renderGreyscale();
        composites++;
    } else {
        captureErrors++;
    }

    uint32_t now = millis();

    if ((uint32_t)(now - lastReportMs) >= 1000) {
        printDiagnostics();
        updateStatus();
        lastReportMs = now;
    }

    // Allow background system processing
    delay(1);
}
