# Correctness fixes — 2026-10-25

Software corrections are implemented in the canonical standalone project, with
three final-source builds and the recorded host/integration checks passing.
Target clang-tidy remains unavailable with the installed generic Clang toolchain;
its failed attempt and supplemental host warnings are preserved. Hardware
acceptance is unexecuted. This is not a claim of successful panel/RF operation.

## Authority, approval and ownership

The user approved the SD replacement (estimated +40–80 production lines) and then
the remaining scoped fixes (estimated +40–100 production lines plus tooling).
No unapproved feature, pin reassignment, visual redesign, source-tree checkout,
SDK/global library edit, installation, upload, RF, TTN access, commit or publication
was performed during the repair stage. Subsequent repository creation/push was
explicitly authorized by the user; see validation.md. No deployment is implied.

The media worker owns one IDF/POSIX mount, slot 0 and a fixed shared3.3V rail
reference. The rail is retained for that worker's lifetime, including failed mounts.
No SD GPIO can reconfigure45/46. The default DEINIT_ARG flag stays intact; a cleanup
callback checks the SDK's slot count against the pre-mount snapshot before calling
slot-0 deinit. With quiescent slot lifecycles this avoids IDF's uncounted-slot cleanup on early init failure.
An early failed initialization can leave the single controller initialized for
reuse. This guard has not established safety against concurrent Hosted recovery;
slot-count changes can race it. Application init calls are ordered
after WiFi.mode; Hosted traffic and SD mount/clock/deinit are not serialized; failed C6 startup requires restart rather than implicit reinit.
An ambiguous unmount error latches storage unavailable until restart, preventing
reuse/double cleanup. This is tested at the API boundary; actual SD/C6 operation
and abnormal firmware resets still need physical tests.

The user subsequently rejected per-physical-transmission airtime reservations,
while accepting nested downlink delivery and timing/airtime fixes. The patch now
contains the downlink callback, those corrections and authenticated JoinAccept
commit ordering; project marker macros and
reservation hooks are removed. PersistentNode owns NVS nonces and one volatile RF
deadline. Completed operations extend it from total airtime; internal MAC replies
proceed normally. Reset clears the deadline. Version1.0 sessions remain rejected.
DevNonce is reserved before joining and cannot move backwards after an ambiguous
TX error. See airtime.md for the reduced enforcement scope and reset limitations.

## Findings: classification, correction and evidence

A = source-proven defect/limitation. B = proven mechanism with outcome/frequency
requiring physical or integration evidence. C = physical prerequisite. The original
review is in the sibling CrowPanelTelemetry-review folder. Original dependency
source hashes remain in correctness-source-hashes.json; exact patched source hashes
and the complete diff are in patches/. References below name the authoritative
function and current implementation/test; see source URLs in sources.md.

| ID | Class / source trace | Correction and verification |
|---|---|---|
| C1 | A. Core pins_arduino.h:73 assigns GPIO45 to SD power; SD_MMC.cpp:284–292 drives it; GPIO/I2C periman detach removes Wire. | Removed SD_MMC and the unused audio board catalog. SdStorage.h uses fixed slot-0 IO_MUX; SDK-pin static_assert, lifecycle tests, three compiles and linked-symbol audit pass. No SDMMCFS begin/end in final ELF. Physical coexistence untested. |
| H1 | B originally: SD_MMC.cpp:228,258 drops DEINIT_ARG; IDF vfs_fat_sdmmc.c call_host_deinit:418 invokes wrong union signature. IDF also calls cleanup after an uncounted early slot-init failure. | SdStorage.h preserves flags and supplies counted-slot-only cleanup. Tests model early-slot failure, counted no-card failure, insertion/removal, three retries and ambiguous unmount failure while C6 owns slot1. All pass. Actual C6 behavior remains hardware-only. |
| H2 | A. RadioLib sendReceive/activateOTAA/receiveClassA/transmitUplink/timeUntilUplink use raw timestamp comparisons. | Patch makes reachable Class-A comparisons wrap-safe and accepts millis==0 as a valid receive reference. Production node has one uint32 deadline; scheduleTransmission is set per application operation. Host tests cover wrapped uint32 application deadlines and HAL inputs, including TX ending at zero; target-width elapsed waits and device receive-window scheduling remain unverified. |
| M1 | A. activateOTAA calls transmitUplink, which adds lastToA; clearSession does not reset it. | Patch resets the airtime counter per join. Six failed real-library joins retain112ms each; the post-call volatile deadline matches each attempt. |
| M2 | B. parseDownlink recursively calls sendReceive, resetting lastToA. | Patch retains the outer counter. Six DevStatusReq commands force an immediate nested reply: two frames,224ms total; that total defers the next application operation, without an individual-frame reservation. |
| M3 | A. Old batch-first scheduling starved demoRequests. | Workers.cpp uses one demoTurn arbitration flag, giving demos an opportunity between batches and a batch after a demo. Actual production scheduling block is exercised with three backlogged batches, continuing demos and7+3 fragments. Neither stream starves; indices/order pass. |
| M4 | A missing validation; B particular physical clipping. INA226 getShuntVoltage returns signed raw*2.5uV; math overflow alone does not prove ADC validity. | Application.cpp reads/checks shunt voltage, flags, current and power; sensorRangeValid rejects both raw ADC endpoints and >= calibrated/configured current limits. Host boundary tests pass. Positive rail32767 is81.9175mV, not81.92mV; the implemented threshold handles it. Bench tolerance tests remain. |
| M5 | A. clearSession resets DR to unused; createSession defaults to midpoint DR3. | Explicit joinDatarate progresses DR5,4,3,2,1,0,0 from bounded retry backoff and resets backoff after join. Pure boundary tests plus real-library failed joins pass. |
| L1 | A, protocol limitation retained. Version1 requires a complete valid U/I/P triplet. | No wire-format extension. Uncalibrated voltage is shown locally; all ten transmitted slots are invalid triplets until the shunt is configured. README/wiring/airtime document this. Encoder/decoder reference unchanged and tested. |
| L2 | A, original arbitrary reservation. | Per-frame reservations and airtime NVS access removed at user request. Reset starts without airtime debt; legacy hold values are ignored. Repeated-reset allowance enforcement is no longer provided. |
| L3 | A. Lifetime charsProcessed and unchecked satellite value gave stale status. | Application.cpp records latest UART reception and expires data/fix/satellite age after10s. Same fields/geometry. Source reviewed; physical GPS/UI test unexecuted. |
| L4 | A. Modal was drawn only on opening; length check discarded empty application messages; nested wrapper discarded app data. | Open modal refreshes; patch forwards every authenticated application downlink before nested MAC handling, in reception order. Real-library outer/inner, empty and maximum-length binary downlink tests pass; physical touch/rendering remains untested. |
| L5 | A. Runtime GT911 errors were ignored. | pollTouch checks status, coordinates and clear transaction; failure returns to existing dashboard with lost/restart status. No silent freeze/retry loop. Source reviewed; physical I2C fault test unexecuted. |
| WiFi socket | A documentation error. Separate onboard IC1 provides WiFi. | Corrected wiring.md and authoritative drawing script; SVG/PNG regenerated. J7 stays selected; wire geometry unchanged. |
| N1 (additional) | A. Old post-join save could overwrite the durable next nonce when transmit returned an error before RadioLib incremented its RAM counter. | activateOTAA preserves max(current,reserved-next) before saving. Real-library ambiguous-launch and reboot tests verify non-reuse; failed nonce NVS writes cause no RF. Launch attempts count toward post-call airtime even on ambiguous errors. |

## Physical prerequisites and disposition

C: exact GT-U7 VCC/TXD/pads, INA226 fitted shunt/rating/straps, Waveshare R22/FB1
population, P4 silicon revision and C6 slave firmware remain unconfirmed. Do not
change TCXO configuration on a schematic assumption. Also unverified:10.1-inch
DSI operation (timing now matches Elecrow V1.2; still unmeasured), GT911
orientation, amplifier enable, codec retention across its clock gap, real supply
headroom, RF switch and frequency error. No software test substitutes for these.

Assembly is conditional on the physical checks. Software is ready for separately
authorized bench bring-up once those prerequisites are established; unattended
TTN operation is NO-GO until the hardware acceptance checklist passes. No flash
or RF permission is implied by this handover. No extra engineering extension is
pending approval, but target static analysis and physical acceptance remain open.
The current removal of airtime reservations and its push were explicitly requested
by the user; this supersedes the historical per-frame pacing requirements in the
unchanged review records.

Full commands, build sizes, test limits, LOC accounting, retained warnings and
cleanup are in validation.md. No claim of an entirely verified system is made.

## Deep review F1–F14

Current source traces, qualifications, invariants, approvals, tests and physical
follow-ups are in [deep-fix-dispositions.md](review/deep-fix-dispositions.md).
A=source-proven failure; B=mechanism confirmed/device consequence uncertain;
C=physical prerequisite. Historical rows above remain the prior repair record.

| Finding | Class | Current disposition | Verification / remaining work |
|---|---|---|---|
| F1 | A | FIXED-SOURCE | Approved JoinAccept validation-before-commit patch; real1.1 crypto, corrupt17/33-byte frames, replay, nonce/reboot and1.0 stop tests. Private-network injection unexecuted. |
| F2 | A | FIXED-SOURCE | Existing upEvent boundary replaces rc-only fragment advancement; counter remedy disproved. Real stage/launch/RX MIC/foreign/nested failure tests. Repetition ambiguity remains conservative. |
| F3 | A | FIXED-SOURCE | Immediate real INA226 calibration-error check; NACK/truezero/voltage-only host tests. Bench calibration/range unexecuted. |
| F4 | B | RETAINED-DOCUMENTED + HARDWARE-ONLY | False serialized-traffic claims corrected. Slot lifecycle preserved; no proven SD mount/Hosted traffic exclusion.50-cycle WiFi/remount stress/recovery unexecuted. |
| F5 | C | HARDWARE-ONLY | Conducted power versus EIRP/ERP corrected; real antenna/power/country compliance not established. |
| F6 | B | RETAINED-DOCUMENTED + HARDWARE-ONLY | Runnable priority2 radio versus priority1 loop mechanism; no unproven affinity/priority change. SF12 latency unmeasured. |
| F7 | A mechanism | RETAINED-DOCUMENTED + HARDWARE-ONLY | Intentional GT911 visible latch/restart retained; clear-write checked, no reachable null latest found. Fault test unexecuted. |
| F8 | A | FIXED-SOURCE | Timestamp only checksum-valid NMEA; real parser/no-fix/garbage/silence/uint32-expiry tests. |
| F9 | A | FIXED-SOURCE | Truthful NVS/radio/DIO2 diagnostics; actual init/join fail-stop tests. |
| F10 | A | FIXED-SOURCE | One approved credential predicate controls Connect and both existing status displays; validation/periodic-update tests. |
| F11 | B | RETAINED-DOCUMENTED | Traced stale/doubled early-return totals over-defer; no lost launched frame found. No assertion blesses over-count as correct. |
| F12 | A | RETAINED-DOCUMENTED + HARDWARE-ONLY | Driver ESP_ERROR_CHECK abort/reboot, serial-only before workers; no graceful DSI fallback. |
| F13 | A | FIXED-SOURCE | JPEGDEC type guard rejects progressive; real fixtures pass. False durable-airtime-debt phrase removed. |
| F14 | B | HARDWARE-ONLY | Target stack-frame evidence recorded; callback buffers do not coexist across recursion as implied. Real watermarks remain open. |
| S1 (additional) | A | FIXED-SOURCE + HARDWARE-ONLY | Target loadImage frame19120 exceeded media stack12288. Approved media stack24576, net0lines/+12KiB runtime reservation. Whole-chain margin unmeasured. |

No approval request remains pending. F4/F6/F7/F11/F12 are retained limits, not
software fixes. TCXO population, RF-switch polarity, antenna/cable/EIRP, GT-U7
VCC/TXD, INA226 shunt/straps/pull-ups, J2/J13 orientation, P4 revision/C6 firmware,
10.1-inch timing, GT911 orientation, NS4263B enable/reset pop and ES8311 MCLK-gap
retention remain physical prerequisites. The pinned schematic confirms the
GPIO30/R147/R71 path only, not its fitted state or measured behavior.
Assembly/bench flashing remain conditional on those checks and separate flash
permission; unattended TTN remains NO-GO until full acceptance including24h run.


## Re-review N1–N9 checkpoint

See [current verification and dispositions](review/rereview-fix-status.md).
The earlier completed-fix statements above describe the prior repair checkpoint,
not resolution of these new findings. Approved N1 JPEG bounds, N5 MAC-flag lifetime
and N6 final radio diagnostics are now source-corrected and tested. Counter-zero
handling and coexistence remain open. The credential-save status overwrite is
corrected; WiFi association/persistence and strict GPS checksum parsing are not
yet resolved. No hardware evidence
was added. Unattended TTN operation remains NO-GO.


## Accepted independent-review corrections — 2026-10-25

Baseline main@1961cfa. Current authoritative traces, scope decisions, approvals,
regressions and validation are in [accepted-review-fixes.md](review/accepted-review-fixes.md).
The historical N4/N7 “open” statements above describe prior checkpoints.

| Finding | Class | Disposition | Evidence / remaining limit |
|---|---|---|---|
| Amplifier control/startup (R1) | A + C | FIXED-SOURCE + HARDWARE-ONLY | GPIO30 HIGH before codec setup and at idle; LOW for playback/drain. Verbatim startup/audio service tests pass. Fitted circuit and sound remain untested. |
| Confirmed network ACK (R2) | A | FIXED-SOURCE | Real RadioLib app3/network7 fixture verifies ACK counter7, including low16 handling at maximum32-bit FCnt. |
| Counter-zero duplicates (R4/N4) | A | FIXED-SOURCE | Approved two-bit accepted-state metadata, reset/restore contract, authenticated commit and overflow rejection; both unicast domains tested. Legacy ambiguous session snapshots fail closed. |
| WiFi association/credential storage (R6/N7) | A/B | FIXED-SOURCE | Accepted STA request + raw association/IP + matching SSID precede one blob write. Verbatim boundary tests include stale/rejected state, open networks and storage failure. Physical event timing/power loss untested. |
| Manufacturer display timing (R8) | B | FIXED-SOURCE + HARDWARE-ONLY | Constructor now matches pinned Elecrow V1.2; physical display/touch still unverified. |
| ES8311 false success (R5) | A | RETAINED-DOCUMENTED | User allowed “fix or ignore”; selectively lost write/read errors remain in audio-driver0.3.1. No third patch or app-side masking added. |
| SD/Hosted sharing (R3/F4/N2) | B | RETAINED-DOCUMENTED + HARDWARE-ONLY | User excluded this fix. WiFi never mounts the card, but shared-controller lifecycle risk and conditional whole-P4 reset remain. |
| Nested MAC recursion (R7/F14) | A/B | RETAINED-DOCUMENTED + HARDWARE-ONLY | User excluded this fix; no general recursion bound or target stack-safety claim. |

Other review observations (JPEG scaling, permissive NMEA checksum helper and demo
single-attempt semantics) were not authorized for correction in this scope and
remain unchanged. Pins/J7, telemetry version1/decoder, UI visuals, source photo,
LoRaWAN1.1 ClassA/EU868, nonce reservation and volatile aggregate pacing are preserved.
Unattended TTN remains NO-GO until the retained risks are resolved or explicitly
accepted with measured operating limits and the complete hardware checklist passes.
