# Independent deep review: CrowPanelTelemetry @ main b80bf07

I worked read-only. I built, ran, flashed and transmitted nothing, and I did not recompute any hashes. All statements about tests and builds refer to saved evidence that I read, not to checks I ran. I traced production code and the isolated pinned/patched dependency tree first. Only after that did I read the dispositions, the previous Claude review and the caller's assessment.

## Verdicts (conditional)

| Stage | Verdict |
|---|---|
| **Assembly** | **Conditional GO.** I found no new source blocker. These physical prerequisites are unchanged and still gate it: J7/J2/J13 orientation, GT‑U7 VCC/TXD level, INA226 shunt/straps, CoreSX1262 R22/FB1 (TCXO) population, RF‑switch polarity, rail headroom. |
| **Supervised bench flashing** | **Conditional GO**, but only with separate flash authorization. Build against an isolated folder in which **both** patches are applied (see finding 3). Use a curated SD card, and connect the antenna and settle conducted power vs EIRP before any RF. Display clock (nominal 48 MHz), DSI/GT911 orientation and the codec MCLK gap still need measuring. |
| **Unattended TTN** | **NO‑GO.** Finding 1 is a remaining §7/Table 17 gap. The retained reboot/SD‑Hosted/recursion/stack/EIRP/codec risks also still apply, and no 24 h acceptance run exists. |

## Disposition of the five fixes

| # | Fix | Disposition | Basis |
|---|---|---|---|
| 1 | Join retry | **PARTIAL** | Power‑up sequence verified compliant: one deadline in `Workers.cpp:45–48`, jitter added after `max(backoff, hold)`, signed‑span clamp, 24 h cap at `:174`. Types are safe: `joinRetry*2` ≤ 172.8 M; jitter + cap ≤ 86.46 M < INT32_MAX; the `int32` conversion is modular in GCC. The in‑run session‑restart path still exceeds the 8.7 s/24 h budget (finding 1). RNG entropy is not established (finding 4). |
| 2 | JPEG EXIF removal | **RESOLVED** | No dangling helper or caller: `GetTIFFInfo`/`TIFF*` are gone, and all `iThumb*`/`ucOrientation` uses are getters or the thumbnail branch. The thumbnail branch (`jpeg.inl:4845–4853`) can no longer be reached: `openRAM` memsets the state (`JPEGDEC.cpp:66`), so `iThumbData == 0` → `JPEG_INVALID_PARAMETER`. APP1 goes through the generic skip (`:1642`). Every iteration advances at least 2 bytes. `usLen < 2` resyncs. Over‑long lengths take the seek path and end in `JPEG_DECODE_ERROR`; reads stay inside `ucFileBuf`. The SOF guard (`:1554–1560`) reads `s[iOffset+7]` only after the length/bounds checks. Progressive files are still rejected by the app (`Workers.cpp:322`, `getJPEGType` at `JPEGDEC.cpp:151`). The decode path is unchanged. README:240–244 describes the trade‑off accurately. |
| 3 | WiFi rejected request | **RESOLVED** | Traced in core `STA.cpp:373–434, 537–552`: `connect()` waits for the old link to drop, otherwise returns false. A rejected request expires `connectAt` (`Workers.cpp:260`). `reportAt` holds the specific error for one 2 s interval. The next offline report says "Connection failed", not "Connecting". No save is armed. If the old link is still up, its real SSID/IP is shown truthfully. Only residual: a cosmetic 2³² ms wrap (see Informational). |
| 4 | RekeyInd ownership | **RESOLVED for the queue/wire behavior**; adjacent pre‑existing defects remain | Details below. Two upstream defects remain on neighbouring code (findings 2 and 5). The fix also makes the ADR‑limit restart reachable in one more case, and that restart feeds finding 1. |
| 5 | Display clock docs | **RESOLVED (docs)** | `Ui.cpp:7` requests 51 MHz. `Arduino_ESP32DSIPanel.cpp:65–66` maps it, `mipi_dsi_hal.c:266–273` computes `roundf(240/51)=5` → 48 MHz, and both installed `clk_tree_defs.h:433` select PLL_F240M. Arduino libs report IDF v5.5.5 b774170 for both variants. The timings match Lesson07 `board_config.h:30–38`. Physical clock, stability and the DISPON question remain open (C). Minor label inconsistency, see Documentation. |

**RekeyInd detail.** I traced these paths against §5.10 (p.42–43, lines 1183–1201):

- **Join enqueue:** `:914–920`.
- **No RX:** `clearMacCommands` keeps persistent CIDs (`:280`, `:3143–3157`).
- **Any authenticated Class A downlink:** keeps only RekeyInd, rebuilt from `rev` (`:2114–2124`). This runs before FOpts processing, so a valid RekeyConf in the same frame still deletes it (`:2772–2773`). A wrong version (`srvVersion != 1`) is ignored, as §5.10 requires.
- **Capacity:** the decision includes retained bytes (`:2259`). Small replies are appended (`:2300–2301`).
- **Port 0:** packs queue + supplied commands into FRMPayload (`:154–159`). FOptsLen is forced to 0 (`:1383`), so the same frame never carries both.
- **Length consistency:** `isValidUplink` against `getMaxPayloadLen()` (`:3793`) keeps FRMPayload ≤ N.
- **Nested TX failure:** keeps RekeyInd; the replies are lost and the network re‑requests them.
- **Session lifecycle:** `clearSession` resets both lengths (`:358`). Export/restore copies the queue (`:521–522`, `:643–644`). The ADR‑limit revert happens at `:103–108`.
- **Delete/clear consumers:** `:280`, `:2252`, `:2773`, `:3151`. All are consistent: CID is included in the length, and every match is found at the scan position.
- **Other sticky answers:** RXParamSetup/RXTimingSetup/DlChannel/TxParamSetup now persist correctly until a Class A downlink.
- **Tests:** they decrypt the actual transmitted commands (`tests/radio.cpp:786–851`, including FCtrl/FPort bytes at `:825`). They do not just restate the implementation.

## Findings, ordered by practical severity

### 1. In‑run session restart re‑enters the fast join schedule and can exceed 8.7 s/24 h (Low–Medium; A mechanism, predicted‑airtime consequence) — NEW

**Trace**
- A successful join resets `joinRetry = 60000` (`Workers.cpp:167`).
- The only in‑run way back to joining is RadioLib's RekeyInd ADR‑limit revert (`LoRaWAN.cpp:103–108`). The fix extends it to cover "downlinks arrive but no valid RekeyConf".
- The rejoin then repeats the boot progression: DR5→DR0, then 4272.5 s holds, then 7680 / 15360 / 30720 / 61440 s.

**Arithmetic** (23‑byte JoinRequest airtimes, same formula as `tools/airtime.py`: DR5 62, DR4 113, DR3 206, DR2 371, DR1 823, DR0 1482 ms; hold = `airtimeDelay(t,1250)+t`)
- The 6th through 11th attempts are all DR0 and occur within about 62.3–62.7 ks of each other, including the ~7 s join duration and up to 60 s jitter per gap. The 12th comes about 124 ks after the 6th.
- So any 24 h window starting at the 6th attempt holds **6 × 1482 = 8.89 s > 8.7 s**.
- A window starting at the restart holds **≈10.46 s**.

**Normative basis:** LoRaWAN 1.1 §7, Table 17 (p.65, lines 1837–1861). T0 is power‑up or reset, not MAC restart.
- The boot trace is compliant. The windows from T0+11 h contain only one or two attempts, which the 60‑day test confirms.
- A restart at uptime ≥ ≈9.73 h (35,024 s), followed by about 18.6 h of failed joins, violates Table 17.
- Realistic trigger: marginal coverage, or an outage that begins between JoinAccept and the first RekeyConf. At DR0 pacing, 64 uplinks take about 5.6 days, so the restart naturally happens at high uptime.

**Smallest remedy (policy decision required):** change only the constant at `Workers.cpp:167` from 60000 to 3840000.
- Boot behaviour is unchanged (`:156`).
- A restart would then rejoin at DR0, at about 0, 1.2 h, 3.3 h, 7.6 h, 16.1 h and 33.2 h: at most 5 × 1.482 = 7.41 s in any 24 h.
- Trade‑off: slower recovery after a restart, with SF12 rejoins.
- Alternative: document this as a retained limitation.

**Regression test:** extend `joinRetryTrace` (`tests/radio.cpp:649–709`):
- join successfully;
- force the ADR‑limit revert at uptimes 10 h and 30 h;
- keep joins failing for 60 days;
- assert `< 8.7 s` for every 24 h window starting at or after T0+11 h, using the existing `transmitted` helper.

It should fail now and pass after the change.

### 2. Port‑0 retained answers overflow the 15‑byte `fOptsDown` (Low; A, pre‑existing upstream, authenticated network server only) — NEW, not a regression

**Trace**
- For FPort 0, `fOptsLen = payLen` (up to 242) (`:1993–1995`).
- `clearMacCommands(..., DOWNLINK)` keeps every `user` CID: LinkCheckAns (3 B), DeviceTimeAns (6 B) and 0x80 Proprietary (6 B) (`:3066–3069`, MacTable `:11, :22, :26`).
- Then `memcpy(this->fOptsDown, fOptsPtr, fOptsLen)` runs into a 15‑byte array (`:2255–2256`, `LoRaWAN.h:1006`).
- Three DeviceTimeAns (18 B) already overwrite `fOptsUpLen`, `fOptsDownLen` and `lwMode`. Larger payloads reach the keys, `devNonce` and `fCntUp` (`LoRaWAN.h:1007–1043`).
- A corrupted `fOptsUpLen` then corrupts FCtrl and `getMaxPayloadLen` (`:1415`, `:3793`).

**Reachability:** only an MIC‑valid frame from the network server. The device never sends LinkCheckReq or DeviceTimeReq, so TTN is not expected to send such a frame.

**Remedy (needs patch‑scope decision):** bound the copy at its authority (`:2255`). The alternative is to document a trusted‑NS assumption.

**Test:** a real‑library port‑0 downlink with 3× DeviceTimeAns, then assert `getFOptsUpLen()`, `isActivated()` and a valid next uplink on the wire. ASan does not detect this intra‑object overflow.

### 3. README IDE steps omit the JPEGDEC patch, and an unpatched JPEGDEC builds silently (Low; documentation/build contract) — NEW

- `README.md:53–54` tells IDE users to patch RadioLib only ("unpatched RadioLib is rejected").
- The later Build section (`:225–226`) mentions both patches.
- Unpatched RadioLib fails to compile because of the `applicationDownlink … override` (`Workers.cpp:84`). Nothing equivalent detects an unpatched JPEGDEC, which still has the SOF write overflow and the EXIF out‑of‑bounds reads.
- `make compile` without `LIBRARIES` would also pick up global libraries (`Makefile:19`).
- **Remedy:** a documentation fix only at `:53–54`, stating that JPEGDEC is not detected at build time. No marker is proposed; you rejected marker macros.

### 4. Jitter source is pseudo‑random on the P4 by IDF's own criteria (Low; B)

- The pinned v5.5.5 bootloader always calls `bootloader_random_disable()` before the app starts (`bootloader_utility.c:791–792`). On the P4 this stops the SAR‑ADC/TRNG entropy (`bootloader_random_esp32p4.c:73–94`).
- No app‑side re‑enable exists (`hw_random.c`). The P4 has no local RF; the C6 hosts WiFi.
- `random.rst:41–47` then says the output "should be considered as pseudo‑random only". Its non‑RF note (`:70–72`) contradicts the bootloader source; I resolved that from source.
- §7 allows a seeded PRNG but requires a distinct sequence per device. Per‑device distinctness is not established.
- **Remedy:** documentation. `protocol-parser-fix-dispositions.md:35` says "hardware RNG jitter"; qualify that as pseudo‑random in this configuration. No code change is needed for compliance.

### 5. RekeyInd ADR‑limit check uses equality (Info/Low; A, upstream)

- `this->fCntUp == (1UL << this->adrLimitExp)` (`:103`).
- An ADRParamSetupReq that lowers the limit below the current FCntUp (`:2779–2781`) means the §5.10 revert never fires.
- This needs a non‑conformant server (one that sends ADR params but no valid RekeyConf).
- `>=` would be a one‑character fix, but it is patch scope and needs your decision.

### 6. Retained JPEGDEC DQT/DHT length underflow reads out of bounds (Low; A mechanism, B consequence; retained upstream, outside fix 2)

- `usLen -= 2` and the DQT loop `usLen -= 65/129` on `uint16_t` wrap for short segments (`jpeg.inl:1600–1601, 1611–1639`).
- `JPEGGetHuffTables` then receives a huge `iLen` (`:837–872`).
- Writes stay bounded: quant index ≤ 255, Huffman offsets ≤ 2183 B inside `usUnalignedPixels`. Reads run past `ucFileBuf` into `ucHuffDC`/`usHuffAC` and possibly beyond the object.
- Expected result: wrong pixels or a decode error, not a fault. Nothing is measured.
- This is consistent with "not a complete decoder audit" (`correctness-status.md:171`), but the ASan/bounds evidence cannot detect it (see Test gaps).
- **Remedy:** treat SD content as curated, or approve two guards.

### Informational (no action proposed)
- **WiFi status wrap:** `millis()-connectAt` is modular. With no new request and no link, "Connecting..." reappears for about 20 s once every 49.7 days.
- **`eventUp->confirming`** is still always false (`:245` clears it before `:251` reads it). The app does not use it.
- **No‑SOF JPEGs:** `getJPEGType()` reports BASELINE for any non‑0xC2 mode, including files with no processed SOF. The app does not reject 0×0 dimensions (`Workers.cpp:322`). I did not trace whether such a file survives `JPEGMakeHuffTables` and decoding. This is a test suggestion, not a finding.

## Already accepted/retained limits (restated, not reclassified)

These remain as documented:
- volatile aggregate pacing with no per‑frame reservation, and immediate rejoin after reboot;
- unpaced internal MAC replies;
- unbounded nested MAC recursion (`:2295`), with no stack proof;
- SD/Hosted shared‑controller lifecycle;
- ES8311 false‑success returns;
- `ESP_ERROR_CHECK` abort in DSI init (`Arduino_ESP32DSIPanel.cpp:36–100`);
- GT911 latch;
- priority‑2 busy‑wait;
- JPEG scale off‑by‑one (1201 px is rejected);
- the GPIO30 startup pulse;
- TCXO = 0 V;
- 14 dBm conducted vs EIRP.

Durable pre‑RF DevNonce reservation is intact (`Workers.cpp:49–72`): reserve → TX → `max(devNonce, next)` → re‑save → fail‑stop. JoinNonce is persisted at the same time.

## Comparison with the previous Claude review (f23e393) and caller qualifications

| Previous finding | Status now |
|---|---|
| F1 join budget | Fixed for the power‑up path. Finding 1 is a different path that neither the earlier review nor the fix covered. |
| F2 EXIF out‑of‑bounds | Fixed by removal. |
| F3 display | Docs fixed. The caller's qualification is correct: do not infer that DISPON is needed. |
| F4 WiFi status | Fixed. |
| F5 same‑SSID race | The caller's qualification is source‑consistent: installed `esp_wifi.h:1025` documents `ESP_ERR_WIFI_STATE` for `set_config` while connecting. It is still unproven, and C6 firmware remains unverified. |
| F6 amplifier pulse | Unchanged, B/C. |
| F7 RekeyInd | Fixed. Its suggested test is now present with wire inspection. |

## Test gaps
- **No session‑restart join‑budget test** (finding 1). The 60‑day trace covers the power‑up sequence only. It also uses host 64‑bit `RadioLibTime_t`, deterministic RNG streams and simulated receive windows.
- **No port‑0 test with more than 15 retained answer bytes** (finding 2). `testMacAnswerRetention` uses 9 bytes.
- **ADR‑limit revert is not exercised through the production worker join loop.** `testRekeyRestoreAndLimit` calls `activateOTAA` directly.
- **JPEG:**
  - no fixture with an APP1 larger than 2 KB, which camera photos produce and which exercises the parser's seek/refill path;
  - no APP length 0/1 or beyond‑EOF fixture;
  - pixels are digest‑checked only at scale 0;
  - ASan cannot see intra‑object overreads inside `JPEGIMAGE`, so "ASan/bounds pass" does not cover finding 6.
- **WiFi:** the stub does not model the 1 s disconnect wait, Hosted RPC failures or event timing.
- **Crypto fixtures** use RadioLib's own primitives, so they are not independent vectors.

## Misleading or inaccurate documentation
- `README.md:53–54`: see finding 3.
- `protocol-parser-fix-dispositions.md:30–31`: states the invariant "failed joins must obey … Table 17" without restricting it to the power‑up path (finding 1). `airtime.md:173–179` is scoped more accurately.
- `protocol-parser-fix-dispositions.md:35`: "hardware RNG jitter" (finding 4).
- Display fix classification: "B, FIXED‑DOCS" in the dispositions (`:119`) vs "A source calculation/C physical" in `correctness-status.md:172`.

## Normative and electrical sources used, with retrieval limits
- **LoRaWAN 1.1 specification:** the local LoRa Alliance PDF text (`official-docs/LoRaWAN-1.1.pdf.txt`), official URL https://resources.lora-alliance.org/technical-specifications/lorawan-specification-v1-1. I used §7 / Table 17 (p.65) and §5.10 (p.42–43). I did not fetch it online and did not verify the hash.
- **ESP32‑P4 datasheet** (local copy): pin table lines 769–773 and 850–860, and lines 1225–1227 on GPIO24/25 USB pads. This GPIO24/25 note was already addressed in `rereview-fix-status.md:111–122`, and I did not re‑derive it.
- **ESP‑IDF v5.5.5 sources and `random.rst`** (pinned tree).
- **Not read:** the ES8311, NS4263B, INA226, GT911, RTC6603SP and CoreSX1262 PDFs. GT‑U7 ratings are unknown, and I make no electrical claim from them.

## What I did not inspect
- **Tests:** `audio.cpp`, `codec.cpp`, `sensor.cpp`, `gps.cpp`, `sd-storage.cpp`, `wifi-form.cpp`, `decoder.js`, stubs and fixtures. I read `radio.cpp` only at lines 530–950.
- **Tools:** everything except `test_jpeg.py`.
- **Logs and hashes:** of the validation logs I read only `final-build-results.json`. No hash was recomputed and no patch application was re‑checked.
- **Docs:**
  - read only through targeted searches: `validation.md`, `hardware-checklist.md`, `wiring.md`, `accepted-review-fixes.md`, `rereview-fix-status.md`, the proposals;
  - not read: `ui.md`, `deep-fix-dispositions.md`, `artifacts/`.
- **Dependencies not read:**
  - RadioLib: lines 1690–1960 (`receiveDownlink`/parse header), the SX126x driver, HAL, `setDutyCycle`;
  - JPEGDEC: MCU decode/IDCT, `readRAM`/`seekMem`;
  - core: `NetworkInterface`, `WiFiGeneric`, Preferences, ESP_I2S;
  - INA226, TinyGPS++, audio‑driver, esp‑hosted.
- **Manufacturer and harness files:** `panel.sch`/`.brd` were not opened; `sources.json`, `run.py`, `stream.jsonl` and `invocation.json` were not read.
