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
static uint32_t randomState = 1, randomCalls = 0;
static uint32_t esp_random() {
    ++randomCalls;
    randomState = randomState * 1664525U + 1013904223U;
    return randomState;
}
struct Preferences {
    bool failBytes = false, failOpen = false, failBoot = false;
    bool begin(const char*, bool) {
        return !failOpen;
    }
    bool isKey(const char*) const {
        return !nonces.empty();
    }
    uint16_t getUShort(const char*, uint16_t fallback) const {
        return fallback;
    }
    size_t putUShort(const char*, uint16_t value) {
        return failBoot ? 0 : sizeof(value);
    }
    size_t getBytes(const char*, uint8_t* out, size_t length) const {
        size_t count = length <= nonces.size() ? length : 0;
        memcpy(out, nonces.data(), count);
        return count;
    }
    std::vector<uint8_t> nonces;
    size_t putBytes(const char*, const uint8_t* bytes, size_t length) {
        if (!failBytes) {
            nonces.assign(bytes, bytes + length);
        }
        return failBytes ? 0 : length;
    }
};
enum class EventKind { Radio, Downlink };
struct TaskSuspended {};
static void vTaskSuspend(void*) {
    throw TaskSuspended{};
}
static std::vector<std::string> messages, radioMessages;
static void post(EventKind kind, const char* text) {
    if (kind == EventKind::Radio) {
        radioMessages.emplace_back(text);
    } else if (kind == EventKind::Downlink) {
        messages.emplace_back(text);
    }
}
// Generated verbatim from the production class by tools/test_radio.py.
#include "Airtime-reference.h"
#include "PersistentNode-under-test.h"

class TestHal : public RadioLibHal {
public:
    bool irqHigh = true;
    TestHal()
        : RadioLibHal(0, 1, 0, 1, 1, 0) {
    }
    void pinMode(uint32_t, uint32_t) override {
    }
    void digitalWrite(uint32_t, uint32_t) override {
    }
    uint32_t digitalRead(uint32_t) override {
        return irqHigh ? 1 : 0;
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
    bool failLaunch = false, failStage = false, realisticAirtime = false, signalReceive = true;
    std::vector<uint8_t> txSpreadingFactors;
    size_t failOnTx = 0, beginCalls = 0;
    int16_t beginResult = 0, switchResult = 0;
    template <typename... Args>
    int16_t begin(Args...) {
        ++beginCalls;
        return beginResult;
    }
    void setRfSwitchPins(int, int) {
    }
    int16_t setDio2AsRfSwitch(bool) {
        return switchResult;
    }
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
    uint8_t spreadingFactor = 7;
    int16_t setDataRate(DataRate_t rate, ModemType_t) override {
        spreadingFactor = rate.lora.spreadingFactor;
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
    RadioLibTime_t calculateTimeOnAir(ModemType_t, DataRate_t rate, PacketConfig_t, size_t length) override {
        return realisticAirtime ? referenceToA[rate.lora.spreadingFactor - 7][length] : 112000;
    }
    int16_t stageMode(RadioModeType_t value, RadioModeConfig_t* cfg) override {
        mode = value;
        if (mode == RADIOLIB_RADIO_MODE_TX) {
            staged.assign(cfg->transmit.data, cfg->transmit.data + cfg->transmit.len);
        }
        return failStage && mode == RADIOLIB_RADIO_MODE_TX ? RADIOLIB_ERR_TX_TIMEOUT : 0;
    }
    int16_t launchMode() override {
        if (mode == RADIOLIB_RADIO_MODE_TX) {
            txSpreadingFactors.push_back(spreadingFactor);
            starts.push_back(clockMs);
            sent.push_back(staged);
        } else {
            active.clear();
            if (!replies.empty()) {
                active = replies.front();
                replies.pop_front();
            }
            if (callback && signalReceive) {
                callback();
            }
        }
        return failLaunch || (mode == RADIOLIB_RADIO_MODE_TX && failOnTx == sent.size()) ? RADIOLIB_ERR_TX_TIMEOUT : 0;
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
    bool requestAuthenticated(std::vector<uint8_t> frame) {
        return verifyMIC(frame.data(), frame.size(), nwkKey);
    }
    uint32_t acceptedNonce() const {
        return joinNonce;
    }
    std::vector<uint8_t> gatewayAccept(uint32_t nonce, bool valid, size_t length = 33, uint8_t revision = 1) {
        std::vector<uint8_t> plain(length, 0), encrypted(length, 0);
        plain[0] = 0x20;
        hton<uint32_t>(&plain[1], nonce, 3);
        hton<uint32_t>(&plain[4], 0x13, 3);
        hton<uint32_t>(&plain[7], 0x01020304);
        plain[11] = revision << 7;
        plain[12] = 1;
        RadioLibSoftwareAES128 cipher;
        uint8_t derivation[16] = {RADIOLIB_LORAWAN_JOIN_ACCEPT_JS_INT_KEY}, integrity[16] = {};
        hton<uint64_t>(&derivation[1], devEUI);
        cipher.init(nwkKey);
        cipher.encryptECB(derivation, sizeof(derivation), integrity);
        std::vector<uint8_t> micInput(11 + length, 0);
        micInput[0] = RADIOLIB_LORAWAN_JOIN_REQUEST_TYPE;
        hton<uint64_t>(&micInput[1], joinEUI);
        hton<uint16_t>(&micInput[9], devNonce);
        memcpy(&micInput[11], plain.data(), length);
        uint32_t mic =
            revision ? generateMIC(micInput.data(), micInput.size() - 4, integrity) : generateMIC(plain.data(), length - 4, appKey);
        hton<uint32_t>(&plain[length - 4], mic);
        if (!valid) {
            plain[length - 1] ^= 1;
        }
        encrypted[0] = plain[0];
        cipher.init(nwkKey);
        cipher.decryptECB(&plain[1], length - 1, &encrypted[1]);
        return encrypted;
    }
    std::vector<uint8_t> decryptUplink(const std::vector<uint8_t>& frame) {
        size_t port = 8 + (frame[5] & 15);
        std::vector<uint8_t> payload(frame.size() - port - 5);
        uint16_t counter = frame[6] | uint16_t(frame[7]) << 8;
        processAES(&frame[port + 1], payload.size(), appSKey, payload.data(), devAddr, counter, 0, 0, true);
        return payload;
    }
    std::vector<uint8_t> gatewayFrame(uint32_t counter,
                                      const std::string& payload,
                                      uint8_t requests,
                                      bool rekey = false,
                                      bool network = false,
                                      bool confirmed = false,
                                      const std::vector<uint8_t>& commands = {},
                                      bool macPayload = false) {
        assert((!network && !macPayload) || payload.empty());
        size_t length = (network && !macPayload ? 12 : 13) + requests + payload.size();
        std::vector<uint8_t> frame(16 + length, 0);
        frame[16] = confirmed ? 0xA0 : 0x60;
        hton<uint32_t>(&frame[17], devAddr);
        frame[21] = macPayload ? 0 : requests;
        hton<uint16_t>(&frame[22], counter);
        std::vector<uint8_t> opts = commands.empty() ? std::vector<uint8_t>(requests, RADIOLIB_LORAWAN_MAC_DEV_STATUS) : commands;
        assert(opts.size() == requests);
        if (rekey) {
            assert(requests == 2);
            opts = {RADIOLIB_LORAWAN_MAC_REKEY, 1};
        }
        processAES(opts.data(),
                   opts.size(),
                   nwkSEncKey,
                   &frame[macPayload ? 25 : 24],
                   devAddr,
                   counter,
                   1,
                   macPayload ? 0 : (network ? 1 : 2),
                   true);
        if (macPayload) {
            frame[24] = 0;
        } else if (!network) {
            frame[24 + requests] = 10;
            processAES(reinterpret_cast<const uint8_t*>(payload.data()),
                       payload.size(),
                       appSKey,
                       &frame[25 + requests],
                       devAddr,
                       counter,
                       1,
                       0,
                       true);
        }
        frame[0] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
        frame[RADIOLIB_LORAWAN_BLOCK_DIR_POS] = 1;
        hton<uint32_t>(&frame[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], devAddr);
        hton<uint32_t>(&frame[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], counter);
        frame[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = length - 4;
        uint32_t mic = generateMIC(frame.data(), frame.size() - 4, sNwkSIntKey);
        hton<uint32_t>(&frame[frame.size() - 4], mic);
        return std::vector<uint8_t>(frame.begin() + 16, frame.end());
    }
    std::vector<uint8_t> uplinkCommands(const std::vector<uint8_t>& frame) {
        uint8_t options = frame[5] & 15, port = frame[8 + options];
        size_t length = port == 0 ? frame.size() - 13 : options;
        assert(port != 0 || options == 0); // MAC commands cannot occupy both FOpts and port0.
        std::vector<uint8_t> commands(length);
        processAES(frame.data() + (port == 0 ? 9 : 8),
                   length,
                   nwkSEncKey,
                   commands.data(),
                   devAddr,
                   ntoh<uint16_t>(&frame[6]),
                   0,
                   port == 0 ? 0 : 1,
                   true);
        return commands;
    }
    int16_t eraseCommand(std::vector<uint8_t>& commands, uint8_t cid) {
        uint8_t length = commands.size();
        int16_t rc = deleteMacCommand(cid, commands.data(), &length, RADIOLIB_LORAWAN_UPLINK);
        if (rc == 0) {
            for (size_t i = length; i < commands.size(); ++i) {
                assert(commands[i] == 0);
            }
        }
        commands.resize(length);
        return rc;
    }
    void pruneCommands(std::vector<uint8_t>& commands) {
        uint8_t length = commands.size();
        clearMacCommands(commands.data(), &length, RADIOLIB_LORAWAN_UPLINK);
        commands.resize(length);
    }
    bool uplinkAcknowledges(const std::vector<uint8_t>& frame, uint16_t downCounter) {
        std::vector<uint8_t> micInput(16 + frame.size() - 4, 0);
        micInput[0] = RADIOLIB_LORAWAN_MIC_BLOCK_MAGIC;
        hton<uint16_t>(&micInput[RADIOLIB_LORAWAN_BLOCK_CONF_FCNT_POS], downCounter);
        micInput[RADIOLIB_LORAWAN_MIC_DATA_RATE_POS] = channels[RADIOLIB_LORAWAN_UPLINK].dr;
        micInput[RADIOLIB_LORAWAN_MIC_CH_INDEX_POS] = channels[RADIOLIB_LORAWAN_UPLINK].idx;
        hton<uint32_t>(&micInput[RADIOLIB_LORAWAN_BLOCK_DEV_ADDR_POS], devAddr);
        hton<uint32_t>(&micInput[RADIOLIB_LORAWAN_BLOCK_FCNT_POS], ntoh<uint16_t>(&frame[6]));
        micInput[RADIOLIB_LORAWAN_MIC_BLOCK_LEN_POS] = frame.size() - 4;
        memcpy(&micInput[16], frame.data(), frame.size() - 4);
        uint32_t mic = generateMIC(micInput.data(), micInput.size(), sNwkSIntKey);
        return (frame[5] & RADIOLIB_LORAWAN_FCTRL_ACK) && (mic & 0xFFFF) == ntoh<uint16_t>(&frame[frame.size() - 4]);
    }
    std::vector<uint8_t> sessionSnapshot(bool legacy, bool network, uint32_t counter) {
        const uint8_t* bytes = getBufferSession();
        std::vector<uint8_t> snapshot(bytes, bytes + RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
        if (legacy) {
            snapshot[RADIOLIB_LORAWAN_SESSION_STATUS] &= 0x0F;
        }
        hton<uint32_t>(&snapshot[network ? RADIOLIB_LORAWAN_SESSION_N_FCNT_DOWN : RADIOLIB_LORAWAN_SESSION_A_FCNT_DOWN], counter);
        hton<uint16_t>(&snapshot[RADIOLIB_LORAWAN_SESSION_SIGNATURE], checkSum16(snapshot.data(), snapshot.size() - 2));
        return snapshot;
    }
};
static void initialize(ProbeNode& node) {
    uint8_t nwk[16] = {1}, app[16] = {2};
    assert(node.beginOTAA(1, 2, nwk, app) == 0);
    node.setDutyCycle(false);
    node.defer(0);
}
static void testConfirmedCounters() {
    for (bool network : {false, true}) {
        Preferences store;
        TestPhy phy;
        ProbeNode node(&phy, store);
        initialize(node);
        node.activeSession();
        uint8_t payload = 1;
        phy.replies.push_back(node.gatewayFrame(3, "prior", 0));
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) > 0);
        phy.replies.push_back(node.gatewayFrame(7, network ? "" : "confirmed", 1, false, network, true));
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) > 0);
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) == 0);
        assert(node.uplinkAcknowledges(phy.sent.back(), 7));
        assert(!node.uplinkAcknowledges(phy.sent.back(), 3));
    }
}
static void testDownlinkCounters() {
    for (bool network : {false, true}) {
        Preferences store;
        TestPhy phy;
        ProbeNode node(&phy, store);
        initialize(node);
        assert(node.setDatarate(5) == 0);
        phy.replies.push_back(node.gatewayAccept(1, true));
        assert(node.activateOTAA(nullptr) == RADIOLIB_LORAWAN_NEW_SESSION);
        uint8_t payload = 1;
        {
            auto fresh = node.sessionSnapshot(false, network, 0);
            Preferences freshStore;
            TestPhy freshPhy;
            ProbeNode restored(&freshPhy, freshStore);
            initialize(restored);
            assert(restored.setBufferNonces(node.getBufferNonces()) == 0 && restored.setBufferSession(fresh.data()) == 0);
            assert(restored.LoRaWANNode::activateOTAA() == RADIOLIB_LORAWAN_SESSION_RESTORED);
            for (bool domain : {network, !network}) {
                freshPhy.replies.push_back(restored.gatewayFrame(0, "", 1, false, domain));
                restored.scheduleTransmission(millis());
                assert(restored.sendReceive(&payload, 1, 1) > 0);
                freshPhy.replies.push_back(restored.gatewayFrame(0, "", 1, false, domain));
                restored.scheduleTransmission(millis());
                assert(restored.sendReceive(&payload, 1, 1) < 0);
            }
        }
        auto zero = node.gatewayFrame(0, network ? "" : "zero", 1, false, network);
        auto corrupt = zero;
        corrupt.back() ^= 1;
        phy.replies.push_back(corrupt);
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) == RADIOLIB_ERR_MIC_MISMATCH);
        messages.clear();
        phy.replies.push_back(zero);
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) > 0);
        size_t deliveries = messages.size();
        phy.replies.push_back(zero);
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) < 0);
        assert(messages.size() == deliveries);
        std::vector<uint8_t> nonces(node.getBufferNonces(), node.getBufferNonces() + RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
        for (bool legacy : {false, true}) {
            auto snapshot = node.sessionSnapshot(legacy, network, 0);
            Preferences rebootStore;
            TestPhy rebootPhy;
            ProbeNode reboot(&rebootPhy, rebootStore);
            initialize(reboot);
            assert(reboot.setBufferNonces(nonces.data()) == 0);
            int16_t rc = reboot.setBufferSession(snapshot.data());
            if (legacy) {
                assert(rc == RADIOLIB_ERR_SESSION_DISCARDED);
            } else {
                assert(rc == 0);
                assert(reboot.LoRaWANNode::activateOTAA() == RADIOLIB_LORAWAN_SESSION_RESTORED);
                assert(rebootPhy.sent.empty());
                rebootPhy.replies.push_back(zero);
                reboot.scheduleTransmission(millis());
                assert(reboot.sendReceive(&payload, 1, 1) < 0);
                rebootPhy.replies.push_back(node.gatewayFrame(1, network ? "" : "next", 1, false, network));
                reboot.scheduleTransmission(millis());
                assert(reboot.sendReceive(&payload, 1, 1) > 0);
            }
        }
        for (uint32_t previous : {UINT32_C(0xFFFE), UINT32_C(0xFFFFFFFE)}) {
            auto snapshot = node.sessionSnapshot(false, network, previous);
            Preferences restoredStore;
            TestPhy restoredPhy;
            ProbeNode restored(&restoredPhy, restoredStore);
            initialize(restored);
            assert(restored.setBufferNonces(nonces.data()) == 0 && restored.setBufferSession(snapshot.data()) == 0);
            assert(restored.LoRaWANNode::activateOTAA() == RADIOLIB_LORAWAN_SESSION_RESTORED);
            auto last = restored.gatewayFrame(previous + 1, "", 1, false, network, true);
            restoredPhy.replies.push_back(last);
            restored.scheduleTransmission(millis());
            assert(restored.sendReceive(&payload, 1, 1) > 0);
            restoredPhy.replies.push_back(last);
            restored.scheduleTransmission(millis());
            assert(restored.sendReceive(&payload, 1, 1) < 0);
            assert(restored.uplinkAcknowledges(restoredPhy.sent.back(), static_cast<uint16_t>(previous + 1)));
            restoredPhy.replies.push_back(restored.gatewayFrame(previous + 2, "", 1, false, network));
            restored.scheduleTransmission(millis());
            assert((restored.sendReceive(&payload, 1, 1) > 0) == (previous == 0xFFFE));
        }
        node.clearSession();
        assert(node.setDatarate(5) == 0);
        phy.replies.push_back(node.gatewayAccept(2, true));
        assert(node.activateOTAA(nullptr) == RADIOLIB_LORAWAN_NEW_SESSION);
        phy.replies.push_back(node.gatewayFrame(0, "", 1, false, network));
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) > 0);
    }
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
static void initializeRadio(ProbeNode& node, TestPhy& radio, Preferences& store, bool& ready) {
    const uint8_t appKey[16] = {2}, nwkKey[16] = {1};
#include "Radio-init-under-test.h"
    (void) boot;
}
static void joinIteration(ProbeNode& node, bool& ready, uint32_t& joinRetry) {
#include "Radio-join-under-test.h"
}
static void stopIteration(bool ready) {
    uint8_t anyApp = 1, anyNwk = 1;
#include "Radio-stop-under-test.h"
}
static void testInitialization() {
    for (unsigned fault = 0; fault < 5; ++fault) {
        Preferences store;
        TestPhy phy;
        ProbeNode node(&phy, store);
        bool ready = true;
        store.failOpen = fault == 0;
        phy.beginResult = fault == 1 ? RADIOLIB_ERR_CHIP_NOT_FOUND : 0;
        phy.switchResult = fault == 2 ? RADIOLIB_ERR_UNKNOWN : 0;
        store.failBoot = fault == 3;
        if (fault == 4) {
            store.nonces.assign(3, 0); // too short to be a valid saved nonce buffer
        }
        radioMessages.clear();
        bool suspended = false;
        try {
            initializeRadio(node, phy, store, ready);
            stopIteration(ready);
        } catch (const TaskSuspended&) {
            suspended = true;
        }
        assert(suspended && !ready && phy.sent.empty());
        assert(radioMessages.size() == 1);
        if (fault == 0) {
            assert(phy.beginCalls == 0 && radioMessages.back().find("NVS open failed") == 0);
        } else if (fault == 1 || fault == 2) {
            assert(radioMessages.back().find("Radio init failed") == 0);
        } else if (fault == 4) {
            assert(radioMessages.back().find("Invalid saved nonces/config") == 0);
        } else {
            assert(radioMessages.back().find("Radio stopped:") == 0);
        }
        uint32_t retry = 60000;
        joinIteration(node, ready, retry);
        assert(phy.sent.empty()); // failed initialization cannot reach RF
    }
    Preferences store;
    TestPhy phy;
    ProbeNode node(&phy, store);
    initialize(node);
    uint32_t retry = 60000;
    bool ready = true;
    phy.replies.push_back(node.gatewayAccept(1, true, 33, 0));
    joinIteration(node, ready, retry);
    assert(!ready && !node.isActivated()); // authenticated 1.0 still fails closed
    bool suspended = false;
    try {
        stopIteration(ready);
    } catch (const TaskSuspended&) {
        suspended = true;
    }
    assert(suspended && radioMessages.back().find("Radio stopped:") == 0);
    size_t sent = phy.sent.size();
    joinIteration(node, ready, retry);
    assert(phy.sent.size() == sent);
}
static void testElapsedLoops() {
    Preferences store;
    TestPhy phy;
    ProbeNode node(&phy, store);
    initialize(node);
    node.activeSession();
    uint8_t payload = 1;
    LoRaWANEvent_t event{};
    clockMs = 100000;
    phy.hal.irqHigh = false;
    assert(node.sendReceive(&payload, 1, 1, false, &event) == RADIOLIB_ERR_TX_TIMEOUT);
    assert(event.fPort == 0 && phy.sent.size() == 1 && clockMs > 100112);
    uint64_t start = clockMs;
    phy.hal.irqHigh = true;
    phy.signalReceive = false;
    node.scheduleTransmission(millis());
    assert(node.sendReceive(&payload, 1, 1, false, &event) == 0);
    assert(event.fPort == 1 && clockMs > start + 2000);
}
static void testJoinLoop() {
    Preferences store;
    TestPhy phy;
    phy.realisticAirtime = true;
    ProbeNode node(&phy, store);
    initialize(node);
    bool ready = true;
    uint32_t retry = 60000;
    for (unsigned n = 0; n < 6; ++n) {
        clockMs += 10000000;
        joinIteration(node, ready, retry);
        assert(ready && phy.sent.size() == n + 1 && phy.sent.back()[17] == n);
        assert(phy.txSpreadingFactors.back() == 7 + n);
        assert(node.getLastToA() == referenceToA[n][23] / 1000);
        assert(!node.canTransmit());
        assert(retry == std::min<uint32_t>(60000U << (n + 1), 86400000));
    }
    store.failBytes = true;
    clockMs += 10000000;
    joinIteration(node, ready, retry);
    assert(!ready && phy.sent.size() == 6);
}
static std::vector<uint64_t> joinRetryTrace(uint32_t seed) {
    clockMs = 0;
    randomState = seed;
    randomCalls = 0;
    Preferences store;
    TestPhy phy;
    phy.realisticAirtime = true;
    ProbeNode node(&phy, store);
    initialize(node);
    bool ready = true;
    uint32_t retry = 60000;
    constexpr uint64_t Hour = 3600000, Day = 24 * Hour, Horizon = 60 * Day;
    std::vector<uint64_t> airtimes, waits;
    while (clockMs < Horizon) {
        uint32_t previousRetry = retry;
        joinIteration(node, ready, retry);
        assert(ready && phy.sent.size() == airtimes.size() + 1);
        uint64_t toaUs = referenceToA[phy.txSpreadingFactors.back() - 7][23];
        airtimes.push_back(toaUs);
        uint64_t end = clockMs, low = 0, high = Day + 60000;
        while (low < high) {
            uint64_t middle = (low + high) / 2;
            clockMs = end + middle;
            if (node.canTransmit()) {
                high = middle;
            } else {
                low = middle + 1;
            }
        }
        clockMs = end + low;
        assert(node.canTransmit());
        uint64_t base = std::max<uint64_t>(previousRetry, airtimeDelay(toaUs / 1000, 1250) + toaUs / 1000);
        assert(low >= base && low <= base + 60000);
        waits.push_back(low);
    }
    auto transmitted = [&](uint64_t begin, uint64_t length) {
        uint64_t total = 0, end = begin + length;
        for (size_t n = 0; n < airtimes.size(); ++n) {
            uint64_t start = phy.starts[n] * 1000;
            if (start < end && start + airtimes[n] > begin) {
                total += std::min(start + airtimes[n], end) - std::max(start, begin);
            }
        }
        return total;
    };
    constexpr uint64_t HourUs = Hour * 1000, DayUs = Day * 1000;
    assert(transmitted(0, HourUs) < 36000000);
    assert(transmitted(HourUs, 10 * HourUs) < 36000000);
    assert(transmitted(11 * HourUs, DayUs) < 8700000);
    for (size_t n = 0; n < phy.starts.size(); ++n) {
        uint64_t start = phy.starts[n] * 1000, end = start + airtimes[n];
        for (uint64_t candidate : {start, end, start >= DayUs ? start - DayUs : UINT64_C(0), end >= DayUs ? end - DayUs : UINT64_C(0)}) {
            if (candidate >= 11 * HourUs && candidate + DayUs <= Horizon * 1000) {
                assert(transmitted(candidate, DayUs) < 8700000);
            }
        }
    }
    assert(randomCalls == phy.sent.size() && retry == Day);
    assert(phy.starts.back() > UINT32_MAX); // Actual retry decisions crossed the target millis wrap.
    return waits;
}
static void testRekeyQueueOperations() {
    Preferences store;
    TestPhy phy;
    ProbeNode node(&phy, store);
    const std::vector<uint8_t> queue{
        RADIOLIB_LORAWAN_MAC_DEV_STATUS, 1, 2, RADIOLIB_LORAWAN_MAC_REKEY, 1, RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP};
    for (uint8_t cid : {RADIOLIB_LORAWAN_MAC_DEV_STATUS, RADIOLIB_LORAWAN_MAC_REKEY, RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP}) {
        auto commands = queue;
        assert(node.eraseCommand(commands, cid) == 0);
        std::vector<uint8_t> expected;
        if (cid != RADIOLIB_LORAWAN_MAC_DEV_STATUS) {
            expected.insert(expected.end(), {RADIOLIB_LORAWAN_MAC_DEV_STATUS, 1, 2});
        }
        if (cid != RADIOLIB_LORAWAN_MAC_REKEY) {
            expected.insert(expected.end(), {RADIOLIB_LORAWAN_MAC_REKEY, 1});
        }
        if (cid != RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP) {
            expected.push_back(RADIOLIB_LORAWAN_MAC_RX_TIMING_SETUP);
        }
        assert(commands == expected);
    }
    auto missing = queue;
    assert(node.eraseCommand(missing, 0xFF) == RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND && missing == queue);
    std::vector<uint8_t> malformed{RADIOLIB_LORAWAN_MAC_DEV_STATUS, 1};
    assert(node.eraseCommand(malformed, RADIOLIB_LORAWAN_MAC_DEV_STATUS) == RADIOLIB_ERR_INVALID_CID && malformed.size() == 2);
    std::vector<uint8_t> repeated{
        RADIOLIB_LORAWAN_MAC_DEV_STATUS, 1, 2, RADIOLIB_LORAWAN_MAC_REKEY, 1, RADIOLIB_LORAWAN_MAC_DEV_STATUS, 3, 4};
    node.pruneCommands(malformed);
    assert(malformed.size() == 2);
    node.pruneCommands(repeated);
    assert(repeated == (std::vector<uint8_t>{RADIOLIB_LORAWAN_MAC_REKEY, 1}));
}
static void testMacAnswerRetention() {
    clockMs = 0;
    Preferences store;
    TestPhy phy;
    ProbeNode node(&phy, store);
    initialize(node);
    node.activeSession();
    uint8_t payload = 1;
    const std::vector<uint8_t> commands{RADIOLIB_LORAWAN_MAC_REKEY,
                                        1,
                                        RADIOLIB_LORAWAN_MAC_LINK_CHECK,
                                        7,
                                        2,
                                        RADIOLIB_LORAWAN_MAC_DEV_STATUS,
                                        RADIOLIB_LORAWAN_MAC_DEVICE_TIME,
                                        4,
                                        3,
                                        2,
                                        1,
                                        128};
    phy.replies.push_back(node.gatewayFrame(0, "combined answers", commands.size(), false, false, false, commands));
    assert(node.sendReceive(&payload, 1, 1) > 0);
    uint8_t margin = 0, gateways = 0;
    assert(node.getMacLinkCheckAns(&margin, &gateways) == 0 && margin == 7 && gateways == 2);
    uint32_t epoch = 0;
    uint16_t fraction = 0;
    assert(node.getMacDeviceTimeAns(&epoch, &fraction, false) == 0);
    assert(epoch >= UINT32_C(0x01020304) && epoch <= UINT32_C(0x01020306) && fraction < 1000);
}
static void testRekeyLifetime() {
    const std::vector<uint8_t> indication{RADIOLIB_LORAWAN_MAC_REKEY, 1};
    for (unsigned scenario = 0; scenario < 4; ++scenario) {
        clockMs = 0;
        Preferences store;
        TestPhy phy;
        ProbeNode node(&phy, store);
        initialize(node);
        assert(node.setDatarate(5) == 0);
        phy.replies.push_back(node.gatewayAccept(1, true));
        assert(node.activateOTAA(nullptr) == RADIOLIB_LORAWAN_NEW_SESSION);
        node.setADR(false);
        uint8_t payload = 1;
        phy.replies.push_back(node.gatewayFrame(0, "ordinary", 0));
        assert(node.sendReceive(&payload, 1, 1) > 0);
        assert(node.uplinkCommands(phy.sent.back()) == indication);
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) == 0);
        assert(node.uplinkCommands(phy.sent.back()) == indication);
        auto corrupt = node.gatewayFrame(1, "unauthenticated", 2, true);
        corrupt.back() ^= 1;
        phy.replies.push_back(corrupt);
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) == RADIOLIB_ERR_MIC_MISMATCH);
        phy.replies.push_back(node.gatewayFrame(1, "wrong confirmation", 2, false, false, false, {RADIOLIB_LORAWAN_MAC_REKEY, 2}));
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) > 0);
        phy.replies.push_back(node.gatewayFrame(2, "small reply", 4));
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) > 0);
        assert(node.uplinkCommands(phy.sent.back()) == indication);
        node.scheduleTransmission(millis());
        if (scenario == 3) {
            assert(node.sendReceive(static_cast<const uint8_t*>(nullptr), 0, 0) == 0);
        } else {
            assert(node.sendReceive(&payload, 1, 1) == 0);
        }

        auto small = node.uplinkCommands(phy.sent.back());
        assert(small.size() == 14 && small[0] == indication[0] && small[1] == 1);
        for (size_t i = 2; i < small.size(); i += 3) {
            assert(small[i] == RADIOLIB_LORAWAN_MAC_DEV_STATUS);
        }
        size_t before = phy.sent.size();
        phy.replies.push_back(node.gatewayFrame(3, "large reply", 5));
        if (scenario == 1) {
            phy.replies.push_back(node.gatewayFrame(4, "", 2, true, true, false, {}, true));
        } else if (scenario == 2) {
            phy.failOnTx = before + 2;
        }
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) > 0);
        assert(phy.sent.size() == before + 2);
        auto mac = node.uplinkCommands(phy.sent.back());
        assert((phy.sent.back()[5] & 15) == 0 && phy.sent.back()[8] == 0);
        assert(mac.size() == 17 && mac[0] == indication[0] && mac[1] == 1);
        phy.failOnTx = 0;
        if (scenario == 3) {
            node.scheduleTransmission(millis());
            assert(node.sendReceive(static_cast<const uint8_t*>(nullptr), 0, 0) == 0);
            assert(node.uplinkCommands(phy.sent.back()) == indication);
        }
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) == 0);
        assert(node.uplinkCommands(phy.sent.back()) == (scenario == 1 ? std::vector<uint8_t>{} : indication));
        if (scenario != 1) {
            phy.replies.push_back(node.gatewayFrame(
                5, "confirmed", 3, false, false, false, {RADIOLIB_LORAWAN_MAC_REKEY, 1, RADIOLIB_LORAWAN_MAC_DEV_STATUS}));
            node.scheduleTransmission(millis());
            assert(node.sendReceive(&payload, 1, 1) > 0);
            node.scheduleTransmission(millis());
            assert(node.sendReceive(&payload, 1, 1) == 0);
            auto commands = node.uplinkCommands(phy.sent.back());
            assert(commands.size() == 3 && commands[0] == RADIOLIB_LORAWAN_MAC_DEV_STATUS);
        } else {
            phy.replies.push_back(
                node.gatewayFrame(5, "unsolicited wrong version", 2, false, false, false, {RADIOLIB_LORAWAN_MAC_REKEY, 2}));
            node.scheduleTransmission(millis());
            assert(node.sendReceive(&payload, 1, 1) > 0);
            node.scheduleTransmission(millis());
            assert(node.sendReceive(&payload, 1, 1) == 0 && node.uplinkCommands(phy.sent.back()).empty());
        }
    }
}
static void testRekeyRestoreAndLimit() {
    const std::vector<uint8_t> indication{RADIOLIB_LORAWAN_MAC_REKEY, 1};
    for (bool confirmed : {false, true}) {
        clockMs = 0;
        Preferences store;
        TestPhy phy;
        ProbeNode node(&phy, store);
        initialize(node);
        assert(node.setDatarate(5) == 0);
        phy.replies.push_back(node.gatewayAccept(1, true));
        assert(node.activateOTAA(nullptr) == RADIOLIB_LORAWAN_NEW_SESSION);
        node.setADR(false);
        uint8_t payload = 1;
        if (confirmed) {
            phy.replies.push_back(node.gatewayFrame(0, "confirmed", 2, true));
            assert(node.sendReceive(&payload, 1, 1) > 0);
        }
        auto snapshot = node.sessionSnapshot(false, false, 0);
        std::vector<uint8_t> nonces(node.getBufferNonces(), node.getBufferNonces() + RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
        Preferences rebootStore;
        TestPhy rebootPhy;
        ProbeNode reboot(&rebootPhy, rebootStore);
        initialize(reboot);
        assert(reboot.setBufferNonces(nonces.data()) == 0 && reboot.setBufferSession(snapshot.data()) == 0);
        assert(reboot.LoRaWANNode::activateOTAA() == RADIOLIB_LORAWAN_SESSION_RESTORED && rebootPhy.sent.empty());
        reboot.setADR(false);
        for (unsigned n = 0; n < (confirmed ? 65U : 64U); ++n) {
            reboot.scheduleTransmission(millis());
            assert(reboot.sendReceive(&payload, 1, 1) == 0);
            assert(reboot.uplinkCommands(rebootPhy.sent.back()) == (confirmed ? std::vector<uint8_t>{} : indication));
        }
        size_t sent = rebootPhy.sent.size();
        reboot.scheduleTransmission(millis());
        int16_t rc = reboot.sendReceive(&payload, 1, 1);
        if (confirmed) {
            assert(rc == 0 && reboot.isActivated() && rebootPhy.sent.size() == sent + 1);
        } else {
            assert(rc == RADIOLIB_ERR_NETWORK_NOT_JOINED && !reboot.isActivated() && rebootPhy.sent.size() == sent);
            assert(reboot.setDatarate(5) == 0);
            rebootPhy.replies.push_back(reboot.gatewayAccept(2, true));
            assert(reboot.activateOTAA(nullptr) == RADIOLIB_LORAWAN_NEW_SESSION);
            assert(reboot.sendReceive(&payload, 1, 1) == 0 && reboot.uplinkCommands(rebootPhy.sent.back()) == indication);
        }
    }
}
static void testJoinRetryWindows() {
    auto first = joinRetryTrace(1), second = joinRetryTrace(2);
    assert(first != second);
    Preferences store;
    TestPhy phy;
    clockMs = 0;
    ProbeNode node(&phy, store);
    initialize(node);
    node.defer(UINT32_MAX, 60000);
    clockMs = INT32_MAX - 1;
    assert(!node.canTransmit());
    ++clockMs;
    assert(node.canTransmit());
}
static void testTransmitOutcome() {
    Preferences storage;
    TestPhy phy;
    ProbeNode node(&phy, storage);
    initialize(node);
    node.activeSession();
    node.payloadLimit = 36;
    Batch batch;
    bool ready = true, demoTurn = false;
    uint8_t offset = 0;
    size_t sentBefore = phy.sent.size();
    phy.failStage = true;
    serviceIteration(node, ready, batch, offset, demoTurn);
    assert(offset == 0 && phy.sent.size() == sentBefore);
    phy.failStage = false;
    phy.failLaunch = true;
    clockMs += 3600000;
    serviceIteration(node, ready, batch, offset, demoTurn);
    assert(offset == 0);
    phy.failLaunch = false;
    auto foreign = node.gatewayFrame(1, "foreign", 0);
    foreign[1] ^= 1;
    phy.replies.push_back(foreign);
    clockMs += 3600000;
    serviceIteration(node, ready, batch, offset, demoTurn);
    assert(offset == 7 && radioMessages.back().find("sent; RX error") != std::string::npos);
    auto badMic = node.gatewayFrame(1, "bad MIC", 0);
    badMic.back() ^= 1;
    phy.replies.push_back(badMic);
    clockMs += 3600000;
    serviceIteration(node, ready, batch, offset, demoTurn);
    assert(offset == 10 && radioMessages.back().find("sent; RX error") != std::string::npos);
    assert(node.decryptUplink(phy.sent.back())[6] == 0x73);
    offset = 0;
    phy.replies.push_back(node.gatewayFrame(1, "outer", 6));
    phy.failOnTx = phy.sent.size() + 2;
    clockMs += 3600000;
    serviceIteration(node, ready, batch, offset, demoTurn);
    assert(offset == 7); // outer completed even though its nested MAC launch failed
}
static void testMacPayloadLifetime() {
    Preferences storage;
    TestPhy phy;
    ProbeNode node(&phy, storage);
    initialize(node);
    node.activeSession();
    uint8_t payload = 1;
    for (unsigned step = 0; step < 3; ++step) {
        unsigned expected = step == 0 ? 2 : 1;
        uint8_t requests = step == 0 ? 6 : (step == 1 ? 1 : 2);
        size_t before = phy.sent.size();
        clockMs += 3600000;
        phy.replies.push_back(node.gatewayFrame(step + 1, "MAC lifetime", requests, step == 2));
        node.scheduleTransmission(millis());
        assert(node.sendReceive(&payload, 1, 1) > 0);
        assert(phy.sent.size() - before == expected);
        assert(node.getLastToA() == expected * 112);
        assert(messages.back() == "Port 10: MAC lifetime");
    }
}
static void testJoinAuthentication() {
    for (unsigned fault = 0; fault < 5; ++fault) {
        for (size_t length : {17U, 33U}) {
            Preferences storage;
            TestPhy phy;
            ProbeNode node(&phy, storage);
            initialize(node);
            assert(node.setDatarate(5) == 0);
            auto corrupt = node.gatewayAccept(0xFFFFFE, false, length, fault == 1 ? 0 : 1);
            if (fault == 2) {
                corrupt.pop_back();
            } else if (fault == 3) {
                corrupt[0] = 0x60;
            } else if (fault == 4) {
                corrupt[5] ^= 0x80; // accept encrypted for another root key/device
            }
            phy.replies.push_back(corrupt);
            LoRaWANJoinEvent_t event{};
            assert(node.activateOTAA(&event) == (fault == 2 || fault == 3 ? RADIOLIB_ERR_DOWNLINK_MALFORMED : RADIOLIB_ERR_MIC_MISMATCH));
            assert(storage.nonces[RADIOLIB_LORAWAN_NONCES_JOIN_NONCE] == 0);
            assert(node.acceptedNonce() == 0 && node.getVersionMajor() == 1);
            node.clearSession();
            assert(node.setDatarate(5) == 0);
            phy.replies.push_back(node.gatewayAccept(1, true, length));
            assert(node.activateOTAA(&event) == RADIOLIB_LORAWAN_NEW_SESSION);
            assert(node.acceptedNonce() == 1 && node.getVersionMajor() == 1);
            assert(phy.sent[0][17] == 0 && phy.sent[1][17] == 1);
            assert(node.requestAuthenticated(phy.sent[0]) && node.requestAuthenticated(phy.sent[1]));
            node.clearSession();
            assert(node.setDatarate(5) == 0);
            phy.replies.push_back(node.gatewayAccept(1, true, length));
            assert(node.activateOTAA(&event) == RADIOLIB_ERR_JOIN_NONCE_INVALID);
            Preferences rebootStorage;
            TestPhy rebootPhy;
            ProbeNode reboot(&rebootPhy, rebootStorage);
            initialize(reboot);
            assert(reboot.setBufferNonces(storage.nonces.data()) == 0 && reboot.acceptedNonce() == 1);
        }
    }
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
    testMacPayloadLifetime();
    testConfirmedCounters();
    testDownlinkCounters();
    testJoinAuthentication();
    testTransmitOutcome();
    testInitialization();
    testJoinLoop();
    testJoinRetryWindows();
    testRekeyLifetime();
    testRekeyQueueOperations();
    testMacAnswerRetention();
    testRekeyRestoreAndLimit();
    testElapsedLoops();
    std::puts(
        "PASS: real RadioLib + production node: 60-day randomized join windows/wrap, RekeyInd wire/queue/reset/restore, aggregate pacing, "
        "recovery, "
        "fairness/fragments and NVS "
        "fail-stop");
    return 0;
}
