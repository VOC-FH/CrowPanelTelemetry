#pragma once
#include <stdint.h>

// Copy EUIs as hexadecimal integers. Copy keys in TTN's displayed byte order.
#ifndef DEV_EUI
#define DEV_EUI UINT64_C(0x0000000000000000)
#endif
#ifndef JOIN_EUI
#define JOIN_EUI UINT64_C(0x0000000000000000)
#endif
#ifndef APP_KEY
#define APP_KEY {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#endif
#ifndef NWK_KEY
#define NWK_KEY {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
#endif

// Mandatory: inspect your actual shunt. Zero disables current/power calibration.
#ifndef INA_SHUNT_OHMS
#define INA_SHUNT_OHMS 0.0f
#endif
#define INA_MAX_CURRENT_A 0.8f
#define INA_ADDRESS 0x40
#define GPS_BAUD 9600
// The published CoreSX1262 schematic shows a fixed 3V3 TCXO supply via R22.
// 0 disables DIO3 supply control; confirm R22/FB1 population on your module.
#define RADIO_TCXO_VOLTAGE 0.0f
#define AUDIO_FILE "/audio/demo.wav"
// Public TTN allowance: 30 s/day = 1250 ms/hour. See docs/airtime.md.
#define TTN_AIRTIME_MS_PER_HOUR 1250

namespace Pins {
    constexpr int Sck = 2, Mosi = 3, Miso = 4, Cs = 5;
    constexpr int Reset = 25, Busy = 49, Irq = 50, RxEn = 51, TxEn = 52;
    constexpr int Sda = 45, Scl = 46, GpsRx = 48;
    constexpr int TouchReset = 40, TouchIrq = 42;
    constexpr int SdClk = 43, SdCmd = 44, SdD0 = 39;
    constexpr int Bclk = 22, Lrclk = 21, Dout = 23, Mclk = 24, Amp = 30;
    constexpr int BacklightPower = 29, Backlight = 31;
    constexpr int WifiClk = 18, WifiCmd = 19, WifiD0 = 17;
    constexpr int WifiD1 = 16, WifiD2 = 15, WifiD3 = 14, WifiReset = 32;
} // namespace Pins
