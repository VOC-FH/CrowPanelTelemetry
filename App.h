#pragma once
#include "Telemetry.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

enum class EventKind { Radio, Downlink, Wifi, Media, Image };
struct Event {
    EventKind kind;
    char text[1056] = {};
    uint16_t* pixels = nullptr;
    uint16_t width = 0, height = 0;
};
struct WifiCommand {
    char ssid[33] = {}, password[65] = {};
};
enum class MediaCommand { Previous, Next, Play, Remount };
struct Model {
    Reading reading;
    String radio = "Starting radio...", downlink = "No downlink received";
    String wifi = "Starting WiFi...", media = "Mounting SD...";
    String gps = "Waiting for GPS data", image = "No image";
    uint32_t minute = 0, lostBatches = 0;
    uint8_t collected = 0;
    bool calibrated = false, sensor = false;
};
extern QueueHandle_t events, batches, demoRequests, mediaCommands, wifiCommands;
extern bool audioReady;
void post(EventKind kind, const char* message);
void startWorkers();
bool beginUi();
void updateUi(Model& model, bool force);
void pollTouch();
void presentImage(Event& event);
void setupApplication();
void loopApplication();
