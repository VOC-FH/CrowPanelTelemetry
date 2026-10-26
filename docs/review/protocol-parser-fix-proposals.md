# Five selected corrections — checkpoint and gated proposals

2026-10-26; canonical main@c86439247b2840ed94099a34bc8c72b722f0976f.
The user requested the five corrections discussed after Claude's f23e393 review.
This is a partial implementation checkpoint, not completion of those five fixes.
No commit, push, installation, flashing, RF or TTN operation was authorized or done.
The independent review and caller assessment remain unchanged historical records.

## Completed independent corrections

### Rejected WiFi request feedback

Invariant: an API-rejected request must not appear pending after the initial
specific error report, and cannot arm credential saving. The WiFi worker owns
request outcome, its existing pending flag and report/connection timers.
Core3.3.12 STA.cpp:373–434 returns false for rejected configuration/connection.
Workers.cpp previously set connectAt to now for both accepted and rejected calls;
the periodic offline branch then displayed Connecting for20seconds.

The same connectAt now represents either an accepted pending request or an expired
rejected attempt. Its unsigned timestamp is backdated by20001ms only on rejection;
reportAt independently keeps the specific rejection visible for the existing2s
report interval. The next offline report correctly says Connection failed. Live
link/IP reporting, credential persistence and UI layout are unchanged. No new
flag, timer, cache or helper; two added/two removed application lines (net0).

The verbatim worker regression rejects a request while offline, checks the initial
specific message and next periodic failure, and repeats across uint32 wrap. It
failed before and passes after the correction. Existing old-link and storage
assertions remain. Baseline and current full independent host suites pass.

### Requested versus derived display clock

Invariant: documentation must distinguish requested timing from a source-derived
hardware divider and from an actual measurement. No display code change.
README, sources and hardware checklist now identify51MHz as the request matching
Elecrow, and48MHz as the nominal result from IDF's240MHz source / rounded divider5.
IDFv5.5.5 mipi_dsi_hal.c:266–273 and esp_lcd_panel_dpi.c:265–282 establish the path;
both installed P4 clock headers select PLL_F240M at:433. Neither frequency nor
image stability was physically measured. UI visuals and all bus arguments remain.

## Proposed corrections — NOT APPLIED

The global engineering rule says to reduce first, reshape second and obtain
explicit approval before net production growth or architectural extensions.
The earlier generic request to fix identifies scope; it does not provide the
required quantified choice below. Work on the independent corrections above
was completed before stopping for this decision.

### P1. Join retry policy using the existing authority

Authoritative state: radio worker joinRetry (volatile exponential backoff),
RadioLib's actual per-operation lastToA, and PersistentNode's sole volatile deadline.
Durable DevNonce reservation and all accepted aggregate pacing remain unchanged.
Current max(joinRetry, airtime hold) at DR0 permits about30s/day indefinitely,
while LoRaWAN1.1 section7/Table17 has a stricter later join budget and requires
randomized retry intervals. No additional uptime tracker or persisted debt.

Proposed outline:

- Increase the existing exponential joinRetry ceiling from3600000 to86400000ms.
  Keep the initial60000ms and existing DR progression. Sustained failures will
  eventually retry once a day; early attempts still use the current stricter RF
  airtime hold.
- In the existing failure branch, replace defer(joinRetry) with
  defer(joinRetry + esp_random() % 60001U), adding0–60s jitter.
- Include esp_random.h explicitly. Estimate:3 added/2 removed application physical
  lines, net+1; no new state, task, helper, callback, key or pacing authority.

Reduction alone cannot add randomness or prevent the one-hour ceiling from
persisting. Reshaping the existing backoff absorbs the policy without an uptime
state machine. The trade-off is slower recovery after a prolonged outage.
Test the actual extracted join loop for at least48 simulated hours, with independent
reference DR airtimes and two seeded RNG streams, against all Table17 windows and
bounds, nonce fail-stop and wrap behavior. Verify the conservative policy before
claiming compliance; no RF experiment is authorized.

### P2. JPEG: remove unused EXIF handling (preferred reduction)

Authoritative parser: JPEGDEC1.8.4 JPEGParseInfo APP1 case at jpeg.inl:1653–1677 and
private TIFF helpers:1276–1376. File-derived first/thumbnail-directory offsets are
read before any matching buffer/segment lower/upper bound. Application admission
happens after openRAM, so a downstream filter cannot repair those reads.

Search found no project consumer of getOrientation, getThumbWidth/getThumbHeight,
hasThumb or decodeThumb. Baseline JPEG pixel decoding does not consume ucOrientation;
its other references are metadata getter/parser only. The existing generic marker
walker already skips unhandled APP segments via iOffset += usLen.

Proposed outline: delete the APP1 EXIF interpretation case (25 physical lines) and
its four now-unused private TIFF helper functions/comments (101 physical lines).
Keep normal JPEG marker traversal, pixel decoding, SOF bounds guard and progressive
rejection. No app-side parser, wrapper, macro or new helper. Estimate:0 added/
126 removed dependency lines. This expands the reviewed JPEG patch scope but
reduces code and removes the unnecessary parsing responsibility entirely.

Trade-off: EXIF orientation/thumbnail metadata APIs in this isolated patched
library will no longer provide extracted metadata. Current sketch rendering is
unchanged because it never used that metadata. JPEGs with otherwise valid pixels
and bad/ignored EXIF may load, rather than fail solely for that unused metadata.
The patch is deliberately for this template; do not apply it as a general JPEGDEC
replacement for applications that need EXIF.

Test pixel-equivalent decoding with little/big-endian valid EXIF and invalid first/
second directory offsets, excessive tag counts and short APP1 bodies under real
JPEGDEC ASan/bounds. Existing SOF, progressive and ordinary baseline tests remain.
This removes the identified EXIF path; it is not a complete JPEG-decoder audit.

Alternative (not proposed by default): preserve EXIF by reshaping GetTIFFInfo and
both directory paths to validate every header/table/link/thumbnail file range
before arithmetic/dereference. That needs additional bounds/error propagation and
more production code. Only pursue it if retaining unused metadata is required.

### P3. RadioLib RekeyInd queue lifetime

Invariant: pending RekeyInd is retransmitted in every relevant uplink until a
valid RekeyConf, without retaining unrelated responses after their prescribed
lifetime or producing invalid FOpts+port0 combinations. RadioLib's existing MAC
queue remains the single authority; do not add an application rekey flag.

Trace: the join queues RekeyInd at LoRaWAN.cpp:912–918; no-RX command cleanup
retains persistent commands at:279–280; parseDownlink indiscriminately clears the
uplink queue at:2117–2119; the later small-response copy at:2298–2300 can overwrite
anything retained; the MAC-only migration at:154–160 clears the queue as well.
The ADR-limit rejoin check at:103–108 depends on that queue. Merely deleting the
first clear does not fix all paths and would retain unrelated sticky responses.

Proposed outline: reshape existing queue cleanup, valid-RekeyConf handling and
packet construction together so RekeyInd survives ordinary, nested and MAC-only
uplinks until authenticated confirmation; retain the existing immediate replies
and aggregate airtime behavior. Use existing queue operations, no persistent flag,
callback, HAL layer or application-side workaround. Include queue-removal repair
before using it: current deleteMacCommand:3120–3143 asks getMacLen without the CID
and zeros the wrong range after memmove. That method has no current call site, so
calling it uncorrected would introduce corruption rather than fix the handshake.

Estimate:about40 added/20 removed dependency physical lines (net+20), subject to
verification of the concrete diff. This is a RadioLib patch-scope expansion, not
permission for unrelated command/recursion/pacing changes. A scalar restore flag
in the application or a temporary byte snapshot that ignores nested calls would
create a competing authority and is rejected.

Real-library regressions must inspect transmitted/decrypted MAC commands for:
ordinary data downlink without RekeyConf, unrelated MAC requests, incorrect and
valid RekeyConf, MAC-only/nested replies, no duplicate indication, reset/session
restore, queue capacity and ADR-limit rejoin. Verify deletion at the first/middle/
last queue position without damaging remaining commands. If the estimate or queue
ownership cannot hold, stop with a revised proposal rather than add parallel state.

## Scope preserved

Suspected same-SSID race and amplifier startup pulse remain unproven/unmodified.
Retained SD/Hosted sharing, ES8311 errors, nested recursion, volatile RF pacing,
all pins/J7, UI visuals, source photo and version1 telemetry protocol are unchanged.
No new feature, branch, checkout, archive or global install. Full three-variant
firmware validation belongs to the final approved five-fix revision; this checkpoint
has an enabled-prev3 compile only. See validation.md for exact evidence/limits.


## Subsequent approval and implementation

The user explicitly approved P1–P3 after receiving the quantified proposals.
They are now implemented; the NOT APPLIED statements above describe the original
checkpoint. See protocol-parser-fix-dispositions.md and validation.md's final
section for actual architecture, accounting, results and limits. No commit or
push is authorized by this approval.
