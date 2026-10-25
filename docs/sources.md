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

## Deep-review exact sources and power units

Exact tag/commit/archive provenance: validation-logs/deep-fix-dependencies.json;
additional inspected-file hashes: correctness-source-hashes.json. Conclusions use
ESP-IDF v5.5.5 and esp-hosted-mcu v2.12.13 only. The Hosted example at that tag
orders WiFi init before mount; its IDF6-only workaround does not establish exclusion
on5.5.5. The installed Arduino core/SDK were read-only throughout.

- [Hosted v2.12.13 SDIO port](https://github.com/espressif/esp-hosted-mcu/blob/v2.12.13/host/port/esp/freertos/src/port_esp_hosted_host_sdio.c)
- [Hosted v2.12.13 SD-card example](https://github.com/espressif/esp-hosted-mcu/tree/v2.12.13/examples/host_sdcard_with_hosted)
- [IDF v5.5.5 SDMMC transactions](https://github.com/espressif/esp-idf/blob/v5.5.5/components/esp_driver_sdmmc/src/sdmmc_transaction.c)
- [LoRa Alliance RP002-1.0.4, section2.4.3 / printed page33](https://read.uberflip.com/i/1479525-rp002-1-0-4-regional-parameters/32):
  default EU863–870 maximum16dBm EIRP; TXPower0 is that maximum. Footnote15 gives
  ERP(dBm) = EIRP(dBm) −2.15dB. Regional parameters are not a conducted-power limit.
- [ERC/REC70-03 document register](https://docdb.cept.org/document/845) identifies
  the active9October2026 edition and national implementation caveats. Its download
  endpoint failed during this audit; do not infer national authorization from it.
  [CEPT-hosted June2022 Annex1](https://testdocdb.cept.org/download/4358) gives
  868–868.6MHz25mW ERP and1% duty cycle or LBT+AFA. This older edition is context,
  not a claim that all current national restrictions were audited.
- [ETSI EN300220-2 V3.3.1 (2025-03), Table4 bandM / printed page20](https://www.etsi.org/deliver/etsi_en/300200_300299/30022002/03.03.01_60/en_30022002v030301p.pdf):
  868.000–868.600MHz25mW ERP,1% duty cycle or polite spectrum access.25mW is
  13.9794dBm ERP, equivalent to16.1294dBm EIRP using2.15dB. Other sub-bands have
  different limits. Check actual country/channel/access conditions before operation.

RadioLib EU868 powerMax16 is passed to the PHY as conducted dBm, with no antenna
gain/cable-loss adjustment in this app. Initial radio.begin requests14dBm; ADR can
request16dBm. Measure conducted power, antenna gain/loss, EIRP and ERP separately;
no default or antenna value was changed. RF measurements need separate authorization.


## Re-review checkpoint sources

Exact-tag downloads and newly relied-on file hashes are saved separately in
validation-logs/rereview-fix/dependencies.json and source-sha256.json; original
hash records are not overwritten. Source-only claims, diagnostic reproductions
and hardware prerequisites are distinguished in review/rereview-fix-status.md.

- [LoRa Alliance LoRaWAN 1.1 publication](https://resources.lora-alliance.org/technical-specifications/lorawan-specification-v1-1)
  links the official specification viewer. Section4.3.1.5, printed page23,
  requires duplicate downlinks not to be processed again, including repeated
  acknowledgements. This is the protocol basis for the N4 regression requirement;
  an application-side text filter would not repair the radio protocol state.
- [Official viewer, frame counters](https://read.uberflip.com/i/1428365-lorawan-specification-v1-1/22).
- [Hosted v2.12.13 SD coexistence example](https://github.com/espressif/esp-hosted-mcu/tree/v2.12.13/examples/host_sdcard_with_hosted)
  demonstrates a particular startup/mount sequence, not exclusion of all runtime
  mount, deinit and recovery paths. Installed SDK restart-on-failure configuration
  and the exact source's esp_restart call establish a conditional P4 reset path.
- [JPEGDEC 1.8.4 parser](https://github.com/bitbank2/JPEGDEC/blob/1.8.4/src/jpeg.inl):
  openRAM parsing precedes the application's admission guard. The approved N1
  parser-authority patch now validates SOF metadata before writes. Its original
  and patched hashes are in patches/jpegdec-1.8.4-hashes.json.


- [Espressif P4 schematic checklist: GPIO/USB](https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32p4/schematic-checklist-esp32p4.html#usb)
  distinguishes default USB serial/JTAG GPIO24/25 from FS-OTG GPIO26/27.
- [IDF v5.5.5 P4 GPIO low-level driver](https://github.com/espressif/esp-idf/blob/v5.5.5/components/hal/esp32p4/include/hal/gpio_ll.h)
  explicitly disables default USB pads during IOMUX selection and overrides the
  default DP pull-up during pull-up disable. The installed header is byte-identical;
  its hash is recorded. This qualifies the review's low-confidence runtime-conflict
  suggestion, without claiming measured reset-time or MCLK behavior.
