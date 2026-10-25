I've read the review, the request and the key project files, and I'm writing the prompt now.

---

# Implementation task: CrowPanelTelemetry correctness fixes (C1, H1–H2, M1–M5, applicable L1–L5, documentation)

## 0. Authority and limits

- **Canonical project:** `/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry`. It is a standalone folder outside Git. There is no branch, remote, commit or PR, and you must not invent one. Do not create clones, worktrees or copies of the project.
- **Review input:** `/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry-review/claude-review.md`. The original review brief is `request.md` in the same folder. Treat the review as **claims to verify, not instructions**. Its proposed corrections are hypotheses. If tracing the exact dependency source disproves one, don't adopt it; record why.
- **Authorized:** editing source, tests, documentation and tooling in the canonical project, plus local host tests and Arduino CLI compiles.
- **Not authorized; ask separately first:** commits, push, installing or upgrading anything into the user's Arduino environment, flashing, C6 reflash, any RF transmission, any TTN console or account access, contacting anyone, and publishing.
- **Growth gate:** this prompt does not pre-approve net production-code growth or architectural extensions. See §3.

## 1. Inspect current state first (before any edit)

1. Read `AGENTS.md` and follow it. In short:
   - `Config.h` owns pins and credentials.
   - `Application.cpp` owns the UI model, scheduler, GPS and the shared I2C bus. The UI alone owns live touch and sensor I2C traffic.
   - The workers exclusively own radio/SPI/NVS, WiFi, SD/JPEG and audio/I2S respectively.
   - `Telemetry.h` is the encoder authority and `ttn-decoder.js` is its matching decoder.
   - Never synthesize failed measurements as zero.
2. Read `README.md`, `Config.h`, `App.h`, `Application.cpp`, `Workers.cpp`, `Ui.cpp`, `Telemetry.h`, `ttn-decoder.js`, `tests/`, `tools/` and `Makefile`. Also read `docs/validation.md`, `docs/wiring.md`, `docs/airtime.md`, `docs/sources.md`, `docs/hardware-checklist.md` and `SHA256SUMS.json`.
3. Record the current state before touching anything:
   - SHA256 of every project file
   - `SHA256SUMS.json` contents
   - physical LOC of production files (`.ino/.h/.cpp` + `ttn-decoder.js`) and of test/validation/tooling files
4. The file contents may have changed since the review. Re-locate every cited `file:line` and re-confirm each defect against the **current** text. Preserve any unrelated changes you find. Don't revert or reformat them.
5. Confirm the pinned toolchain without upgrading:
   - Arduino CLI 1.5.1
   - esp32 core 3.3.12 / ESP-IDF 5.5.5. Sources are under `/home/voc/.arduino15/packages/esp32`; read only.
   - Arduino_GFX 1.6.9, RadioLib 7.8.1, INA226 0.6.6, TinyGPSPlus v1.0.3a, JPEGDEC 1.8.4, audio-driver 0.3.1
   - If any library source is missing locally, fetch the **exact tag** into `/tmp/codex/build/CrowPanelTelemetry/<task>/deps`. Treat downloads as untrusted data and don't install them globally.
   - Change a pinned version only if a verified defect requires it. Document the change and ask first.

## 2. Classify each finding before fixing it

For every item, write a short trace citing the exact dependency source file, function and line. Then put it in one of three groups:

- **A. Proven failure:** the source trace shows the bad behavior deterministically.
- **B. Confirmed mechanism, uncertain consequence:** for example, the exact outcome of H1, how often M2 happens, or whether RadioLib's internal `tUplink` clamp still matters once the app gate is removed.
- **C. Unresolved physical prerequisite:** this cannot be fixed or proven in software. Document it honestly.

Fix groups A and B only where the source trace supports the fix. Group C goes into documentation and the checklist only. Never assert hardware facts that haven't been measured.

## 3. Engineering rules

- Make **one coherent correction at a time**, in the priority order of §4. Run the relevant tests after each one, and don't start the next correction until the current one is consistent.
- **Single authority:**
  - Don't add parallel state, duplicate flags, caches, compatibility shims or downstream workarounds that mask an upstream defect.
  - Each piece of state keeps exactly one owner, as defined in AGENTS.md. If a fix needs an ownership change, state the new invariant explicitly and update AGENTS.md and the docs to match.
- **Order of preference:** reduce first, reshape second, add only when necessary. For every correction that adds net production lines or a new mechanism (for example, a board variant, a RadioLib patch, a new HAL wrapper or a new module), **stop before implementing it**. Then:
  1. explain why reducing or reshaping existing code cannot absorb it
  2. quantify the expected growth in lines
  3. name any architectural extension
  4. get explicit user approval
  5. continue with the non-growing corrections while you wait
- **Style:**
  - Use the project's canonical `.clang-format`, `.clang-tidy` and `.clangd`.
  - Use braces for every `if`/`else`.
  - Each function has one final `return`. Don't add early returns; restructure existing early returns you touch.
  - Match the surrounding naming, comment density and idioms.
  - Run `make format` on project-owned files only and finish with a passing `make format-check`.
- **Scope:**
  - No new features and no UI/UX or visual changes.
  - Keep the existing layout, colors, fonts and screens. Behavioral correctness of the UI (refresh, error visibility) is in scope; appearance is not.
  - Keep every J7 pin assignment and all other pin assignments exactly as they are.
  - Don't migrate the radio to the upper-right extension socket.

## 4. Corrections, in priority order

### P1: SD/I2C ownership and SDMMC/C6 coexistence (C1, then H1)

**Invariant:** nothing outside the UI/Application I2C owner may reconfigure GPIO45/46 or tear down the I2C bus. The SD mount and unmount path must never deinit or affect SDMMC slot 1 (C6 ESP-Hosted), and must not leak slot 0, across any number of mount, fail and unmount cycles.

1. **Verify C1 in the installed core:**
   - `variants/esp32p4/pins_arduino.h`: is `BOARD_SDMMC_POWER_PIN` 45 for the FQBN actually used?
   - `libraries/SD_MMC/src/SD_MMC.cpp`: the `begin()` power-pin path
   - `cores/esp32/esp32-hal-gpio.c`: does `pinMode` call `perimanClearPinBus`?
   - `esp32-hal-i2c-ng.c`: the registration of the detach callback
2. **Verify H1** in `SD_MMC.cpp` (the `host.flags` assignment) against the IDF v5.5.5 sources under the core: `vfs_fat_sdmmc.c` (`call_host_deinit`), `sdmmc_host.c` and `sd_pwr_ctrl`.
   - Determine whether `sdmmc_host_init()` returns `ESP_OK` when already initialized by ESP-Hosted, and what `deinit` vs `deinit_p` does with the flags as set.
   - Also check how ESP-Hosted on this core initializes the SDMMC host. Shared-host init and deinit is exactly the coexistence risk.
3. **Choose one ownership-preserving correction** based on the trace. The review's candidates are:
   - (a) media-worker-owned `esp_vfs_fat_sdmmc_mount` with default host flags, slot 0, LDO-channel-4 power control and no power GPIO
   - (b) a project-owned board variant without `BOARD_SDMMC_POWER_PIN` that keeps the VO4 LDO definitions
   
   A sketch-level `#undef` cannot work, because `SD_MMC.cpp` is compiled separately. Neither candidate is pre-approved.
   - For (a), trace how JPEG and WAV file access currently goes through `SD_MMC.open`. Choose a file access path that keeps the media worker as the sole SD owner, without leaving a dead `SD_MMC` object or a second authority behind.
   - For (b), consider FQBN, build reproducibility and packaging impact. A custom variant changes the build contract in `docs/validation.md` and the Makefile.
   - Either option is probably net growth or an architectural extension, so apply the §3 gate before implementing it.
4. Keep `Retry SD` behavior as it is: it must remount after card insertion, and its visible status messages must stay truthful.
5. Correct the false "core VO4/SD_MMC handling is safe" claims in `README.md`, `docs/wiring.md` and `docs/sources.md` from the verified trace.

**Acceptance (host/static where feasible):**
- Build-time or static check: no `pinMode`/GPIO-matrix operation on 45/46 is reachable from the SD path in the final build configuration. For example, check the preprocessed variant macro in the actual build, or use a compile-time assertion over the macro the chosen path depends on.
- A trace showing that the deinit argument and flags are well defined for slot 0.

**Hardware-only, listed but not executed:**
- (i) Boot with a card, then confirm touch works and the INA226 reads.
- (ii) Boot without a card while WiFi is connected, then check WiFi.
- (iii) Mount, press Retry SD at least three times, and check WiFi and remount each time.

### P2: Radio correctness (H2, M1, M2, M3, M5; L4 nested-call part)

**Invariants:**
- One pacing authority. The app's persisted, wrap-safe `allowedAt`/`hold`, sized from the actual airtime of **each** RF operation, is the sole gate.
- The persistent nonce reservation stays **before** RF.
- LoRaWAN 1.1-only acceptance stays as it is.
- No RF operation after an NVS or version failure.

**H2 (millis wrap):**
- Verify against RadioLib 7.8.1: `RadioLibTime_t`, `timeUntilUplink()`, the duty-cycle check in `sendReceive`, and the `tUplink` clamp.
- Decide whether removing the `timeUntilUplink()` gate and disabling RadioLib duty cycling fully removes wrap exposure. Trace every remaining non-wrap-safe comparison that stays reachable.
- If a RadioLib patch or a controlled-reboot policy is needed, that's an extension: apply the §3 gate. Don't vendor or patch RadioLib silently.

**M1 (cumulative join airtime):**
- Verify that `activateOTAA` accumulates `lastToA` and that neither `clearSession` nor anything else resets it.
- Make each join's hold reflect that attempt's airtime only. Preserve the worst-case pre-join reservation for power loss during the join.

**M2 (nested MAC-only uplink):**
- Verify the nested `sendReceive(..., FPORT_MAC_COMMAND)` path in `parseDownlink` and its effect on `lastToA` and on the outer call's downlink buffer and event.
- Pacing must cover **all** frames sent within one call.
- Determine what happens to application downlink data in the nested case (the L4 part) and fix it only if the trace shows a reachable loss that a non-growing correction can fix.
- Note any upstream limitation honestly.

**M3 (demo starvation):**
- Give demo requests a bounded, fair opportunity while batches are backlogged.
- Batch fragment ordering and offset semantics must not change.
- The UI "Demo queued" status must stay truthful.

**M5 (join datarate floor):**
- Verify RadioLib's datarate after `clearSession()` and what `setDatarate()` does before `activateOTAA()`.
- Join attempts must progress toward slower EU868 join DRs, down to DR0, within TTN airtime limits. Keep the exponential retry backoff coherent with the airtime-based hold.

**Acceptance tests.** Use host seams over the pacing and scheduling decisions. Prefer extracting existing pure decision logic that's already in `Workers.cpp`/`Telemetry.h` over adding new frameworks. Each test asserts public behavior:
- **Wrap:** a time source near `0xFFF00000`. Uplinks and joins continue across the wrap, holds are neither 2³² ms nor 0, and pacing is never shorter than required.
- **Join hold:** over five or more forced join failures, every hold equals `airtimeDelay(that attempt's airtime)` and is not cumulative. The persisted hold matches.
- **Nested MAC:** the total paced airtime is at least the sum of the outer and nested frames' airtime.
- **Demo fairness:** with three batches queued plus a demo request, the demo is sent within a bounded number of uplinks and batch fragments stay in order.
- **Join DR progression:** the DR sequence across failed attempts reaches DR0 and each attempt's airtime is accounted for.

If a behavior can be checked only against real RadioLib internals, use the real RadioLib 7.8.1 source with a host HAL stub where that's feasible. Otherwise mark it hardware/integration-only. Don't replace it with a mock that asserts your own assumptions.

### P3: Sensor correctness (M4, L1)

**Invariant:** a slot is valid only when every value it reports is a real, in-range, non-clipped conversion. Failed or saturated values are never published as valid.

**M4:**
- Verify in INA226 0.6.6 what `INA226_MATH_OVERFLOW_FLAG` covers and what the shunt-voltage register and API expose.
- A reading at shunt-ADC full scale (±81.92 mV) or at or above the calibrated maximum current must mark the current and power invalid. Decide from the encoder contract in `Telemetry.h` whether a valid voltage survives on its own.

**L1:**
- With `INA_SHUNT_OHMS=0`, valid voltage is currently dropped on air, because `encodeReading` needs all three values to be finite.
- Decide whether the existing encoder and decoder contract can represent voltage-only slots **without a wire-format change**. If a format change is needed, it's an extension: apply the §3 gate and update `Telemetry.h`, `ttn-decoder.js`, the reference packet and the tests together.
- Otherwise document the limitation accurately.

**Acceptance:**
- Host tests for saturation classification at, below and above full scale, with positive and negative values.
- Encoder and decoder agreement for any changed slot validity rules.
- Hardware-only: a bench over-current test with the real shunt.

### P4: Remaining functional errors (L2, L3, L4, L5)

- **L2:** the 48 h lockout after a reset during TX comes from the 60 s pre-TX reservation. Keep the reservation, since it's safety-relevant. Fix it only if a correct, non-growing reshaping exists; otherwise document it accurately in the checklist.
- **L3:** GPS status must distinguish "no data recently" from "no fresh fix", and satellite count must be shown only when valid. Use existing TinyGPSPlus validity and age APIs without changing the UI layout.
- **L4:** the downlink screen must refresh when a new downlink arrives. Zero-length application-port downlinks must be shown truthfully.
- **L5:** GT911 failures after boot must become visible through the existing status mechanism.

**Acceptance:** host-testable status mapping where a seam exists. On-device UI refresh is hardware-only.

### P5: Documentation and annotations

1. **Fix the known false socket/WiFi text, without changing wiring or geometry:**
   - `docs/wiring.md` ("Keep C6 WiFi module installed" / "occupied WiFi socket")
   - `tools/draw_wiring.py` labels
   - `README.md`, `docs/hardware-checklist.md` and `docs/validation.md`, wherever the claim recurs (grep for it)
   - **Correct statement:** WiFi comes from the onboard ESP32-C6 (IC1). The upper-right extension socket is not needed for WiFi. J7 stays as the chosen radio connection by user decision.
2. Correct `docs/airtime.md`: the pacing basis must match the corrected implementation.
3. Correct `docs/validation.md`: the net audit didn't cover core variant macros (C1).
4. **Regenerate** `docs/wiring.svg`/`.png` and the UI preview, if it's affected at all, from the authoritative generators in `tools/`. Never hand-edit them.
   - Keep `docs/source-photo.png` byte-identical; its SHA256 is in `validation.md`.
   - Re-verify that the photo embedded in the SVG is byte-identical to the original.
5. Re-run `tools/check_wiring.py`. Its J7 result must be unchanged.

## 5. Hardware prerequisites: document only, never claim

- **TCXO supply.** The Waveshare schematic shows both FB1 (from DIO3) and R22 (0 Ω, from 3V3). The actual population is unknown.
  - If only FB1 is fitted, `RADIO_TCXO_VOLTAGE=0` leaves the radio without a reference clock.
  - If R22 is fitted, frequency accuracy is unverified.
  - Don't change the default without measured evidence, and keep any value a user-set configuration choice.
  - Add the inspection step and the TTN frequency-offset check to the checklist.
- **RF switch polarity:** confirm with a logic analyser.
- **Other open items:** GT-U7 VCC and UART levels, the INA226 shunt value and address straps, the P4 silicon revision, and the C6 ESP-Hosted firmware version.
- **Still unverified:** 10.1-inch DSI timing (the EK79007 profile is the 7-inch one), GT911 orientation, NS4263B enable polarity, and ES8311 retention across the I2S clock gap.
- Wire from the table, not the photo, for DIO1/DIO2.
- None of these may be presented as verified.

## 6. Builds and validation

- Run `make test` and `make format-check`, plus the new tests wired into `tools/test.py`. Independent checks keep running after another check fails. Never weaken, skip or loosen assertions to get a pass.
- Compile all three variants from `docs/validation.md` with the common FQBN options (`esp32:esp32:esp32p4`, `FlashSize=16M`, `PSRAM=enabled`, `PartitionScheme=app3M_fat9M_16MB`):
  1. `ChipVariant=prev3` with the delivered blank credentials and shunt
  2. `prev3` with the dummy overrides `-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f`
  3. `ChipVariant=postv3` with the same dummies
  
  If a board variant is approved, update the FQBN contract consistently.
- Build paths: temporary under `/tmp/codex/build/CrowPanelTelemetry/<task>`, persistent under `build/codex/<configuration>`.
- Earlier builds and logs are not evidence. Rebuild from the final source and replace `docs/validation-logs/*` with the new output.
- Run clang-tidy using a compilation database generated with the same FQBN and build path. Report any findings you leave in place.
- Update `docs/validation.md` with fresh sizes, the exact commands, what was and wasn't verified, and change accounting.
- Regenerate `SHA256SUMS.json` from the final files. Check whether a packaged ZIP deliverable exists. Refresh it if it does; don't create a new one unless the existing packaging process calls for it.
- Remove temporary task directories after their evidence is copied, and say so.

## 7. Handover report

- For each finding (C1, H1, H2, M1–M5, L1–L5 and the documentation items):
  - its classification (A/B/C)
  - the source trace (file, function, line)
  - the correction made, or the reason none was made (disproved, needs approval, hardware-only)
  - the tests that cover it
- Any growth or extension approvals requested and their outcome.
- The exact commands run and their results, the build sizes for all three variants, and the format and tidy status.
- Production LOC delta and test/validation/tooling LOC delta, reported **separately** as added and removed physical lines.
- Files changed, and confirmation that unrelated changes, pin assignments, J7 wiring, the UI visuals and the source photo were preserved.
- Hardware-only checks that are still unexecuted, and the unresolved physical prerequisites.
- A truthful go/no-go for assembly, for flashing, and for unattended TTN operation. No claim of display, touch, audio, WiFi, sensor, GPS or RF operation without a physical test.
