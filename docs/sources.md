# Primary sources checked 2026-10-25

Board repository revision inspected: [cfcbfdbb1fd05997bc5e24dfeaece7fc44b6f61e](https://github.com/Elecrow-RD/CrowPanel-Advanced-10.1inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/commit/cfcbfdbb1fd05997bc5e24dfeaece7fc44b6f61e).
The downloaded source hashes in wiring-verification.txt govern the actual audit.

## Board and modules

- [Elecrow V1.2 PDF](https://github.com/Elecrow-RD/CrowPanel-Advanced-10.1inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/blob/master/Eagle_SCH%26PCB/1.2/ESP32-P4%20Display%2010.1%20inch%20V1.2.pdf)
- [Elecrow V1.2 Eagle schematic](https://github.com/Elecrow-RD/CrowPanel-Advanced-10.1inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/blob/master/Eagle_SCH%26PCB/1.2/ESP32-P4%20Display%2010.1%20inch%20V1.2.sch)
- [Elecrow V1.2 Eagle board](https://github.com/Elecrow-RD/CrowPanel-Advanced-10.1inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/blob/master/Eagle_SCH%26PCB/1.2/ESP32-P4%20Display%2010.1%20inch%20V1.2.brd)
- [Elecrow product wiki](https://media-cdn.elecrow.com/wiki/CrowPanel_Advanced_10.1inch_ESP32-P4_HMI_AI_Display_1024x600_IPS.html): overview only; some radio code predates V1.2 and is not the wiring authority.
- [Waveshare Core1262](https://www.waveshare.com/wiki/Core1262-868M) and [Chinese wiki](https://www.waveshare.net/wiki/Core1262-868M): module interface and corrected RF-switch explanation.
- [Waveshare circuit](https://files.waveshare.com/upload/c/c1/CoreSX1262_Sch.pdf): independent RXEN/TXEN, ANT, TCXO supply alternatives.
- [Texas Instruments INA226](https://www.ti.com/product/INA226) and [datasheet](https://www.ti.com/lit/ds/symlink/ina226.pdf): voltage, shunt, I2C and conversion limits.
- GT-U7: **no authoritative schematic for this exact photographed breakout was established**. Its power/UART specifications and exact pad identification remain conditional. Generic GT-U7 reseller claims were not used as proof.

The schematic PDF was visually inspected and the Eagle XML was programmatically
traced, including J7 pad placement/rotation, level shifters and optional NC parts.
`wiring-verification.txt` records source hashes and the automated net checks.
`tools/check_wiring.py` reproduces these checks against your downloaded Eagle files.

## Current library releases and inspected interfaces

- [Espressif Arduino core 3.3.12](https://github.com/espressif/arduino-esp32/releases/tag/3.3.12), ESP-IDF5.5.5. Local `boards.txt`, P4 variant, hosted HAL, LDO management, SD_MMC, WiFi and ESP_I2S sources inspected. Subsequent correctness tracing found the GPIO45 SD power macro and loss of the slot-deinit flag: the earlier inspection did not establish safe peripheral coexistence. The corrected application uses IDF directly; see correctness-status.md.
- [Arduino_GFX v1.6.9](https://github.com/moononournation/Arduino_GFX/releases/tag/v1.6.9). Its EK79007 DSI implementation and CrowPanel Advanced P4 profile establish the library timing/initialization interface; physical 10.1-inch panel timing still needs testing.
- [RadioLib 7.8.1](https://github.com/jgromes/RadioLib/releases/tag/7.8.1). The exact source is used with the project patch; original/patched hashes and the diff are included. Aggregate operation pacing, immediate nested replies, nonce failure paths and wrap behavior are integration-tested with simulated hardware.
- [INA226 0.6.6](https://github.com/RobTillaart/INA226/releases/tag/0.6.6). Calibration return code, individual read errors, overflow and completed conversions inspected.
- [TinyGPSPlus v1.0.3a](https://github.com/mikalhart/TinyGPSPlus/releases/tag/v1.0.3a).
- [JPEGDEC 1.8.4](https://github.com/bitbank2/JPEGDEC/releases/tag/1.8.4).
- [audio-driver v0.3.1](https://github.com/pschatzmann/arduino-audio-driver/releases/tag/v0.3.1). ES8311 codec setup; it supplies codec registers, while bundled ESP_I2S owns audio transport.
- [RadioLib persistence reference](https://github.com/radiolib-org/radiolib-persistence). Reviewed for API behavior; the sketch adds pre-transmission durable nonce reservation instead of copying a demo's post-join-only save sequence.
- [RadioLib TTN device setup](https://github.com/jgromes/RadioLib/wiki/LoRaWAN:-Device-setup-on-TTN): MAC1.1.0 and Regional Parameters1.0.4.
- [TTN OTAA1.1](https://www.thethingsnetwork.org/docs/lorawan/end-device-activation/).
- [TTN airtime calculator](https://www.thethingsnetwork.org/airtime-calculator) and [duty-cycle/fair-use documentation](https://www.thethingsnetwork.org/docs/lorawan/duty-cycle/).

This is original application code. No CrowPanel demonstration source, vendor
library snapshot or generated SquareLine UI was copied into the project.
Third-party drivers remain dependencies and retain their respective licenses.
The rejected AI wiring illustration is excluded; final wiring uses a deterministic
SVG overlay containing the original photo bytes unchanged.

## Repair evidence

- [IDF v5.5.5 FAT/SD mount cleanup](https://github.com/espressif/esp-idf/blob/v5.5.5/components/fatfs/vfs/vfs_fat_sdmmc.c)
- [IDF v5.5.5 shared SDMMC host](https://github.com/espressif/esp-idf/blob/v5.5.5/components/esp_driver_sdmmc/src/sdmmc_host.c)
- Local core pins_arduino.h, SD_MMC.cpp, GPIO/I2C peripheral manager and fixed/shared
  LDO interfaces were traced; source hashes are correctness-source-hashes.json.
- patches/radiolib-7.8.1.patch records the precise dependency correction; its hash
  manifest is authoritative for applying/verifying it. No installed SDK was patched.
