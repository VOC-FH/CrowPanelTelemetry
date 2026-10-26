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

## Accepted independent-review corrections — exact sources

- [NS4263B manufacturer-authored Jun.2023 V1.0 datasheet, sections5/10.1](https://atta.szlcsc.com/upload/public/pdf/source/20240723/A4C23324CBEADB3DCF9750E7545A6E89.pdf)
  specifies SD HIGH >100µs for shutdown and initial LOW/falling edge for AB mode.
  This exact B datasheet is distributor-hosted. [Nsiway's product page](https://www.nsiway.com.cn/list_37/88.html)
  embeds older NS4263 family images with a matching SD truth table; those images
  are corroboration, not a substitute for the B revision or fitted-board inspection.
  The pinned panel schematic connects GPIO30 through R147 to U4.1; no inversion
  was found in that documented path. Physical population remains unverified.
- [Elecrow V1.2 Lesson07 board configuration at cfcbfdbb](https://github.com/Elecrow-RD/CrowPanel-Advanced-10.1inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/blob/cfcbfdbb1fd05997bc5e24dfeaece7fc44b6f61e/example/V1.2/Arduino_Code/Lesson07-Turn_on_the_screen/board_config.h#L30)
  requests51MHz and H70/160/160,V10/23/21 for1024×600. Arduino_GFX1.6.9
  `Arduino_ESP32DSIPanel.cpp:7–12,65–78` maps these constructor arguments directly
  to DPI timing. Aligning those numbers does not verify display operation.
- [LoRaWAN1.1 section4.4.2, ACK MIC input](https://read.uberflip.com/i/1428365-lorawan-specification-v1-1/26)
  uses the least-significant16 bits of the confirmed downlink counter. Section4.3.1.5
  (linked above) prohibits duplicate processing. The approved RadioLib7.8.1 patch
  now handles received app/network counters and the initial/accepted-zero state
  at the protocol authority. Original and patched hashes remain separate in patches/.
- [Arduino core3.3.12 STA connect/disconnect](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/STA.cpp#L373)
  supplies an accepted-request boolean and waits for disconnection of an existing
  link. Raw connection/IP bits are cleared before delayed Arduino status events;
  `NetworkInterface.cpp:316–324` reads those bits. The worker uses those public
  interfaces, the requested SSID and its existing pending flag.
- [Core3.3.12 Preferences putBytes](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/Preferences/src/Preferences.cpp#L285)
  sets one blob, commits, and returns its nonzero length only on success. The
  approved single record replaces separate SSID/password commits, including the
  ambiguous empty-password string result. Physical power-loss recovery is untested.
- [Core3.3.12 Hosted WiFi initialization](https://github.com/espressif/arduino-esp32/blob/3.3.12/libraries/WiFi/src/WiFiGeneric.cpp#L274)
  disables driver NVS and forces RAM storage under CONFIG_ESP_HOSTED_ENABLED;
  no second credential persistence mechanism or redundant app control was added.
- [Pinned ES8311 source](https://github.com/pschatzmann/arduino-audio-driver/blob/v0.3.1/src/Codecs/es8311/ES8311.h#L313)
  drops selected init/volume/read transaction errors (`:504,553–570,637–641`).
  This confirmed limitation is retained under the user's “fix or ignore” scope,
  rather than masked downstream or silently expanded into a third dependency patch.

New source hashes are in correctness-source-hashes.json; final traces and test
limits are in review/accepted-review-fixes.md. All dependency versions remain pinned.


The requested pixel clock is distinct from the derived hardware setting. Exact
IDFv5.5.5 `components/hal/mipi_dsi_hal.c:266–273` rounds the divider to the nearest
integer; `components/esp_lcd/dsi/esp_lcd_panel_dpi.c:265–282` applies it. Both
installed P4 SDK `soc/clk_tree_defs.h:433` select PLL_F240M by default, so the
51MHz request yields nominal240/round(240/51)=48MHz. This is source-derived;
physical frequency and image stability have not been measured. No display
constructor, controller initialization sequence or UI visual was changed here.


## Approved protocol/parser source corrections

- [LoRaWAN1.1 specification](https://resources.lora-alliance.org/technical-specifications/lorawan-specification-v1-1),
  section7/Table17 (printed page65): join-request aggregate budgets and random
  retransmission intervals. Section5.10 (printed page42): RekeyInd persists until
  valid RekeyConf, with ADR-limit fallback. The previous expert-review source
  provenance records the official PDF hash; new tests exercise current source,
  rather than relying on the review's conclusions.
- RadioLib7.8.1's existing queue remains authoritative. The patch now covers
  sendReceive/composeUplink port0 packaging, parseDownlink queue retention/append,
  execMacCommand's valid confirmation, clearSession's paired buffer/length reset,
  and shared queue deletion/pruning. No application-side handshake state or new
  session metadata was added. See [protocol-parser-fix-dispositions.md](review/protocol-parser-fix-dispositions.md) for current
  exact line references and before/after regressions.
- JPEGDEC1.8.4's APP1 EXIF interpretation and its four private TIFF helpers are
  deleted from this isolated profile. The marker walker skips APP1 like other
  unused metadata. This is the approved reduction, not an added app-side parser
  or a general-purpose EXIF fix for other JPEGDEC applications. SOF bounds and
  progressive admission tests remain; exact hashes/diffs are authoritative in patches/.

## PlatformIO alongside Arduino — 2026-10-26

The build configuration pins [pioarduino55.03.312](https://github.com/pioarduino/platform-espressif32/releases/tag/55.03.312),
which packages Arduino3.3.12 and IDF5.5.5 and provides the
[P4 EV-board build manifest](https://github.com/pioarduino/platform-espressif32/blob/55.03.312/boards/esp32-p4-evboard.json).
Its engine constraint requires PlatformIO Core>=6.2.0; Core6.2.0 is pinned in
requirements-platformio.txt. The earlier6.1.19 probe was rejected. Core6.2's
SCons4.11.1 exposes a logging error in this pinned framework's verbose binary action;
normal-mode builds pass. This is documented, not fixed by a third dependency patch.

[PlatformIO package references](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_install.html#local-folder),
[source filtering](https://docs.platformio.org/en/latest/projectconf/sections/env/options/build/build_src_filter.html)
and [core directory configuration](https://docs.platformio.org/en/latest/projectconf/sections/platformio/options/directory/core_dir.html)
establish the shared local-library and isolated-build setup. The
[IDE settings documentation](https://docs.platformio.org/en/latest/integration/ide/vscode.html#settings)
describes selecting a custom Core. The existing tools/patch_dependency.py remains
the only patch implementation; both workflows select the same prepared libraries.

Exact platform tag/asset metadata, package versions, relevant framework parity,
SDK/compiler byte comparisons and build records are in
validation-logs/platformio-support/. Both SDK targets match3155 shared header,
archive-library, linker and flags/configuration files against the read-only
Arduino installation; the C++ compiler executable also matches. These comparisons
establish build inputs, not actual hardware or byte-identical firmware behavior.


## Included Elecrow V1.2 hardware files (2026-10-26)

The manufacturer files are stored unchanged in hardware/elecrow-v1.2/:
[schematic PDF](hardware/elecrow-v1.2/schematic-v1.2.pdf),
[EAGLE schematic](hardware/elecrow-v1.2/panel-v1.2.sch) and
[EAGLE board](hardware/elecrow-v1.2/panel-v1.2.brd).
They were downloaded directly from Elecrow's repository at
cfcbfdbb1fd05997bc5e24dfeaece7fc44b6f61e, the existing wiring-audit baseline.
[Provenance](hardware/elecrow-v1.2/provenance.json) records the original filenames,
immutable URLs, Git blob IDs, byte counts and SHA-256 hashes. The schematic and
board hashes match the earlier audit; no vendor file was edited or converted.
These are CAD sources and a viewing PDF, not SPICE models. The two course
documents explain the distinction and give a connector-tracing exercise.
