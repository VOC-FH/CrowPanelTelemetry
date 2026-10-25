#pragma once
#include <cmath>
#include <cstdint>
#include <string>
uint32_t millis();
void delay(uint32_t ms);
#define F(value) (value)
template <typename A, typename B>
auto min(A a, B b) {
    return a < b ? a : b;
}
using byte = uint8_t;
constexpr double TWO_PI = 6.2831853071795864769;
inline double radians(double value) {
    return value * TWO_PI / 360;
}
inline double degrees(double value) {
    return value * 360 / TWO_PI;
}
inline double sq(double value) {
    return value * value;
}
class String : public std::string {
public:
    using std::string::string;
    using std::string::operator=;
    String(unsigned long value)
        : std::string(std::to_string(value)) {
    }
};
