# Deep expert re-review of CrowPanelTelemetry @ 90e811b (main)

> Editorial note (2026-10-25): At the user's request, the wording below was edited for a defensive engineering focus. This is an edited presentation of Claude's review, not a new Claude assessment or independent verification. Findings, source references and unresolved risks remain review claims. The original response is available in Git at `c6fe271124666c82f92733a8c0f161697761367d`; its SHA-256 is `0385e0d7cc56b4bfb63705a739382e5b132b0f045dc2fc27837c2c9a440e7e34`. Wording changes do not establish whether a service will accept a request.

This was a read-only review. I used only Read/Glob/Grep: no files changed, no commands run, no RF, no TTN access. I traced the current production sources first, then the pinned dependencies (patched RadioLib 7.8.1, INA226 0.6.6, TinyGPSPlus 1.0.3a, JPEGDEC 1.8.4, Arduino_GFX 1.6.9, IDF v5.5.5, Hosted v2.12.13), the installed core 3.3.12 P4 SDK configs, and the pinned `panel.sch`. I read your dispositions and the prior review only after that.

Classes follow your convention: **A** = source-proven failure; **B** = confirmed mechanism, device consequence uncertain; **C** = physical prerequisite.

## Executive verdict

**Fixes that hold up:** F1, F2, F3, F10, the progressive half of F13, and S1 (static) check out against current code and exact dependency source.

**Partly resolved:**
- **F8:** a small residual false positive remains.
- **F9:** the specific diagnostic is visible on serial only; the panel shows the generic stop text.

**Correctly retained, with one change:** F4, F6, F7, F11, F12 and F14 are honestly retained. However, **F4's consequence is no longer just uncertain.** The installed P4 SDK builds Hosted with restart-on-failure, so an SDIO transport failure reboots the whole P4 (N2).

**Highest-priority input-validation finding:** JPEGDEC 1.8.4 does not bound the SOF component count before writing decoder state (N1). This occurs inside `openRAM`, before the application's baseline/type guard. An invalid image loaded automatically at startup may cause recurring decoder failures or resets. Since the airtime deadline is volatile, repeated resets can also cause repeated join attempts (N3). This interaction needs documentation and validation.

**Also new:**
- RadioLib accepts a replayed downlink while its counter is still 0 (N4).
- RadioLib's `isMACPayload` flag stays set after a nested MAC reply (N5).
- Two small error-visibility issues (N6, N7).
- Two schematic-derived items (N8 amplifier enable order, N9 SD DAT3 divider).

**Readiness:**

| Stage | Verdict | Conditions |
|---|---|---|
| Assembly | **Conditional GO** | The C items below, now including N8/N9 measurements |
| Supervised bench flashing | **Conditional GO** | Use only known-good baseline JPEGs until N1 is decided. RF only with separate authorization and an EIRP plan (F5): with any antenna above about 0 dBi net gain, TXPower 0 likely exceeds 25 mW ERP |
| Unattended TTN | **NO-GO** | N1, N2/F4, N3, F5, plus measured stacks/latency and the full checklist including the 24 h run |

## Actionable findings

### N1 — JPEGDEC needs a component-count bounds check before decoder-state writes (Medium; High if SD contents are untrusted). Class A mechanism / B consequence

**Where (dependency):**
- `JPEGDEC.h:63` sets `MAX_COMPS_IN_SCAN 4`; `:225` declares `JPCI[4]`.
- `jpeg.inl:1678–1708` (`JPEGParseInfo`, SOF0/SOF2) loops `for (i=0; i<ucNumComponents; i++)` with `ucNumComponents = s[iOffset+7]` (0–255, taken from the file). Only `quant_tbl_no ≤ 3` is checked.
- The reviewer identifies adjacent decoder file, buffer and pointer state as potentially affected on RV32. Exact effects depend on the target object layout and have not been established by an on-device test.
- Later DHT processing uses decoder pointers (`jpeg.inl:843, 857, 867`), so the component-count check must occur before these state writes and subsequent parsing.

**Where (project):**
- `Workers.cpp:310–312`: the JPEGDEC object lives on the media stack, and `openRAM` runs before the guard at `:320`.
- `Workers.cpp:372–406`: the first remount at boot calls `loadImage(images[0])`.

**Input-validation requirement:** reject component counts outside the supported array capacity before writing component metadata. The reviewer predicts a target fault for some invalid inputs; the physical outcome remains untested.

**Invariant broken:** SD content must only produce "JPG failed", never memory corruption.

**User impact:**
- A panic or memory corruption inside the media task.
- If the file sorts first, a persistent boot loop.
- During each loop the radio task, created first (`Workers.cpp:448`), joins immediately (N3).
- README:169–170 ("progressive or unsupported JPEGs show an error") is not true for this class of file.

**Narrow remedy:** a one-line bound `ucNumComponents > MAX_COMPS_IN_SCAN → JPEG_UNSUPPORTED_FEATURE` in JPEGDEC. Ship it as a hash-checked dependency patch using your existing RadioLib patch machinery, or take an upstream release if one fixes it (I could not check that read-only). Either way it needs your approval.

**Alternative:** an app-side pre-scan of the SOF Nf byte before `openRAM`. This is a second mini-parser, which you have disfavoured, and it is easy to get wrong.

**Regression test:** use local invalid-input fixtures covering component counts above the supported capacity. Check safe rejection by `openRAM()` and run bounds/memory diagnostics. A sanitizer run alone is not proof that all adjacent-field writes are detected.

### N2 — Hosted restart-on-failure turns F4 (and any C6 fault) into a full P4 reboot (Medium). Class B; the escalation path is source-proven

**Where:**
- Installed `esp32p4-libs/3.3.12/sdkconfig:3953` and `esp32p4_es-libs/3.3.12/sdkconfig:3951` set `CONFIG_ESP_HOSTED_TRANSPORT_RESTART_ON_FAILURE=y` (Kconfig default y, `Kconfig:1946–1952`).
- Hosted v2.12.13 `sdio_drv.c:1204–1214` and `:1222–1232`: one failed interrupt/register read calls `_h_restart_host()`, logging "Host is resetting itself, to avoid any sdio race condition".
- Also `:1269–1276`, `:565–580` and `:762–777`.
- `port_esp_hosted_host_os.c:867–871` implements `_h_restart_host()` as `esp_restart()`.

**Link to F4:**
- IDF 5.5.5 `sdmmc_host_set_card_clk` (`sdmmc_host.c:301–352`, reached from `init_slot` at `:823`) changes the controller-wide divider outside the transaction mutex.
- The app calls it from `SdStorage::remount` at boot (`Workers.cpp:246` then `:250`, which overlaps `WiFi.begin`) and on every Retry SD.

**Additional race:** the `deinit_p` guard (`SdStorage.h:30–37`) compares a snapshot with a global count that Hosted can change concurrently (`sdmmc_host.c:836–840`, `:862–873`, `:906–924`). If Hosted's slot count changes between the snapshot and cleanup, an uncounted `deinit_slot(0)` can drop the count to 0 and tear the controller down under Hosted. The guard is correct only under quiescence. README:96–97 should say so.

**Impact:**
- An SDIO error caused by an SD lifecycle operation, or a C6 brown-out or crash, reboots everything.
- Queued batches are lost (RAM only) and LoRaWAN rejoins unpaced (N3).
- Hosted is initialized even when WiFi is never configured (`Workers.cpp:245`), so a WiFi-independent telemetry node inherits the C6's failure modes.

**Narrow remedies (each is your decision, with explicit trade-offs):**
1. **Reduce:** do the initial mount (and its first decode) before `WiFi.mode`, and make Retry SD require a restart once Hosted is up. This removes app-initiated lifecycle operations during Hosted traffic. It costs runtime Retry SD and adds WiFi start latency.
2. **Reshape:** call `WiFi.mode` lazily, only when saved credentials exist or the user submits credentials. Nodes without WiFi then never start the C6 transport. It costs a later discovery of "C6 unavailable".

An app mutex does not help, because Hosted's bus lock is private (`sdio_drv.c:1499`). A custom sdkconfig is not feasible with the stock core.

**Physical check:** 50+ Retry SD cycles under WiFi traffic plus cold boots. Log `esp_reset_reason()` in a bench-only build and count "Host is resetting itself".

### N3 — Every reboot joins immediately; concrete reboot sources now exist (Medium for unattended). Class B — an evaluation of the accepted policy, not a demand to restore it

**Where:**
- `allowedAt = 0` (`Workers.cpp:99`), so `canTransmit()` is true at boot.
- The join at `:155–160` runs within milliseconds of radio init.
- Each attempt makes two NVS nonce writes (`:53`, `:63–65`).

**Reboot sources reachable after the radio starts:** N1, N2, stack overflow (F14, unmeasured), configASSERT in `wifiTask` (`:246`), and physically a 3.3 V brown-out during TX.

**Impact:**
- A boot loop of about 3–5 s gives a 61 ms SF7 JoinRequest each cycle. That exceeds the 1 % g1 sub-band duty cycle and the TTN daily allowance within about an hour.
- DevNonce exhaustion would take about 65 k boots.

**Remedy:**
- First, remove the reboot sources (N1, N2).
- Optional (new policy, needs your approval): seed the existing deadline once at boot with `node.defer(joinRetry)`. This is one line. A crash loop shorter than 60 s then produces no RF; the cost is a 60 s first-join delay. No persistence and no reservations.

**Test:** a bench build that reboots every N s, under RF shielding or on a private network with an SDR monitor. Count JoinRequests per hour.

### N4 — RadioLib needs distinct initial and accepted-zero downlink-counter states (Low–Medium). Class A, upstream 7.8.1

**Where:** `LoRaWAN.cpp:2022` only treats the frame as a rollover when `devFCnt32 > 0`. After a legitimate FCnt 0 frame the stored counter stays 0 (`:2093/:2096`), so the identical frame passes the MIC again (`:2076`).

**Affected state:** a newly established session may not distinguish an initial counter from an already accepted counter value of zero. Duplicate handling for both network and application downlink counters needs verification against LoRaWAN 1.1, including permitted retransmission semantics.

**Impact:**
- MAC commands are re-executed; DevStatusReq, for example, forces replies and airtime.
- The application text is shown again (via `applicationDownlink`).
- This violates the strictly increasing FCntDown rule.

**Remedy:** an upstream fix or a small patch expansion that tracks "no frame accepted yet" distinctly from counter value 0. Needs approval. The alternative is to document it as accepted for a display-only application port.

**Test:** use the existing local real-library test interface to verify duplicate handling at counter zero, separately for application and network downlinks. Assert the protocol-required result and verify that application data and MAC commands are not processed again where prohibited. No over-the-air test is needed for this regression.

### N5 — `isMACPayload` stays set after a nested MAC reply (Low). Class A, upstream

**Where:** the nested call `sendReceive(fOptsRe, fOptsReLen, 0)` (`LoRaWAN.cpp:2287`) has `lenUp > 0`. The flag is cleared only on the `lenUp == 0` branch (`:154–162`).

**Impact:**
- After the first reply over 15 bytes, every later downlink that carries any FOpts goes through `:2279`. It sends an immediate extra uplink, even an empty port-0 frame when no answer is needed.
- These are extra unpaced individual frames (they are counted in the aggregate) and give extra recursion exposure for F14.

**Remedy:** an upstream one-liner, or document it.

**Test:** after the existing six-DevStatusReq nested test, send one frame containing only RekeyConf or a single DevStatusReq. Expect no extra TX.

### N6 — The generic stop message hides the specific diagnostic on the panel (Low). Class A — the F9 residual

**Where:** specific messages are posted at `Workers.cpp:131–133` and `:141–143`, then the generic one at `:226–227`. `Application.cpp:116–121` drains all events in one pass, so `model.radio` ends on the generic text. The "Invalid saved nonces… see recovery guide" pointer is lost from the panel.

**Remedy (reshape):** suspend directly in the init-failure branch after its specific post. Keep the generic post for join/NVS stops only.

**Test:** in `testInitialization`, assert that the last radio message is the specific one.

### N7 — WiFi credentials can be saved on a stale `WL_CONNECTED` (Low). Class B

**Where:**
- `Workers.cpp:258–261` ignores the results of `disconnect` (100 ms wait, `WiFiSTA.h:169`) and `begin`.
- `STA.cpp:379–384`: `connect()` returns false if the old link is still up after 1 s.
- `STA.cpp:113–187`: `_status` is updated by the asynchronous event task.

**Effect:** the next 2 s report can see the old link's `WL_CONNECTED` and persist the new, unverified SSID/password (`:265–274`). This contradicts README:108.

**Sub-point:** for an empty password, `putString(...) == 0` is true on both success and failure (`:269`).

**Remedy (reshape):** arm `saveOnConnect` only if `begin` did not return `WL_CONNECT_FAILED`, and only after a non-connected status has been observed since the command.

**Test:** a fake status sequence that stays CONNECTED for one report after a command must not save.

### N8 — The amplifier enable line is left at its pull-up level during codec init (Low). Class B/C

**Where (schematic):**
- Net AUDIO_OUT_SD contains GPIO30, R147 pin 1, R71 pin 1 and Q10's gate (`panel.sch:18636–18656`).
- R71 is 10 k (`:9523`, no NC marking); its pin 2 is on VDD_3V3 (`:15057`).
- R147 is 0 Ω and goes to NS4263B U4 pin 1 (`:18693–18698`).
- Q10 (AO3401_NC) and both NS4168 parts are NC (`:9310`, `:9488`, `:9522`).

**Where (code):** `Application.cpp:43–44` drives GPIO30 low only after the I2S clock start and stop and the ES8311 init (`:27–42`).

**Remedy (zero net lines):** move those two existing lines to the top of `setupApplication`. The reset-to-setup window remains a hardware property.

**Check:** scope GPIO30 and the speaker output during a cold boot, before and after the move. Confirm NS4263B CTRL polarity.

### N9 — SD DAT3/CS appears to be a 10 k/10 k divider (Low). Class C

**Where:** net SDCS (`:14201–14210`) has R29 10 k to VDD_3V3 (`:14853`) and R33 10 k to GND (`:12782`). Both are listed as fitted (`:9115`, `:9117`).

**Concern:** if both are fitted, DAT3 sits at about 1.65–1.8 V, which is undefined for the card's SD-versus-SPI mode selection at CMD0. Elecrow's own 1-bit SDMMC use suggests it works in practice.

**Check:** measure the DAT3 idle voltage and confirm R33 population.

## Prior findings: independent status

| ID | Status | What I checked | Missing proof / remaining risk |
|---|---|---|---|
| F1 | **Resolved** | `processJoinAccept` (`LoRaWAN.cpp:745–844`): length, MType, decryption with the existing root key, anti-replay, local JSIntKey, MIC; JoinNonce/rev/DevAddr committed only after MIC (`:842–844`). CFList, keys and buffers come after. Pre-MIC member writes are only `tDownlink`/`launchDuration`. The 1.0 accept fails closed (`Workers.cpp:161–162, 226–229`). JoinNonce 0 conflation (`:795`) is mitigated because the 1.1 MIC binds DevNonce (`:824–829`). | A foreign frame in RX1 still masks a genuine accept in RX2 (`:1829–1834`, `:1012–1021`), costing one paced retry. Test crypto is correct in construction but self-referential (built with RadioLib's own `hton`/`generateMIC`/`processAES`). Interop with TTN is unproven. |
| F2 | **Resolved; conservative residuals** | `sendReceive`: a TX error returns before the event is filled (`:199–212`); the event is filled after RX (`:248–259`); parse errors come after that (`:284–285`). The `fCntUp` remedy is correctly disproved (`:205`). | Partial nbTrans repetitions and TX timeout after real RF cause duplicates. A demo request is dropped on "not confirmed" (`Workers.cpp:184`, `:215–222`). Nested errors are invisible. N5. |
| F3 | **Resolved** | `setMaxCurrentShunt` ignores the calibration write result (`INA226.cpp:312`, `:335`); `getLastError` returns and clears it (`:497–502`). The check is immediate (`Application.cpp:75–78`). True zero is preserved. | `setAverage`/`setMode` read-modify-write ignores errors (`:146–154`, `:382–390`); timing may change but values stay valid. OVF may be cleared by the CVRF poll (datasheet-dependent); the range checks make it redundant. The voltage-only limitation (all samples FF at shunt 0) is retained by design. |
| F4 | **Retained, escalated** | Mechanism re-verified on IDF 5.5.5 and Hosted 2.12.13 (slot deinit at `port…sdio.c:307–311`). | N2: the reboot consequence is source-proven. The guard snapshot race exists. Hardware frequency is unmeasured. "Outcome unmeasured" in the dispositions should now read "outcome includes a P4 reboot". |
| F5 | **Retained C (RF blocker)** | `powerMax` 16 dBm is applied as conducted power. | 16 dBm conducted plus a 2.15 dBi dipole gives about 16 dBm ERP (≈40 mW), above 25 mW ERP in g1. Measure before any RF. |
| F6 | **Retained B (justified)** | `ArduinoHal::yield` → `vPortYield` (`ArduinoHal.cpp:195–199`, `esp32-hal-misc.c:118`); loopTask is priority 1 on core 1 (`main.cpp:113`). Busy loops at `LoRaWAN.cpp:1642`, `1665`, `1673` are bounded by scanGuard, or by roughly max-frame ToA plus 300 ms when a preamble is detected. | Lowering the radio to priority 1 would expose RX window timing to time-slicing, so retaining it is reasonable. Latency is unmeasured. |
| F7 | **Retained (intentional)** | Latch at `Ui.cpp:272–277`; `latest` is set before the first poll. | Checklist item 10 (disturbing I2C) will likely latch touch, because GT911 shares the bus. Note it in the checklist. |
| F8 | **Mostly resolved** | `encode` returns true for any checksum-valid sentence (`TinyGPS++.cpp:56–78`, `:159–199`). | `fromHex` accepts non-hex characters (`:103–111`), so at the wrong baud rate noise passes about once per 256 checksum terms, briefly showing "No fresh fix". Low. |
| F9 | **Partial** | The specific rc and NVS text are correct. | N6: the panel shows only the generic text. |
| F10 | **Resolved** | One predicate (`Ui.cpp:60–67`) drives Connect (`:227`) and both render paths (`:82`, `:138`); core limits match (`STA.cpp:386–394`). | Saving is subject to N7. |
| F11 | **Retained (correct)** | Early returns before `:173`; nested `outerToA` (`:2286–2288`). I found no path that under-counts. | N5 increases nested frequency. |
| F12 | **Retained (correct)** | `ESP_ERROR_CHECK` at `Arduino_ESP32DSIPanel.cpp:36–108`; this happens before workers, so no RF. | The amplifier pull-up stays active through a DSI boot loop (N8). |
| F13 | **Progressive resolved; claim qualified** | `getJPEGType` (`JPEGDEC.cpp:149–151`) is checked at `Workers.cpp:320`. | N1 happens before that check. |
| F14 | **Retained B** | The outer callback returns (`:2149–2151`) before the nested call (`:2287`); the retraction stands. Known frames on the deepest path: 3616+80+384+288+80+384+1120+1088 ≈ 7.0 KB, plus libc `vfprintf`, FreeRTOS and the RISC-V/FPU context, which are not in the `.su` files. My estimate is ≈8.5–9.5 KB of 12 KB. | High-water marks are required; frame sums are not a proof. |
| S1 | **Resolved statically** | `loadImage` is 19120 bytes (JPEGDEC object plus a 1 KB `Event`); the decode chain is ≈20.1 KB; `fopen`/FATFS frames are unknown. | Unmeasured; roughly 3–4 KB of apparent margin. |

## Tests and evidence inspected; gaps

**Inspected:** `tests/radio.cpp`, `sensor.cpp`, `gps.cpp`, `jpeg.cpp`, the Wire stub, `tools/test_radio.py`, `test_sensor.py`, `test_jpeg.py`, `stack-frames.txt` (grep), README, `airtime.md`, `wiring.md`, `ui.md`, `hardware-checklist.md`, `validation.md`, `correctness-status.md`, and the dispositions.

**Not read in detail:** `wifi-form.cpp`, `sd-storage.cpp`, `codec.cpp`, the full `sources.md`, the hash JSONs, and the validation logs other than the stack frames.

**Strengths:**
- Real libraries are used.
- Production code is extracted with `str.index` on literal code lines. This is brittle, but it fails loudly.
- The patch hash is asserted before the tests run.
- The JoinAccept and downlink fixtures follow the LoRaWAN 1.1 formulas (JSIntKey derivation, MIC input, ECB direction, B0 block).

**Gaps:**
- Crypto is self-referential; no independent vector or real network.
- No FCnt-replay test (N4), no malformed-JPEG or ASan test (N1), no sticky-flag test (N5).
- The SD double does not model Hosted concurrency or restart (N2).
- The INA226 double does not model a register reset.
- WiFi status timing is not modelled (N7).
- `RadioLibTime_t` is 64-bit on the host. I verified by inspection that the patched comparisons are modular on 32-bit, but no test proves target-width RX/TX elapsed loops.

**Docs to correct:**
- `validation.md:6–7`: "HEAD remains 6fada32" is stale.
- README:96–97 (guard holds only under quiescence), README:108 (N7) and README:169–170 (N1).
- The F4 disposition wording, and checklist item 6, which should also check "does not reboot the board".

**Conclusions that depend on unmeasured hardware:** all of N2/N3's trigger frequency, F5, F6, F14, S1, N8 and N9. Physical prerequisites are unchanged otherwise: TCXO R22/FB1, RXEN/TXEN polarity, GT-U7 VCC/TXD, the INA226 shunt and straps, J2/J13 orientation, P4 silicon versus Chip Variant, C6 firmware versus Hosted 2.12.13, 10.1-inch DSI timing (`Ui.cpp:7`), GT911 orientation, and ES8311 behaviour across the MCLK gap.

**One low-confidence item to check:** GPIO24/25 are the P4's USB full-speed PHY pads. Confirm that no residual USB-JTAG pad pull-up affects MCLK (GPIO24) or the radio RESET (GPIO25).