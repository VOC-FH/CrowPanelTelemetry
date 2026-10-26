#include "WifiCommand-under-test.h"

#include <Arduino.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
#include <vector>

static uint32_t now = 2000;
uint32_t millis() {
    return now;
}
static constexpr int pdTRUE = 1;
static int pdMS_TO_TICKS(int value) {
    return value;
}
static std::deque<WifiCommand> wifiCommands;
static int xQueueReceive(std::deque<WifiCommand>& queue, void* out, int) {
    bool available = !queue.empty();
    if (available) {
        *static_cast<WifiCommand*>(out) = queue.front();
        queue.pop_front();
    }
    return available ? pdTRUE : 0;
}
enum class EventKind { Wifi };
static std::vector<std::string> messages;
static void post(EventKind, const char* text) {
    messages.emplace_back(text);
}
struct WifiTransport {
    struct {
        bool accepted = true, linked = true, ip = true, immediate = false;
        unsigned calls = 0;
        String ssid = "Old";
        bool connected() const {
            return linked;
        }
        bool hasIP() const {
            return ip;
        }
        bool connect(const char* requested, const char*) {
            ++calls;
            if (accepted) {
                linked = ip = immediate;
                if (immediate) {
                    ssid = requested;
                }
            }
            return accepted;
        }
    } STA;
    String SSID() const {
        return STA.ssid;
    }
    struct Address {
        String toString() const {
            return "192.0.2.1";
        }
    };
    Address localIP() const {
        return {};
    }
} static WiFi;
struct PreferencesTransport {
    bool failWrite = false;
    unsigned writes = 0;
    std::vector<uint8_t> blob;
    size_t getBytes(const char* key, void* out, size_t length) const {
        assert(strcmp(key, "credentials") == 0);
        size_t count = length >= blob.size() ? blob.size() : 0;
        if (count > 0) {
            memcpy(out, blob.data(), count);
        }
        return count;
    }
    size_t putBytes(const char* key, const void* data, size_t length) {
        assert(strcmp(key, "credentials") == 0 && length == sizeof(WifiCommand));
        ++writes;
        if (!failWrite) {
            const auto* bytes = static_cast<const uint8_t*>(data);
            blob.assign(bytes, bytes + length);
        }
        return failWrite ? 0 : length;
    }
};
struct State {
    WifiCommand command;
    bool saveOnConnect = false;
    uint32_t reportAt = 0, connectAt = 0;
};
static WifiCommand load(PreferencesTransport& prefs, bool ok = true) {
    WifiCommand command{};
#include "WifiLoad-under-test.h"
    return command;
}
static void iteration(PreferencesTransport& prefs, State& state, bool ok = true, bool online = true) {
    auto& command = state.command;
    auto& saveOnConnect = state.saveOnConnect;
    auto& reportAt = state.reportAt;
    auto& connectAt = state.connectAt;
    now += 2000;
#include "WifiWorker-under-test.h"
    assert(reportAt == now);
}
static void queue(const char* ssid, const char* password) {
    WifiCommand command;
    snprintf(command.ssid, sizeof(command.ssid), "%s", ssid);
    snprintf(command.password, sizeof(command.password), "%s", password);
    wifiCommands.push_back(command);
}
int main() {
    PreferencesTransport prefs;
    State state;
    assert(load(prefs).ssid[0] == 0);
    prefs.blob.assign(sizeof(WifiCommand) - 1, 1);
    assert(load(prefs).ssid[0] == 0);
    prefs.blob.assign(sizeof(WifiCommand), 1);
    assert(load(prefs).ssid[0] == 0 && load(prefs).password[0] == 0);
    prefs.blob.clear();
    iteration(prefs, state);
    assert(prefs.writes == 0);
    WiFi.STA.accepted = false;
    queue("New", "password");
    iteration(prefs, state);
    assert(prefs.writes == 0 && !state.saveOnConnect);
    assert(messages.back() == "Connection request failed / retry setup");
    iteration(prefs, state);
    assert(prefs.writes == 0);
    for (uint32_t start : {UINT32_C(0), UINT32_C(0xFFFFFC18)}) {
        State rejected;
        now = start;
        WiFi.STA.linked = WiFi.STA.ip = false;
        queue("Rejected", "password");
        iteration(prefs, rejected);
        assert(messages.back() == "Connection request failed / retry setup");
        iteration(prefs, rejected);
        assert(messages.back() == "Connection failed / tap WiFi setup");
        assert(!rejected.saveOnConnect && prefs.writes == 0);
    }
    WiFi.STA.accepted = true;
    queue("New", "");
    iteration(prefs, state);
    assert(prefs.writes == 0 && state.saveOnConnect && messages.back() == "Connecting...");
    WiFi.STA.linked = true;
    WiFi.STA.ssid = "New";
    iteration(prefs, state);
    assert(prefs.writes == 0); // Association without an address is not connection completion.
    WiFi.STA.ip = true;
    WiFi.STA.ssid = "Old";
    iteration(prefs, state);
    assert(prefs.writes == 0); // A different association cannot authorize this record.
    WiFi.STA.ssid = "New";
    iteration(prefs, state);
    assert(prefs.writes == 1 && !state.saveOnConnect);
    WifiCommand saved = load(prefs);
    assert(strcmp(saved.ssid, "New") == 0 && saved.password[0] == 0);
    iteration(prefs, state);
    assert(prefs.writes == 1);
    for (bool failure : {false, true}) {
        auto before = prefs.blob;
        prefs.failWrite = failure;
        WiFi.STA.immediate = true;
        queue("New", "changed-password");
        iteration(prefs, state);
        assert(!state.saveOnConnect);
        assert((messages.back().find("saving credentials failed") != std::string::npos) == failure);
        if (failure) {
            assert(prefs.blob == before);
        } else {
            assert(strcmp(load(prefs).password, "changed-password") == 0);
        }
    }
    unsigned writes = prefs.writes;
    queue("New", "");
    iteration(prefs, state); // Failed open-network write is distinguishable from success.
    assert(prefs.writes == writes + 1 && messages.back() == "Connected; saving credentials failed");
    writes = prefs.writes;
    queue("New", "password");
    iteration(prefs, state, false);
    assert(prefs.writes == writes && !state.saveOnConnect);
    assert(messages.back() == "Connected; saving credentials failed");
    assert(load(prefs, false).ssid[0] == 0);
    unsigned calls = WiFi.STA.calls;
    queue("Other", "password");
    iteration(prefs, state, true, false);
    assert(WiFi.STA.calls == calls && prefs.writes == writes);
    WiFi.STA.immediate = false;
    queue("Missing", "password");
    iteration(prefs, state);
    now += 20001;
    iteration(prefs, state);
    assert(prefs.writes == writes && messages.back() == "Connection failed / tap WiFi setup");
    std::puts("PASS: production WiFi association/result, atomic open/protected credentials, malformed load and NVS failures");
    return 0;
}
