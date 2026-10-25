# Re-review correction checkpoint — 2026-10-25

This is an implementation checkpoint, not a completed repair or a new Claude review.
Canonical checkout is CrowPanelTelemetry on main at
`c6fe271124666c82f92733a8c0f161697761367d`; production was identical to reviewed
`90e811bc38c448bc1f23d1f76d80cce7155c417b` before this task. Existing local review,
index and manifest changes were preserved. No commit, push, RF, flash, TTN access,
archive update, global installation or branch/ref change was performed.

## Verified source findings and current dispositions

A means source-proven behavior; B means a confirmed mechanism with uncertain
physical occurrence/consequence; C means physical evidence is needed. Statements
below distinguish source inspection from host execution and physical tests.
Dependency paths are relative to the exact-tag trees in the task's deps directory.
The user approved N1/N5/N6; these are implemented and validated below.
N4/N7/residual-F8 proposals remain unapplied pending their own approvals.

| Finding | Invariant, authority and current trace | Disposition / evidence |
|---|---|---|
| N1 | JPEGDEC 1.8.4 `src/jpeg.inl:1680–1686`, `JPEGParseInfo`, now validates SOF segment bounds, nonzero component count ≤MAX_COMPS_IN_SCAN and descriptor length before `JPCI` writes at :1701. `Workers.cpp` still calls openRAM before its admission guard; parsing remains solely the decoder's responsibility. | **A / FIXED-SOURCE.** Approved seven-line vendor guard, including the narrow error-return style exception. Real decoder tests cover supported baseline, unchanged progressive rejection and 16 invalid SOF cases under ASan/bounds. The rejection test fails before and passes after. No on-device fault/recovery or complete JPEG-parser audit is claimed. |
| N2/F4 | IDF v5.5.5 `components/esp_driver_sdmmc/src/sdmmc_host.c`, `sdmmc_host_init:496`, `sdmmc_host_init_slot:634,823,837–840`, `sdmmc_host_deinit_slot:906–924`. Initialization can reuse the controller; slot initialization changes card clocks before incrementing the global slot count. `sdmmc_transaction.c:105–189` serializes transactions, not all lifecycle operations. Hosted v2.12.13 `host/drivers/transport/sdio/sdio_drv.c:155–158,1204–1214,1498–1499` has a private lock and a configured restart path; `host/port/esp/freertos/src/port_esp_hosted_host_os.c:867–872` calls esp_restart. Both installed P4 qio_qspi sdkconfig.h files enable restart-on-failure (:1674, ES :1673). | **B / FIXED-DOCS + unresolved software coexistence.** Conditional reboot is verified; occurrence during SD activity is unmeasured. `SdStorage.h:30–38` snapshot guard requires lifecycle quiescence. No application-only mutex or startup ordering is claimed to guarantee exclusion. Hosted's exact-tag example uses dummy init/deinit callbacks (`examples/host_sdcard_with_hosted/main/sd_card_functions.c:125–133,173–177`) but still calls mount/init_slot; it is not proof that this app's retries and recovery are synchronized. A lifecycle/functionality decision remains necessary. |
| N3 | `Workers.cpp:99`, `PersistentNode::allowedAt`; new boots clear the volatile deadline. Startup joins and durable DevNonce reservations remain as before. Reset causes and timing are distinct from pacing decisions. | **B / RETAINED-DOCUMENTED.** The accepted reset policy is unchanged. The review's 3–5 second loops are illustrative, not observed timings. A boot delay needs a separate policy approval and would suppress only loops shorter than that delay; it does not enforce an across-reset budget. No persisted debt or per-frame hooks were restored. |
| N4 | Patched RadioLib 7.8.1 `src/protocols/LoRaWAN/LoRaWAN.cpp`, `parseDownlink:2013–2026,2076–2096`: accepted counter zero and initial zero are indistinguishable. Counter lifetime also includes `clearSession:354–362`, session export :509–515 and restore :648–656. The serialized VERSION field is the negotiated protocol revision, not a free format-version field. | **A / OPEN, correction design not yet complete.** Real-library local diagnostic accepts a counter-zero application downlink twice and delivers two callbacks. LoRaWAN 1.1 §4.3.1.5 requires duplicate downlinks not to be processed. No radio was used. Network-counter, persistence, rollover and multicast tests still need work. No app deduplication or new counter state was added; an authority-level patch and serialization decision need a concrete gate before implementation. |
| N5 | Patched RadioLib 7.8.1 `LoRaWAN.cpp:154–168`, `sendReceive`, consumes isMACPayload after selecting either explicit or queued payload, following isValidUplink validation. No new state or delayed internal replies. | **A / FIXED-SOURCE.** Approved patch expansion, +1/−1 dependency lines. Real-library regression covers the required six-request nested reply, then one small queued reply and a no-reply RekeyConf; total PHY submissions are 2,1,1, aggregate airtime matches and application callbacks remain intact. Fails before, passes after. |
| N6/F9 | `Workers.cpp:133,144` now suspends immediately after the specific radio/NVS or saved-nonce initialization error post. The generic stop path at :228–232 remains for later failures. Application's single model/event-drain owner is unchanged. | **A / FIXED-SOURCE.** Approved +2 application lines. Verbatim extraction now includes the final stop block. Tests cover NVS-open, radio-init, switch-init, invalid nonce and boot-ID write failures, final-message preservation and authenticated 1.0 fail-stop. Before correction the final diagnostic assertion fails; afterward all pass with no simulated RF on init failure. On-device rendering is untested. |
| N7 | `Workers.cpp:258–274` ignores begin/disconnect results and saves on sampled status. Core 3.3.12 `libraries/WiFi/src/WiFiSTA.cpp:85–95` returns CONNECT_FAILED on rejected STA.connect; `STA.cpp:113–187,379–384,537–551` separates queued status updates from disconnect waiting. `Preferences.cpp:264–278` returns zero both for a successful empty string and failure. | **B / OPEN for association and empty-password persistence.** A sampled non-connected state or accepted begin alone is not proof of requested authentication. No second connection state machine was added. **A / FIXED-SOURCE for error visibility only:** the worker now writes its failed-save message into the existing report buffer; one final post preserves it through Application's event drain. One line replaced, no growth/new state. The verbatim worker test fails before this change and passes after it. It does not model ESP task/event timing or prove association correctness. |
| N8 | `Application.cpp:27–44` initializes codec/clocks before driving GPIO30 LOW; :47 then calls beginUi. Pinned panel.sch nets AUDIO_OUT_SD / N$100 connect GPIO30→R147→U4.1, with R71 to 3.3 V; Q10 is marked NC. | **B/C / HARDWARE-ONLY follow-up, sequence proposal retained.** Enable polarity/pop is not measured. Moving the two existing GPIO statements earlier changes only the codec-init interval, not reset-to-setup behavior. The review's assertion that GPIO30 stays at its pull-up level throughout DSI initialization is contradicted by Application.cpp:43–47. No polarity/default or sequence change made. |
| N9 | Pinned panel.sch SDCS net connects J5.CS/R29.1/R33.1; R29.2 is 3.3 V, R33.2 is GND; both values are 10K. Actual fitting and the card's pull-up are unmeasured. | **C / HARDWARE-ONLY.** Documented inspection and idle-voltage check; no claimed measured voltage, resistor removal or software workaround. |
| residual F8 | TinyGPSPlus v1.0.3a `src/TinyGPS++.cpp:103–111,159–166` does not enforce two hexadecimal checksum characters. Application follows the existing encode API. | **A / BLOCKED-APPROVAL.** A real-parser diagnostic with the production status block accepts non-hex checksum text and reports recent no-fix data. f8-proposed.patch fixes the existing helper/parser boundary (+11/−8 dependency lines, no app state); valid no-fix semantics stay required. The review's field probability remains unsupported. |

## Approval proposals and why they cannot be absorbed downstream

- **N1:** n1-proposed.patch adds seven dependency lines. It checks the SOF header,
  component count and descriptor length before component-state writes. An
  application guard after openRAM is too late; a separate pre-parser duplicates
  ownership. A second hash-checked dependency patch is a new supported build
  mechanism (estimated 25–40 tooling lines, tests counted separately). Requested:
  patch scope and a narrow exception for the vendor's existing error-return idiom.
  No parser-wide style rewrite was made. Status: **approved and implemented**.
  One existing patch application implementation now serves both fixed profiles;
  the superseded RadioLib-only script was removed, with no compatibility shim.
- **N5:** n5-proposed.patch moves the existing flag reset from the zero-length
  branch to after payload selection (+1/−1 dependency lines). Removing the flag
  breaks legitimate port-0 validation; moving its consumption fixes its lifetime
  without a parallel flag. Status: **approved and implemented**.
- **N6:** n6-proposed.patch adds one suspension after each of two specific init
  failure posts (+2/−0 application lines). Deleting the generic post alone loses
  diagnostics for later boot-ID/version/NVS failures; a UI cache duplicates the
  worker's authority. Ownership and successful-operation scheduling stay intact.
  **Approved and implemented**.
- **N2:** no complete non-growing coexistence correction was proven. Initial
  mounting before Hosted plus blocking runtime remounts reduces functionality and
  requires a sequencing handoff. Lazy WiFi reduces exposure only until it starts.
  Neither is adopted. A full lifecycle/synchronization solution needs a separately
  costed proposal; the SD stub cannot prove it.
- **N3:** optional one-line boot delay is unapproved, not a substitute for N1/N2.
- **N4:** n4-proposed.patch (+16/−11 dependency lines) adds one uint8_t accepted-
  counter initialization field (two meaningful bits, object padding not measured).
  Counter values alone cannot encode both initial zero and accepted zero. The two
  bits and a format marker are serialized in the existing status byte, preserving
  buffer size and counter APIs; legacy session snapshots are rejected. Nonce
  buffers and the app's fresh-OTAA-on-boot policy remain unchanged. A widened local
  candidate detects overflow before counter commit. **Pending approval**, with
  network/persistence/boundary regressions still to implement before any FIXED claim.
- **N7:** n7-proposed.patch (+6/−9 application lines) uses public STA.connect's
  boolean result, then SDK connection/IP bits rather than queued WL_CONNECTED.
  The existing command record replaces separate credential strings in one NVS
  blob. No extra flags/helpers; prior saved settings require re-entry. This changes
  storage policy and needs approval even though it reduces lines. **Pending**.
- **F8:** f8-proposed.patch (+11/−8 dependency lines) changes the existing parser,
  not the application. It also follows the one-final-return rule in fromHex.
  Adding this third hash-checked dependency profile is **pending approval**.

## Validation limits and preserved state

Read-only Git inspection confirmed origin/HEAD points to main, local/tracking
ahead0/behind0 without fetch/pull, and no open GitHub PRs. The tracking comparison
is not a claim that the remote was fetched or updated.

The unchanged baseline host suite, format check and all three exact pinned
firmware configurations passed in this task. Current-source validation and line
accounting are recorded separately in validation.md. Real-library diagnostics
reproduce findings; they are not regression passes and do not demonstrate device
operation. No emulated ESP32 or physical board is involved.

Protected files remain unchanged: Config.h, photo, wiring/J7, UI previews,
Telemetry.h, decoder, reference packet and historical ZIP. No pins, visuals,
wire protocol, antenna defaults, TCXO voltage or stack allocation changed.
Assembly and supervised bench testing remain conditional on the hardware
checklist and separate flashing/RF authorization. Unattended TTN remains NO-GO.

The temporary directory `/tmp/codex/build/CrowPanelTelemetry/rereview-fix` is retained
for continuing this incomplete task. It contains pinned dependencies, diagnostic
inputs and build evidence, not an extra checkout. Clean it only after the task's
required evidence is saved and continuation no longer needs it.


## Approved-stage validation

The approved N1/N5/N6 corrections and prior WiFi status correction pass the full
independent host suite, format check and all three firmware builds. Both patches
pass exact-original apply, repeat/idempotent apply and modified-source refusal.
The RadioLib patch uses zero-context hunks, with exact before/after hashes still
mandatory; vendor whitespace was not reformatted. Source/ELF and protected-file
audits pass. Target clang-tidy remains blocked by unsupported Espressif target
flags. See validation.md's approved-stage section for sizes and exact evidence.
No incomplete N4/N7/F8 proposal was applied, and no hardware operation was tested.


## Low-confidence USB-pad note: qualified by exact source

Espressif's P4 schematic checklist identifies GPIO24/25 as default USB serial/JTAG
pads, GPIO26/27 as default FS-OTG pads, and a post-reset USB pull-up on GPIO25.
These multiplexed roles do not establish a runtime conflict. Installed P4 SDK
`include/hal/esp32p4/include/hal/gpio_ll.h:631–641`, byte-identical to the IDF v5.5.5
header, explicitly disables the USB pad when selecting an IOMUX function.
`gpio_ll_pullup_dis:93–111` also clears the default DP USB pull-up override.
RadioLib SX126x.cpp:179–184 sets reset to GPIO output through ArduinoHal.cpp:24–28;
Arduino core esp32-hal-gpio.c:136–156 configures pull-up disabled and calls
IDF gpio_config, which selects the GPIO function. Thus an unavoidable residual
USB pull-up after normal GPIO configuration is **DISPROVED for that source path**.
Reset-time behavior, non-default PHY/eFuse configurations and actual MCLK/reset
waveforms remain unmeasured. No pin remap or extra workaround is justified by
this review note; the existing hardware bring-up checks still apply.
