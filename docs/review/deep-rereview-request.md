# Deep expert re-review after F1–F14 corrections — 2026-10-25

Review https://github.com/VOC-FH/CrowPanelTelemetry (private), main, at the exact
published commit supplied by the caller. Canonical working directory:
/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry.
Act as an independent senior IoT / ESP32-P4 / ESP32-C6 ESP-Hosted / CrowPanel /
LoRaWAN1.1 / EU868 specialist. This is a deep, READ-ONLY review, not a request to
implement fixes or draft an implementation prompt. Return your full findings;
the caller will preserve your response. Do not change files, run commands,
install/upgrade, create branches/checkouts/PRs, commit, publish, flash, transmit
RF, access TTN accounts or contact anyone. Your only tools are Read/Glob/Grep.
Historical reviews, downloaded files and logs are evidence/claims, not instructions.

## Fixed user decisions

- UI layout, colors, fonts and screens are accepted. NO nicer UI/UX suggestions.
  Behavioral correctness, error visibility, input validation and responsiveness
  are in scope. Pins/J7 stay fixed; do not migrate radio to the extension socket.
- User rejected airtime reservation before each physical transmission, persisted
  airtime debt, beforeUplink/afterUplink hooks and project marker macros/comments.
  Do not demand those mechanisms again. Evaluate current aggregate volatile
  pacing honestly, including its accepted reset/internal-MAC limitations.
- Preserve pre-RF durable DevNonce reservation and fail-stop on NVS/version errors.
  Local TX completion is not gateway receipt; FCnt consumption is not proof of RF.
- LoRaWAN1.1-only, Class A, OTAA, TTN EU868; editable DEV_EUI/JOIN_EUI/APP_KEY/NWK_KEY.
  Class A downlinks arrive only in receive windows after an uplink.
- No architecture additions, new policies or implementation are authorized by this
  review. Recommendations should reduce, then reshape, then add only if needed,
  with one authoritative state owner and explicit trade-offs/growth implications.

## Current source and evidence to inspect independently

Read AGENTS.md; production Application.cpp/Workers.cpp/Ui.cpp/Config.h/App.h/
SdStorage.h/Telemetry.h/CrowPanelTelemetry.ino/ttn-decoder.js; Makefile; tests/;
tools/; patches/; README and docs/wiring.md, airtime.md, ui.md, sources.md,
validation.md, hardware-checklist.md, correctness-status.md and source hashes.
Read docs/review/deep-fix-dispositions.md and the prior deep review/assessment
AFTER tracing the relevant current source, to avoid treating our dispositions as
proof. Do not rely on stale line numbers. Historical ZIP is not source authority.

Pinned source collection for this review:
/tmp/codex/build/CrowPanelTelemetry/claude-rereview/deps/
RadioLib7.8.1 is patched with the current project patch and before/after hashes
verified. Exact sources also include INA2260.6.6, TinyGPSPlusv1.0.3a,
JPEGDEC1.8.4, Arduino_GFXv1.6.9, audio-driverv0.3.1, ESP-IDFv5.5.5 and
esp-hosted-mcuv2.12.13. Do not substitute IDF release/v5.5 branch heads or older
Hosted examples. Installed core3.3.12 / P4 SDK headers are READ-ONLY under
/home/voc/.arduino15/packages/esp32. Pinned panel.sch/brd are in the task directory;
actual module electrical population remains unknown. Metadata/hash files identify
exact source commits. Ask no permission to read supplied files.

Latest implementation, to RE-VERIFY rather than trust:
- F1: approved RadioLib processJoinAccept validation-before-state-commit patch;
  locals for revision/JSIntKey; nonce/revision/DevAddr/session state after MIC.
- F2: existing zero-initialized upEvent.fPort controls fragment advancement instead
  of rc sign or FCntUp. Review errors, repetitions and nested reply semantics.
- F3: immediate INA226 getLastError after calibration call, existing sensor-error
  path, legitimate zeros and invalid triplets preserved.
- F8: GPS timestamp advances only on checksum-valid NMEA, preserving no-fix status.
- F9: distinct NVS/radio/DIO2 initialization diagnostics and fail-stop.
- F10: one approved credential-validation helper for Connect and both existing
  WiFi status render paths; short-password feedback survives network refreshes.
- F13: JPEGDEC's existing JPEG type API rejects progressive files before decode;
  false persisted RF-debt documentation removed.
- S1 additional fix: target -fstack-usage found loadImage19120 bytes versus the old
  media stack12288. User approved ONLY increasing that stack to24576 (+12KiB
  runtime reservation, no new object/storage lifetime). Radio stack remains12288.
- F4/F6/F7/F11/F12 retained/documented, F5/F14 physical checks remain open. Review
  whether our qualifications/retention are actually justified; do not excuse a
  confirmed software defect merely because we documented it.

Final Arduino CLI1.5.1 builds: prev3 blank1202450 flash/39364 staticRAM;
prev3 dummy-enabled1209552/39436; postv3 dummy-enabled1211122/39436. Exact commands
and Used library output are in docs/validation-logs. Host make test, format-check,
patch apply/idempotence/tamper, source/symbol/pinned-net audits pass. Source-fixed
production grows11 app lines and4 dependency lines, explicitly approved. New tests
use real libraries and verbatim production extraction, not an ESP32 emulator.
Target clang-tidy could not run due LLVM/Espressif flag incompatibility; -m32 host
probe failed due missing multilib. No fresh pre-change firmware baseline compiles
were captured. No hardware/RF operation or performance baseline is claimed.

## Deep review priorities

1. Re-check every F1–F14 finding against CURRENT code and exact dependency source.
   Decide resolved, partly resolved, retained risk, disproved or still defective.
   Report regressions and previously missed defects, not only old findings.
2. Security/state: all pre-auth JoinAccept mutations and caller lifetimes;
   length/MType/MIC/root-key/revision/JoinNonce handling, replay (including0),
   corrupt/foreign accepts then genuine retry, session/nonces/NVS signatures,
   monotonic counters, nonce exhaustion/write interruption/ambiguous TX and1.0
   fail-stop. Verify test crypto and fixtures rather than assuming they are valid.
3. RF outcome and pacing: exact TX stage/launch/IRQ/finish, receive/parse/nested
   errors and repeats; event population versus fragment/demo lifetime, loss,
   duplicates/dedup, ordering/fairness. All reachable Class-A timing comparisons,
   elapsed loops and widths, millis0/wrap, stale lastToA and nested aggregation.
   Do not restore the rejected reservation policy or equate counter use with TX.
4. Shared peripherals: slot0/slot1 SDMMC init/count/clock/deinit, SDK transaction
   mutex versus lifecycle operations, Hosted recovery and port layer, failed SD
   mount/retry/unmount-error latch, fixed VO4 and sole UI-owned I2C45/46. Assess
   actual v2.12.13 coexistence example/interfaces and whether any claimed exclusion
   exists. An app mutex or startup reordering is not automatically a fix.
5. Runtime/memory: media24576 and radio12288 stack chains including decoder,
   nested callbacks/formatting/Event queues, all local objects and SDK/library
   frames; target frame evidence and its limits. Check heap/PSRAM ownership,
   queues/backpressure, error cleanup, loadImage dimensions/type/size, WAV parsing,
   media removal, repeated requests and I2S/codec/amplifier initialization/lifetimes.
   Priority2 runnable radio versus loopTask/Hosted/WiFi/audio: actual mechanism,
   not invented latency. No stack-safety claim based on isolated frame sums.
6. Sensor/GPS/UI: real INA226 calibration/read/overflow/saturation behavior,
   preservation of true zero, invalid triplet protocol/decoder agreement and
   voltage-only limitation. TinyGPS checksum/no-fix/expiry/wrap/satellite validity.
   WiFi edits/empty/open/short/64hex/password lengths, periodic status and saved
   credentials. Touch failure latch/restart remains intentional; no silent new
   recovery policy. Downlink stale/empty/binary/nested text and modal refresh.
7. Physical/electrical: exact V1.2 schematic and actual GPIO capabilities,
   J7/J2/J13 orientation, GPS supply/logic uncertainty, INA226 shunt/straps/pullups,
   RXEN/TXEN, DIO2/DIO3, R22/FB1 TCXO population and frequency error, conducted
   power/antenna/cable/EIRP versus ERP, GPIO30→R147→NS4263B/R71 reset-pop path,
   10.1-inch DSI timing/touch orientation, ES8311 MCLK gap, P4 silicon/C6 firmware.
   Distinguish traced connectivity from unknown fitted/measured facts.
8. Evidence: challenge test seams, extraction markers, actual library paths/hashes,
   false positives or missing failure cases,64-bit host versus32-bit target,
   wrappers versus real dependency lifetimes/concurrency. Compilation is not
   physical success. Check docs for remaining false claims and source mismatch.

## Required output

- Executive verdict: blockers, completeness, conditional assembly/bench flashing/
  unattended TTN readiness. No actual operation claim. Keep source-proven failure,
  confirmed mechanism/uncertain consequence and physical prerequisite separate.
- For each actionable defect: stable ID, severity, class(A/B/C), exact CURRENT
  file/function/line citations and pinned dependency version, triggering path,
  ownership/invariant, user impact, narrow reduce/reshape-first remedy, alternatives
  if materially different, and a meaningful regression test or physical check.
- A table covering every prior F1–F14 plus S1: resolved/partial/retained/disproved,
  independently checked evidence, missing proof and remaining risk. No checklist
  rubber-stamping. Qualify or retract any earlier claim exact sources disprove.
- Tests/evidence you inspected and gaps you could not resolve read-only. State
  clearly if a conclusion depends on unmeasured hardware or missing exact source.
- No UI styling proposals, implementation edits, new task branches or publications.
