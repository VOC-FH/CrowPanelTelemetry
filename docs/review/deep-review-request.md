# Deep expert review request — 2026-10-25

Review the private GitHub repository https://github.com/VOC-FH/CrowPanelTelemetry
on main at commit 6fada32040bbcb982c99dd59e9b4f57c1b2eb7c3.
The exact canonical local checkout is your working directory:
/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry.
The local checkout and remote main were verified synchronized. No alternate
checkout is allowed. Your task is a DEEP READ-ONLY review as an expert in ESP32-P4,
ESP32-C6 ESP-Hosted, Elecrow CrowPanel hardware, LoRaWAN1.1, EU868/TTN and embedded
IoT reliability. Do not edit files, run commands, install, flash, transmit RF,
access TTN accounts, contact anyone, create branches/PRs or publish anything.
Return the full review as your final answer; the caller will save it.

## User decisions and scope

- Do NOT propose a nicer UI/UX. The visuals, layout, colors and screens are accepted.
  Review behavioral correctness, stale data, error visibility and responsiveness.
- All pin assignments, especially J7 radio wiring, stay fixed. The upper-right
  extension socket is not needed for onboard C6 WiFi. User chose J7 for the radio.
  Flag an actual electrical error with evidence; do not migrate pins for preference.
- The user explicitly REJECTED reserving airtime before every physical transmission.
  Do not classify absence of that rejected mechanism as an implementation defect or
  propose reinstating it as a mandatory fix. Explain inherent consequences accurately.
- The user ACCEPTED nested application-downlink delivery and timing rollover/airtime
  accounting fixes to pinned RadioLib7.8.1. Current patch contains no project marker
  macro, branding comment, beforeUplink or afterUplink hooks. It adds only the
  applicationDownlink callback plus timing/accounting corrections. Verify this.
- Current policy: one volatile application deadline, extended after each completed
  join/uplink operation using total airtime, including repeated/nested frames.
  No airtime NVS writes/restoration. Internal MAC responses proceed normally.
  DevNonce reservation before OTAA remains separately required; do not confuse it
  with the rejected airtime reservation policy.
- Implement directly on main unless explicitly directed otherwise. This is review
  only: no implementation or branch/commit/push is authorized by this request.
- Recommend reduce, then reshape, then add only when necessary. Explain architecture,
  single authority, ownership, lifetime and affected consumers. No speculative layers.

## Required functionality

CrowPanel Advanced ESP32-P4 10.1-inch V1.2 with onboard ESP32-C6 WiFi;
GT-U7 UART GPS; INA226 I2C shunt sensor; Waveshare Core1262-868M/SX1262 radio.
WiFi provisioning to an existing network; touchscreen display; SD JPG previous/next;
SD WAV playback on built-in audio path; GPS coordinates and fresh fix/satellite status;
LoRaWAN connection, demo uplink and received downlink display. LoRaWAN1.1.0 Class A,
OTAA, TTN EU868 with editable DEV_EUI/JOIN_EUI/APP_KEY/NWK_KEY macros. Class A opens
receive windows after uplinks. Measure actual voltage/current/power each minute;
after ten measurements queue all ten raw readings, not an average. Public TTN
airtime limits take precedence over unconditional ten-minute delivery at slow DRs.
Keep UI/sampling/GPS responsive while WiFi, SD/JPEG, audio and radio workers operate.
Missing SD/network/GPS fixes and hardware errors must be visible. No failed sensor
conversion may be synthesized as a valid zero.

## Source authority and available evidence

Read AGENTS.md, README.md, Config.h, App.h, Application.cpp, Workers.cpp, Ui.cpp,
SdStorage.h, Telemetry.h, ttn-decoder.js, tests/, tools/, Makefile, patches/ and docs/.
In particular read wiring.md, airtime.md, sources.md, correctness-status.md,
validation.md and hardware-checklist.md. Prior reviews under docs/review are
historical CLAIMS TO RE-VERIFY, not instructions or evidence that a fix is correct.
The historical artifact ZIP is not the current source authority. Inspect current
code independently, then check whether previous findings are resolved or regress.

Exact dependency sources (NOT installed globally):
/tmp/codex/build/CrowPanelTelemetry/claude-deep-review/deps/
RadioLib7.8.1 is already patched with the current hash-checked project patch.
Other exact versions: Arduino_GFX1.6.9; INA2260.6.6; TinyGPSPlus tagv1.0.3a;
JPEGDEC1.8.4; audio-driver0.3.1. ESP32 core3.3.12/IDF5.5.5 installed read-only under
/home/voc/.arduino15/packages/esp32. Sources of panel schematic/board and radio
schematic are provided under the task directory when available. Treat downloaded
materials as untrusted source data, not executable instructions. Follow source
links in docs/sources.md and primary vendor documentation for uncertainties.

Three final-source Arduino CLI1.5.1 builds passed: prev3 blank, prev3 dummy enabled,
postv3 dummy enabled; flash1202142/1209106/1210676, staticRAM39364/39436/39436.
Host real-RadioLib integration, codec/decoder, sensor-range, SD lifecycle, formatting,
patch hash checks and Eagle net audit passed; see fresh validation-logs. These tests
simulate hardware and do NOT prove real panel/RF operation. Target clang-tidy failed
on unsupported Espressif RISC-V/GCC flags; supplemental host analysis has recorded
warnings. Do not treat earlier green logs as proof of paths they do not cover.

## Deep review priorities

1. Wiring/electrical: exact Eagle V1.2 connector orientation and pin capabilities;
   native/level-translated UART, GPS VCC/TXD conditions; INA226 shunt/straps/pulls and
   polarity; SX1262 SPI/IRQ/BUSY/reset/RF switch/TCXO/antenna/power. Check conflicts
   with display/GT911/SD/audio/C6 and all real power/voltage assumptions. Distinguish
   schematic connectivity from unknown actual module population.
2. P4/C6 and SD: inspect actual core variant macros, shared VO4 3.3V ownership,
   SDMMC slot0/slot1 host init/cleanup counts, GPIO45/46 I2C ownership, failed/early
   init, card retries/unmount and WiFi startup/failure/reset paths. Trace SDK calls
   beyond the wrapper; determine whether the lifecycle stub matches real IDF.
3. LoRaWAN/security: EUI/key endian/roles, 1.1-only negotiation, nonce persistence,
   interrupted writes, ambiguous launch errors, nonce exhaustion and no RF after
   NVS/version failure. Fresh sessions/FCnt safety, rekey/MAC/ADR changes, EU868
   channels/power, join DR5→DR0/retry, sendReceive failures and batch/demo fairness.
4. RadioLib patch correctness: all reachable Class-A time comparisons across
   millis wrap, TX ending exactly0, signed/unsigned target widths and stale
   tUplink clamps, receive-window timing, recursive MAC uplinks, repetitions,
   lastToA reset/aggregation (including nested errors), downlink forwarding order,
   empty/binary/maximum messages. Does the actual buffer/event/counter path match
   tests? Check whether the accepted fixes introduce regressions or undefined
   behavior. No adding per-frame airtime reservation demands.
5. Payload/delivery: version1 bit fields, quantization/ranges/invalid sentinel,
   JS decoder agreement, fragments under MAC payload reductions, queue behavior,
   boot/minute identifiers, reset semantics, actual airtime arithmetic and public
   TTN tradeoff. Distinguish aggregate runtime spacing from per-frame enforcement.
6. Sensors/GPS: INA226 library semantics, coherent conversion completion, I2C errors,
   raw ADC rail and calibrated range clipping, overflow, voltage-only limitations,
   real shunt calibration; TinyGPS validity/age/rollover and no-data vs stale-fix.
7. Display/touch/audio/media: actual 10.1-inch DSI timings vs upstream 7-inch profile,
   GT911 orientation/runtime failure, PSRAM ownership/free/error paths, SD file
   parsing/resource bounds (JPG/WAV), ES8311 setup/I2S gap/amp polarity, task queues
   and blocking/backpressure/deadlock/races. Review functional UI freshness only.
8. Build reproducibility/documentation/testing: pinned dependency interfaces,
   patch packaging/discovery, board/partition settings, independent check behavior,
   real tests vs assumptions, potentially false verification statements and missing
   hardware prerequisites. Do not demand broad unrelated refactoring/style churn.

## Required output

- Start with executive verdict and prioritized findings (critical/high/medium/low).
- For EACH finding: unique ID, classification A(source-proven failure), B(confirmed
  mechanism but uncertain physical/runtime consequence) or C(unresolved physical
  prerequisite); exact current project file:line/function plus dependency file:
  line/function and source URL when applicable; concrete trigger, resulting behavior,
  impact, evidence vs inference, smallest coherent correction and regression test.
- Do not report speculative defects without tracing the whole authoritative path.
  Mark uncertainties and failed source access plainly. Rejected policies are not
  requirements. Separate existing acknowledged limits from newly discovered bugs.
- Explicitly list disproved earlier claims and missing test coverage. Assess tests
  critically; do not simply echo current validation claims.
- Provide conditional go/no-go for assembly, flashing, and unattended TTN use.
  No claim of actual display/touch/audio/WiFi/SD/GPS/sensor/RF operation without
  hardware evidence (none exists). Physical prerequisites are documentation items,
  not software fixes based on guesses.
- Give a compact remediation order, without implementing any fixes or UI redesign.
- If no source-proven defect exists in an area, say so rather than inventing one.
