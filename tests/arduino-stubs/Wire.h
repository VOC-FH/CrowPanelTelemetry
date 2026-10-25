#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>
class TwoWire {
public:
    std::array<uint16_t, 256> registers{};
    int nackRegister = -1;
    uint8_t selected = 0, readOffset = 0;
    std::vector<uint8_t> bytes;
    void beginTransmission(uint8_t) {
        bytes.clear();
    }
    size_t write(uint8_t value) {
        bytes.push_back(value);
        return 1;
    }
    uint8_t endTransmission() {
        uint8_t result = 0;
        if (!bytes.empty()) {
            selected = bytes[0];
            if (bytes.size() == 3) {
                if (selected == nackRegister) {
                    result = 2;
                } else {
                    registers[selected] = uint16_t(bytes[1]) << 8 | bytes[2];
                }
            }
        }
        return result;
    }
    uint8_t requestFrom(uint8_t, uint8_t count) {
        readOffset = 0;
        return count;
    }
    int read() {
        return readOffset++ == 0 ? registers[selected] >> 8 : registers[selected] & 255;
    }
};
extern TwoWire Wire;
