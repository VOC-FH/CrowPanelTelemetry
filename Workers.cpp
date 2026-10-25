#include "App.h"
#include "Config.h"
#include "SdStorage.h"

#include <ESP_I2S.h>
#include <JPEGDEC.h>
#include <Preferences.h>
#include <RadioLib.h>
#include <SPI.h>
#include <WiFi.h>
#include <algorithm>
#include <cstdio>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>

QueueHandle_t events, batches, demoRequests, mediaCommands, wifiCommands;
struct Sound {
    uint8_t* bytes;
    size_t length;
};
static QueueHandle_t sounds;

void post(EventKind kind, const char* message) {
    Event event{};
    event.kind = kind;
    strlcpy(event.text, message, sizeof(event.text));
    xQueueSend(events, &event, portMAX_DELAY);
}

// Sole LoRaWAN state authority remains RadioLib. Only persistence uses protected
// fields: reserve the NEXT nonce durably before RadioLib transmits the current one.
// Every boot starts a fresh OTAA session, so frame counters cannot roll backwards.
class PersistentNode : public LoRaWANNode {
public:
    PersistentNode(PhysicalLayer* phy, Preferences& storage)
        : LoRaWANNode(phy, &EU868)
        , store(storage) {
    }
    bool healthy = true;
    bool canTransmit() const {
        return static_cast<int32_t>(millis() - allowedAt) >= 0;
    }
    void defer(uint32_t ms) {
        uint32_t now = millis();
        allowedAt = now + std::max(ms, static_cast<int32_t>(now - allowedAt) >= 0 ? 0U : allowedAt - now);
    }
    int16_t activateOTAA(LoRaWANJoinEvent_t* event) override {
        int16_t rc = RADIOLIB_ERR_UNKNOWN;
        if (devNonce < UINT16_MAX && healthy) {
            uint16_t next = devNonce + 1;
            hton<uint16_t>(&bufferNonces[RADIOLIB_LORAWAN_NONCES_DEV_NONCE], next);
            healthy = store.putBytes("nonces", getBufferNonces(), RADIOLIB_LORAWAN_NONCES_BUF_SIZE) == RADIOLIB_LORAWAN_NONCES_BUF_SIZE;
            hton<uint16_t>(&bufferNonces[RADIOLIB_LORAWAN_NONCES_DEV_NONCE], devNonce);
            getBufferNonces();
            if (healthy) {
                scheduleTransmission(millis());
                rc = LoRaWANNode::activateOTAA(event);
                accountAirtime();
                // A TX error may follow actual RF: never undo the durable reservation.
                devNonce = std::max(devNonce, next);
                hton<uint16_t>(&bufferNonces[RADIOLIB_LORAWAN_NONCES_DEV_NONCE], devNonce);
                healthy =
                    store.putBytes("nonces", getBufferNonces(), RADIOLIB_LORAWAN_NONCES_BUF_SIZE) == RADIOLIB_LORAWAN_NONCES_BUF_SIZE &&
                    healthy;
            }
        } else {
            healthy = false;
        }
        return rc;
    }

    void accountAirtime() {
        uint32_t toa = getLastToA();
        uint32_t allowance = std::min<uint32_t>(band->dutyCycle, dutyCycle ? dutyCycle : band->dutyCycle);
        if (TTN_AIRTIME_MS_PER_HOUR > 0) {
            allowance = std::min<uint32_t>(allowance, TTN_AIRTIME_MS_PER_HOUR);
        }
        defer(std::min<uint64_t>(INT32_MAX, static_cast<uint64_t>(airtimeDelay(toa, allowance)) + toa));
    }

protected:
    void applicationDownlink(const uint8_t* down, size_t length, uint8_t port) override {
        char text[1056];
        size_t used = snprintf(text, sizeof(text), "Port %u: %s", port, length == 0 ? "(empty)" : "");
        for (size_t n = 0; n < length && used + 4 < sizeof(text); ++n) {
            if (down[n] >= 32 && down[n] <= 126) {
                text[used++] = static_cast<char>(down[n]);
            } else {
                used += snprintf(text + used, sizeof(text) - used, "\\x%02X", down[n]);
            }
        }
        text[used] = 0;
        post(EventKind::Downlink, text);
    }

private:
    Preferences& store;
    uint32_t allowedAt = 0;
};

static void radioTask(void*) {
    SPIClass spi(FSPI);
    spi.begin(Pins::Sck, Pins::Miso, Pins::Mosi, Pins::Cs);
    Module module(Pins::Cs, Pins::Irq, Pins::Reset, Pins::Busy, spi, SPISettings(2000000, MSBFIRST, SPI_MODE0));
    SX1262 radio(&module);
    Preferences store;
    PersistentNode node(&radio, store);
    const uint8_t appKey[16] = APP_KEY, nwkKey[16] = NWK_KEY;
    uint8_t anyApp = 0, anyNwk = 0;
    for (size_t n = 0; n < 16; ++n) {
        anyApp |= appKey[n];
        anyNwk |= nwkKey[n];
    }
    bool ready = DEV_EUI != 0 && anyApp != 0 && anyNwk != 0;
    if (!ready) {
        post(EventKind::Radio, "Fill DEV_EUI, JOIN_EUI, APP_KEY, NWK_KEY");
    }
    if (ready) {
        ready = store.begin("crow-lora", false);
        int16_t rc = radio.begin(868.1, 125, 7, 5, RADIOLIB_SX126X_SYNC_WORD_PUBLIC, 14, 8, RADIO_TCXO_VOLTAGE);
        ready = ready && rc == RADIOLIB_ERR_NONE;
        radio.setRfSwitchPins(Pins::RxEn, Pins::TxEn);
        if (ready) {
            ready = radio.setDio2AsRfSwitch(false) == RADIOLIB_ERR_NONE;
        }
        if (!ready) {
            char text[100];
            snprintf(text, sizeof(text), "Radio/NVS init failed (%d); inspect wiring/TCXO", rc);
            post(EventKind::Radio, text);
        }
    }
    if (ready) {
        ready = node.beginOTAA(JOIN_EUI, DEV_EUI, nwkKey, appKey) == RADIOLIB_ERR_NONE;
        if (ready && store.isKey("nonces")) {
            uint8_t nonces[RADIOLIB_LORAWAN_NONCES_BUF_SIZE] = {};
            ready = store.getBytes("nonces", nonces, sizeof(nonces)) == sizeof(nonces) && node.setBufferNonces(nonces) == RADIOLIB_ERR_NONE;
        }
        if (!ready) {
            post(EventKind::Radio, "Invalid saved nonces/config; see recovery guide");
        }
        node.setDutyCycle(false); // The application spaces completed operations by aggregate airtime.
        node.setADR(true);
    }
    uint16_t boot = ready ? store.getUShort("boot", 0) + 1 : 0;
    ready = ready && store.putUShort("boot", boot) == sizeof(boot);

    Batch batch;
    uint8_t offset = 10;
    bool demoTurn = true;
    uint32_t joinRetry = 60000;
    for (;;) {
        if (ready && node.canTransmit() && !node.isActivated()) {
            post(EventKind::Radio, "Joining TTN / LoRaWAN 1.1 Class A...");
            ready = node.setDatarate(joinDatarate(joinRetry)) == RADIOLIB_ERR_NONE;
            if (ready) {
                LoRaWANJoinEvent_t event{};
                int16_t rc = node.activateOTAA(&event);
                bool joined = rc == RADIOLIB_LORAWAN_NEW_SESSION && (node.getVersionMajor() == 1);
                ready = node.healthy && !(rc == RADIOLIB_LORAWAN_NEW_SESSION && node.getVersionMajor() != 1);
                if (joined) {
                    joinRetry = 60000;
                    post(EventKind::Radio, "Joined / Class A / EU868 / ADR enabled");
                } else {
                    node.defer(joinRetry);
                    char text[120];
                    snprintf(text, sizeof(text), "Join failed (%d), or server is not 1.1; retry paced", rc);
                    post(EventKind::Radio, text);
                    joinRetry = std::min(joinRetry * 2, 3600000UL);
                    node.clearSession();
                }
            }
        }
        if (ready && node.isActivated() && offset == 10 && (!demoTurn || uxQueueMessagesWaiting(demoRequests) == 0) &&
            xQueueReceive(batches, &batch, 0) == pdTRUE) {
            offset = 0;
        }
        if (ready && node.isActivated() && node.canTransmit()) {
            bool demo = false;
            uint8_t request;
            if (offset == 10) {
                demo = xQueueReceive(demoRequests, &request, 0) == pdTRUE;
            }
            if (offset < 10 || demo) {
                uint8_t payload[MaxPacketSize] = {};
                uint8_t maxLen = node.getMaxPayloadLen();
                uint8_t count = maxLen > HeaderSize ? std::min<int>((maxLen - HeaderSize) / 4, 10 - offset) : 0;
                size_t length = demo ? 4 : encodeFragment(batch, boot, offset, count, payload);
                if (demo) {
                    memcpy(payload, "DEMO", 4);
                }
                if (demo || count > 0) {
                    uint8_t down[255] = {};
                    size_t downLen = sizeof(down);
                    LoRaWANEvent_t upEvent{}, downEvent{};
                    post(EventKind::Radio, "Sending / opening Class A receive windows...");
                    node.scheduleTransmission(millis());
                    int16_t rc = node.sendReceive(payload, length, demo ? 2 : 1, down, &downLen, false, &upEvent, &downEvent);
                    node.accountAirtime();
                    ready = node.healthy;
                    if (rc < 0) {
                        node.defer(60000);
                    }
                    char text[1056];
                    snprintf(text,
                             sizeof(text),
                             "TX %s (%d) / %lu ms airtime / queued %u",
                             rc >= 0 ? "sent" : "error",
                             rc,
                             static_cast<unsigned long>(node.getLastToA()),
                             static_cast<unsigned>(uxQueueMessagesWaiting(batches)));
                    post(EventKind::Radio, text);
                    if (rc >= 0) {
                        if (demo) {
                            demoTurn = false;
                        } else {
                            offset += count;
                            demoTurn = offset == 10;
                        }
                    }
                }
            }
        }
        if (!ready && DEV_EUI != 0 && anyApp && anyNwk) {
            post(EventKind::Radio, "Radio stopped: init, 1.1 negotiation or NVS failure");
            // No further RF operations after a persistence or version failure.
            vTaskSuspend(nullptr);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void mediaTask(void*);
static void wifiTask(void*) {
    Preferences prefs;
    bool ok = prefs.begin("crow-wifi", false);
    WifiCommand command{};
    if (ok) {
        strlcpy(command.ssid, prefs.getString("ssid").c_str(), sizeof(command.ssid));
        strlcpy(command.password, prefs.getString("password").c_str(), sizeof(command.password));
    }
    WiFi.setPins(Pins::WifiClk, Pins::WifiCmd, Pins::WifiD0, Pins::WifiD1, Pins::WifiD2, Pins::WifiD3, Pins::WifiReset);
    bool online = WiFi.mode(WIFI_STA);
    configASSERT(xTaskCreate(mediaTask, "media", 12288, nullptr, 1, nullptr) == pdPASS);
    if (!online) {
        post(EventKind::Wifi, "C6 unavailable / check ESP-Hosted firmware");
    } else if (command.ssid[0]) {
        WiFi.begin(command.ssid, command.password);
    } else {
        post(EventKind::Wifi, "Not configured / tap WiFi setup");
    }
    uint32_t reportAt = 0, connectAt = millis();
    bool saveOnConnect = false;
    for (;;) {
        if (xQueueReceive(wifiCommands, &command, pdMS_TO_TICKS(100)) == pdTRUE && online) {
            WiFi.disconnect(false, false);
            WiFi.begin(command.ssid, command.password);
            connectAt = millis();
            saveOnConnect = true;
        }
        if (millis() - reportAt >= 2000) {
            char text[160];
            if (WiFi.status() == WL_CONNECTED) {
                snprintf(text, sizeof(text), "%s / %s", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
                if (saveOnConnect && ok) {
                    bool saved = prefs.putString("ssid", command.ssid) > 0;
                    saved = prefs.putString("password", command.password) == strlen(command.password) && saved;
                    if (!saved) {
                        post(EventKind::Wifi, "Connected; saving credentials failed");
                    }
                    saveOnConnect = false;
                }
            } else {
                snprintf(text,
                         sizeof(text),
                         "%s",
                         !online
                             ? "C6 unavailable / check firmware and restart"
                             : (command.ssid[0] ? (millis() - connectAt > 20000 ? "Connection failed / tap WiFi setup" : "Connecting...")
                                                : "Not configured / tap WiFi setup"));
            }
            post(EventKind::Wifi, text);
            reportAt = millis();
        }
    }
}

static int jpegDraw(JPEGDRAW* block) {
    for (int y = 0; y < block->iHeight && block->y + y < 270; ++y) {
        memcpy(static_cast<uint16_t*>(block->pUser) + (block->y + y) * 600 + block->x,
               block->pPixels + y * block->iWidth,
               std::min(block->iWidthUsed, 600 - block->x) * 2);
    }
    return 1;
}
static void loadImage(const String& path) {
    Event event{};
    event.kind = EventKind::Image;
    strlcpy(event.text, path.c_str(), sizeof(event.text));
    FILE* file = fopen((String("/sdcard") + path).c_str(), "rb");
    struct stat info{};
    size_t size = file && fstat(fileno(file), &info) == 0 ? info.st_size : 0;
    uint8_t* compressed = size > 0 && size <= 4 * 1024 * 1024 ? static_cast<uint8_t*>(ps_malloc(size)) : nullptr;
    bool ok = compressed && fread(compressed, 1, size, file) == size;
    if (file) {
        fclose(file);
    }
    JPEGDEC jpeg;
    if (ok) {
        ok = jpeg.openRAM(compressed, size, jpegDraw) != 0;
        if (ok) {
            int scale = 0;
            while (scale < 3 && ((jpeg.getWidth() >> scale) > 600 || (jpeg.getHeight() >> scale) > 270)) {
                ++scale;
            }
            event.width = (jpeg.getWidth() + (1 << scale) - 1) >> scale;
            event.height = (jpeg.getHeight() + (1 << scale) - 1) >> scale;
            ok = event.width <= 600 && event.height <= 270;
            event.pixels = ok ? static_cast<uint16_t*>(ps_calloc(600 * 270, 2)) : nullptr;
            ok = ok && event.pixels;
            if (ok) {
                jpeg.setUserPointer(event.pixels);
                jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
                const int scales[] = {0, JPEG_SCALE_HALF, JPEG_SCALE_QUARTER, JPEG_SCALE_EIGHTH};
                ok = jpeg.decode(0, 0, scales[scale]) != 0;
            }
            jpeg.close();
        }
    }
    free(compressed);
    if (!ok) {
        free(event.pixels);
        event.pixels = nullptr;
        strlcpy(event.text, "JPG failed / size, format or SD error", sizeof(event.text));
    }
    xQueueSend(events, &event, portMAX_DELAY);
}
static uint32_t little32(const uint8_t* p) {
    return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24;
}
static void loadSound() {
    FILE* file = fopen("/sdcard" AUDIO_FILE, "rb");
    struct stat info{};
    uint8_t header[44] = {};
    bool ok = audioReady && file && fread(header, 1, 44, file) == 44 && memcmp(header, "RIFF", 4) == 0 &&
              memcmp(header + 8, "WAVEfmt ", 8) == 0 && little32(header + 16) == 16 && header[20] == 1 && header[21] == 0 &&
              header[22] == 2 && header[23] == 0 && little32(header + 24) == 48000 && header[34] == 16 && header[35] == 0 &&
              memcmp(header + 36, "data", 4) == 0;
    Sound sound{};
    sound.length = little32(header + 40);
    ok = ok && sound.length > 0 && sound.length % 4 == 0 && sound.length <= 4 * 1024 * 1024 && fstat(fileno(file), &info) == 0 &&
         info.st_size >= 44 && sound.length <= static_cast<size_t>(info.st_size - 44);
    sound.bytes = ok ? static_cast<uint8_t*>(ps_malloc(sound.length)) : nullptr;
    ok = ok && sound.bytes && fread(sound.bytes, 1, sound.length, file) == sound.length;
    if (file) {
        fclose(file);
    }
    if (ok && xQueueSend(sounds, &sound, 0) == pdTRUE) {
        post(EventKind::Media, "Playing demo.wav / 48 kHz PCM");
    } else {
        free(sound.bytes);
        post(EventKind::Media, "WAV/codec error or playback queue full / see guide");
    }
}
static void mediaTask(void*) {
    SdStorage storage;
    bool mounted = false;
    std::vector<String> images;
    int index = 0;
    MediaCommand command = MediaCommand::Remount;
    for (;;) {
        if (command == MediaCommand::Remount) {
            mounted = storage.remount();
            images.clear();
            if (mounted) {
                DIR* dir = opendir("/sdcard/images");
                struct dirent* entry = dir ? readdir(dir) : nullptr;
                while (entry && images.size() < 128) {
                    String name = entry->d_name, lower = name;
                    lower.toLowerCase();
                    struct stat info{};
                    if ((lower.endsWith(".jpg") || lower.endsWith(".jpeg")) &&
                        stat((String("/sdcard/images/") + name).c_str(), &info) == 0 && S_ISREG(info.st_mode)) {
                        images.push_back(String("/images/") + name);
                    }
                    entry = readdir(dir);
                }
                if (dir) {
                    closedir(dir);
                }
                std::sort(images.begin(), images.end());
                index = 0;
            }
            post(EventKind::Media,
                 mounted ? (images.empty() ? "SD ready / no JPGs in /images" : "SD ready")
                         : "SD mount/unmount failed / Retry SD or restart");
        } else if (mounted && command == MediaCommand::Play) {
            loadSound();
        } else if (mounted && !images.empty()) {
            index = (index + (command == MediaCommand::Next ? 1 : -1) + images.size()) % images.size();
        }
        if (mounted && !images.empty() && command != MediaCommand::Play) {
            loadImage(images[index]);
        }
        if (!mounted && command != MediaCommand::Remount) {
            post(EventKind::Media, "SD unavailable / tap Retry SD");
        }
        xQueueReceive(mediaCommands, &command, portMAX_DELAY);
    }
}
static void audioTask(void*) {
    I2SClass output;
    output.setPins(Pins::Bclk, Pins::Lrclk, Pins::Dout, -1, Pins::Mclk);
    bool ok = output.begin(I2S_MODE_STD, 48000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    Sound sound{};
    for (;;) {
        xQueueReceive(sounds, &sound, portMAX_DELAY);
        if (ok && audioReady) {
            digitalWrite(Pins::Amp, HIGH);
            size_t done = 0;
            while (done < sound.length && ok) {
                size_t chunk = std::min<size_t>(1024, sound.length - done);
                size_t wrote = output.write(sound.bytes + done, chunk);
                ok = wrote > 0;
                done += wrote;
                vTaskDelay(1);
            }
            // Drain the short DMA tail before disabling the amplifier.
            vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(Pins::Amp, LOW);
            post(EventKind::Media, ok ? "Playback finished" : "I2S write failed");
        } else {
            post(EventKind::Media, "Audio unavailable / codec or I2S initialization failed");
        }
        free(sound.bytes);
    }
}
void startWorkers() {
    events = xQueueCreate(24, sizeof(Event));
    batches = xQueueCreate(6, sizeof(Batch));
    demoRequests = xQueueCreate(1, sizeof(uint8_t));
    mediaCommands = xQueueCreate(4, sizeof(MediaCommand));
    wifiCommands = xQueueCreate(1, sizeof(WifiCommand));
    sounds = xQueueCreate(1, sizeof(Sound));
    configASSERT(events && batches && demoRequests && mediaCommands && wifiCommands && sounds);
    configASSERT(xTaskCreate(radioTask, "lora", 12288, nullptr, 2, nullptr) == pdPASS);
    configASSERT(xTaskCreate(wifiTask, "wifi", 8192, nullptr, 1, nullptr) == pdPASS);
    configASSERT(xTaskCreate(audioTask, "audio", 6144, nullptr, 2, nullptr) == pdPASS);
}
