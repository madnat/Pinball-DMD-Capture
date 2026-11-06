#include <Arduino.h>

// ============================================================
// PINBALL DMD V1.2g — CPU TIMING GAP DIAGNOSTIC
//
// Original ESP32 / TTGO T-Display
// Serial: 115200
//
// No GPIO reads, PCNT calls, TFT operations or serial output
// during measurement.
//
// This is a diagnostic, not DMD acquisition.
// ============================================================

constexpr uint32_t TEST_DURATION_US = 80000;
constexpr uint32_t GAP_THRESHOLD_US = 1;
constexpr uint32_t MAX_EVENTS = 128;

struct GapEvent {
    uint32_t offsetCycles;
    uint32_t gapCycles;
};

DRAM_ATTR GapEvent events[MAX_EVENTS];

uint32_t cyclesPerUs;
uint32_t testDurationCycles;
uint32_t thresholdCycles;

uint32_t storedEvents;
uint32_t totalEvents;
uint32_t maxGapCycles;
uint32_t maxGapOffsetCycles;
uint32_t elapsedCycles;
uint32_t iterations;
uint32_t testNumber = 0;

// Direct original-ESP32 CPU cycle-counter read.
// Forced inline so there is no function call in the loop.
static inline __attribute__((always_inline))
uint32_t readCycles() {
    uint32_t value;
    asm volatile("rsr.ccount %0" : "=a"(value));
    return value;
}

// Keep the measurement routine in internal instruction RAM.
void IRAM_ATTR measureTiming() {
    noInterrupts();

    const uint32_t start = readCycles();
    uint32_t previous = start;

    uint32_t localStored = 0;
    uint32_t localTotal = 0;
    uint32_t localMaximum = 0;
    uint32_t localMaximumOffset = 0;
    uint32_t localIterations = 0;
    uint32_t now = start;

    do {
        now = readCycles();

        const uint32_t gap = now - previous;
        const uint32_t offset = previous - start;

        previous = now;
        localIterations++;

        if (gap > localMaximum) {
            localMaximum = gap;
            localMaximumOffset = offset;
        }

        if (gap >= thresholdCycles) {
            localTotal++;

            if (false && localStored < MAX_EVENTS) {
                events[localStored].offsetCycles = offset;
                events[localStored].gapCycles = gap;
                localStored++;
            }
        }
    } while ((uint32_t)(now - start) < testDurationCycles);

    storedEvents = localStored;
    totalEvents = localTotal;
    maxGapCycles = localMaximum;
    maxGapOffsetCycles = localMaximumOffset;
    elapsedCycles = now - start;
    iterations = localIterations;

    interrupts();
}

double toUs(uint32_t cycles) {
    return (double)cycles / (double)cyclesPerUs;
}

void reportTiming() {
    testNumber++;

    Serial.printf(
        "\nCPU timing test %lu\n",
        (unsigned long)testNumber
    );

    Serial.printf(
        "Duration=%.3f us iterations=%lu\n",
        toUs(elapsedCycles),
        (unsigned long)iterations
    );

    Serial.printf(
        "Maximum gap=%.3f us (%lu cycles) "
        "at offset=%.3f us\n",
        toUs(maxGapCycles),
        (unsigned long)maxGapCycles,
        toUs(maxGapOffsetCycles)
    );

    Serial.printf(
        "Gaps >= %lu us: %lu; stored=%lu\n",
        (unsigned long)GAP_THRESHOLD_US,
        (unsigned long)totalEvents,
        (unsigned long)storedEvents
    );

    for (uint32_t i = 0; i < storedEvents; i++) {
        Serial.printf(
            "  Event %lu: offset=%.3f us gap=%.3f us",
            (unsigned long)(i + 1),
            toUs(events[i].offsetCycles),
            toUs(events[i].gapCycles)
        );

        if (i > 0) {
            const uint32_t separation =
                events[i].offsetCycles -
                events[i - 1].offsetCycles;

            Serial.printf(
                " sincePrevious=%.3f us",
                toUs(separation)
            );
        }

        Serial.println();
    }

    if (totalEvents > storedEvents) {
        Serial.println(
            "Event buffer full; additional gaps were counted."
        );
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    cyclesPerUs = ESP.getCpuFreqMHz();
    testDurationCycles =
        cyclesPerUs * TEST_DURATION_US;
    thresholdCycles =
        cyclesPerUs * GAP_THRESHOLD_US;

    Serial.println();
    Serial.println("PINBALL DMD V1.2g");
    Serial.println("CPU TIMING GAP DIAGNOSTIC");
    Serial.println("80 ms measurement; interrupts disabled.");
    Serial.println("No DMD inputs or peripherals used.");
    Serial.printf(
        "CPU=%lu MHz core=%d threshold=%lu us\n",
        (unsigned long)cyclesPerUs,
        xPortGetCoreID(),
        (unsigned long)GAP_THRESHOLD_US
    );
}

void loop() {
    // Finish pending output before entering measurement.
    Serial.flush();

    measureTiming();
    reportTiming();

    // Allow normal scheduling between bounded tests.
    delay(1000);
}