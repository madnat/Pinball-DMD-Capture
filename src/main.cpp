
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "soc/gpio_struct.h"
#include <string.h>

TFT_eSPI tft;

// Williams DMD wiring
constexpr int PIN_RD    = 26;
constexpr int PIN_DCLK  = 33;
constexpr int PIN_SDATA = 13;

// DMD geometry
constexpr int WIDTH = 128;
constexpr int HEIGHT = 32;
constexpr int BYTES_PER_ROW = 16;
constexpr int FRAME_BYTES = 512;

// Acquisition
constexpr int BURST_FRAMES = 16;
constexpr uint32_t ROW_TIMEOUT_US = 600;
constexpr uint32_t FRAME_TIMEOUT_US = 9000;
constexpr uint32_t MARKER_TIMEOUT_US = 25000;

// Measured refresh period
constexpr uint32_t EXPECTED_PERIOD_US = 8192;

// Maximum time to wait for the next row-zero burst
constexpr uint32_t NEXT_FRAME_TIMEOUT_US = 9000;

uint8_t frames[BURST_FRAMES][FRAME_BYTES];

struct FrameInfo {
    uint32_t startUs;
    uint32_t crc;
    uint16_t litPixels;
    uint16_t captureUs;
    uint8_t status;
};

FrameInfo info[BURST_FRAMES];

enum {
    FRAME_OK = 0,
    FRAME_SYNC_TIMEOUT = 1,
    FRAME_ROW_TIMEOUT = 2,
    FRAME_TOTAL_TIMEOUT = 3
};

static inline bool readClock() {
    return (GPIO.in1.val & (1UL << 1)) != 0;
}

static inline bool readData() {
    return (GPIO.in & (1UL << 13)) != 0;
}

static inline bool readRD() {
    return (GPIO.in & (1UL << 26)) != 0;
}

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

// Existing proven 128-bit row capture
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

// Capture 32 rows without waiting for RD.
// Used after the initial synchronisation.
uint8_t captureFrameRows(uint8_t *destination,
                         FrameInfo &result) {
    memset(destination, 0, FRAME_BYTES);

    result.startUs = micros();

    uint8_t status = FRAME_OK;

    noInterrupts();

    for (int row = 0; row < HEIGHT; row++) {
        if ((uint32_t)(micros() - result.startUs)
            > FRAME_TIMEOUT_US) {
            status = FRAME_TOTAL_TIMEOUT;
            break;
        }

        int bits = captureRow(
            &destination[row * BYTES_PER_ROW]
        );

        if (bits != WIDTH) {
            status = FRAME_ROW_TIMEOUT;
            break;
        }
    }

    interrupts();

    result.captureUs =
        (uint16_t)(micros() - result.startUs);

    return status;
}

uint32_t crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFFUL;

    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 1)
                ? (crc >> 1) ^ 0xEDB88320UL
                : crc >> 1;
        }
    }

    return ~crc;
}

uint16_t countLitPixels(const uint8_t *data) {
    uint16_t total = 0;

    for (int i = 0; i < FRAME_BYTES; i++)
        total += __builtin_popcount(
            (unsigned int)data[i]
        );

    return total;
}

void acquireBurst() {
    memset(info, 0, sizeof(info));

    // Synchronise only the first frame using RD.
    if (!waitForFrameMarker(MARKER_TIMEOUT_US)) {
        info[0].status = FRAME_SYNC_TIMEOUT;
        return;
    }

    // Capture the first frame.
    info[0].status =
        captureFrameRows(frames[0], info[0]);

    // Attempt subsequent frames immediately.
    for (int i = 1; i < BURST_FRAMES; i++) {
        info[i].status =
            captureFrameRows(frames[i], info[i]);

        if (info[i].status != FRAME_OK)
            break;
    }

    // Calculate CRC32 after acquisition.
    for (int i = 0; i < BURST_FRAMES; i++) {
        if (info[i].status == FRAME_OK) {
            info[i].crc =
                crc32(frames[i], FRAME_BYTES);
            info[i].litPixels =
                countLitPixels(frames[i]);
        }
    }
}

void printBurst() {
    Serial.println();
    Serial.println("V0.8.2 CONSECUTIVE FRAME TEST");
    Serial.println(
        "Frame DeltaUs CaptureUs CRC32    Lit Status"
    );

    int successful = 0;

    for (int i = 0; i < BURST_FRAMES; i++) {
        uint32_t delta = 0;

        if (i > 0 &&
            info[i].startUs &&
            info[i - 1].startUs) {
            delta =
                info[i].startUs -
                info[i - 1].startUs;
        }

        if (info[i].status == FRAME_OK)
            successful++;

        const char *statusText = "NOT_CAPTURED";

        switch (info[i].status) {
            case FRAME_OK:
                statusText = "OK";
                break;
            case FRAME_SYNC_TIMEOUT:
                statusText = "SYNC_TIMEOUT";
                break;
            case FRAME_ROW_TIMEOUT:
                statusText = "ROW_TIMEOUT";
                break;
            case FRAME_TOTAL_TIMEOUT:
                statusText = "FRAME_TIMEOUT";
                break;
        }

        Serial.printf(
            "%5d %7lu %9u %08lX %4u %s\n",
            i,
            (unsigned long)delta,
            info[i].captureUs,
            (unsigned long)info[i].crc,
            info[i].litPixels,
            statusText
        );
    }

    Serial.printf(
        "BURST SUMMARY: successful=%d/%d\n",
        successful, BURST_FRAMES
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
    tft.setCursor(10, 10);
    tft.println("DMD V0.8.2");

    Serial.println();
    Serial.println("PINBALL DMD CAPTURE");
    Serial.println("Diagnostic V0.8.2");
    Serial.println("Consecutive frame acquisition");
}

void loop() {
    acquireBurst();
    printBurst();
    delay(2000);
}
