#include "App.h"
#include "Config.h"

#include <AudioDriver.h>
#include <ESP_I2S.h>
#include <INA226.h>
#include <TinyGPS++.h>
#include <Wire.h>
#include <algorithm>

Model model;
INA226 meter(INA_ADDRESS, &Wire);
TinyGPSPlus gps;
bool audioReady = false;
static Batch collecting;
static uint32_t nextSample = 0;
static bool displayReady = false;
static uint32_t lastGpsData = 0;

void setupApplication() {
    pinMode(Pins::Amp, OUTPUT);
    digitalWrite(Pins::Amp, HIGH);
    Serial.begin(115200);
    Serial.printf("CrowPanel V1.2; ESP32 revision %d; core 3.3.12\n", ESP.getChipRevision());
    Wire.begin(Pins::Sda, Pins::Scl, 100000);
    Wire.setTimeOut(20);
    model.sensor = meter.begin();
    // Initialize the real V1.2 codec, before worker tasks: UI owns I2C thereafter.
    using namespace audio_driver;
    static DriverDeviceInfo codecPins;
    auto& codec = AudioDriverES8311;
    static I2SClass clock;
    clock.setPins(Pins::Bclk, Pins::Lrclk, Pins::Dout, -1, Pins::Mclk);
    bool clockOk = clock.begin(I2S_MODE_STD, 48000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    codecPins.addI2C(PinFunction::CODEC, Wire, false);
    CodecConfig cfg;
    cfg.input_device = ADC_INPUT_NONE;
    cfg.output_device = DAC_OUTPUT_ALL;
    cfg.i2s.rate = RATE_48K;
    cfg.i2s.bits = BIT_LENGTH_16BITS;
    cfg.i2s.mode = MODE_SLAVE;
    cfg.i2s.fmt = I2S_NORMAL;
    audioReady = clockOk && codec.begin(cfg, codecPins) && codec.setVolume(35);
    clock.end();
    Serial1.setRxBufferSize(4096);
    Serial1.begin(GPS_BAUD, SERIAL_8N1, Pins::GpsRx, -1);
    displayReady = beginUi();
    startWorkers();
    nextSample = millis() + 60000;
    if (displayReady) {
        updateUi(model, true);
    }
}
void loopApplication() {
    uint32_t now = millis();
    while (Serial1.available()) {
        lastGpsData = gps.encode(Serial1.read()) ? now : lastGpsData;
    }
    bool fresh = gps.location.isValid() && gps.location.age() < 10000;
    if (fresh) {
        char text[100];
        String satellites = gps.satellites.isValid() && gps.satellites.age() < 10000 ? String(gps.satellites.value()) : "--";
        snprintf(text, sizeof(text), "%.6f, %.6f  |  %s sats", gps.location.lat(), gps.location.lng(), satellites.c_str());
        model.gps = text;
    } else {
        model.gps = (gps.passedChecksum() == 0 || now - lastGpsData >= 10000) ? "No valid NMEA / check UART, baud"
                                                                              : "No fresh fix / move antenna outdoors";
    }
    if (static_cast<int32_t>(now - nextSample) >= 0) {
        // Account for missed deadlines as invalid slots; never fabricate catch-up readings.
        do {
            model.minute++;
            Reading reading;
            bool currentSlot = static_cast<int32_t>(now - nextSample) < 60000;
            model.sensor =
                meter.isConnected() &&
                (INA_SHUNT_OHMS <= 0 || (meter.setMaxCurrentShunt(INA_MAX_CURRENT_A, INA_SHUNT_OHMS) == 0 && meter.getLastError() == 0));
            model.calibrated = model.sensor && INA_SHUNT_OHMS > 0;
            if (currentSlot && model.sensor) {
                // Trigger one coherent conversion after calibration. The bounded
                // <=60 ms sensor wait leaves radio/audio on their own tasks.
                bool converted = meter.setAverage(INA226_16_SAMPLES) && meter.setModeShuntBusTrigger() && meter.waitConversionReady(60);
                reading.voltage = meter.getBusVoltage();
                bool ok = meter.getLastError() == 0 && converted;
                if (model.calibrated) {
                    float shunt = meter.getShuntVoltage();
                    ok = meter.getLastError() == 0 && ok;
                    uint16_t flags = meter.getAlertRegister();
                    ok = meter.getLastError() == 0 && ok && !(flags & INA226_MATH_OVERFLOW_FLAG);
                    reading.current = meter.getCurrent();
                    ok = meter.getLastError() == 0 && ok;
                    reading.power = meter.getPower();
                    ok = meter.getLastError() == 0 && ok &&
                         sensorRangeValid(shunt, reading.current, std::min(INA_MAX_CURRENT_A, meter.getMaxCurrent()));
                }
                if (!ok) {
                    model.sensor = false;
                    reading = Reading{};
                }
            }
            model.reading = reading;
            if (model.collected == 0) {
                collecting.firstMinute = model.minute;
            }
            collecting.samples[model.collected++] = reading;
            if (model.collected == 10) {
                if (xQueueSend(batches, &collecting, 0) != pdTRUE) {
                    model.lostBatches++;
                }
                model.collected = 0;
            }
            nextSample += 60000;
        } while (static_cast<int32_t>(now - nextSample) >= 0);
    }
    Event event;
    while (xQueueReceive(events, &event, 0) == pdTRUE) {
        if (event.kind == EventKind::Radio) {
            model.radio = event.text;
        } else if (event.kind == EventKind::Downlink) {
            model.downlink = event.text;
        } else if (event.kind == EventKind::Wifi) {
            model.wifi = event.text;
        } else if (event.kind == EventKind::Media) {
            model.media = event.text;
        } else if (event.kind == EventKind::Image) {
            model.image = event.text;
            if (displayReady) {
                presentImage(event);
            }
            free(event.pixels);
        }
        Serial.println(event.text);
    }
    if (displayReady) {
        pollTouch();
        updateUi(model, false);
    }
    delay(5);
}
