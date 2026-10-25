#include "Config.h"
#include "Telemetry.h"

#include <INA226.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
TwoWire Wire;
static uint32_t clockMs = 0;
uint32_t millis() {
    return clockMs;
}
void delay(uint32_t ms) {
    clockMs += ms;
}
static struct {
    bool sensor = false, calibrated = false;
    Reading reading;
} model;
static INA226 meter(INA_ADDRESS, &Wire);
static Reading sample() {
    bool currentSlot = true;
    Reading reading;
#include "Sensor-under-test.h"
    return model.reading;
}
int main() {
    Wire.registers[2] = 9600;
    Wire.registers[6] = 8;
    Wire.nackRegister = 5;
    Reading failed = sample();
    if (INA_SHUNT_OHMS > 0) {
        assert(!model.sensor && !model.calibrated);
        assert(!std::isfinite(failed.current) && !std::isfinite(failed.power));
    } else {
        assert(model.sensor && !model.calibrated && failed.voltage == 12);
    }
    Wire.nackRegister = -1;
    Reading zero = sample();
    assert(model.sensor && zero.voltage == 12);
    uint8_t encoded[4];
    encodeReading(zero, encoded);
    if (INA_SHUNT_OHMS > 0) {
        assert(model.calibrated && zero.current == 0 && zero.power == 0);
        assert(!(encoded[0] == 255 && encoded[1] == 255 && encoded[2] == 255 && encoded[3] == 255));
    } else {
        assert(!model.calibrated && !std::isfinite(zero.current));
        assert(encoded[0] == 255 && encoded[1] == 255 && encoded[2] == 255 && encoded[3] == 255);
    }
    std::puts("PASS: production acquisition + real INA226 calibration NACK, true zero and voltage-only contract");
    return 0;
}
