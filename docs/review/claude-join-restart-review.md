## Session-restart JoinRequest budget at main@f936218

### 1. Verdict: PASS

For the pinned/default application behaviour, this change fixes Finding 1. I found no blocking defect within this restricted scope.

**Source path**
- The pinned RadioLib `LoRaWAN.cpp:103-107` clears the session at `fCntUp == 2^adrLimitExp` (default 64) while RekeyInd is still pending. That matches LoRaWAN 1.1 §5.10 (txt lines 1183–1186).
- It then returns `NETWORK_NOT_JOINED` without RF (`:111-112`).
- On the worker's next loop, `Workers.cpp:158` sees `!isActivated()` and re-enters the join block.
- `clearSession` (`:354-393`) is the only session clear that can happen in-run; the others are in `beginOTAA`/`beginABP`.
- `joinRetry` is now 3840000 (`Workers.cpp:167`). `joinDatarate(3840000)` in `Telemetry.h:56-62` halves five times (1920000 down to 120000) and returns **DR0**. Every later value also returns DR0.
- A failed attempt sets the deadline to max(3840000, actual hold) plus 0–60 s jitter (`Workers.cpp:47,170`). The doubling (`:174`) goes 7.68M → 15.36M → 30.72M → 61.44M → min(122.88M, 86.4M). The largest product is 172.8M, so uint32 does not overflow, and the deadline stays far below the `INT32_MAX` clamp.

**Table 17 arithmetic** (EU868, `TTN_AIRTIME_MS_PER_HOUR=1250` in `Config.h:30`)
- A 23-byte DR0 JoinRequest takes 1482.752 ms (reference formula). `getLastToA` reports 1482 ms.
- The hold after it is `airtimeDelay(1482,1250)+1482` = 4,271,040 + 1,482 = 4,272,522 ms (1.187 h).
- Minimum start times relative to the first restart join: 0, ≥1.19 h, ≥3.32 h, ≥7.59 h, ≥16.12 h, ≥33.19 h, then +24 h each.
- Any five consecutive starts fit inside 16.2 h, but any six span at least 33.19 h. So at most 5 × 1.4828 = **7.41 s per 24 h**, under the 8.7 s limit with 1.29 s to spare.
- The cold-boot DR5 join happens near T0, which is before T0+11 h, so it cannot enter any later window (Table 17, txt line 3044).
- Robustness check with `TTN_AIRTIME_MS_PER_HOUR=0`: the hold is about 150 s, so the retry dominates. Starts are 0, 1.07, 3.2, 7.47, 16.0, 33.07 h, which is still at most 5 per 24 h.
- Minimality check: a base of 1920000 would allow 6 joins in 24 h (8.90 s), and the regression would catch that.

### 2. Remaining defects and test gaps (none blocking)

- **The documentation slightly overstates "normal joined traffic unchanged."** A session created by a restart join keeps the join DR. `setDatarate` → `createSession` (`LoRaWAN.cpp:3159-3162`, `:420-431`) sets the uplink DR, and `activateOTAA` (`:943-1038`) does not change it. So after an in-run restart, uplinks now start at SF12 instead of whatever higher DR the old DR5→DR0 progression got accepted at. A full fragment is about 2.8 s of airtime, giving roughly 2.2 h pacing until the network sends LinkADRReq. Cold-boot sessions are unaffected. `docs/airtime.md:199,219` and `join-restart-fix.md:21-23` mention only "SF12 joins"/slower recovery, not the slower uplinks in the new session.
- **Not covered: repeated succeed-then-revert cycles.** The regression only tests one failed-rejoin series. A success sets `joinRetry` back to 3840000. The next revert needs 64 uplinks without RekeyConf, which is at least about 67 h at DR0, so this is harmless with a compliant 1.1 network server. Two cases fall outside that assumption:
  - A network that raises the DR to DR5 via LinkADRReq and never sends RekeyConf, combined with frequent demo uplinks, gives cycles of about 4 h. That allows 6–7 DR0 joins in 24 h, about 8.9–10.4 s.
  - A network that shortens ADR_ACK_LIMIT via ADRParamSetupReq while still withholding RekeyConf would compress the cycles further.

  Both require a network server that breaks §5.10, and the old code was worse in the same case. So this is not a counterexample to the fix, just a boundary that is not documented or tested.
- **Minor: the cold-boot 60000 is retyped, not extracted.** `tests/radio.cpp:584,592,634,659` hard-code it. `tools/test_radio.py:30-31` stops the extracted init block at `Batch batch;`, so a change to `Workers.cpp:156` would go unnoticed.
- **Minor provenance issue:** the PASS line in `after-radio.log` lacks "boot/restart", while current `tests/radio.cpp:1182` and `tests.log:38` have it. That log was probably produced by an earlier revision of the test. `tests.log` matches the current source.

### 3. Cold boot, normal traffic, trade-off documentation

- **Cold boot is unchanged:** line `:156` still initialises 60000, the failure branch is unchanged, and DR5→DR0 plus the original 60-day windows are still asserted (`tests/radio.cpp:927`, `:627-648`).
- **The service block is unchanged:** `joinRetry` is only read in the join block. The single deadline, jitter, nonce reservation and fail-stop are unchanged.
- **The regression tests the real production path.** It uses the verbatim join and service blocks (`test_radio.py:27-35`) and real patched RadioLib. The revert is triggered naturally by RekeyInd going unanswered, with no forced counter. Airtimes come independently from `tools/airtime.py`.
- The sliding-window checks include every frame-start/end alignment plus the explicit 11 h boundary, so they cover the true extrema. The run lasts 60 days and crosses the `millis()` wraparound (`:733`).
- One self-referential spot: the `base` check at `:706` uses the production `previousRetry`. The budget assertions make up for that.
- **Trade-off documentation:** accurate for DR0 joins and the 64 min / 24 h progression, but it leaves out the DR0 starting rate of the post-restart session (see §2).

### 4. What I inspected, and limits

I read:
- `AGENTS.md`, `Workers.cpp:1-236`, `Telemetry.h`
- `tests/radio.cpp` (PHY/ProbeNode, the extracted-block seams, `joinRetryTrace`, `testJoinRetryWindows`)
- `tools/test_radio.py`, `tools/airtime.py`
- `docs/review/join-restart-fix.md` and the session-restart section of `docs/airtime.md`
- `before-radio.log`, `after-radio.log`, `tests.log`, `build-results.json`, `accounting.json`
- In RadioLib 7.8.1 + patch: `sendReceive` prologue, `clearSession`, `createSession`, `activateOTAA`, `setDatarate`
- LoRaWAN 1.1 Table 17 and §5.10 RekeyInd text

I ran nothing. I didn't read `audit.json` or `spec-source.json`, and I didn't verify the patch hashes. The saved logs are the developer's claims. Host `RadioLibTime_t` is 64-bit, and RF timing, entropy and interoperability were not measured. This is not a system-wide verdict.
