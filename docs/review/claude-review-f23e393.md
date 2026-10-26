# Independent deep review: CrowPanelTelemetry @ main f23e393

I traced the production sources and the exact pinned dependencies first. The CoreSX1262 module schematic I used is the Waveshare-hosted copy. I read the project's own review records only after that. I did not run, build, flash or transmit anything. Statements about tests and builds below refer to saved evidence, not checks I ran.

## Readiness verdict

| Stage | Verdict |
|---|---|
| **Assembly** | **Conditional GO.** I found no source blocker. These physical checks still gate it: J7/J2/J13 orientation, GT‑U7 supply and TXD voltage level, INA226 shunt and address straps, CoreSX1262 R22/FB1 (TCXO supply) population, rail headroom. |
| **Supervised bench flashing** | **Conditional GO**, with separate flashing authorization. Use a curated SD card (see finding 2). Attach the antenna and settle conducted power vs EIRP before any RF. Verify the display, which actually runs at 48 MHz (finding 3). |
| **Unattended TTN** | **NO‑GO.** New finding 1 is a normative LoRaWAN 1.1 §7 violation. The retained reboot/rejoin, SD/Hosted, stack and EIRP risks and the unexecuted 24‑hour run also still apply. |

## New and residual findings, by severity

### 1. Join retries exceed the LoRaWAN 1.1 §7 limit after 11 hours of failed joins (Medium, A)

**Trace**
- After a failed join, the deadline is the larger of `defer(joinRetry)` (`Workers.cpp:169`, capped at 1 h at `:173`) and `accountAirtime()` (`:59`, `:73–80`).
- `joinDatarate` (`Telemetry.h:56–63`) reaches DR0 once the retry interval is ≥ 1,920,000 ms.
- A 23‑byte SF12 JoinRequest is about 1482 ms on air (my calculation, matching the tools/airtime formula). `airtimeDelay(1482, 1250) + 1482` ≈ 4,272.5 s, so one attempt every ~71 min.
- That is ≈ 20.2 attempts/day ≈ **30.0 s per 24 h**, indefinitely.
- The intervals are deterministic; nothing randomizes them.

**Normative text** (local official LoRaWAN 1.1 PDF, `official-docs/LoRaWAN-1.1.pdf`, sha256 c202b49f…; page 65, lines 1837–1861, Table 17)
- Join-request airtime must stay under 36 s in the first hour, under 36 s over hours 1–11, and **under 8.7 s per 24 h after that**, "whichever is more constraining".
- The interval after RX2 "SHALL be random and follow a different sequence for every device".
- The first two windows pass (≈1.6 s and ≤12.5 s). The third is exceeded about 3.4×, and the randomness rule is not met.

**When it happens:** a device left unattended without coverage, with wrong keys, or during a network outage. The prior N3 finding covered reboot loops, which is a different trigger.

**Smallest remedy** (this changes pacing policy, so it needs your decision):
- In the existing join-failure branch only, when uptime exceeds 11 h, defer by at least `ceil(lastToA × 86,400,000 / 8,700)` ms. T0 is power‑up/reset per Table 17, so this stays volatile and needs no persistence.
- Add a random component (radio or ESP RNG) to that same `defer`.
- No new authority, no NVS.

**Regression test:** extend `testJoinLoop` (`tests/radio.cpp`) to run 48 simulated hours of no JoinAccept with the reference airtimes. Assert all three Table 17 windows, and that the interval sequences differ for two seeds.

### 2. JPEGDEC EXIF APP1 parsing reads out of bounds using offsets taken from the file (Medium if SD content can be untrusted, Low for curated cards; A mechanism, B consequence)

**Trace**
- `JPEGParseInfo` handles `0xffe1` at `jpeg.inl:1653–1677`.
- `IFD = TIFFLONG(...)` comes straight from the file. `GetTIFFInfo(pPage, bMotorola, IFD+iOffset+8)` (`:1662`) then indexes `ucFileBuf[iOffset]` and up to 256×12 bytes of tags (`:1345–1375`) **with no bounds check**.
- The thumbnail path re-reads `s[IFD+iOffset+8]` (`:1668`). Its guard `IFD+iOffset+8 < 2048` (`:1669`) accepts negative values.
- The buffer is 2048 bytes (`JPEGDEC.h:57`). All of this runs inside `openRAM`, before the app's baseline check (`Workers.cpp:313–321`).

**Consequence:** a malformed EXIF block reads arbitrary memory. Large offsets fault and reboot the P4; together with the retained N3 behaviour, that means an immediate rejoin. Valid camera files are not affected.

This is separate from the approved SOF-write guard (N1). The README already says decoder checks are "not a comprehensive audit".

**Remedy options** (need your decision):
- Add one more bounded guard to the existing JPEGDEC patch: require 0 ≤ IFD and that the full IFD fits within `iBytesRead`.
- Or explicitly treat SD content as trusted.
- The app cannot avoid this path without writing a second parser.

**Regression test:** APP1 fixtures with IFD values 0x7FFFFFF0 and 0x80000000, and a tag count that runs past the buffer, run under ASan. A bounds-only sanitizer will not catch the pointer arithmetic.

### 3. Display clock is 48 MHz, not 51 MHz; init-sequence evidence is incomplete (Low, A for the clock; C for the init sequence)

**Clock**
- `Ui.cpp:7` passes 51 MHz. Arduino_GFX turns that into `dpi_clock_freq_mhz=51` with the default clock source (`Arduino_ESP32DSIPanel.cpp:63–90`).
- IDF 5.5.5 picks `div = roundf(240/51) = 5` (`mipi_dsi_hal.c:266–273`, `esp_lcd_panel_dpi.c:266–283`).
- The default source is PLL_F240M in **both** installed SDKs (`clk_tree_defs.h:433` in esp32p4-libs and esp32p4_es-libs).
- Actual pixel clock: **48 MHz**, about 51.9 Hz refresh. The README:245 and accepted-review-fixes claim "51 MHz pixel clock".
- Lane budget is fine: 768 Mb/s over two 1000 Mb/s lanes.

**Init sequence**
- `ek79007_init_operations` (`Arduino_DSI_Display.h:776–789`) sends PAD/2‑lane, vendor registers and SLPOUT, but no DISPON (0x29).
- Elecrow's V1.2 config uses the ESP32_Display_Panel default EK79007 sequence, whose source is not among the pinned inputs. I cannot tell whether DISPON is required.

**Conflicting documentation, resolved**
- Elecrow's config sets `MIPI_PHY_LDO_ID (-1)` (`esp_panel_board_custom_conf.h:249`).
- The pinned schematic routes VO3 (`ESP32_LDO3`) through R80 (0 Ω) to `VDD_MIPI_DPHY` (`panel.sch:9176`, `15298–15320`, `16687–16701`).
- The P4 datasheet recommends 2.25–2.5–2.75 V for that pin (text line 5117).
- So Arduino_GFX's VO3 = 2.5 V is correct for this board.

**Remedy:** correct the documentation only. Measure the real clock and image stability on the bench.

### 4. WiFi status says "Connecting…" after a rejected request (Low, A — misleading status)

- `Workers.cpp:258–263` posts the failure and sets `reportAt = connectAt`.
- Two seconds later `:277–283` reports "Connecting..." for about 18 s, then "Connection failed".
- When the rejection was not the old link refusing to drop (for example an `esp_wifi_set_config`/`esp_wifi_connect` error, `STA.cpp:415–432`), nothing is actually connecting.
- **Remedy (no new state):** on rejection, back-date `connectAt` so the existing timeout branch reports failure. Tradeoff: `connectAt` takes on a second meaning.
- **Test:** in `tests/wifi-worker.cpp:124–130`, advance 2 s after a rejection and assert the message is not "Connecting...".

### 5. Changing the password on the same SSID while an association is still in flight (Low, B)

- The save gate is raw link + IP + `WiFi.SSID()`, which reports the live association (`STA.cpp:665–674`).
- That gate cannot tell *which* password produced the association.
- If A ("Home", correct password) is still associating when B ("Home", wrong password) calls `set_config`, `connected()` is still false, so no disconnect happens (`STA.cpp:379–384`).
- If A then completes, B's password is saved. Consequence: one failed boot, then re-entry.
- **Recommendation:** bench test only; any fix would add state.

### 6. GPIO30 drives LOW briefly before going HIGH at startup (Low, B/C)

- `pinMode(OUTPUT)` (`Application.cpp:21`) calls `gpio_config` with the output register still 0 from reset (`esp32-hal-gpio.c:133–165`), before `digitalWrite(HIGH)` (`:182–183`).
- On the NS4263B, a falling edge enables AB mode, and SD must stay high for more than 100 µs to shut down (datasheet §5, lines 50, 172, 313–316).
- Result: a short enable pulse. Whether it causes a pop is a hardware question. Fix only if the scope shows one.

### 7. RadioLib (upstream) drops a queued RekeyInd on any Class A downlink (Low, A mechanism / B consequence)

- `LoRaWAN.cpp:2117–2119` clears `fOptsUp` whether or not RekeyConf was received.
- §5.10 (lines 1183–1186, p.42): RekeyInd SHALL be sent in all uplinks until RekeyConf, with a revert to Join after ADR_ACK_LIMIT. The revert check at `:103–108` then never sees it.
- This is harmless if TTS piggybacks RekeyConf, which is its usual behaviour.
- **Recommendation:** document it, or decide on extending the patch scope.
- **Test:** a FPort‑10 downlink with no FOpts right after join; the next uplink must still carry RekeyInd (it currently would not).

**Informational, no action:**
- `eventUp->confirming` is always false (cleared at `:245` before being read at `:251`); the app does not use it.
- A replayed downlink is rejected but reported as a MIC‑mismatch "RX error" plus a 60 s defer, rather than silently ignored.

## Independent disposition of the five corrections

| # | Disposition | Evidence |
|---|---|---|
| 1. Amplifier | **Resolved in source, plus Low residual (finding 6).** | GPIO30 is HIGH before I2S/codec init (`Application.cpp:21–22` vs `:33–44`). LOW only during writes and the 100 ms drain, HIGH afterwards, including write failure and codec failure (`Workers.cpp:421–436`). The schematic path is GPIO30 → R147 (0 Ω) → U4 (NS4263B) pin 1 SD, with R71 10 k pull-up; Q10 and both NS4168s are marked NC (`panel.sch:9310,9488,9522,9523,9648,9655,18636–18698`). The truth table agrees: high = shutdown. `auto_clear=true` and a 30 ms DMA depth (`ESP_I2S.cpp:33`) make the 100 ms drain sufficient. Physical: fitted components, pops. |
| 2. Confirmed-downlink ACK MIC | **Resolved.** | `confFCntDown = (uint16_t)devFCnt32` (`:2107`) uses the authenticated domain counter (`:2017–2021`). 0xFFFFFFFF becomes 0xFFFF, which stays distinct from FCNT_NONE (`LoRaWAN.h:151`). It is consumed at `:1416` and `:1461–1462`, matching §4.4.2 (p.27). Nested replies: the inner `sendReceive` carries the ACK and clears it at `:245`; a confirmed frame arriving in the nested window keeps its ACK for the next uplink. Retransmissions under NbTrans reuse it. Session export/restore round-trips it (`:513`, `:658`). |
| 3. First zero vs accepted zero | **Resolved for unicast.** | Two bits are reset in `clearSession` (`:363`) and committed only after MIC passes (`:2079–2101`). Rollover check at `:2027`; 64-bit exhaustion rejection at `:2035`. App and network domains are independent, matching §4.3.1.5 lines 628–630. Snapshot marker and restore at `:528`, `:544`, `:616–617`. Same-FCnt replays are rejected, consistent with §4.3.1.3 ("SHALL not be retransmitted using the same frame counter") and §4.3.1.5 lines 659–660. The app never restores sessions and the nonce format is unchanged. Multicast is out of scope. |
| 4. WiFi | **Resolved, with Low residuals (findings 4 and 5).** | `connect()` waits for the old link to disconnect before arming (`STA.cpp:379–384,537–552`). Link and IP bits are raw (`NetworkInterface.cpp:316–322`; cleared at `STA.cpp:228`). One blob with exact-size and NUL validation (`Workers.cpp:242–245`). NVS-open and write failures are visible (`:270–273`). Hosted forces RAM storage (`WiFiGeneric.cpp:274–301`), so there is no competing credential store. |
| 5. Display | **Numbers match; qualified.** | Constructor mapping matches Elecrow `board_config.h:32–38`. EK79007 with two lanes (`0xB2 0x10`). Reset 41 and backlight 31/29 match the schematic nets. Real clock is 48 MHz and DISPON is unverified (finding 3). |

## Retained policies and risks (not new defects)

**Your decisions, preserved as directed:**
- Volatile aggregate pacing with no per-frame reservation; immediate rejoin after reboot (N3).
- SD/Hosted shared-controller lifecycle and the Hosted restart path (F4/N2/R3).
- Unbounded nested MAC recursion (F14/R7).
- ES8311 false-success returns (R5).
- Single-attempt demo sends.

**Previously known limits, still present:**
- F5: EIRP/ERP at 16 dBm conducted.
- F6: priority-2 busy-wait in the radio task.
- F7: GT911 error latch until restart.
- F11: conservative over-deferral of airtime.
- F12: DSI init aborts via `ESP_ERROR_CHECK`.
- N9: SD DAT3 divider.
- TCXO configuration (C).

**Low-priority observations, still unchanged:**
- JPEG scaling boundary: a 1201 px-wide image picks 1/2 scale, which produces 601 px and is rejected visibly instead of trying 1/4 (`Workers.cpp:316–321`).
- TinyGPS checksum helper: from the prior review; I did not re-read TinyGPS++.

**Compared with `accepted-review-fixes.md`:**
- R1, R2, R4/N4 and R6/N7 are confirmed.
- R8 is **qualified**: the "51 MHz" claim is contradicted by the IDF divider.
- The retained items are restated accurately.
- Minor labelling: `accepted-review-fixes.md:48` calls the NS4263B PDF "manufacturer-authored" without saying it is distributor-hosted (LCSC); `sources.md:117` does label it correctly.

## Evidence limits
- The local GT911 PDF is the integrator-hosted Waveshare copy. It has no register map, and the Renesas programming guide returned 403. The register addresses 0x8140, 0x814E and 0x814F used in `Ui.cpp` are **not verified from documents**.
- I did not fetch the uberflip pages; I used the hashed local LoRa Alliance PDF instead.
- The NS4263B start-up time value is truncated in the text extraction.
- The exact GT‑U7 carrier's ratings are unknown.

## Test gaps
- No long-horizon join-rate test (finding 1).
- No EXIF fixtures (finding 2).
- No check of the status shown after a rejected WiFi request (finding 4).
- The display test compares numbers only; it does not check the realized clock.
- No RekeyInd-persistence test (finding 7).
- Host `RadioLibTime_t` is 64-bit, there is no ESP32 emulation, and crypto fixtures use RadioLib's own primitives.

## What I did not inspect
- **Docs:** `ui.md`, `visual-method.md`, `radio-nets.json`, `wiring-verification.txt`, `airtime-calculation.txt`.
- **Logs and hashes:** all of `docs/validation-logs/*`. `SHA256SUMS.json`, `correctness-source-hashes.json` and `change-accounting.json` were not read, and no hash was recomputed.
- **Review history:** I read only `accepted-review-fixes.md` and parts of `claude-deep-rereview.md`. Not read: `deep-fix-dispositions.md`, `rereview-fix-status.md`, the earlier reviews/prompts, and `artifacts/`.
- **Tests:** `audio.cpp`, `codec.cpp`, `decoder.js`, `gps.cpp`, `jpeg.cpp`, `sensor.cpp`, `sd-storage.cpp`, `wifi-form.cpp`, the stubs and fixtures. `radio.cpp` and `wifi-worker.cpp` only partially.
- **Tools:** all except `test.py`, `test_radio.py`, `test_wifi_worker.py` and `patch_dependency.py`.
- **Dependency sources:** RadioLib SX126x and Hal code, TinyGPS++, Arduino_GFX drawing paths, ESP32_Display_Panel (not available), the IDF sdmmc/Hosted transport (retained area, not re-traced).
- **Official docs:** the ES8311 and RTC6603SP PDFs; the P4 datasheet beyond the power sections.
- **Harness files:** `run.py`, `stream.jsonl`, `invocation.json`, `stderr.txt`, `lorawan-viewer.html`.
