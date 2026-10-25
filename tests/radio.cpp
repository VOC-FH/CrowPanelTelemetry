#include "Config.h"
#include "Telemetry.h"

#include <RadioLib.h>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

static uint64_t clockMs = 0;
static uint32_t millis() {
    return static_cast<uint32_t>(clockMs);
}
struct Preferences {
    bool failBytes = false;
    std::vector<uint8_t> nonces;
    size_t putBytes(const char*, const uint8_t* bytes, size_t length) {
        if (!failBytes) {
            nonces.assign(bytes, bytes + length);
        }
        return failBytes ? 0 : length;
    }
};
enum class EventKind { Radio, Downlink };
static std::vector<std::string> messages;
static void post(EventKind kind, const char* text) {
    if (kind == EventKind::Downlink) {
        messages.emplace_back(text);
    }
}
// Generated verbatim from the production class by tools/test_radio.py.
#include "PersistentNode-under-test.h"

class TestHal : public RadioLibHal {
public:
    TestHal()
        : RadioLibHal(0, 1, 0, 1, 1, 0) {
    }
    void pinMode(uint32_t, uint32_t) override {
    }
    void digitalWrite(uint32_t, uint32_t) override {
    }
    uint32_t digitalRead(uint32_t) override {
        return 1;
    }
    void attachInterrupt(uint32_t, void (*)(void), uint32_t) override {
    }
    void detachInterrupt(uint32_t) override {
    }
    void delay(RadioLibTime_t ms) override {
        clockMs += ms;
    }
    void delayMicroseconds(RadioLibTime_t us) override {
        clockMs += (us + 999) / 1000;
    }
    RadioLibTime_t millis() override {
        return ::millis();
    }
    RadioLibTime_t micros() override {
        return ::millis() * 1000UL;
    }
    long pulseIn(uint32_t, uint32_t, RadioLibTime_t) override {
        return 0;
    }
    void spiBegin() override {
    }
    void spiBeginTransaction() override {
    }
    void spiTransfer(uint8_t*, size_t, uint8_t*) override {
    }
    void spiEndTransaction() override {
    }
    void spiEnd() override {
    }
    void yield() override {
        ++clockMs;
    }
};
class TestPhy : public PhysicalLayer {
public:
    TestHal hal;
    Module module{&hal, 1, 2, 3, 4};
    bool failLaunch = false;
    RadioModeType_t mode{};
    void (*callback)() = nullptr;
    std::vector<uint64_t> starts;
    std::vector<std::vector<uint8_t>> sent;
    std::deque<std::vector<uint8_t>> replies;
    std::vector<uint8_t> active, staged;
    TestPhy() {
        for (size_t n = 0; n < 10; ++n) {
            irqMap[n] = 1UL << n;
        }
    }
    Module* getMod() override {
        return &module;
    }
    int16_t standby() override {
        return 0;
    }
    int16_t checkDataRate(DataRate_t, ModemType_t) override {
        return 0;
    }
    int16_t setDataRate(DataRate_t, ModemType_t) override {
        return 0;
    }
    int16_t setFrequency(float) override {
        return 0;
    }
    int16_t checkOutputPower(int8_t p, int8_t* clipped) override {
        *clipped = p;
        return 0;
    }
    int16_t setOutputPower(int8_t) override {
        return 0;
    }
    int16_t invertIQ(bool) override {
        return 0;
    }
    int16_t setPreambleLength(size_t) override {
        return 0;
    }
    int16_t setSyncWord(uint8_t*, size_t) override {
        return 0;
    }
    uint8_t randomByte() override {
        return 42;
    }
    RadioLibTime_t calculateTimeOnAir(ModemType_t, DataRate_t, PacketConfig_t, size_t) override {
        return 112000;
    }
    int16_t stageMode(RadioModeType_t value, RadioModeConfig_t* cfg) override {
        mode = value;
        if (mode == RADIOLIB_RADIO_MODE_TX) {
            staged.assign(cfg->transmit.data, cfg->transmit.data + cfg->transmit.len);
        }
        return 0;
    }
    int16_t launchMode() override {
        if (mode == RADIOLIB_RADIO_MODE_TX) {
            starts.push_back(clockMs);
            sent.push_back(staged);
        } else {
            active.clear();
            if (!replies.empty()) {
                active = replies.front();
                replies.pop_front();
            }
            if (callback) {
                callback();
            }
        }
        return failLaunch ? RADIOLIB_ERR_TX_TIMEOUT : 0;
    }
    int16_t finishTransmit() override {
        return 0;
    }
    void setPacketReceivedAction(void (*value)()) override {
        callback = value;
    }
    void clearPacketReceivedAction() override {
        callback = nullptr;
    }
    uint32_t getIrqFlags() override {
        return 1UL << (active.empty() ? RADIOLIB_IRQ_TIMEOUT : RADIOLIB_IRQ_RX_DONE);
    }
    int16_t clearIrqFlags(uint32_t) override {
        return 0;
    }
    size_t getPacketLength(bool) override {
        return active.size();
    }
    int16_t readData(uint8_t* out, size_t length) override {
        memcpy(out, active.data(), length);
        return 0;
    }
};
class ProbeNode : public PersistentNode {
public:
    using PersistentNode::PersistentNode;
    void activeSession() {
        createSession();
        sessionStatus = RADIOLIB_LORAWAN_SESSION_ACTIVE;
        rev = 1;
        devAddr = 0x01020304;
    }
    uint8_t payloadLimit = 0;
    uint8_t getMaxPayloadLen() override {
        return payloadLimit ? payloadLimit : LoRaWANNode::getMaxPayloadLen();
    }
    std::vector<uint8_t> decryptUplink(const std::vector<uint8_t>& frame) {
        size_t port = 8 + (frame[5] & 15);
        std::vector<uint8_t> payload(frame.size() - port - 5);
        uint16_t counter = frame[6] | uint16_t(frame[7]) << 8;
        processAES(&frame[port + 1], payload.size(), appSKey, payload.data(), devAddr, counter, 0, 0, true);
        return payload;
    }
    std::vector<uint8_t> gatewayFrame(uint32_t counter, const std::string& payload, uint8_t requests) {
        size_t length = 13 + requests + payload.size();
        std::vector<uint8_t> frame(16 + length, 0);
        frame[16] = 0x60;
        hton<uint32_t>(&frame[17], devAddr);
        frame[21] = requests;
        hton<uint16_t>(&frame[22], counter);
        std::vector<uint8_t> opts(requests, RADIOLIB_LORAWAN_MAC_DEV_STATUS);
        processAES(opts.data(), opts.size(), nwkSEncKey, &frame[24], devAddr, counter, 1, 2, true);
        frame[24 + requests] = 10;
        processAES(
            reinterpret_cast<const uint8_t*>(payload.data()), payload.size(), appSKey, &frame[25 + requests], devAddr, counter, 1, 0, true);
        frame[0] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
        frame[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = 1;
        hton<uint32_t>(&frame[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], devAddr);
        hton<uint32_t>(&frame[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], counter);
        frame[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = length - 4;
        uint32_t mic = generateMIC(frame.data(), frame.size() - 4, sNwkSIntKey);
        hton<uint32_t>(&frame[frame.size() - 4], mic);
        return std::vector<uint8_t>(frame.begin() + 16, frame.end());
    }
};
static void initialize(ProbeNode& node) {
    uint8_t key[16] = {1};
    assert(node.beginOTAA(1, 2, key, key) == 0);
    node.setDutyCycle(false);
    node.defer(0);
}
struct TestQueue {
    std::deque<Batch> records;
    std::deque<uint8_t> requests;
};
static TestQueue measurements, requests;
static TestQueue* batches = &measurements;
static TestQueue* demoRequests = &requests;
constexpr int pdTRUE = 1;
static unsigned uxQueueMessagesWaiting(TestQueue* q) {
    return q == batches ? q->records.size() : q->requests.size();
}
static int xQueueReceive(TestQueue* q, void* out, int) {
    bool available = uxQueueMessagesWaiting(q) > 0;
    if (available) {
        if (q == batches) {
            *static_cast<Batch*>(out) = q->records.front();
            q->records.pop_front();
        } else {
            *static_cast<uint8_t*>(out) = q->requests.front();
            q->requests.pop_front();
        }
    }
    return available ? pdTRUE : 0;
}
static void serviceIteration(ProbeNode& node, bool& ready, Batch& batch, uint8_t& offset, bool& demoTurn) {
    uint16_t boot = 1;
#include "Radio-service-under-test.h"
}
int main() {
    Preferences store;
    TestPhy phy;
    ProbeNode node(&phy, store);
    initialize(node);
    clockMs = UINT32_MAX - 200ULL;
    node.defer(0);
    node.defer(1000);
    clockMs += 999;
    assert(!node.canTransmit());
    ++clockMs;
    assert(node.canTransmit());
    clockMs = UINT32_MAX - 111ULL; // 112 ms TX ends at millis()==0: valid RX reference.
    node.defer(0);
    for (int n = 0; n < 6; ++n) {
        assert(node.setDatarate(joinDatarate(60000UL << n)) == 0);
        LoRaWANJoinEvent_t event{};
        int16_t rc = node.activateOTAA(&event);
        assert(rc == RADIOLIB_ERR_NO_JOIN_ACCEPT);
        assert(node.getLastToA() == 112); // resets per attempt, never cumulative
        assert(!node.canTransmit());
        clockMs += airtimeDelay(112, 1250) + 111;
        assert(!node.canTransmit());
        ++clockMs;
        assert(node.canTransmit());
        assert(node.healthy);
        node.clearSession();
    }
    assert(phy.sent.size() == 6);
    assert(phy.starts.front() == UINT32_MAX - 111ULL);
    for (size_t n = 1; n < phy.sent.size(); ++n) {
        assert(phy.starts[n] - phy.starts[n - 1] >= airtimeDelay(112, 1250));
        assert(phy.sent[n][17] == n); // unique DevNonce on each attempted join
    }
    clockMs += 3600000;
    phy.failLaunch = true;
    assert(node.setDatarate(5) == 0);
    LoRaWANJoinEvent_t event{};
    assert(node.activateOTAA(&event) == RADIOLIB_ERR_TX_TIMEOUT);
    assert(node.getLastToA() == 112 && !node.canTransmit());
    node.clearSession();
    clockMs += 3600000;
    phy.failLaunch = false;
    assert(node.setDatarate(5) == 0);
    assert(node.activateOTAA(&event) == RADIOLIB_ERR_NO_JOIN_ACCEPT);
    assert(phy.sent[6][17] == 6 && phy.sent[7][17] == 7); // failed RF must not reuse nonce
    node.activeSession();
    clockMs += 3600000;
    phy.replies.push_back(node.gatewayFrame(1, "outer", 6)); // 18 response bytes force nested MAC uplink
    phy.replies.push_back(node.gatewayFrame(2, "inner", 0));
    node.scheduleTransmission(millis());
    uint8_t payload[] = {1};
    assert(node.sendReceive(payload, sizeof(payload), 1) > 0);
    assert(node.getLastToA() == 224);
    assert(messages == (std::vector<std::string>{"Port 10: outer", "Port 10: inner"}));
    assert(phy.starts.back() - phy.starts[phy.starts.size() - 2] < airtimeDelay(112, 1250));
    node.accountAirtime();
    assert(!node.canTransmit());
    clockMs += airtimeDelay(224, 1250) + 223;
    assert(!node.canTransmit());
    ++clockMs;
    assert(node.canTransmit());
    clockMs += 3600000;
    phy.replies.push_back(node.gatewayFrame(3, "", 0));
    node.scheduleTransmission(millis());
    assert(node.sendReceive(payload, sizeof(payload), 1) > 0);
    assert(messages.back() == "Port 10: (empty)");
    assert(node.setDatarate(5) == 0);
    clockMs += 3600000;
    size_t maximum = node.getMaxPayloadLen();
    std::string binary(maximum, '\x01');
    binary[1] = '\0';
    binary[2] = '\xFF';
    phy.replies.push_back(node.gatewayFrame(4, binary, 0));
    node.scheduleTransmission(millis());
    assert(node.sendReceive(payload, sizeof(payload), 1) > 0);
    assert(messages.back().size() == 9 + maximum * 4);
    assert(messages.back().substr(0, 21) == "Port 10: \\x01\\x00\\xFF");
    clockMs = 0; // reboot: monotonic clock restarts; only NVS survives
    TestPhy rebootPhy;
    ProbeNode reboot(&rebootPhy, store);
    initialize(reboot);
    assert(reboot.setBufferNonces(store.nonces.data()) == 0);
    assert(reboot.canTransmit()); // No persisted airtime debt after restart.
    assert(reboot.setDatarate(5) == 0);
    assert(reboot.activateOTAA(&event) == RADIOLIB_ERR_NO_JOIN_ACCEPT);
    assert(rebootPhy.sent.front()[17] == 8);
    reboot.activeSession();
    reboot.payloadLimit = 36; // simulate MAC payload reduction; actual scheduler is unchanged
    Batch queued;
    for (uint32_t minute : {10U, 20U, 30U}) {
        queued.firstMinute = minute;
        for (Reading& reading : queued.samples) {
            reading = {12, 0.1f, 1.2f};
        }
        measurements.records.push_back(queued);
    }
    requests.requests.push_back(1);
    bool ready = true, demoTurn = true;
    uint8_t offset = 10;
    Batch working;
    for (unsigned step = 0; step < 10; ++step) {
        if (step % 3 == 1) {
            requests.requests.push_back(1);
        }
        clockMs += 3600000;
        serviceIteration(reboot, ready, working, offset, demoTurn);
        assert(ready);
        auto payloadBytes = reboot.decryptUplink(rebootPhy.sent.back());
        if (step % 3 == 0) {
            assert(payloadBytes == (std::vector<uint8_t>{'D', 'E', 'M', 'O'}));
        } else {
            assert(payloadBytes[0] == 1 && payloadBytes[5] == (step / 3 + 1) * 10);
            assert(payloadBytes[6] == (step % 3 == 1 ? 0x07 : 0x73));
        }
    }
    assert(measurements.records.empty() && requests.requests.empty());
    Preferences failedStore;
    failedStore.failBytes = true;
    TestPhy failedPhy;
    ProbeNode failedNode(&failedPhy, failedStore);
    initialize(failedNode);
    assert(failedNode.setDatarate(5) == 0);
    assert(failedNode.activateOTAA(&event) < 0);
    assert(!failedNode.healthy && failedPhy.sent.empty());
    std::puts("PASS: real RadioLib + production node: joins, wrap, aggregate pacing, immediate nested MAC/downlinks, nonce TX-error/reset "
              "recovery, "
              "fairness/fragments and NVS "
              "fail-stop");
    return 0;
}
