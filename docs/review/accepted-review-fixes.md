# Accepted independent-review corrections — 2026-10-25

Canonical checkout: `/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry`.
Baseline/current HEAD: `1961cfa053d8ee950e1ba252cf9721e2f7833e6d`, branch main.
The working tree was clean before implementation; cached origin/main matched HEAD,
and origin/HEAD identified main. No fetch/pull, branch change, commit, push, global
installation, flash, RF, TTN access, contact or archive modification was performed.
This is implementation after the user's independent-review dispositions, not a
new Claude assessment. Historical review files remain unchanged.

A = source-proven failure; B = confirmed mechanism with device effect unmeasured;
C = physical prerequisite. Source paths below refer to the exact pinned dependency
versions; line numbers for RadioLib refer to the final patched source. Original
and patched hashes are in patches/. New primary-source hashes are in
../correctness-source-hashes.json. All six library versions are unchanged.

## Scope, invariant and approvals

The user selected amplifier control, confirmed-frame ACK selection, zero-counter
non-processing, WiFi association/storage and manufacturer display timing for fixes.
ES8311 was “fix or ignore”; it is retained and documented. SD/Hosted coexistence
and unbounded nested MAC recursion were explicitly excluded. JPEG scaling, GPS
checksum parsing and demo retry policy remain outside this correction scope.

Two concrete gates were approved before implementation:

1. RadioLib: one byte containing two accepted-downlink bits, reset/serialized at
   the existing protocol/session authority; reject old ambiguous session snapshots.
   Estimate22 added/14 removed dependency lines; actual17 added/12 removed,
   including ACK selection. Snapshot compatibility changes are approved. No second
   counter, callback, persistence key or application-side duplicate filter.
2. WiFi: accepted STA request plus live association/IP and requested SSID before
   saving; replace two separate strings with one credential blob using the existing
   pending flag/timers. Estimate12 added/15 removed application lines. Actual WiFi
   change13 added/14 removed; all application changes18 added/19 removed.
   One-time re-entry of existing saved WiFi settings was explicitly approved.

These estimates were approval bounds for the described architecture, not permission
for unrelated mechanisms. No further addition or policy extension was introduced.
The existing worker boundaries absorb all application changes. No approval pending.

## Amplifier control and startup (R1)

**A + C; FIXED-SOURCE + HARDWARE-ONLY.** The output authority is GPIO30: HIGH must
mean shutdown throughout codec initialization, idle and errors; LOW must span
playback and its DMA tail. No second amplifier abstraction is needed.

NS4263B manufacturer-authored Jun.2023 V1.0 datasheet sections5/10.1 specifies
HIGH >100µs shutdown, initial LOW/falling edge AB mode. The pinned V1.2 panel
schematic AUDIO_OUT_SD connects GPIO30 through fitted-in-schematic R1470Ω to U4.1,
with R71 pull-up; the Q10 alternative is NC. Fitted population remains a C item.

Application.cpp:20–25 moves the existing GPIO initialization ahead of codec/I2C
setup and selects HIGH. Workers.cpp:414–438 selects LOW during writes/drain and
HIGH afterward, including a failed write. Setup failure leaves the amplifier shut
down. No ownership/lifetime change or net production growth. The GPIO handoff is
setup first, then exclusive audio-worker use.

`tests/audio.cpp` exercises verbatim setup lines and the production service body:
startup/idle state, successful playback/drain, zero-byte write failure and failed
codec/I2S initialization. It is a GPIO/I2S transport double, not a real amplifier
or ESP32. Scope reset-to-setup, fitted polarity and speaker output on the bench.

## Confirmed-frame ACK counter (R2)

**A; FIXED-SOURCE.** ACK MIC input must belong to the authenticated confirmed frame,
regardless of application/network counter domain. RadioLib owns those counters;
app-side substitution would duplicate authority.

RadioLib7.8.1 `parseDownlink`, LoRaWAN.cpp:2090–2107, commits the authenticated
counter and now selects `(uint16_t)devFCnt32` for `confFCntDown`. Previously every
confirmed frame selected `aFCntDown`. `composeUplink` at:1416 sets ACK when present;
`micUplink` at:1461–1462 inserts its low16 bits into B1, as LoRaWAN1.1 section4.4.2
requires. The low16 assignment also keeps valid FCnt0xFFFFFFFF distinct from the
32-bit FCNT_NONE sentinel. Session export/restore preserves that value; clearing
and post-TX consumption remain the existing authority and lifetime.

Real-library tests receive app3 then confirmed app/network7 and validate that the
next uplink's ACK MIC binds7 rather than3. Counter-boundary tests cover confirmed
0xFFFF and0xFFFFFFFF. Before the selection fix the app3/network7 test failed;
before low16 handling the maximum-counter ACK test failed. Both pass afterward.
Crypto fixtures use RadioLib primitives; this is not independent crypto
certification or measured TTN interoperability.

## Initial versus accepted-zero downlinks (R4/N4)

**A; FIXED-SOURCE.** An authenticated counter-zero frame may be processed once per
unicast counter domain/session. Repeated application delivery and MAC execution
must stop inside the protocol parser, before reaching consumers.

RadioLib7.8.1 LoRaWAN.h:1043 adds `downlinkSeen`, bits0/1 for App/Nwk. `clearSession`
LoRaWAN.cpp:363 resets it. `getBufferSession`:528 serializes it in unused status
bits with a format marker. `setBufferSession`:537–550 rejects pre-marker snapshots
before consuming them and:616–617 restores status/metadata. The session-buffer
size and nonce-buffer format are unchanged. The application deliberately restores
nonces only and performs fresh OTAA after reboot; no authenticated nonce is reset.

`parseDownlink`:2011–2039 distinguishes accepted zero during candidate counter
reconstruction. A64-bit local candidate rejects32-bit exhaustion before truncation.
The accepted bits change only at:2101, after MIC validation:2081. Thus a failed MIC
cannot consume initial-zero acceptance, and replayed frames cannot execute MAC or
application callbacks again. Multicast initial-zero behavior, ClassB/C and relay
are not claimed fixed or supported by this regression scope.

The real RadioLib test first failed on duplicate zero with the previous parser.
Passing tests cover both domains, corrupt-zero MIC then valid zero, no repeated
application callback, fresh session export/restore with independent App/Nwk zero
acceptance, accepted-zero restoration, legacy snapshot rejection, 16-bit rollover,
32-bit exhaustion, and a new genuine OTAA session accepting zero again. Existing
join authentication/nonces, nested data, fragments and pacing regressions pass.

## WiFi association and credential persistence (R6/N7)

**A/B; FIXED-SOURCE with hardware follow-up.** One WiFi worker owns the request,
connection decision and persisted credential record. Saving must correspond to
the accepted request, never a delayed status from the preceding association.
One write must contain the whole SSID/password pair, including an empty password.

Core3.3.12 `STAClass::connect`, STA.cpp:373–434, waits for an existing connection's
`disconnect(true,1000)` and returns false on failure before arming the request.
`disconnect`:537 onward waits on live status bits. `_onStaEvent`:228 clears link/IP
bits before asynchronous Arduino event delivery. `NetworkInterface::connected` /
`hasIP`, NetworkInterface.cpp:316–324, read those raw bits. The legacy
`WiFi.status()` path reads a separately updated Arduino status instead.

Workers.cpp:257–274 reuses `saveOnConnect`, `connectAt` and `reportAt`: accepted
`WiFi.STA.connect` arms saving; raw association+IP and matching `WiFi.SSID()` permit
it. A rejected request reports its failure for the existing report interval and
cannot save. Startup loading:239–245 requires the exact blob size and terminated
fields. The existing NVS-open result gates saving and failures are visibly reported.
No parallel pending flag, status cache or event callback was introduced.

`Preferences::putBytes`, core3.3.12 Preferences.cpp:285–299, uses one nvs_set_blob /
commit and returns the nonzero record length only on success. This removes the
empty-string ambiguity and separate SSID/password commits. Old keys are ignored;
no fallback/migration copies them. WiFi credentials must be re-entered once.
`WiFiGeneric.cpp:274–277,299–300` disables driver NVS and forces RAM storage for
Hosted builds already; no redundant persistence switch was added. Saved credentials
are not encrypted by this template. LoRa credentials/nonces are untouched.

`tests/wifi-worker.cpp` exercises extracted WifiCommand, load and service blocks
at the STA/Preferences boundary: refused request with old live link, accepted
request awaiting link/IP, wrong SSID, rapid same-SSID reconnect, open/protected
records, malformed loads, NVS-open/write failures, no repeated save and offline/
timeout feedback. These doubles model the source API contract, not the actual
asynchronous scheduler or NVS flash recovery. Physical event timing and power-loss
atomic recovery require board tests. No claim that a failed commit necessarily
left the old record unchanged on real flash; single-record storage prevents a
mixed SSID/password pair, while power-loss behavior remains untested.

## Manufacturer display timing (R8)

**B; FIXED-SOURCE + HARDWARE-ONLY.** The display bus owns scan timing; UI geometry
must remain unchanged. Elecrow's pinned cfcbfdbb V1.2 Lesson07 board_config.h:30–38
specifies51MHz, H70/160/160,V10/23/21. Arduino_GFX1.6.9
Arduino_ESP32DSIPanel.cpp:7–12,65–78 maps constructor values directly to DPI timing.
Ui.cpp:7 now uses those values; EK79007,1024×600, two1000-Mbps lanes and accepted
UI visuals are unchanged. A source comparison passes. Compilation does not
establish physical display stability, touch orientation or fitted panel identity.

## Retained/excluded findings

- **R3/F4/N2, B: RETAINED-DOCUMENTED + HARDWARE-ONLY.** WiFi itself never mounts
  SD, but media slot0 and Hosted slot1 share the controller. IDF5.5.5
  sdmmc_host_set_card_clk / init_slot change controller state outside the transaction
  lock; the API contract excludes concurrent lifecycle/clock changes. Hosted2.12.13
  can call esp_restart on transport failure; both installed P4 SDKs enable it.
  No mount sequencing, retry reduction or synchronization patch was authorized here.
- **R5, A: RETAINED-DOCUMENTED.** audio-driver0.3.1 ES8311.h init:313–504, volume:
  553–570 and readReg:637–641 lose selected transport errors. The previous real-
  driver reproducer confirms selective-write false success. Under “fix or ignore”,
  retain it honestly; audioReady is not register-programming proof. No dependency
  patch, readback cache or downstream masking was added.
- **R7/F14, A/B: RETAINED-DOCUMENTED + HARDWARE-ONLY.** RadioLib parseDownlink's
  recursive MAC reply path has no general depth bound. The prior31-frame local
  reproducer is not target stack proof. User explicitly excluded this fix. Existing
  aggregate airtime policy and unmeasured radio/media stacks remain as documented.
- Other review observations were not selected: JPEG scaling boundaries, TinyGPSPlus
  checksum-helper permissiveness and demo single-attempt semantics remain unchanged.

See ../validation.md for final commands, build sizes, analysis limits and asset
checks; ../change-accounting.json for separate physical-line counts. Hardware
checklist additions are unexecuted. Assembly and supervised flashing remain
conditional on physical prerequisites, with flashing/RF separately authorized.
Unattended TTN remains NO-GO pending accepted limits and full hardware acceptance.
