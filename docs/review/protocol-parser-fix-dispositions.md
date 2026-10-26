# Five protocol/parser corrections — final dispositions

2026-10-26. Canonical checkout is
`/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry`,
main at `c86439247b2840ed94099a34bc8c72b722f0976f`, with these fixes uncommitted.
No commit, push, branch change, global install, archive change, flashing, RF,
TTN access or external contact was performed. The original review records remain
unchanged; this is an independently tested implementation disposition.

## Authority and approved scope

The five selected claims were traced against the current application and exact
pinned sources. The user approved P1–P3 in
[the quantified proposals](protocol-parser-fix-proposals.md). That approved the
existing-deadline join policy change (estimated net +1 application line), removal
of unused EXIF interpretation (126 dependency lines), and queue-owned RekeyInd
patch expansion (estimated net +20 dependency lines). All were implemented within
those boundaries. The WiFi feedback reshape and display-clock documentation need
no additional state or production growth. No approvals remain pending for these
five corrections.

The unchanged Config owns pins/credentials; the radio worker owns radio/NVS and
its existing volatile deadline; RadioLib's existing queue owns pending RekeyInd;
the media worker owns its isolated JPEGDEC parser; the WiFi worker owns request
outcome/reporting. No duplicate handshake flag, parser, retry timer, persisted
RF debt, pre-transmission reservation or callback was added.

## 1. Join retry policy — A, FIXED-SOURCE; hardware follow-up

Invariant: failed joins must obey the existing stricter aggregate airtime gate
and the LoRaWAN 1.1 section 7/Table 17 join budgets, with randomized intervals.
The old indefinitely repeated one-hour ceiling could not meet the later budget.
`Workers.cpp:45–48` now centralizes deadline clamping and adds jitter after the
maximum of the retry and existing airtime hold; `:77–80` uses that authority.
The existing failure branch at `:170–174` supplies hardware RNG jitter 0–60 s
and doubles the existing backoff to a 24 h ceiling. Initial backoff, DR progression,
DevNonce reservation and aggregate volatile pacing are retained. Slower recovery
after prolonged outages is the approved trade-off. No boot-delay or persisted
pacing policy was introduced; reboot resets this deadline as previously accepted.

Real RadioLib 7.8.1 tests run the extracted production join loop for 60 simulated
days with two deterministic RNG streams and independent reference DR airtimes.
They check the first-hour, following-ten-hour and later sliding-day budgets,
jitter after the airtime hold, prolonged backoff, nonce failures, DR progression,
32-bit application timestamp wrap and signed-span saturation. The old implementation
fails the new ceiling regression. This is simulated timing, not ESP32 emulation,
RF compliance measurement, entropy validation or target-width RadioLib loop proof.

## 2. JPEG EXIF parsing — A mechanism/B target consequence, FIXED-SOURCE

Invariant: unused file metadata must not be interpreted through unbounded TIFF
offsets before application admission. Exact JPEGDEC 1.8.4 APP1 handling called
private TIFF readers before checking directory ranges. The baseline sanitizer
reproducer faults in TIFFSHORT/GetTIFFInfo; no P4 fault is claimed.

The approved reduction deletes the APP1 interpretation branch and four private
TIFF helpers: 0 added/126 removed dependency physical lines. `JPEGParseInfo`
now starts at patched `src/jpeg.inl:1470`; the generic marker walker skips APP1
at `:1642`. The existing SOF guard at `:1555` remains. The application still
rejects progressive JPEGs through the existing type API. No parallel parser or
upgraded dependency was introduced. This isolated profile intentionally does not
extract EXIF orientation/thumbnail metadata; otherwise valid image pixels may
load even when that ignored metadata is invalid. It is not a general-purpose
EXIF-compatible JPEGDEC replacement or a complete decoder audit.

Real-library ASan/bounds tests cover baseline pixel equivalence, little/big-endian
EXIF, invalid first/second IFD offsets, excessive tag counts, unaligned offsets,
short APP1 bodies, progressive and invalid SOF fixtures. Existing rejection
assertions remain. Photo/UI assets and the rendering code are unchanged.

## 3. Rejected WiFi request feedback — A, FIXED-SOURCE

Invariant: a rejected connect request must not be presented as pending or arm
credential saving. Core 3.3.12 `libraries/WiFi/src/STA.cpp:373–434` supplies the
existing authoritative boolean result. `Workers.cpp:259–264` reuses connectAt:
accepted calls start the timer; rejected calls expire it. reportAt still preserves
the specific diagnostic for the existing 2 s reporting interval. The next offline
report is failure, not Connecting. No new flag/timer/storage path or UI geometry.
The extracted worker regression failed before and passes after, including uint32
wrap. Existing stale-link, credential identity and atomic-storage checks remain.
This does not claim a newly proven same-SSID driver race or change driver policy.

## 4. RekeyInd lifetime — A, FIXED-SOURCE; interoperability untested

Invariant: pending RekeyInd remains in the existing queue until authenticated,
matching-version RekeyConf, across ordinary, nested, MAC-only and restored-session
paths, without retaining unrelated commands or sending FOpts together with port 0.
LoRaWAN 1.1 section 5.10 is the normative basis. Exact patched RadioLib 7.8.1:

- `src/protocols/LoRaWAN/LoRaWAN.cpp:154–164` copies queued and supplied MAC
  commands into port-0 FRMPayload without clearing the queue.
- `composeUplink:1383` derives wire FOpts length as zero for port 0; all packet
  offsets use that local value. Queue length retains its single meaning.
- `parseDownlink:2117–2124` retains only pending RekeyInd on authenticated Class-A
  reception; `:2259` includes retained bytes in response capacity decisions and
  the small-response append preserves them. Immediate MAC replies stay immediate.
- `execMacCommand:2772–2773` removes it only for matching-version confirmation.
- `deleteMacCommand:3119–3141` now includes CID in command length, validates it,
  moves once and clears the vacated tail. `clearMacCommands:3143–3158` uses that
  same deletion authority, processing shifted commands without skipping them.
  Its uplink and retained downlink-answer consumers are tested.
- `clearSession:358` resets queue lengths with their buffers. The reset regression
  exposed the prior mismatched lifetime (zero bytes with a nonzero length).
  Clearing both is necessary to guarantee exactly one RekeyInd after a fresh join.

No member/header/session-format change, new persistent key or application-side
handshake state. Actual RadioLib source delta is 60 added/73 removed lines, a
reduction relative to the approved estimate. Tests inspect decrypted transmitted
commands using real RadioLib: ordinary/no-RX, wrong-version/corrupt confirmations,
small/full queues, explicit port 0, nested replies and nested TX failure, matching
network-counter confirmation, unrelated responses, queue deletion positions,
malformed queues, repeated pruning, session export/restore and ADR-limit rejoin.
Public LinkCheck/DeviceTime answer retrieval also passes after shared pruning.
The baseline real-library reproducer fails after an ordinary application downlink.
Old buggy session snapshots are not claimed repaired; the application still joins
fresh. Crypto fixtures use the library primitives and are not independent network
interoperability vectors. Recursion and target stack safety remain unmeasured.

## 5. Display clock documentation — B, FIXED-DOCS; physical measurement pending

The request is 51 MHz, matching Elecrow's V1.2 configuration. IDF v5.5.5
`components/hal/mipi_dsi_hal.c:266–273` rounds the divider; the DPI path is
`components/esp_lcd/dsi/esp_lcd_panel_dpi.c:265–282`. Both installed P4 clock
headers select PLL_F240M at `soc/clk_tree_defs.h:433`, yielding nominal
240/round(240/51) = 48 MHz. Documentation now distinguishes request, source-derived
result and measurement. Ui.cpp is byte-identical; no measured display claim.

## Validation, accounting and remaining limits

[Final validation](../validation.md#protocolparser-corrections--final-2026-10-26)
and [evidence](../validation-logs/protocol-parser-fixes/) contain exact build
commands, logs, protected hashes and accounting. Full independent host suite and
format check pass. All three firmware configurations compile. Target clang-tidy
was attempted but cannot analyze the ESP xesppie/GCC arguments and target headers;
this is an analysis limitation, not a clean tidy result.

Physical lines relative to c864392 (added/removed separately): application 8/7;
patched dependency source 60/199; tests 387/30; build/test tooling 26/2. Thus
application net +1, dependency net −139. This cumulative final entry supersedes
the partial checkpoint; do not add both. Patch hunk/context lines are not source
LOC. Test-only random sources/transport stubs are not firmware mechanisms.

Config, pins/J7, Application.cpp, Ui.cpp, telemetry encoder/decoder/reference,
source photo, wiring/UI preview assets, formatting settings, historical review
records and historical ZIP match the recorded baseline hashes. All three build
copies match canonical C++/headers after Arduino's inserted #line directive;
selected external libraries are isolated exact versions. Enabled linked callback
is present; reservation hooks and SDMMCFS mount/unmount symbols are absent.

Assembly remains conditional on the documented physical identification, voltage,
TCXO/RF-switch/antenna, shunt and connector checks. Supervised flashing still
requires separate authorization. Unattended TTN operation remains NO-GO until
retained SD/Hosted coexistence/reboot risks, codec error limits, stacks/latency,
RF power and the complete hardware checklist including 24 h operation are resolved
or measured acceptable. No display, touch, WiFi, SD, GPS, sensor, audio or RF
operation is claimed. The added bench cases were documented, not executed.
