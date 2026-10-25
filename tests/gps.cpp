#include <Arduino.h>
#include <TinyGPS++.h>
#include <cassert>
#include <cstdio>
#include <deque>
static uint32_t clockMs = 0, lastGpsData = 0;
uint32_t millis() {
    return clockMs;
}
void delay(uint32_t ms) {
    clockMs += ms;
}
static TinyGPSPlus gps;
static struct {
    String gps;
} model;
static struct {
    std::deque<char> bytes;
    size_t available() const {
        return bytes.size();
    }
    char read() {
        char value = bytes.front();
        bytes.pop_front();
        return value;
    }
} Serial1;
static void service() {
#include "Gps-under-test.h"
}
static void sentence(const std::string& body) {
    uint8_t checksum = 0;
    for (char value : body) {
        checksum ^= value;
    }
    char suffix[8];
    snprintf(suffix, sizeof(suffix), "*%02X\r\n", checksum);
    std::string frame = "$" + body + suffix;
    Serial1.bytes.insert(Serial1.bytes.end(), frame.begin(), frame.end());
    service();
}
int main() {
    for (char c : std::string("wrong baud garbage\r\n")) {
        Serial1.bytes.push_back(c);
    }
    service();
    assert(model.gps.find("No valid NMEA") == 0);
    clockMs = UINT32_MAX - 5000;
    sentence("GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W");
    sentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,");
    assert(model.gps.find("48.117300") != std::string::npos && model.gps.find("8 sats") != std::string::npos);
    clockMs += 9999;
    service();
    assert(model.gps.find("48.117300") != std::string::npos);
    ++clockMs;
    service();
    assert(model.gps.find("No valid NMEA") == 0);
    sentence("GPRMC,123520,V,,,,,,,230394,,,N");
    assert(model.gps.find("No fresh fix") == 0);
    clockMs += 10000;
    service();
    assert(model.gps.find("No valid NMEA") == 0);
    std::puts("PASS: real TinyGPSPlus + production GPS status: garbage, valid no-fix/fix, satellites, silence and uint32 wrap");
    return 0;
}
