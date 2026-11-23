#pragma once
#include <stdint.h>
#include <string.h>

namespace dmd_ws {
constexpr unsigned width = 128;
constexpr unsigned height = 32;
constexpr unsigned planeBytes = width * height / 8;
constexpr char dimensionsCommand[] = "dimensions";
constexpr char frameCommand[] = "gray2Planes";
constexpr unsigned dimensionsBytes = sizeof(dimensionsCommand) + 8;
constexpr unsigned frameHeaderBytes = sizeof(frameCommand) + 4;
constexpr unsigned frameBytes = frameHeaderBytes + 2 * planeBytes;

inline void putU32(uint8_t *out, uint32_t value) {
    for (unsigned byte = 0; byte < 4; ++byte) out[byte] = uint8_t(value >> (8 * byte));
}

inline void dimensions(uint8_t (&out)[dimensionsBytes]) {
    memcpy(out, dimensionsCommand, sizeof(dimensionsCommand));
    putU32(out + sizeof(dimensionsCommand), width);
    putU32(out + sizeof(dimensionsCommand) + 4, height);
}

inline uint64_t acquisitionTime(uint64_t nowUs, uint32_t capturedUs) {
    return nowUs - uint32_t(uint32_t(nowUs) - capturedUs);
}

inline void gray2(uint8_t (&out)[frameBytes], const uint8_t *pixels, uint32_t timestampMs) {
    memcpy(out, frameCommand, sizeof(frameCommand));
    putU32(out + sizeof(frameCommand), timestampMs);
    for (unsigned i = 0; i < planeBytes; ++i) {
        uint8_t low = 0, high = 0;
        for (unsigned bit = 0; bit < 8; ++bit) {
            const uint8_t shade = pixels[8 * i + bit];
            low |= (shade & 1u) << bit;
            high |= ((shade >> 1) & 1u) << bit;
        }
        out[frameHeaderBytes + i] = low;
        out[frameHeaderBytes + planeBytes + i] = high;
    }
}
}
