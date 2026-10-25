#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>

struct Reading {
    float voltage = NAN, current = NAN, power = NAN;
};
struct Batch {
    Reading samples[10];
    uint32_t firstMinute = 0;
};
// 11-bit voltage (20 mV), signed 11-bit current (10 mA), 10-bit power (100 mW).
// 0xFFFFFFFF represents an invalid/out-of-range sample.
constexpr size_t HeaderSize = 7, MaxPacketSize = 47;
inline void encodeReading(const Reading& r, uint8_t* out) {
    const bool valid = std::isfinite(r.voltage) && std::isfinite(r.current) && std::isfinite(r.power) && r.voltage >= 0 &&
                       r.voltage <= 36 && std::fabs(r.current) <= 10.23f && r.power >= 0 && r.power <= 102.2f;
    uint32_t packed = UINT32_MAX;
    if (valid) {
        const uint32_t u = std::lround(r.voltage * 50);
        const uint32_t i = static_cast<uint16_t>(static_cast<int16_t>(std::lround(r.current * 100))) & 0x7FF;
        const uint32_t p = std::lround(r.power * 10);
        packed = (u << 21) | (i << 10) | p;
    }
    for (size_t n = 0; n < 4; ++n) {
        out[n] = packed >> (24 - n * 8);
    }
}
inline size_t encodeFragment(const Batch& b, uint16_t boot, uint8_t offset, uint8_t count, uint8_t* out) {
    out[0] = 1;
    out[1] = boot >> 8;
    out[2] = boot;
    out[3] = b.firstMinute >> 16;
    out[4] = b.firstMinute >> 8;
    out[5] = b.firstMinute;
    out[6] = static_cast<uint8_t>((offset << 4) | count);
    for (size_t n = 0; n < count; ++n) {
        encodeReading(b.samples[offset + n], out + HeaderSize + n * 4);
    }
    return HeaderSize + count * 4;
}

// Operation pacing uses the stricter of public-network, regional and network limits.
// Add 1 ms for the stack's integer airtime truncation; bound wrap-safe deadlines.
inline uint32_t airtimeDelay(uint32_t airtimeMs, uint32_t allowanceMsPerHour) {
    uint32_t delay = 0;
    if (airtimeMs > 0 && allowanceMsPerHour > 0) {
        const uint64_t numerator = (static_cast<uint64_t>(airtimeMs) + 1) * 3600000;
        const uint64_t interval = (numerator + allowanceMsPerHour - 1) / allowanceMsPerHour;
        delay = interval > INT32_MAX ? INT32_MAX : static_cast<uint32_t>(interval);
    }
    return delay;
}

inline uint8_t joinDatarate(uint32_t retryMs) {
    uint8_t dr = 5;
    while (dr > 0 && retryMs >= 120000) {
        --dr;
        retryMs /= 2;
    }
    return dr;
}
inline bool sensorRangeValid(float shunt, float current, float limit) {
    return std::isfinite(shunt) && std::isfinite(current) && shunt > -32768 * 2.5e-6f && shunt < 32767 * 2.5e-6f &&
           std::fabs(current) < limit;
}
