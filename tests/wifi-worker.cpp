#include <Arduino.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static uint32_t now = 2000;
uint32_t millis() {
    return now;
}
static constexpr int WL_CONNECTED = 3, pdTRUE = 1;
static int pdMS_TO_TICKS(int value) {
    return value;
}
static int wifiCommands = 0;
static int xQueueReceive(int, void*, int) {
    return 0;
}
enum class EventKind { Wifi };
static std::vector<std::string> messages;
static void post(EventKind, const char* text) {
    messages.emplace_back(text);
}
struct WifiTransport {
    int status() const {
        return WL_CONNECTED;
    }
    String SSID() const {
        return "Example";
    }
    struct Address {
        String toString() const {
            return "192.0.2.1";
        }
    };
    Address localIP() const {
        return {};
    }
    void disconnect(bool, bool) {
    }
    int begin(const char*, const char*) {
        return WL_CONNECTED;
    }
} static WiFi;
struct PreferencesTransport {
    bool failWrite = false;
    size_t putString(const char*, const char* value) const {
        return failWrite ? 0 : strlen(value);
    }
};
static void iteration(PreferencesTransport& prefs, bool& saveOnConnect) {
    struct {
        char ssid[33] = "Example";
        char password[65] = "password";
    } command;
    bool online = true, ok = true;
    uint32_t reportAt = 0, connectAt = 0;
#include "WifiWorker-under-test.h"
    assert(reportAt == now);
}
int main() {
    PreferencesTransport prefs;
    for (bool failure : {false, true}) {
        messages.clear();
        prefs.failWrite = failure;
        bool pending = true;
        iteration(prefs, pending);
        assert(!pending && !messages.empty());
        assert((messages.back().find("saving credentials failed") != std::string::npos) == failure);
        assert(messages.size() == 1);
    }
    return 0;
}
