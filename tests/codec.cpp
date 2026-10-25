#include "Telemetry.h"

#include <cassert>
#include <cstdio>
#include <initializer_list>
#include <limits>

int main() {
    Batch batch;
    batch.firstMinute = 0x010203;
    batch.samples[0] = {12.34f, -0.37f, 4.6f};
    batch.samples[1] = {36.0f, 10.23f, 102.2f};
    batch.samples[2] = {0, -10.23f, 0};
    batch.samples[3] = {36.01f, 0, 0};
    batch.samples[4] = {1, 10.24f, 1};
    batch.samples[5] = {1, 1, 102.3f};
    batch.samples[6] = {1, NAN, 1};
    batch.samples[7] = {1, 1, std::numeric_limits<float>::infinity()};
    batch.samples[8] = {0, 0, 0};
    batch.samples[9] = {3.3f, 0.22f, 0.7f};
    uint8_t packet[MaxPacketSize];
    size_t len = encodeFragment(batch, 0x1234, 0, 10, packet);
    assert(len == 47 && packet[0] == 1 && packet[1] == 0x12 && packet[2] == 0x34);
    assert(packet[3] == 1 && packet[4] == 2 && packet[5] == 3 && packet[6] == 10);
    for (size_t n = 3; n <= 7; ++n) {
        for (size_t j = 0; j < 4; ++j) {
            assert(packet[7 + n * 4 + j] == 255);
        }
    }
    std::printf("[");
    for (size_t n = 0; n < len; ++n) {
        std::printf("%s%u", n == 0 ? "" : ",", packet[n]);
    }
    std::printf("]\n");
    // Minimum EU868 capacity including all 15 FOpts bytes leaves 36 app bytes.
    // Seven samples then three samples preserve every sample and its index.
    len = encodeFragment(batch, 0x1234, 0, 7, packet);
    assert(len == 35 && len <= 51 - 15 && packet[6] == 7);
    len = encodeFragment(batch, 0x1234, 7, 3, packet);
    assert(len == 19 && packet[6] == 0x73);
    for (uint8_t offset = 0; offset < 10; ++offset) {
        for (uint8_t count = 1; count <= 10 - offset; ++count) {
            len = encodeFragment(batch, 1, offset, count, packet);
            assert(len == HeaderSize + 4 * count && len <= MaxPacketSize);
        }
    }
    assert(airtimeDelay(112, 1250) == 325440);
    assert(airtimeDelay(2629, 1250) == 7574400);
    assert(airtimeDelay(0, 1250) == 0 && airtimeDelay(2000, 0) == 0);
    assert(airtimeDelay(UINT32_MAX, 1) == INT32_MAX);
    const uint8_t expectedDr[] = {5, 4, 3, 2, 1, 0, 0};
    const uint32_t retries[] = {60000, 120000, 240000, 480000, 960000, 1920000, 3600000};
    for (size_t n = 0; n < 7; ++n) {
        assert(joinDatarate(retries[n]) == expectedDr[n]);
    }
    assert(sensorRangeValid(0.080f, 0.79f, 0.8f));
    assert(sensorRangeValid(-0.080f, -0.79f, 0.8f));
    for (float shunt : {32767 * 2.5e-6f, -32768 * 2.5e-6f, 0.082f, -0.082f, NAN}) {
        assert(!sensorRangeValid(shunt, 0.1f, 0.8f));
    }
    assert(!sensorRangeValid(0.080f, 0.8f, 0.8f));
    assert(!sensorRangeValid(-0.080f, -0.8f, 0.8f));
    assert(!sensorRangeValid(0, NAN, 0.8f));
    return 0;
}
