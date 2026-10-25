#include "App.h"
#include "Config.h"

#include <Arduino_GFX_Library.h>
#include <Wire.h>

static Arduino_ESP32DSIPanel bus(10, 160, 160, 1, 23, 12, 52000000, 1000);
static Arduino_DSI_Display
    display(1024, 600, &bus, 0, true, 41, ek79007_init_operations, sizeof(ek79007_init_operations) / sizeof(lcd_init_cmd_t));
static constexpr uint16_t Bg = 0x0885, Card = 0x1128, Ink = 0xEF7D, Muted = 0x8CF4, Mint = 0x47D5, Amber = 0xFD8A;
static uint8_t touchAddress = 0x5D;
enum class Screen { Dashboard, Wifi, Downlink };
static Screen screen = Screen::Dashboard;
static bool passwordField = false, touchDown = false;
static uint8_t keyboardPage = 0;
static WifiCommand credentials;
static Model* latest;
static uint16_t* imagePixels;

struct Rect {
    int x, y, w, h;
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
};
static constexpr Rect Previous{44, 520, 90, 46}, Next{144, 520, 90, 46};
static constexpr Rect Play{244, 520, 190, 46}, Remount{444, 520, 180, 46};
static constexpr Rect Demo{704, 490, 280, 56}, Wifi{802, 18, 198, 44};
static const char* keyboards[] = {"1234567890qwertyuiopasdfghjkl;zxcvbnm,./@_-+=!?()[]{}",
                                  "1234567890QWERTYUIOPASDFGHJKL:ZXCVBNM<>?@_-=+!#$%^&*",
                                  "0123456789!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~abcdefghij"};
static void text(int x, int y, const String& value, uint8_t size = 2, uint16_t color = Ink) {
    display.setTextSize(size);
    display.setTextColor(color);
    display.setCursor(x, y);
    display.print(value);
}
static void lines(int x, int y, const String& value, int columns, int rows, uint16_t color = Ink) {
    for (int row = 0; row < rows; ++row) {
        text(x, y + row * 20, value.substring(row * columns, (row + 1) * columns), 2, color);
    }
}
static void button(const Rect& r, const char* label, bool accent = false) {
    display.fillRoundRect(r.x, r.y, r.w, r.h, 10, accent ? Mint : Card);
    int len = strlen(label) * 12;
    text(r.x + (r.w - len) / 2, r.y + (r.h - 16) / 2, label, 2, accent ? Bg : Ink);
}
static void drawImage() {
    display.fillRoundRect(24, 210, 640, 366, 16, Card);
    if (imagePixels) {
        display.draw16bitRGBBitmap(44, 230, imagePixels, 600, 270);
    } else {
        text(88, 348, "Your SD images appear here", 2, Muted);
    }
    button(Previous, "<-");
    button(Next, "->");
    button(Play, "Play sound", true);
    button(Remount, "Retry SD");
}
static const char* wifiInputError() {
    const size_t length = strlen(credentials.password);
    return !credentials.ssid[0] ? "Enter an SSID"
           : (length == 0 || (length >= 8 && length <= 63) ||
              (length == 64 && strspn(credentials.password, "0123456789abcdefABCDEF") == 64))
               ? nullptr
               : "Password: 8-63 characters or 64 hex digits; empty for open WiFi";
}
static void drawKeyboard() {
    display.fillScreen(Bg);
    text(32, 28, "CONNECT TO WIFI", 3);
    text(32, 76, "SSID", 2, Muted);
    text(32, 146, "PASSWORD (tap field to select)", 2, Muted);
    display.fillRoundRect(24, 98, 976, 40, 8, passwordField ? Card : Mint);
    display.fillRoundRect(24, 168, 976, 40, 8, passwordField ? Mint : Card);
    text(36, 110, credentials.ssid, 2, passwordField ? Ink : Bg);
    String mask;
    for (size_t n = 0; n < strlen(credentials.password); ++n) {
        mask += '*';
    }
    text(36, 180, mask, 2, passwordField ? Bg : Ink);
    if (latest) {
        lines(32, 225, wifiInputError() ? wifiInputError() : latest->wifi, 78, 2, Muted);
    }
    const char* keys = keyboards[keyboardPage];
    for (size_t n = 0; n < strlen(keys) && n < 50; ++n) {
        char label[] = {keys[n], 0};
        button({32 + int(n % 10) * 96, 274 + int(n / 10) * 44, 88, 38}, label);
    }
    button({32, 510, 150, 52}, "Delete");
    button({194, 510, 150, 52}, "Space");
    button({356, 510, 150, 52}, "abc/ABC/#!");
    button({680, 510, 150, 52}, "Connect", true);
    button({842, 510, 150, 52}, "Close");
    text(32, 576, "WPA/WPA2 Personal or open WiFi / credentials saved after connection", 1, Muted);
}
void updateUi(Model& model, bool force) {
    latest = &model;
    static uint32_t lastRefresh = 0;
    bool refresh = force || millis() - lastRefresh >= 1000;
    if (refresh && screen == Screen::Dashboard) {
        if (force) {
            display.fillScreen(Bg);
            text(24, 20, "FIELD STATION", 3);
            text(26, 52, "ENERGY / LOCATION / LORAWAN", 1, Muted);
            button(Wifi, "WiFi setup");
            drawImage();
        }
        const char* labels[] = {"VOLTAGE", "CURRENT", "POWER"};
        float values[] = {model.reading.voltage, model.reading.current, model.reading.power};
        const char* units[] = {"V", "A", "W"};
        for (int n = 0; n < 3; ++n) {
            int x = 24 + n * 328;
            display.fillRoundRect(x, 84, 312, 106, 14, Card);
            text(x + 18, 98, labels[n], 2, Muted);
            String value = std::isfinite(values[n]) ? String(values[n], 2) : "--";
            text(x + 18, 132, value + " " + units[n], 4, n == 2 ? Mint : Ink);
        }
        display.fillRoundRect(688, 210, 312, 366, 16, Card);
        text(704, 226, "GPS POSITION", 2, Mint);
        lines(704, 254, model.gps, 23, 3);
        text(704, 320, "TTN / CLASS A", 2, Mint);
        lines(704, 346, model.radio, 23, 3);
        text(704, 412, "DOWNLINK (tap)", 2, Muted);
        lines(704, 438, model.downlink, 23, 2);
        button(Demo, "Send demo", true);
        display.fillRect(24, 194, 976, 14, Bg);
        text(28, 196, model.wifi.substring(0, 78), 1, Muted);
        display.fillRect(24, 578, 976, 22, Bg);
        String status = !model.sensor ? "INA226 missing/error" : (!model.calibrated ? "INA226: check Config.h" : "INA226 ready");
        status = (touchAddress == 0 ? String("GT911 lost/restart | ") : String()) + status;
        status += " | minute " + String(model.minute) + " | batch " + String(model.collected) + "/10 | queued " +
                  String(uxQueueMessagesWaiting(batches)) + " | dropped " + String(model.lostBatches);
        text(24, 578, status, 1, Amber);
        text(24, 590, (model.image + " | " + model.media).substring(0, 156), 1, Muted);
        lastRefresh = millis();
    } else if (refresh && screen == Screen::Wifi) {
        display.fillRect(32, 225, 960, 40, Bg);
        lines(32, 225, wifiInputError() ? wifiInputError() : model.wifi, 78, 2, Muted);
        lastRefresh = millis();
    } else if (refresh && screen == Screen::Downlink) {
        display.fillScreen(Bg);
        text(32, 28, "LAST TTN DOWNLINK", 3);
        lines(32, 100, model.downlink, 78, 14);
        text(32, 420, "Nonprintable bytes appear as hexadecimal escapes.", 2, Muted);
        button({842, 510, 150, 52}, "Close");
        lastRefresh = millis();
    }
}
void presentImage(Event& event) {
    free(imagePixels);
    imagePixels = event.pixels;
    event.pixels = nullptr;
    if (screen == Screen::Dashboard) {
        drawImage();
    }
}
static bool readTouch(uint16_t reg, uint8_t* data, size_t length) {
    Wire.beginTransmission(touchAddress);
    Wire.write(reg >> 8);
    Wire.write(reg & 0xFF);
    bool ok = Wire.endTransmission(false) == 0;
    if (ok) {
        ok = Wire.requestFrom(touchAddress, length) == length;
    }
    if (ok) {
        for (size_t n = 0; n < length; ++n) {
            data[n] = Wire.read();
        }
    }
    return ok;
}
static void tap(int x, int y) {
    if (screen == Screen::Dashboard) {
        if (Wifi.contains(x, y)) {
            screen = Screen::Wifi;
            drawKeyboard();
        } else if (Rect{704, 412, 280, 68}.contains(x, y)) {
            screen = Screen::Downlink;
            updateUi(*latest, true);
        } else if (Demo.contains(x, y)) {
            uint8_t request = 1;
            bool queued = xQueueSend(demoRequests, &request, 0) == pdTRUE;
            latest->radio = queued ? "Demo queued / waiting for join and airtime" : "A demo is already queued";
        } else {
            MediaCommand command{};
            bool selected = true;
            if (Previous.contains(x, y)) {
                command = MediaCommand::Previous;
            } else if (Next.contains(x, y)) {
                command = MediaCommand::Next;
            } else if (Play.contains(x, y)) {
                command = MediaCommand::Play;
            } else if (Remount.contains(x, y)) {
                command = MediaCommand::Remount;
            } else {
                selected = false;
            }
            if (selected && xQueueSend(mediaCommands, &command, 0) != pdTRUE) {
                latest->media = "Media queue full / wait for current operation";
            }
        }
    } else if (screen == Screen::Downlink) {
        if (Rect{842, 510, 150, 52}.contains(x, y)) {
            screen = Screen::Dashboard;
            updateUi(*latest, true);
        }
    } else {
        char* field = passwordField ? credentials.password : credentials.ssid;
        size_t capacity = passwordField ? sizeof(credentials.password) : sizeof(credentials.ssid);
        size_t used = strlen(field);
        if (Rect{24, 98, 976, 40}.contains(x, y)) {
            passwordField = false;
        } else if (Rect{24, 168, 976, 40}.contains(x, y)) {
            passwordField = true;
        } else if (Rect{32, 510, 150, 52}.contains(x, y)) {
            if (used > 0) {
                field[used - 1] = 0;
            }
        } else if (Rect{194, 510, 150, 52}.contains(x, y)) {
            if (used + 1 < capacity) {
                field[used] = ' ';
                field[used + 1] = 0;
            }
        } else if (Rect{356, 510, 150, 52}.contains(x, y)) {
            keyboardPage = (keyboardPage + 1) % 3;
        } else if (Rect{680, 510, 150, 52}.contains(x, y)) {
            if (wifiInputError() == nullptr) {
                xQueueOverwrite(wifiCommands, &credentials);
                memset(credentials.password, 0, sizeof(credentials.password));
                screen = Screen::Dashboard;
            }
        } else if (Rect{842, 510, 150, 52}.contains(x, y)) {
            screen = Screen::Dashboard;
            memset(credentials.password, 0, sizeof(credentials.password));
        } else if (Rect{32, 274, 960, 220}.contains(x, y)) {
            int column = (x - 32) / 96, row = (y - 274) / 44;
            size_t index = row * 10 + column;
            if (index < strlen(keyboards[keyboardPage]) && used + 1 < capacity) {
                field[used] = keyboards[keyboardPage][index];
                field[used + 1] = 0;
            }
        }
        if (screen == Screen::Wifi) {
            drawKeyboard();
        } else if (latest) {
            updateUi(*latest, true);
        }
    }
}
void pollTouch() {
    static uint32_t lastPoll = 0;
    if (touchAddress != 0 && millis() - lastPoll >= 20) {
        uint8_t status = 0;
        bool ok = readTouch(0x814E, &status, 1);
        if (ok && (status & 0x80)) {
            uint8_t point[5] = {};
            bool down = (status & 0x0F) > 0 && (status & 0x0F) <= 5;
            ok = !down || touchDown || readTouch(0x814F, point, sizeof(point));
            if (ok && down && !touchDown) {
                int x = point[1] | (point[2] << 8), y = point[3] | (point[4] << 8);
                if (x < 1024 && y < 600) {
                    tap(x, y);
                }
            }
            touchDown = down;
            Wire.beginTransmission(touchAddress);
            Wire.write(0x81);
            Wire.write(0x4E);
            Wire.write(0);
            ok = Wire.endTransmission() == 0 && ok;
        }
        if (!ok) {
            touchAddress = 0;
            touchDown = false;
            screen = Screen::Dashboard;
            updateUi(*latest, true);
        }
        lastPoll = millis();
    }
}
bool beginUi() {
    pinMode(Pins::BacklightPower, OUTPUT);
    digitalWrite(Pins::BacklightPower, LOW);
    pinMode(Pins::Backlight, OUTPUT);
    digitalWrite(Pins::Backlight, LOW);
    bool ok = display.begin();
    if (ok) {
        display.setTextWrap(false);
        display.fillScreen(Bg);
        digitalWrite(Pins::Backlight, HIGH);
    }
    pinMode(Pins::TouchIrq, OUTPUT);
    digitalWrite(Pins::TouchIrq, LOW);
    pinMode(Pins::TouchReset, OUTPUT);
    digitalWrite(Pins::TouchReset, LOW);
    delay(15);
    digitalWrite(Pins::TouchReset, HIGH);
    delay(60);
    pinMode(Pins::TouchIrq, INPUT);
    delay(60);
    uint8_t id[4] = {};
    bool touchOk = readTouch(0x8140, id, sizeof(id));
    if (!touchOk) {
        touchAddress = 0x14;
        touchOk = readTouch(0x8140, id, sizeof(id));
    }
    if (!touchOk) {
        touchAddress = 0;
    }
    Serial.printf("Display: %s; GT911: %s\n", ok ? "initialized" : "failed", touchOk ? "responding" : "missing");
    return ok;
}
