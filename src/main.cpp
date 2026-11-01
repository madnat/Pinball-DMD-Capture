
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "soc/gpio_struct.h"
#include <string.h>

TFT_eSPI tft;

// ============================================================
// PINBALL DMD CAPTURE V1.1
// Optimised TFT rendering using RGB565 framebuffer
//
// Williams J603 -> TTGO T-Display
// ============================================================

// DMD input pins
constexpr int PIN_RD    = 26;
constexpr int PIN_DCLK  = 33;
constexpr int PIN_SDATA = 13;

// DMD geometry
constexpr int DMD_W = 128;
constexpr int DMD_H = 32;
constexpr int ROW_BYTES = 16;
constexpr int FRAME_BYTES = 512;
constexpr int PIXEL_COUNT = DMD_W * DMD_H;

// TFT geometry (rotation 1)
constexpr int TFT_W = 240;
constexpr int TFT_H = 135;

// Corrected V1.0 proportions
constexpr int SCALE_X = 1;
constexpr int SCALE_Y = 1;

constexpr int IMAGE_X =
    (TFT_W - DMD_W * SCALE_X) / 2;

constexpr int IMAGE_Y =
    (TFT_H - DMD_H * SCALE_Y) / 2;

// Capture timeouts
constexpr uint32_t ROW_TIMEOUT_US = 600;
constexpr uint32_t FRAME_TIMEOUT_US = 9000;
constexpr uint32_t MARKER_TIMEOUT_US = 25000;

// ============================================================
// FRAME BUFFERS
// ============================================================

// Three binary DMD refreshes
uint8_t frames[3][FRAME_BYTES];

// Reconstructed four-level image
uint8_t grey[PIXEL_COUNT];

// RGB565 framebuffer: 128 x 32 x 2 = 8192 bytes
uint16_t rgbFrame[PIXEL_COUNT];

// Four-level palette
const uint16_t palette[4] = {
    TFT_BLACK,
    0x52AA,
    0xAD55,
    TFT_WHITE
};

// ============================================================
// DIAGNOSTICS
// ============================================================

uint32_t composites = 0;
uint32_t captureErrors = 0;

uint32_t lastReportMs = 0;
uint32_t lastSignature = 0;

uint32_t lastCaptureUs = 0;
uint32_t lastRenderUs = 0;
uint32_t lastConvertUs = 0;

uint32_t minRenderUs = UINT32_MAX;
uint32_t maxRenderUs = 0;
uint64_t totalRenderUs = 0;

uint16_t histogram[4] = {0};

uint32_t previousReportComposites = 0;

// ============================================================
// FAST GPIO ACCESS
// ============================================================

static inline bool readClock() {
    return (GPIO.in1.val & (1UL << 1)) != 0;
}

static inline bool readData() {
    return (GPIO.in & (1UL << 13)) != 0;
}

static inline bool readRD() {
    return (GPIO.in & (1UL << 26)) != 0;
}

// ============================================================
// FRAME SYNCHRONISATION
// ============================================================

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

// ============================================================
// CAPTURE ONE ROW
// ============================================================

int captureRow(uint8_t *destination) {
    memset(destination, 0, ROW_BYTES);

    uint32_t startCycles = ESP.getCycleCount();

    uint32_t timeoutCycles =
        ESP.getCpuFreqMHz() * ROW_TIMEOUT_US;

    int captured = 0;

    for (int bit = 0; bit < DMD_W; bit++) {

        while (readClock()) {
            if ((uint32_t)(
                ESP.getCycleCount() - startCycles
            ) > timeoutCycles)
                return captured;
        }

        while (!readClock()) {
            if ((uint32_t)(
                ESP.getCycleCount() - startCycles
            ) > timeoutCycles)
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

// ============================================================
// CAPTURE ONE COMPLETE DMD REFRESH
// ============================================================

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

// ============================================================
// CAPTURE THREE CONSECUTIVE REFRESHES
// ============================================================

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

// ============================================================
// FOUR-LEVEL GREYSCALE RECONSTRUCTION
// ============================================================

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

// ============================================================
// V1.1: CONVERT GREYSCALE TO RGB565
// ============================================================

void convertToRGB565() {
    uint32_t start = micros();

    for (int i = 0; i < PIXEL_COUNT; i++) {
        rgbFrame[i] = palette[grey[i]];
    }

    lastConvertUs = micros() - start;
}

// ============================================================
// V1.1: SINGLE-BLOCK TFT TRANSFER
// ============================================================

void renderGreyscale() {
    uint32_t start = micros();

    tft.startWrite();

    tft.pushImage(
        IMAGE_X,
        IMAGE_Y,
        DMD_W,
        DMD_H,
        rgbFrame
    );

    tft.endWrite();

    lastRenderUs = micros() - start;

    if (lastRenderUs < minRenderUs)
        minRenderUs = lastRenderUs;

    if (lastRenderUs > maxRenderUs)
        maxRenderUs = lastRenderUs;

    totalRenderUs += lastRenderUs;
}

// ============================================================
// TFT STATUS
// ============================================================

void updateStatus(uint32_t fps) {
    tft.fillRect(
        0, 109,
        TFT_W, 26,
        TFT_BLACK
    );

    tft.setTextSize(1);
    tft.setTextColor(
        TFT_WHITE,
        TFT_BLACK
    );

    tft.setCursor(4, 111);
    tft.printf(
        "DMD V1.1  FPS: %lu",
        (unsigned long)fps
    );

    tft.setCursor(4, 122);
    tft.printf(
        "Err:%lu  TFT:%lu us",
        (unsigned long)captureErrors,
        (unsigned long)lastRenderUs
    );
}

// ============================================================
// SERIAL DIAGNOSTICS
// ============================================================

void printDiagnostics(uint32_t fps) {
    Serial.println();
    Serial.println("V1.1 RGB565 FRAMEBUFFER");

    Serial.printf(
        "Composites=%lu Errors=%lu FPS=%lu\n",
        (unsigned long)composites,
        (unsigned long)captureErrors,
        (unsigned long)fps
    );

    Serial.printf(
        "Capture=%lu us Convert=%lu us Render=%lu us\n",
        (unsigned long)lastCaptureUs,
        (unsigned long)lastConvertUs,
        (unsigned long)lastRenderUs
    );

    uint32_t avgRenderUs =
        composites > 0
        ? (uint32_t)(totalRenderUs / composites)
        : 0;

    Serial.printf(
        "Render min=%lu avg=%lu max=%lu us\n",
        (unsigned long)(
            minRenderUs == UINT32_MAX ? 0 : minRenderUs
        ),
        (unsigned long)avgRenderUs,
        (unsigned long)maxRenderUs
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

// ============================================================
// SETUP
// ============================================================

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
    tft.setSwapBytes(true);

    tft.fillScreen(TFT_BLACK);

    tft.setTextColor(
        TFT_WHITE,
        TFT_BLACK
    );

    tft.setTextSize(2);
    tft.setCursor(10, 55);
    tft.println("DMD V1.1");

    Serial.println();
    Serial.println("PINBALL DMD CAPTURE");
    Serial.println("Version V1.1");
    Serial.println("RGB565 framebuffer rendering");

    delay(500);

    tft.fillScreen(TFT_BLACK);
    lastReportMs = millis();
}

// ============================================================
// MAIN LOOP
// ============================================================

void loop() {
    if (captureTriple()) {
        reconstructGreyscale();

        convertToRGB565();
        renderGreyscale();

        composites++;
    } else {
        captureErrors++;
    }

    uint32_t now = millis();
    uint32_t elapsed = now - lastReportMs;

    if (elapsed >= 1000) {

        uint32_t framesThisPeriod =
            composites - previousReportComposites;

        uint32_t fps =
            (framesThisPeriod * 1000UL) / elapsed;

        printDiagnostics(fps);
        updateStatus(fps);

        previousReportComposites = composites;
        lastReportMs = now;
    }

    delay(1);
}
