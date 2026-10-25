# Deep-review implementation dispositions — 2026-10-25

Canonical checkout: `/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry`.
Branch `main`; baseline/unchanged HEAD `6fada32040bbcb982c99dd59e9b4f57c1b2eb7c3`.
Local implementation only: no commit, push, PR, installation, flashing, RF or TTN
access. Raw review/request/prompt files remain historical and unchanged.

A = proven source failure; B = confirmed mechanism with uncertain device outcome;
C = physical prerequisite. Dispositions below distinguish software corrections
from retained limits. No test here is ESP32 emulation or physical operation.
Dependency line numbers refer to exact versions in `../sources.md` and
`../correctness-source-hashes.json`; RadioLib references use the final patched7.8.1.

## F1 — A, FIXED-SOURCE; confirmed and expanded beyond the proposed rollback

Invariant: unauthenticated JoinAccept content must not commit authenticated state.
RadioLib owns protocol state; PersistentNode owns durable DevNonce reservation.
`LoRaWAN.cpp:745 processJoinAccept` checks length/type at752/773, decrypts using
existing revision at780, checks anti-replay at793, derives a LOCAL integrity key
at814 and verifies MIC at829/836. Only after verification does it copy JSIntKey
at832 and commit JoinNonce/revision/DevAddr at842–844. RX1 offset/RX2 DR and
RX delays flow through execMacCommand at852/857, CFList at861, session keys at875
onwards, nonce buffer at917 and session buffer at920 onwards, all after MIC.
HomeNetId/DLSettings were already locals. No other unauthenticated protocol-member
commit was found. The AES engine's scratch key context is cryptographic workspace,
not accepted protocol state.

Caller `activateOTAA:937` creates default session state before RF, reserves/increments
DevNonce and signs its buffer at1000–1005, establishes join RX delays, and calls
processJoinAccept at1020; active status only follows successful processing at1027.
Those changes are join-attempt state, not acceptance of an unauthenticated reply.
`clearSession:354` clears session/counters/channels but retains revision/JoinNonce;
`getBufferNonces:303` signs current buffer; `setBufferNonces:317` restores validated
nonce fields including revision. `beginOTAA:664` runs once at boot and initializes
root-key/revision context; `composeJoinRequest:725` chooses NwkKey for revision1,
AppKey for revision0. Thus a pre-MIC revision change could poison the next request.

Wrong length/type return malformed before acceptance. Corrupt MIC/foreign encrypted
content leave authenticated fields intact; a genuine subsequent accept succeeds.
Exact replay is rejected by the retained JoinNonce check. A genuine1.0 accept can
authenticate, but the application join block (`Workers.cpp:155`) fails closed,
clears the session and stops further RF. Reboot restores accepted JoinNonce and
reserved next DevNonce; authenticated anti-replay state is never deliberately reset.

Approved authority-level patch:11 dependency lines added /7 removed. No application
snapshot or second state authority was introduced. Snapshot/restore of two members
would miss DevAddr/JSIntKey. Reapplying NVS nonce bytes does not restore those
members or session keys/channels; it is not a complete transactional boundary and
must not lower accepted nonce state. Neither alternative was adopted.

Real-library tests cover17/33-byte corrupt/foreign/OptNeg0 MIC failures, wrong
length/type, subsequent genuine1.1 accept, request MIC checked against a DISTINCT
NwkKey, strictly increasing DevNonce, no unauthenticated persisted JoinNonce,
replay rejection and reboot. Production init/join extraction tests1.0 rejection
and NVS failures. Pre-fix reproducer failed; fixed tests pass. Patch fresh apply,
idempotence and tamper refusal pass; patcher still uses before/after hashes.
Hardware/private-network injection remains unexecuted.

## F3 — A, FIXED-SOURCE; confirmed

Invariant: every published current/power value requires successful calibration.
Application/UI owns I2C and the measurement validity decision. INA2260.6.6
`setMaxCurrentShunt:210` ignores the calibration `_writeRegister:312` result and
returns success at335; it makes no later bus transaction. `_readRegister:511` and
`_writeRegister:539` clear `_error`; `getLastError:497` returns and clears it.
Therefore the immediate error check is the existing authoritative API, before
conversion/read traffic can erase the failure.

`Application.cpp:75` now combines connection, calibration result and immediate
getLastError in `model.sensor`, then derives `model.calibrated`. Calibration NACK
uses existing sensor-error presentation and leaves the Reading invalid. No new
state, fake zero, wire-format or I2C owner. Approved net+2 application lines.
The real INA226 transport stub knows only registers/NACK/data. Verbatim acquisition
tests NACK on register5 with later reads succeeding, successful true0A/0W and
shunt0 voltage-only behavior. Encoder/decoder/reference remain unchanged: version1
still sends FFFFFFFF if any triplet value is invalid. Pre-fix test failed; final
passes. Bench shunt/range/calibration tests remain unexecuted.

## F2 — A, FIXED-SOURCE; counter remedy disproved, existing event boundary used

Invariant: batch offset follows proven local outer TX completion, never counter
consumption. RadioLib owns TX outcome; the worker owns batch/demo scheduling.
`sendReceive:202–211` increments FCntUp even after transmitUplink errors, then
returns before event population. Stage/config failure is no launch; attempted
launch or TX timeout is ambiguous. `transmitUplink:1536–1559` waits for TX-done
IRQ and checks finishTransmit. Only success reaches `sendReceive:248–258`, which
populates the caller's zero-initialized upEvent including its application fPort.
RX failures exit the repetition loop but still reach event population; parsing
occurs afterward. If an earlier repetition completed and a later TX fails before
event population, the fragment is conservatively retained: the public event does
not expose that earlier completion. Duplicate sample indices remain acceptable. `parseDownlink:1859` validates DevAddr,
counters/MIC; its nested sendReceive at2287 does not overwrite the outer event.
The nested return code is discarded upstream, so that failure cannot be diagnosed
from the outer rc. No new counter, flag, callback or patch was needed.

`Workers.cpp:207` status/offset now use existing upEvent.fPort!=0. Application ports
are1/2; neither is0. No launch and ambiguous TX retain the fragment; successful
outer TX with clean/no RX, RX/parse failure or nested failure advances it. Status
says sent; RX error when that rc is available, otherwise not confirmed. Negative-rc
backoff and aggregate accounting are unchanged. Gateway receipt remains unknown.

Verbatim service tests stage failure, launch failure, foreign DevAddr and bad MIC,
next fragment7+3 ordering/status, and nested reply launch failure. Separate real
TX-timeout IRQ-spin test verifies event stays unpopulated on timeout. Existing
counter/nonce non-reuse tests remain. Pre-fix offset test failed; final passes.
Hardware fault injection only under separately authorized private-network testing.

## F4 — B, RETAINED-DOCUMENTED + HARDWARE-ONLY; false exclusion claim corrected

Invariant: shared controller init/clock/deinit must not race Hosted transactions.
App media owns slot0/POSIX and Hosted owns slot1, but neither owns a shared lifecycle
lock. IDF5.5.5 `sdmmc_host.c:496` returns ESP_OK when already initialized, without
adding a host-init reference. Slot count increases after successful slot init;
`:823` calls set_card_clk(slot,400), whose `:301–350` updates controller clock/divider
and timeout state. `deinit_slot:906` decrements slot count and only tears down the
controller when no initialized slots remain. Installed `driver/sdmmc_host.h:172–175`
forbids init/deinit/clock reconfiguration concurrency with transactions.
`sdmmc_transaction.c:102` takes s_request_mutex around transactions only, not the
init/clock/deinit functions. FAT `call_host_deinit:418` uses flags to choose
slot deinit_p(slot) versus global deinit(). Existing SdStorage default flags and
slot-count cleanup guard preserve slot0-only app teardown, fixed VO4 and I2C45/46.
The unmount-error latch and truthful Retry SD remain.

Hosted2.12.13 `port_esp_hosted_host_sdio.c:317 hosted_sdio_init` calls host init
at362 and slot init at381. `hosted_sdio_deinit:294` uses slot-specific deinit at308
on this IDF, NOT its older-SDK global-deinit fallback. `sdio_drv.c:155` private
sdio_bus_lock and its transport read/write tasks do not include app mounts;
transport cleanup at398–399 reaches card and bus deinit. GPIO reset/reinitialization
at1649 onwards and transport failure/restart paths at1206–1231 do not create an
app-wide exclusion interface. The exact-tag `host_sdcard_with_hosted` example
orders init_wifi at39 before mount at43; its host-init workaround in
sd_card_functions.c:172–178 is IDF6-only. Neither is proof of5.5.5 transaction
quiescence. No sufficient shared public exclusion API was established.

AGENTS/README/current status now say init CALLS are ordered only. Software race
mechanism is confirmed; device failure frequency/outcome is unmeasured. Retain
runtime Retry SD and current ownership rather than silently reduce functionality
or add an unproven app mutex, transport lifecycle layer or SDK patch. Any such
future correction needs a concrete gated design. SD host stub verifies lifecycle
only, NOT concurrency/Hosted recovery/forceful deinit. At least50 remounts with
continuous WiFi plus boots/failures/recovery/error counts are hardware acceptance.
This finding is NOT fixed in software and prevents an unattended-operation claim.

## F9 — A, FIXED-SOURCE; confirmed

Invariant: failure diagnostics describe the failed boundary and no initialization
failure reaches RF. Radio worker owns NVS/SPI/node. `Workers.cpp:119–133` checks
NVS open before radio begin, carries the actual radio.begin or DIO2 configuration
rc, and posts the distinct reason through the existing Radio event. The eventual
stop message is deliberately generic; serial retains the specific queued reason.
Invalid nonce/config message and boot-ID failure stop behavior remain truthful.
Approved net+2 lines, no extra flag/cache. Verbatim init/join tests cover NVS open,
radio begin, DIO2 setup, malformed nonce buffer, boot-ID write, nonce-write fail
and1.0 stop. No RF launched in those failure cases. Physical diagnostics untested.

## F10 — A, FIXED-SOURCE; confirmed

Invariant: WiFi input rejection remains visible and validation has one authority.
UI owns credential edits; worker owns network status. Core3.3.12 `STA.cpp:391–407`
accepts up to64 password bytes; App.h has65-byte password storage. Approved
`Ui.cpp:60 wifiInputError` is the sole predicate for both Connect at227 and both
existing WiFi status draw paths at82/138. Empty SSID,1–7 characters and invalid
64-byte values show the reason; allow empty password,8–63 characters or64 hex
characters. The reason derives directly from credentials, so two-second worker
status updates cannot overwrite it. No new model field or layout/style change.
Approved net+8 production lines/new helper. Extracted actual predicate and BOTH
render expressions pass short/open/normal/64hex/nonhex and periodic-update cases.
Hardware touch/connection testing remains unexecuted.

## F8 — A, FIXED-SOURCE; confirmed

Invariant: recent GPS data means checksum-valid NMEA, not UART noise. TinyGPSPlus
v1.0.3a `encode:56–77` returns true on checksum-valid completed sentence;
`endOfTermHandler:159–198` also accepts valid no-fix sentences. Existing timestamp
at `Application.cpp:56–69` now updates on that return, and uses passedChecksum
rather than raw charsProcessed. Location/satellite validity and age remain owned
by TinyGPSPlus; no second parser/cache/state. Net−1 line. Real parser plus verbatim
production block passes valid fix/no-fix, garbage/wrong-baud bytes, silence,
satellite expiry and unsigned32 application expiry across wrap. No distinction
between silence and invalid bytes was added. Hardware UART/fix testing unexecuted.

## F13 — A, FIXED-SOURCE; confirmed, documentation also corrected

Invariant: accepted image formats match the resource contract; JPEGDEC owns type
recognition. JPEGDEC1.8.4 `getJPEGType:149` exposes baseline/progressive;
`jpeg.inl:4961–4980` supports DC-only progressive handling with scaling branches,
so openRAM/decode success alone did not reject progressive files. Existing
`Workers.cpp:320` admission guard now requires JPEG_MODE_BASELINE, before pixel
allocation/decode. Existing close path remains used. Net0; no second parser.
Real decoder plus extracted production guard accepts/decodes a baseline fixture
and rejects a progressive fixture. Pre-fix test failed; fixed passes. Fixtures are
small generated test inputs, unrelated to the preserved panel photo.
README's false NVS RF-debt phrase removed; actual nonces+boot keys and volatile
aggregate pacing remain. Hardware progressive/baseline browsing is unexecuted.

## F6 — B, RETAINED-DOCUMENTED + HARDWARE-ONLY; consequence unmeasured

Invariant: RF work must not starve UI/I2C/GPS, and RX windows must remain timely.
`Workers.cpp:448` radio is unpinned priority2; core3.3.12 `main.cpp:113` creates
loopTask priority1 on ARDUINO_RUNNING_CORE1. ArduinoHal yield delegates to the
FreeRTOS yield; RadioLib TX/RX busy loops at1538/1644/1665/1672 remain runnable.
Both SDK variants have two cores,1ms ticks and time slicing; Hosted/WiFi/audio also
compete. Yield does not guarantee a lower-priority UI task will run on that core.
No measured latency is claimed. Changing affinity/priority cannot be proved from
these sources to preserve Class-A timing or avoid Hosted contention, so no
unmeasured scheduling policy change was adopted. Bench SF12 UI/touch/GPS latency
with simultaneous WiFi/audio is required. No new HAL/timer/task introduced.

## F7 — A mechanism, RETAINED-DOCUMENTED + HARDWARE-ONLY; intentional policy

Invariant: touch failure is visible to the UI/I2C owner; no silent freeze.
`Ui.cpp:250–278 pollTouch` checks status, coordinate read and clear-status write;
any failure latches touchAddress0 and displays lost/restart. This is intentional,
not an approved automatic-recovery policy. Setup calls updateUi(model,true) after
beginUi and before any loop poll, setting latest at97; pollTouch is called only
from that initialized application path. No reachable null latest dereference was
found. No re-probe timer/state machine added. Runtime I2C fault/restart test is
hardware-only; host UI state-machine coverage remains a gap.

## F11 — B, RETAINED-DOCUMENTED; conservative over-deferral confirmed

Invariant: aggregate accounting must never lose a launched frame. RadioLib owns
lastToA; app has one volatile deadline. `sendReceive:98–135` can return before
`:173` reset (null pointers, not joined/rekey, duty gate, invalid uplink). The
recursive path at2286–2288 preserves old outer airtime, calls sendReceive and adds
outer again. An early nested return can therefore double the old total; an outer
early return can reuse stale prior airtime. None of those returns launches a frame
that disappears from the total. Existing launch accounting at1526 includes an
ambiguous attempted launch; successful repetition/nested frames accumulate.
No under-counting path was found in this traced flow. Retain conservative
extra delay rather than expand patch scope. Documentation is precise; no test
asserts a known defective over-count as correct. Actual frequency is unmeasured.

## F12 — A, RETAINED-DOCUMENTED + HARDWARE-ONLY; confirmed

Invariant: a documented failure mode must match driver behavior. Arduino_GFX1.6.9
`Arduino_ESP32DSIPanel.cpp:36,51,60,90,95,100,108` uses ESP_ERROR_CHECK. Both
installed SDK variants enable assertions and PRINT_REBOOT with0s delay. The
matching IDF esp_err.h expands failed checks to abort, not a false return.
`Ui.cpp:286` calls display.begin; `Application.cpp:47` runs beginUi before workers
at48. Codec setup has already run with temporary MCLK, clock.end has stopped it,
and amplifier GPIO30 is driven LOW; physical LOW polarity/pop behavior is unknown.
No radio/WiFi/media/audio worker has started. Failure can panic/reboot-loop with
serial-only diagnosis, not a graceful UI fallback. Driver replacement/patch is
an architectural extension outside this approved scope, so not silently added.

## F5 — C, HARDWARE-ONLY; units and profile claim qualified

Invariant: actual radiated power must meet the applicable profile and local band
limits. RadioLib7.8.1 `LoRaWANBands.cpp:26` powerMax16 and
`LoRaWAN.cpp:3386–3387 setPhyProperties` pass max−2·steps as PHY conducted dBm;
`Workers.cpp:124` begins at14dBm, which is not a cap. RP002-1.0.4 section2.4.3
sets default16dBm EIRP. ETSI EN300220-2 V3.3.1 Table4 bandM gives25mW ERP for
868.0–868.6MHz, not25mW EIRP. ERP = EIRP−2.15dB;25mW ERP≈16.13dBm EIRP.
Primary links and CEPT edition/download limits are in sources.md. No antenna gain,
country authorization or power compliance is assumed. Conducted-power and
antenna/cable measurements require separately authorized RF tests. No default,
TCXO voltage, ADR policy or radio subclass was changed.

## F14 — B, HARDWARE-ONLY; static evidence obtained, reviewer claim qualified

Invariant: worker stacks must cover actual call chains. Final target enabled-prev3
build with -fstack-usage reports radioTask3616, applicationDownlink1120, post1088,
sendReceive80 and parseDownlink384 bytes. These individual frames do not prove a
maximum: recursion, SDK calls, formatting and interrupts remain. The outer
applicationDownlink callback atLoRaWAN.cpp:2150 RETURNS before recursive
sendReceive at2287; two nested application callbacks/post buffers are not live
simultaneously as the review implied, although protocol recursion frames are.
Avoiding the duplicate text/Event buffer would change the existing post boundary;
no such mechanism was added. Bench high-water marks remain required.

Additional proven defect S1 (A, FIXED-SOURCE): the same target analysis found
loadImage19120 bytes versus the media task's12288-byte allocation. User explicitly
approved increasing only that task's stack to24576 atWorkers.cpp:246 (+1/−1
physical lines, net0;12KiB more runtime allocation). JPEGDEC remains worker-local,
with its original per-call lifetime; no static decoder/cache introduced. All three
final firmware configurations and the stack build pass after this change.
Whole-call-chain margin and media/radio watermarks still need the bench checklist.

## Test gaps, approvals and verdict

- Actual production init, join, service and node blocks are extracted by markers;
  tests fail if markers/hash checks do not match. No retyped scheduling model.
- DR-dependent join test uses generated tools/airtime.py reference values,
  DR5→DR0 and per-attempt accounting, plus nonce/NVS/version fail-stop.
- Actual RadioLib TX IRQ-timeout and RX initial elapsed-wait loops execute at
  ordinary host times. Host unsigned long is64-bit; -m32 probe failed due missing
  Scrt1.o/crti.o/libstdc++. No install attempted. Wrapped uint32 app deadlines and
  wrapped HAL inputs are covered; target-width elapsed arithmetic/IRQ timing is
  NOT established. Remaining RX ongoing/final300ms loops need integration testing.
- SD stub does not model SDK/Hosted concurrency, forced deinit or resets. UI state
  machine, JPEG/WAV fuzzing, physical stack depth and runtime performance remain
  untested. No physical baseline existed; no latency/performance improvement claim.
- Approvals used: path-only pre-baseline changes; F1 patch11+/7−; F3 app4+/2−;
  F9 app8+/6−; F10 app11+/3− and helper; S1 media stack numeric argument only.
  Approved additions retain one owner each. No other gate proposal is pending.
- No versions changed, no reservation hooks/marker/persisted debt restored.
- Assembly: conditional on the electrical/connector/shunt/TCXO checks. Supervised
  bench flashing: conditional on those prerequisites and separate authorization.
  Unattended TTN: NO-GO until F4/F5 tests, actual stack/latency, full hardware
  checklist and24h run pass. F1–F3 software fixes passing is not whole-system
  acceptance. No display/touch/audio/WiFi/SD/GPS/sensor/RF operation claimed.
