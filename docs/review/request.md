Please act as an independent senior IoT, ESP32-P4, electronics and LoRaWAN 1.1 engineer. Review the completed CrowPanelTelemetry project for FUNCTIONALITY AND CORRECTNESS ONLY.

This is a read-only expert review. Do not change files, implement fixes, create checkouts, install dependencies, flash hardware, send RF packets, access a TTN account, or contact anyone. Return your report in your response. You may inspect the project and the explicitly provided Espressif SDK directory, and use official web documentation/source code for verification. Do not read credentials, personal files or unrelated projects. You do not have shell execution tools; do not claim to have run builds or hardware tests.

NON-NEGOTIABLE SCOPE:
- The user accepts the current UI/UX. Do NOT suggest a nicer UI, visual redesign, different colors/fonts/layout, another graphics framework, or aesthetic enhancements.
- UI behavior correctness (touch, data freshness, errors, responsiveness, clipping of necessary information) is in scope.
- The user explicitly chose to KEEP the current J7 LoRa wiring. Do not propose migrating to the upper-right wireless extension socket as an improvement. Flag only actual functional/electrical defects in the chosen wiring.
- Do not automatically fix anything. Findings and minimal corrections only; fixes will be decided separately.

PROJECT: current working directory /home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry (standalone outside Git).
Start with AGENTS.md, README.md, Config.h, Application.cpp, Workers.cpp, Ui.cpp, App.h, Telemetry.h, ttn-decoder.js, tests/, docs/wiring.md, docs/airtime.md, docs/validation.md, docs/sources.md and tools/check_wiring.py. Inspect docs/wiring.png or SVG against the actual module pinouts, not just the labels. Check the tests against the implementation and look for gaps; compilation alone is not functional proof.

HARDWARE AND REQUESTED BEHAVIOR:
- Elecrow CrowPanel Advanced ESP32-P4 10.1-inch 1024x600, PCB V1.2, onboard C6 WiFi, EK79007 MIPI DSI, GT911 touch, SD, ES8311 codec/NS4263B amplifier and speakers.
- External GT-U7 GPS (receive NMEA), INA226 voltage/current/power monitor, Waveshare Core1262-868M / SX1262.
- WiFi configuration through touchscreen with persisted credentials.
- Browse SD JPGs with <- and ->; button plays SD audio; show fresh GPS coordinates/fix state.
- LoRaWAN MAC 1.1.0, Class A only, OTAA, EU868, TTN. Credentials are editable preprocessor macros DEV_EUI, JOIN_EUI, APP_KEY, NWK_KEY; no ClientId/session keys need filling for OTAA.
- Demo uplink button; show received application downlinks, including binary/text and long messages.
- Sample U/I/P every minute, show readings, and submit all ten readings after each ten slots. Public TTN airtime limits take precedence over unconditional ten-minute delivery under poor RF conditions. Bounded RAM queue/drop reporting is documented; lossless persistent logging was not requested.
- Responsive UI while joining/connecting, decoding/loading media and playing audio. Visible missing-device/resource/network failures.

CURRENT PIN ASSIGNMENT (verify, do not assume):
Radio on J7: SCK=GPIO2 (J7.9), MOSI=3 (J7.11), MISO=4 (J7.13), NSS=5 (J7.15), RESET=25 (J7.17), BUSY=49 (J7.19), DIO1=50 (J7.20), RXEN=51 (J7.18), TXEN=52 (J7.16), 3V3=J7.1, GND=J7.7/all radio grounds. DIO2 unconnected; distinct RXEN/TXEN controlled by RadioLib. ANT to proper868MHz RF feed/antenna.
GPS TXD -> UART1 J2.1/GPIO48; J2.4 ground. Conditional J7.2/5V -> GPS VCC ONLY after exact breakout rating verified. GPS RXD and PPS unused. UART3-IN is5V level shifted and not used for GPS.
INA226 J13.1 SCL46, J13.2 SDA45, J13.3 3V3, J13.4 ground; default0x40 with verified straps; VBS tied to load-side IN-. Exact shunt unverified, Config defaults to0 and disables calibrated I/P until supplied.
C6 WiFi SDIO CLK18/CMD19/D0=17/D1=16/D2=15/D3=14/reset32. SD CLK43/CMD44/D0=39. Audio I2S BCLK22/LRCLK21/DOUT23/MCLK24/amp30. Touch I2C45/46/reset40/INT42; display reset41/backlight31/power29 active low.

KNOWN CORRECTION FROM FOLLOW-UP (IMPORTANT):
The implementation author incorrectly claimed that the upper-right expansion socket must be kept occupied for WiFi. On V1.2 there is a separate onboard ESP32-C6 (IC1) providing WiFi. That extension socket CAN be used for a compatible radio, but the Waveshare footprint needs adaptation and RXEN/TXEN handling. The user asked about this, accepted the correction, and explicitly retained J7. Some shipped diagram/document labels still wrongly say 'keep C6 WiFi module installed' at the extension socket. Report this as a known documentation/annotation error separately, not a reason to redesign the J7 wiring or UI. Do not repeat the false socket/WiFi dependency.

PRIMARY BOARD SOURCES:
Repository revision cfcbfdbb1fd05997bc5e24dfeaece7fc44b6f61e:
https://github.com/Elecrow-RD/CrowPanel-Advanced-10.1inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen
Eagle_SCH&PCB/1.2/ESP32-P4 Display 10.1 inch V1.2.sch
Eagle_SCH&PCB/1.2/ESP32-P4 Display 10.1 inch V1.2.brd
Eagle_SCH&PCB/1.2/ESP32-P4 Display 10.1 inch V1.2.pdf
Raw example:
https://raw.githubusercontent.com/Elecrow-RD/CrowPanel-Advanced-10.1inch-ESP32-P4-HMI-AI-Display-1024x600-IPS-Touch-Screen/cfcbfdbb1fd05997bc5e24dfeaece7fc44b6f61e/Eagle_SCH%26PCB/1.2/ESP32-P4%20Display%2010.1%20inch%20V1.2.sch
Waveshare: https://www.waveshare.com/wiki/Core1262-868M and https://files.waveshare.com/upload/c/c1/CoreSX1262_Sch.pdf
TI: https://www.ti.com/lit/ds/symlink/ina226.pdf
Other primary links are in docs/sources.md. The exact GT-U7 breakout supplier/schematic has not been provided; do not turn generic reseller claims into verified facts.

VERSIONS AND PRIOR VERIFICATION (REASSESS):
Arduino ESP32 core3.3.12 (IDF5.5.5), Arduino_GFX1.6.9, RadioLib7.8.1, INA2260.6.6, TinyGPSPlus tagv1.0.3a, JPEGDEC1.8.4, audio-driver0.3.1; bundled Wire/SPI/WiFi/Preferences/SD_MMC/ESP_I2S.
Three final-source builds passed: P4 beforev3 blank credentials, beforev3 with dummy enabled keys/shunt, and postv3 with enabled dummy keys/shunt. Largest binary1,231,166 bytes of3,145,728, static RAM41,252 bytes; runtime dynamic allocations are additional. Host C++/JS packet tests, airtime checks, formatting and Eagle net/footprint audit passed. No physical test or flash was performed. Temporary research/dependency/build directory was removed; logs are preserved. SDK sources are available under /home/voc/.arduino15/packages/esp32 (allowed additional directory).

ARCHITECTURAL REVIEW FOCUS:
Trace each behavior from authoritative state through transformations, ownership/lifetime, queues, interfaces, presentation and persistence. Find actual failures and faulty responsibility boundaries; avoid adding flags/caches/workarounds as the default recommendation. Source GPIO capability/voltage/strapping/peripheral claims against V1.2, not older examples or generic classic ESP32 rules.
Inspect particularly:
1. Electrical correctness, physical connector orientation, power/logic levels, shared I2C pulls/VO4 LDO, SDMMC/C6 coexistence, radio RF switch/TCXO/antenna assumptions, power-budget limits and header reservations.
2. Actual library APIs AND semantics for the pinned versions: DSI initialization and10.1-inch timing evidence, GT911 reset/address/ready handling, ES8311/I2S clock/start/stop/amplifier path, PCM WAV/header assumptions, JPG decode bounds/pixel stride, memory ownership and queue behavior.
3. LoRaWAN1.1 negotiation, EU868 PHY/regional profile, key/EUI byte order, persistent nonce reservation BEFORE RF, JoinNonce/DevNonce handling across failure/reset/exhaustion, frame counter/session behavior, ADR/rekey/rejoin, sendReceive errors/retries, ACK/downlink handling, MAC-command payload reductions and real airtime accounting.
4. Public TTN pacing arithmetic, persistent delay debt including crashes during RF, millis wrap, scheduling/retry/backlog/demo fairness, stale state and all failure paths. Verify that region/TTN timing claims are supported by the actual RadioLib source.
5. Sensor calibration and conversions, individual read errors/overflow/saturation, reconnect behavior, never publishing failed measurements as valid zeros, minute scheduling, batch timestamps, encoding bounds/resolution and decoder correspondence.
6. GPS freshness, UART behavior, WiFi provisioning persistence/failure/reconnect/security assumptions, UI loop/task responsiveness, blocking calls, memory/resource bounds and error visibility.

REPORT FORMAT:
- Lead with confirmed actionable findings, ordered critical/high/medium/low. For each provide affected file:line, concrete triggering scenario, consequence, evidence/source, and smallest architectural correction. Label uncertain findings as hypotheses with the exact test/evidence needed.
- Separate already-known documentation errors and unresolved hardware prerequisites from new proven firmware defects.
- State which requested features are complete, conditional, broken or unverified. Do not equate successful compilation with functioning hardware.
- Evaluate existing test coverage; list only meaningful additional regression/hardware checks.
- End with a concise go/no-go recommendation for assembling/flashing/operating this as-is. Do not make UI/UX aesthetic suggestions or recommend a socket migration.
