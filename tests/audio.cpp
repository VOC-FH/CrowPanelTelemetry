#include "Config.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

static constexpr int HIGH = 1, LOW = 0, OUTPUT = 1;
static int amplifier = -1;
static std::vector<int> levels;
static std::string status;
static unsigned drained = 0;
static void pinMode(int pin, int mode) {
    assert(pin == Pins::Amp && mode == OUTPUT);
}
static void digitalWrite(int pin, int level) {
    assert(pin == Pins::Amp);
    amplifier = level;
    levels.push_back(level);
}
static unsigned pdMS_TO_TICKS(unsigned ms) {
    return ms;
}
static void vTaskDelay(unsigned ticks) {
    if (ticks == 100) {
        assert(amplifier == LOW);
        ++drained;
    }
}
enum class EventKind { Media };
static void post(EventKind, const char* text) {
    status = text;
}
static struct {
    bool fail = false;
    size_t written = 0;
    size_t write(const uint8_t*, size_t length) {
        assert(amplifier == LOW);
        written += fail ? 0 : length;
        return fail ? 0 : length;
    }
} output;
static void startup() {
#include "Amplifier-start-under-test.h"
}
static bool playback(bool ready, bool initialized) {
    bool ok = initialized, audioReady = ready;
    uint8_t bytes[2048] = {};
    struct {
        uint8_t* bytes;
        size_t length;
    } sound{bytes, sizeof(bytes)};
#include "Audio-under-test.h"
    return ok;
}
int main() {
    startup();
    assert(amplifier == HIGH && levels == std::vector<int>{HIGH});
    assert(playback(true, true));
    assert(output.written == 2048 && drained == 1 && amplifier == HIGH);
    assert(levels == (std::vector<int>{HIGH, LOW, HIGH}));
    assert(status == "Playback finished");
    output.fail = true;
    assert(!playback(true, true));
    assert(amplifier == HIGH && status == "I2S write failed");
    size_t changes = levels.size();
    playback(false, true);
    playback(true, false);
    assert(levels.size() == changes && amplifier == HIGH);
    assert(status.find("Audio unavailable") == 0);
    std::puts("PASS: production amplifier shutdown at startup/idle, active-low playback and write/init failures");
    return 0;
}
