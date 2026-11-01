
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "soc/gpio_struct.h"
#include <string.h>

TFT_eSPI tft;

// Williams J603 -> ESP32
constexpr int PIN_RD    = 26;
constexpr int PIN_DCLK  = 33;
constexpr int PIN_SDATA = 13;

// DMD geometry
constexpr int WIDTH = 128;
constexpr int HEIGHT = 32;
constexpr int BYTES_PER_ROW = 16;
constexpr int FRAME_BYTES = 512;
constexpr int TOTAL_PIXELS = 4096;

// Capture settings
constexpr int BURST_FRAMES = 16;
constexpr uint32_t ROW_TIMEOUT_US = 600;
constexpr uint32_t FRAME_TIMEOUT_US = 9000;
constexpr uint32_t MARKER_TIMEOUT_US = 25000;
constexpr uint32_t BURST_PAUSE_MS = 2000;

// Frame buffers
uint8_t frames[BURST_FRAMES][FRAME_BYTES];

enum FrameStatus : uint8_t {
    FRAME_OK = 0,
    FRAME_SYNC_TIMEOUT,
    FRAME_ROW_TIMEOUT,
    FRAME_TOTAL_TIMEOUT,
    FRAME_NOT_CAPTURED
};

struct FrameInfo {
    uint32_t startUs;
    uint32_t crc;
    uint16_t litPixels;
    uint16_t captureUs;
    FrameStatus status;
};

FrameInfo info[BURST_FRAMES];

// Direct GPIO access
static inline bool readClock() {
    return (GPIO.in1.val & (1UL << 1)) != 0;
}

static inline bool readData() {
    return (GPIO.in & (1UL << 13)) != 0;
}

static inline bool readRD() {
    return (GPIO.in & (1UL << 26)) != 0;
}

// Initial synchronisation
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

// Proven 128-bit row acquisition
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

// Capture 32 consecutive rows
FrameStatus captureFrameRows(
    uint8_t *destination,
    FrameInfo &result
) {
    memset(destination, 0, FRAME_BYTES);

    result.startUs = micros();

    FrameStatus status = FRAME_OK;

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

// CRC32
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

    for (int i = 0; i < FRAME_BYTES; i++) {
        total += __builtin_popcount(
            (unsigned int)data[i]
        );
    }

    return total;
}

// Capture 16 consecutive refreshes
void acquireBurst() {
    memset(info, 0, sizeof(info));

    for (int i = 0; i < BURST_FRAMES; i++) {
        info[i].status = FRAME_NOT_CAPTURED;
    }

    if (!waitForFrameMarker(MARKER_TIMEOUT_US)) {
        info[0].status = FRAME_SYNC_TIMEOUT;
        return;
    }

    for (int i = 0; i < BURST_FRAMES; i++) {
        info[i].status =
            captureFrameRows(frames[i], info[i]);

        if (info[i].status != FRAME_OK)
            break;
    }

    for (int i = 0; i < BURST_FRAMES; i++) {
        if (info[i].status == FRAME_OK) {
            info[i].crc =
                crc32(frames[i], FRAME_BYTES);

            info[i].litPixels =
                countLitPixels(frames[i]);
        }
    }
}

// Pairwise pixel comparison
struct PairAnalysis {
    uint16_t bothOn;
    uint16_t onlyA;
    uint16_t onlyB;
    uint16_t changed;
};

PairAnalysis compareFrames(
    const uint8_t *a,
    const uint8_t *b
) {
    PairAnalysis result = {};

    for (int i = 0; i < FRAME_BYTES; i++) {
        uint8_t both = a[i] & b[i];
        uint8_t aOnly = a[i] & ~b[i];
        uint8_t bOnly = b[i] & ~a[i];

        result.bothOn +=
            __builtin_popcount((unsigned int)both);

        result.onlyA +=
            __builtin_popcount((unsigned int)aOnly);

        result.onlyB +=
            __builtin_popcount((unsigned int)bOnly);

        result.changed +=
            __builtin_popcount(
                (unsigned int)(a[i] ^ b[i])
            );
    }

    return result;
}

// Count per-pixel activity across 3 refreshes
struct TripleAnalysis {
    uint16_t count[4];
    uint32_t signature;
};

TripleAnalysis analyseTriple(
    const uint8_t *a,
    const uint8_t *b,
    const uint8_t *c
) {
    TripleAnalysis result = {};

    // FNV-1a signature of per-pixel activation counts
    uint32_t hash = 2166136261UL;

    for (int i = 0; i < FRAME_BYTES; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            uint8_t mask = 1U << bit;

            uint8_t activation =
                ((a[i] & mask) ? 1 : 0) +
                ((b[i] & mask) ? 1 : 0) +
                ((c[i] & mask) ? 1 : 0);

            result.count[activation]++;

            hash ^= activation;
            hash *= 16777619UL;
        }
    }

    result.signature = hash;
    return result;
}

const char *statusName(FrameStatus status) {
    switch (status) {
        case FRAME_OK: return "OK";
        case FRAME_SYNC_TIMEOUT: return "SYNC_TIMEOUT";
        case FRAME_ROW_TIMEOUT: return "ROW_TIMEOUT";
        case FRAME_TOTAL_TIMEOUT: return "FRAME_TIMEOUT";
        default: return "NOT_CAPTURED";
    }
}

void printAnalysis() {
    Serial.println();
    Serial.println("V0.9 BRIGHTNESS ANALYSIS");
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

        Serial.printf(
            "%5d %7lu %9u %08lX %4u %s\n",
            i,
            (unsigned long)delta,
            info[i].captureUs,
            (unsigned long)info[i].crc,
            info[i].litPixels,
            statusName(info[i].status)
        );
    }

    Serial.printf(
        "CAPTURE SUMMARY: %d/%d successful\n",
        successful, BURST_FRAMES
    );

    Serial.println();
    Serial.println("PAIRWISE PIXEL COMPARISON");
    Serial.println(
        "Pair    BothOn OnlyA OnlyB Changed"
    );

    for (int i = 0; i < BURST_FRAMES - 1; i++) {
        if (info[i].status != FRAME_OK ||
            info[i + 1].status != FRAME_OK)
            continue;

        PairAnalysis p = compareFrames(
            frames[i], frames[i + 1]
        );

        Serial.printf(
            "%2d-%2d %7u %5u %5u %7u\n",
            i, i + 1,
            p.bothOn,
            p.onlyA,
            p.onlyB,
            p.changed
        );
    }

    Serial.println();
    Serial.println("THREE-FRAME ACTIVATION");
    Serial.println(
        "Frames     Off  Once Twice Three Signature"
    );

    // Sliding 3-frame window.
    // This lets us detect phase alignment.
    for (int i = 0; i < BURST_FRAMES - 2; i++) {
        if (info[i].status != FRAME_OK ||
            info[i + 1].status != FRAME_OK ||
            info[i + 2].status != FRAME_OK)
            continue;

        TripleAnalysis t = analyseTriple(
            frames[i],
            frames[i + 1],
            frames[i + 2]
        );

        Serial.printf(
            "%2d-%2d-%2d %6u %5u %5u %5u %08lX\n",
            i, i + 1, i + 2,
            t.count[0],
            t.count[1],
            t.count[2],
            t.count[3],
            (unsigned long)t.signature
        );
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
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("DMD V0.9");

    Serial.println();
    Serial.println("PINBALL DMD CAPTURE");
    Serial.println("Diagnostic V0.9");
    Serial.println("Temporal brightness reconstruction");
}

void loop() {
    acquireBurst();
    printAnalysis();
    delay(BURST_PAUSE_MS);
}
