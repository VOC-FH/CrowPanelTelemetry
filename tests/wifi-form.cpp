#include "WifiCommand-under-test.h"

#include <Arduino.h>
#include <cassert>
#include <cstring>
#include <vector>
static WifiCommand credentials;
static struct {
    String wifi;
} model;
static auto* latest = &model;
static constexpr uint16_t Muted = 0;
static std::vector<String> labels;
static void lines(int, int, const String& value, int, int, uint16_t) {
    labels.push_back(value);
}
#include "WifiForm-under-test.h"
static void render() {
    labels.clear();
#include "WifiStatus-under-test.h"
    assert(labels.size() == 2 && labels[0] == labels[1]);
}
int main() {
    model.wifi = "Connected / network status";
    render();
    assert(!connectAllowed() && labels[0].find("SSID") != std::string::npos);
    strcpy(credentials.ssid, "Example");
    strcpy(credentials.password, "short");
    render();
    assert(!connectAllowed() && labels[0].find("Password") != std::string::npos);
    model.wifi = "New periodic network update";
    render();
    assert(labels[0].find("Password") != std::string::npos);
    strcpy(credentials.password, "12345678");
    render();
    assert(connectAllowed() && labels[0] == model.wifi);
    memset(credentials.password, 'g', 64);
    credentials.password[64] = 0;
    assert(!connectAllowed());
    memset(credentials.password, 'a', 64);
    assert(connectAllowed());
    credentials.password[0] = 0;
    assert(connectAllowed());
    return 0;
}
