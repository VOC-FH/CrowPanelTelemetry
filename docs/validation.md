# Deep-review validation — 2026-10-25

Canonical checkout:
`/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry`.
Private repository: `git@github.com:VOC-FH/CrowPanelTelemetry.git`.
At the prior deep-fix validation checkpoint, default/current branch was `main`; HEAD was
`6fada32040bbcb982c99dd59e9b4f57c1b2eb7c3`; local main/origin/main ahead0/behind0
against the existing tracking ref. No fetch/pull or ref change. Read-only
`gh pr list --repo VOC-FH/CrowPanelTelemetry --state open --json number,title,headRefName,baseRefName,url`
returned `[]`. No commit, push, PR, installation, flashing, RF or TTN access.

The original full-file SHA256/physical-LOC snapshot, manifest contents and dirty
status are preserved in `validation-logs/deep-fix-baseline.json`. Pre-existing
SHA256SUMS/review-index changes and five untracked raw review/request/prompt files
were preserved. Review index was appended only; raw records were not rewritten.
Baseline host tests and format check passed after the approved path-only runner
adjustment. Fresh pre-change firmware compiles were not captured; previous build
sizes are historical, not a measured baseline. Only final-source compiles below
are new evidence. No pre-existing test failure was attributed to this implementation.

Detailed per-finding source traces, approvals, classifications/dispositions and
hardware limits: [deep-fix-dispositions.md](review/deep-fix-dispositions.md).
F1/F2/F3/F8/F9/F10/F13 are source-corrected. F4 concurrency, F6 scheduling,
F7 intentional latch, F11 conservative over-deferral and F12 driver abort remain
explicitly documented; F5/F14 need hardware evidence. Additional confirmed media
stack mismatch S1 was corrected with separate approval.

## Exact configuration and final builds

Arduino CLI1.5.1 commit01f3d4f2b, official isolated release/checksum verification;
installed esp32 core3.3.12 / ESP-IDF5.5.5 (read-only). Dependencies in the exact-tag
provenance file `validation-logs/deep-fix-dependencies.json`:
Arduino_GFX1.6.9, RadioLib7.8.1 with approved patch, INA2260.6.6,
TinyGPSPlusv1.0.3a, JPEGDEC1.8.4, audio-driverv0.3.1. IDFv5.5.5 and
Hostedv2.12.13 source traces use those tags, not branch heads/older examples.
New source-file hashes are in correctness-source-hashes.json; original RadioLib
and patched hashes remain separately authoritative in patches/.
No toolchain or library version changed; no global install/upgrade/index update.
All six external Used library paths resolve under the isolated task deps directory.
Bundled SPI/Wire/Preferences/WiFi/Networking/ESP_I2S resolve to core3.3.12.

Common FQBN:
`esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB`.
3,145,728-byte app partition. Delivered Config.h is byte-identical with blank
credentials/shunt. Enabled builds use only the dummy flags shown below.

| Final source configuration | Flash bytes | Static RAM bytes | Result |
|---|---:|---:|---|
| ChipVariant=prev3, blank | 1,202,450 | 39,364 | PASS |
| ChipVariant=prev3, dummy enabled | 1,209,552 | 39,436 | PASS |
| ChipVariant=postv3, dummy enabled | 1,211,122 | 39,436 | PASS |

All were rebuilt after the approved media-stack correction; verbose output also
captures exact compile/link commands and dependency discovery. Same enabled-prev3
configuration with `-fstack-usage` also passes. Task stacks and PSRAM/heap allocations
are additional to static RAM; media-stack allocation grew12KiB at runtime.
These are configurations of one sketch, not three installed firmwares.

## Commands and evidence

`T=/tmp/codex/build/CrowPanelTelemetry/deep-fix`; dependencies in `$T/deps`.
Separate TMPDIR/cache/build per configuration, all belowT. No alternate checkout,
project copy, persistent build or dependency installation. Read-only source inputs
under `/home/voc/.arduino15/packages/esp32` were never modified.

```sh
python3 tools/patch_radiolib.py "$T/deps/RadioLib"
BUILD="$T" RADIOLIB_DIR="$T/deps/RadioLib" make test
make format
make format-check
python3 tools/check_wiring.py "$T/panel.sch" "$T/panel.brd"
TMPDIR="$T/default-prev3-tmp" ARDUINO_BUILD_CACHE_PATH="$T/default-prev3-cache" \
  "$T/cli/arduino-cli" compile --verbose --jobs 3 \
  --fqbn 'esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant=prev3' \
  --libraries "$T/deps" --build-path "$T/default-prev3" .
# Enabled prev3/postv3 add (and use separate build/tmp/cache names):
# --build-property 'compiler.cpp.extra_flags=-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f'
# stack-prev3 adds -fstack-usage to that same extra_flags property.
clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Application.cpp"
clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Workers.cpp"
clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Ui.cpp"
# Host-width feasibility probe (FAILED; no multilib runtime):
printf 'int main(){return 0;}\n' | TMPDIR="$T" c++ -m32 -x c++ -o "$T/probe32" -
```

Exact argv for every final compile is the first line of the corresponding
validation-logs/{default-prev3,enabled-prev3,enabled-postv3,stack-prev3}.txt;
`deep-fix-build-results.json` records exits. Each independent test keeps running
even if another fails. Final host.txt and format-check.txt pass.
The Makefile BUILD override now respects task-local environment/command-line
settings. An initial runner invocation used its default `local` path because
of an unconditional Make assignment; that task-created directory is cleaned too.
All final checks useT. Path changes were explicitly approved before baseline.

Passing host checks cover encoder/reference/decoder, fragments/saturation/airtime,
SD API lifecycle, actual RadioLib node/init/join/service extraction and crypto,
actual INA226 acquisition against transport NACK/data, actual TinyGPSPlus parser
and status block, actual JPEGDEC baseline/progressive guard, and both WiFi-form
render expressions/Connect predicate. DR-dependent JoinRequest reference values
come from tools/airtime.py; no second airtime formula was added to the test PHY.
Real RadioLib TX IRQ timeout and RX initial elapsed loop execute at ordinary host
times. New tests demonstrated expected failure before the respective fixes; the
f*-before logs are reproducers, not current failures. Final host.txt is the final
result. TinyGPSPlus's existing intentional fallthrough warning is retained.

No host test emulates an ESP32. Host unsigned long/RadioLibTime_t is64-bit;
-m32 linking failed (Scrt1.o/crti.o/libstdc++ missing). Tests cover wrap-safe uint32
application deadlines, age expiry and wrapped HAL values, including TX ending at0;
they do NOT prove P4-width elapsed-loop behavior, target IRQ scheduling or receive
window timing. RX ongoing/final300ms loops, physical UI refresh/touch, concurrency,
JPEG/WAV fuzzing and full runtime call-stack maxima remain gaps. SD doubles do
not model Hosted traffic/reset/recovery/forceful controller deinit.

## Patch, build-input and asset audits

- PASS: fresh exact7.8.1 apply, idempotence, modified-source refusal and real-library
  test before/after hash assertions. See patch-check.txt. No marker macro,
  branding comment, beforeUplink/afterUplink or per-frame reservation restored.
- PASS: final canonical C++/headers match every build tree after Arduino's one-line
  #line prefix. Linked downlink callback present; reservation-hook and SDMMCFS
  begin/end symbols absent. Used library versions/paths checked in verbose output.
  See source-and-symbol-audit.txt and build-input-sha256.json.
- PASS: pinned panel.sch/brd hashes, repeated nine-radio-GPIO/J7/Eagle audit,
  Config/pin assignments, protocol/decoder/reference packet, UI previews, wiring
  graphics/radio-nets and source photo unchanged. The SVG-embedded PNG is identical.
  Source photo SHA256:
  `5b2e2bdfefe5a38b7bbee5ef09f37c363579a8628f27df23f5702b5fcd6624ad`.
  Individual locked hashes are in source-and-symbol-audit.txt.
- PASS: canonical formatter/analysis settings unchanged and aligned with the shared
  baseline. No cosmetic UI changes or generated-preview/wiring regeneration was
  needed: their generators and geometry are unchanged. Behavioral status text changed.
- `git diff --check` for project-owned sources/docs, excluding the dependency patch
  and captured raw logs, passes. Raw Arduino CLI logs retain emitted trailing
  spaces; the patch artifact retains upstream blank CONTEXT-line whitespace in
  newly recorded hunks. Neither vendor source nor raw output was reformatted. This is not a format-check
  failure in project-owned C++.
- The earlier net audit did not cover core variant macros. Stock SD_MMC remains
  unsafe for GPIO45 here; C1's separate IDF/POSIX solution and static assertions
  remain. F4 means this does not prove SD/Hosted runtime exclusion.

## Stack evidence and static analysis

Target enabled-prev3 `-fstack-usage` output is preserved in stack-frames.txt:
radioTask3616, applicationDownlink1120, post1088, sendReceive80, parseDownlink384,
loadImage19120 and mediaTask272 bytes. Individual compiler frames are not a
whole-call-chain bound. The outer callback returns before protocol recursion;
it does not hold a second callback/post buffer alive as the review implied.
The proven19120-byte media-frame versus12288-byte task mismatch was corrected
by the approved single argument change to24576. More stack margin is not proof
of radio/media safety; watermarks with real nested downlinks/JPEG/other activity
remain hardware-only. No persistent instrumentation was added.

Target clang-tidy exits1 for all three changed cpp files using the actual
same-configuration compile_commands.json. Installed LLVM21.1.8 rejects
`rv32imafc_zicsr_zifencei_xesppie`, GCC-only `-fno-tree-switch-conversion` /
`-fstrict-volatile-bitfields`, and cannot resolve target cmath. See tidy-*.txt.
No checks or flags were weakened to claim a pass.
Supplemental host clang-tidy on radio.cpp and sensor.cpp exits0 with warnings,
using the exact commands in host-tidy-0.txt/host-tidy-1.txt. Retained findings:
three canonical MaxSize=-1 configuration warnings; named-parameter, const/static,
implicit-bool, isolated-declaration, redundant-cast, DeMorgan boolean, uppercase
literal suffix and redundant-void suggestions. Host diagnostics do not replace
unavailable target analysis or physical testing.

## Change accounting and approvals

Physical lines relative to the recorded task baseline, added/removed separately:

| Category | Added | Removed | Net |
|---|---:|---:|---:|
| Application production (.ino/.h/.cpp/decoder) | 31 | 20 | +11 |
| Actual patched dependency source | 11 | 7 | +4 |
| Tests (text; two JPEG fixtures separate) | 516 | 13 | +503 |
| Build/test tooling | 127 | 10 | +117 |

Application growth: approved F3+2, F9+2, F10+8; GPS reshape−1. F2/JPEG policy,
uint32 backoff template clarification and approved media-stack argument are net0.
F1's authority-level validation needed locals instead of committing members before
MIC; app-side rollback was incomplete. F10's approved one helper replaces repeated
validation with one authority and no error-state cache. Test/tool growth exercises
real dependencies/production extraction; no new test framework. Binary fixtures
are recorded separately by hashes/bytes, not physical LOC. Historical change
entries remain, with a separate deep_review entry in change-accounting.json.

Approval outcomes: path-only adjustment approved; F1 patch expansion11+/7−
approved; F3 app4+/2− approved; F9 app8+/6− approved; F10 app11+/3− and helper
approved; S1 media-task stack numeric change1+/1− approved. None pending.
F4/F6/F7/F11/F12 remain intentionally unresolved in source, with reasons recorded;
documentation does not label them fixed. No additional architecture was slipped in.

Changed files and preserved pre-existing state are listed in
validation-logs/deep-fix-changes.json. Protected hashes, raw reviews and historical
`artifacts/CrowPanelTelemetry-delivery.zip` remain unchanged. No new archive.
SHA256SUMS.json uses the existing sorted path→SHA256 format/scope, excluding itself,
Git internals and ignored build/checkouts files.

TaskT and the task-created default local host directory are removed after evidence
is copied and checked. No persistent build, extra checkout or global install remains.
Temporary dependency sources/CLI/compilation databases are cleaned too; exact commands,
hashes, source inputs, outputs and stack-frame evidence are retained in docs.

Assembly: conditional on physical connector/voltage/shunt/TCXO checks.
Supervised bench flashing: conditional on those prerequisites and separately
required flashing authorization. Unattended TTN: NO-GO until SD/WiFi stress,
antenna/power compliance, real stacks/latency and the entire hardware checklist,
including24h run, pass. No display/touch/audio/WiFi/SD/GPS/sensor/RF operation claimed.


## Re-review correction checkpoint — 2026-10-25 (incomplete)

Current main HEAD is `c6fe271124666c82f92733a8c0f161697761367d`. Local changes
include the user's authorized editorial changes and the prior drafting records;
these were preserved. No commit/push or branch/ref change was made. The application
change in this checkpoint replaces a WiFi save-failure post with formatting into
the existing report buffer, preventing the next post from hiding the error.
Association/persistence correctness and other re-review findings remain open:
[dispositions and pending proposals](review/rereview-fix-status.md).

Temporary task root is `/tmp/codex/build/CrowPanelTelemetry/rereview-fix` (`T`).
The exact-tag dependencies are retained there for continuation, not installed.
CLI1.5.1 was verified against its official release checksum; installed core3.3.12
and both P4 SDK variants were read-only. Dependencies and source-file hashes are
recorded in validation-logs/rereview-fix/. The unchanged baseline passed host,
format and all three compiles before production edits. Both phases use the common
FQBN documented above, isolated `--libraries "$T/deps"`, separate configuration
build paths, task-controlled TMPDIR and caches. External Used library paths resolve
to the six pinned isolated dependencies; bundled libraries resolve to core3.3.12.

| Configuration | Baseline flash | Current flash | Static RAM (both) | Both builds |
|---|---:|---:|---:|---|
| prev3, blank Config.h | 1,202,482 | 1,202,484 | 39,364 | PASS |
| prev3, dummy credentials/shunt | 1,209,592 | 1,209,594 | 39,436 | PASS |
| postv3, same dummy flags | 1,211,154 | 1,211,156 | 39,436 | PASS |

Exact compile arguments and exit codes are preserved in baseline-build-results.json
and current-build-results.json in that evidence directory; complete output is in
the corresponding log files. These are checkpoint builds, not validation of the
unapplied proposals. No timing/throughput or runtime stack measurement is claimed.

Commands run from the canonical checkout (T expanded to the path above):

```sh
BUILD="$T" RADIOLIB_DIR="$T/deps/RadioLib" make test
make format-check
python3 "$T/build.py" baseline
python3 "$T/build.py" current
BUILD="$T" python3 tools/test_wifi_worker.py
clang-format -i tests/wifi-worker.cpp
clang-tidy --config-file=.clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Workers.cpp"
python3 tools/check_wiring.py "$T/panel.sch" "$T/panel.brd"
```

Host checks and format passed for current production, including the new independent
verbatim WiFi worker check. That check failed before the correction; original
failure and passing output are saved. All independent checks execute even if
another fails. Target clang-tidy returned1: unsupported `xesppie` RISC-V extension,
unknown `-fno-tree-switch-conversion` and `-fstrict-volatile-bitfields`, and missing
`cmath`; it also reports existing canonical check-option warnings. No analysis
configuration was weakened. It is not a successful target analysis.

The temporary diagnostic drivers used the real decoder and real RadioLib with
local inputs/simulated PHY only. N1 bounds-only sanitizer failed at jpeg.inl:1694,
array index4; full UBSan first stopped on the decoder's existing host unaligned
access. N4 delivered a counter-zero application frame twice. N5 submitted two
frames after a subsequent small MAC request where one was expected. These are
reproductions of open findings, not passing regression tests or ESP32 emulation.
They did not transmit RF. Network-counter/persistence coverage is still incomplete.

Audits confirmed canonical C++ bytes in all three build trees after removing only
the Arduino-generated initial #line directive. Enabled ELF has the downlink
callback and no beforeUplink/afterUplink/SDMMCFS symbols. Wiring checks pass with
unchanged J7 results. Protected Config, UI previews, photo, SVG-embedded photo,
wiring, telemetry/decoder/reference and historical archive are byte-identical;
hashes are in protected-sha256.json. No preview regeneration was necessary.

Physical line accounting for this checkpoint (unapplied proposals excluded):

| Category | Added | Removed |
|---|---:|---:|
| Application production | 1 | 1 |
| Dependency production | 0 | 0 |
| Tests | 73 | 0 |
| Test/build tooling | 20 | 1 |

The new test exercises the existing worker report interface, not ESP-Hosted/WiFi
thread scheduling. It does not prove the unresolved association/save policy.
No hardware, RF, stack safety or unattended-operation acceptance is claimed.
The task directory is intentionally retained while patch approvals and repair
work remain pending; no persistent build, extra checkout, global install or archive
was created. Unattended TTN remains NO-GO. Assembly/bench remain conditional on the
physical checklist and separate flashing/RF authorization.


## Approved N1/N5/N6 stage — 2026-10-25

The user approved these three concrete proposals, including N1's narrow vendor
error-return exception and N6's two extra application lines. They are implemented
locally on the same main HEAD. Remaining N4/N7/F8 proposals are unapplied and this
broader task remains incomplete. Earlier checkpoint evidence above is retained.

The single `tools/patch_dependency.py` replaces `tools/patch_radiolib.py`; the old
command in historical validation logs describes the earlier tool revision. Use:

```sh
python3 tools/patch_dependency.py radiolib-7.8.1 "$T/deps/RadioLib"
python3 tools/patch_dependency.py jpegdec-1.8.4 "$T/deps/JPEGDEC"
BUILD="$T" RADIOLIB_DIR="$T/deps/RadioLib" make test
make format
make format-check
python3 "$T/build.py" approved
clang-tidy --config-file=.clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Workers.cpp"
```

Both pinned original/patched source hashes are authoritative in patches/.
The integrity test reverses each patch in a disposable dependency-file tree,
verifies the original hash, then checks fresh apply, repeated apply and refusal of
modified input without changing that input. No installed library/SDK was edited.
No generic plugin/patch framework or compatibility shim was added. RadioLib's
zero-context diff presentation avoids copying trailing vendor whitespace into new
patch context; exact-source hashes remain mandatory before application.

The independent host suite passes. JPEG tests use the real decoder and generated
invalid SOF inputs under `-fsanitize=address,bounds -fno-sanitize-recover=all`;
baseline decoding, progressive rejection and16 invalid metadata cases pass. The
previous full-UBSan alignment diagnostic is retained rather than hidden: that run
stopped on the vendor's existing unaligned access, while bounds-only testing
isolated N1. No complete decoder-input safety claim is made.

The real-library N5 regression confirms required large MAC replies, subsequent
small/no-reply handling, callbacks and aggregate airtime. The N6 test now extracts
the final stop block as well as init/join blocks, modelling suspension at the task
boundary and checking the final posted diagnostic. It covers NVS open, radio and
switch init, invalid nonces, boot-ID write and negotiated1.0 stop paths. N1/N5/N6
regressions each fail before the relevant correction and pass after; their logs
are stored separately, not mistaken for successful baseline checks.

| Approved source configuration | Flash bytes | Static RAM bytes | Result |
|---|---:|---:|---|
| prev3, blank | 1,202,534 | 39,364 | PASS |
| prev3, dummy enabled | 1,209,668 | 39,436 | PASS |
| postv3, same dummies | 1,211,230 | 39,436 | PASS |

Exact commands/results and complete build output are in
validation-logs/rereview-fix/approved-*. No pinned versions, FQBN or board settings
changed. Target clang-tidy was attempted again on the final approved source and
failed on the same unsupported Espressif arch/compiler flags and missing target
C++ headers; no successful target analysis is claimed. Format and diff checks
pass. Protected-file/compiled-source/ELF checks pass; no reservation hook or
SDMMCFS symbol was restored.

An additional same-configuration prev3 enabled build passed with
`compiler.cpp.extra_flags=-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f -fstack-usage`,
using the same libraries/FQBN, jobs3, build path `$T/stack-approved`, TMPDIR
`$T/stack-approved-tmp` and cache `$T/stack-approved-cache`. Static frames:
radioTask3616, loadImage19120, applicationDownlink1120, JPEGParseInfo112,
parseDownlink384, main sendReceive80 bytes (wrapper288). The full relevant .su
entries and build output are saved. These are individual frame sizes, not maximum
stack depth or a watermark measurement;12KiB radio/24KiB media allocations are
unchanged and still require the physical checklist.

Physical line accounting versus this task's HEAD, including the prior status fix:

| Category | Added | Removed |
|---|---:|---:|
| Application production | 3 | 1 |
| Patched dependency production | 8 | 1 |
| Tests | 144 | 11 |
| Test/build tooling | 111 | 27 |

Dependency counts measure changed vendor source, not diff-file context/packaging.
N1 adds7 and N5 moves1 line; N6 adds2 application lines. Test/tooling growth adds
regressions and patch integrity checks; the old patch implementation was removed.
No N4/N7/F8 production proposal is included. Source hashes, line accounting,
protected hashes and command logs are saved under the same evidence directory.
Temporary task sources/builds are retained for continuation; no archive, install,
flash, RF transmission, TTN access, branch, commit or push was performed. UI,
pins/J7, protocol/decoder/reference and source photo remain unchanged. Physical
operation and unattended TTN acceptance remain unverified.


Publication check: after staging the new evidence, full git diff --cached --check
reports trailing spaces in byte-preserved raw tool output and vendor patch context
(including unapplied proposals). These data findings are retained, not reported as
a passing full staged whitespace check. Source/configuration/prose whitespace and
make format-check pass. No source or regression assertion was weakened. The
user separately authorized commit/push; prior local-only statements are historical.


## Accepted independent-review fixes — final validation, 2026-10-25

This section supersedes prior N4/N7 open-state descriptions for the selected fixes;
previous validation records remain historical. Baseline and current HEAD are
`1961cfa053d8ee950e1ba252cf9721e2f7833e6d`, main, with a clean pre-edit tree and
cached origin/main equal to HEAD. No ref change or publication was performed.
Detailed source traces, scope decisions and approvals:
[accepted-review-fixes.md](review/accepted-review-fixes.md).
Evidence: `validation-logs/accepted-review-fixes/`.

### Configuration, commands and results

`T=/tmp/codex/build/CrowPanelTelemetry/accepted-review-fixes`.
Exact six dependencies were freshly extracted into `$T/deps` from the previously
verified exact-release archives; archive SHA256 values matched the saved
`validation-logs/rereview-fix/dependencies.json`. CLI1.5.1 was reused read-only from
`/tmp/codex/build/CrowPanelTelemetry/rereview-fix/cli/arduino-cli` (`CLI` below).
Core3.3.12/IDF5.5.5 and installed SDK sources stayed read-only. Dependency versions
are unchanged: RadioLib7.8.1, INA2260.6.6, TinyGPSPlus1.0.3a, JPEGDEC1.8.4,
Arduino_GFX1.6.9, audio-driver0.3.1. No global install, upgrade or index update.

Baseline `make test` and `make format-check` passed before edits. Final commands:

```sh
python3 tools/patch_dependency.py radiolib-7.8.1 "$T/deps/RadioLib"
python3 tools/patch_dependency.py jpegdec-1.8.4 "$T/deps/JPEGDEC"
RADIOLIB_DIR="$T/deps/RadioLib" make test BUILD="$T/final-tests"
make format
make format-check
python3 tools/check_wiring.py /tmp/codex/build/CrowPanelTelemetry/rereview-fix/panel.sch \
  /tmp/codex/build/CrowPanelTelemetry/rereview-fix/panel.brd
TMPDIR="$T/blank-prev3-tmp" ARDUINO_BUILD_CACHE_PATH="$T/blank-prev3-cache" \
  "$CLI" compile --verbose --jobs 3 \
  --fqbn 'esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant=prev3' \
  --libraries "$T/deps" --build-path "$T/blank-prev3" .
# enabled-prev3/enabled-postv3: separate tmp/cache/build paths and variant;
# add exactly this property (dummy values, never used on air):
# --build-property 'compiler.cpp.extra_flags=-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f'
clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Application.cpp"
clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Workers.cpp"
clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Ui.cpp"
```

The final patch was regenerated against exact pristine7.8.1, retaining the existing
zero-context artifact style and updating both after-hashes. Final `make test`
verifies fresh apply, idempotence, tampered-source refusal and real-library hashes
for both patch profiles. No reservation hook, branding comment or marker macro.
No third dependency patch was introduced. All independent checks run even after
another fails; final suite and format check PASS. Vendor TinyGPSPlus's existing
fallthrough warning remains. ACK-selection, zero-counter and maximum-counter ACK
regressions failed against the corresponding earlier source and pass afterward;
`*-before.log` files are expected reproducer failures, not final test failures.

All three final-source firmware configurations PASS (exact argv/exits in
`build-results.json`, verbose logs named for each configuration):

| Configuration | Flash bytes / 3,145,728 | Static RAM bytes / 327,680 |
|---|---:|---:|
| prev3 blank credentials/shunt | 1,202,320 | 39,364 |
| prev3 dummy enabled | 1,209,266 | 39,436 |
| postv3 dummy enabled | 1,210,836 | 39,436 |

The three are build configurations of one sketch. Dynamic stacks/PSRAM/heap are
additional to static RAM. Every external Used library path resolves underT/deps;
bundled libraries resolve to core3.3.12. Canonical C++/headers match each build's
copies after Arduino's one-line prefix. Actual compilation-database preprocessing
also confirms both dummy key array initializers, rather than relying on verbose
console rendering. Enabled binaries link the downlink callback; all three lack
reservation hooks and SDMMCFS begin/end symbols. See source-and-symbol-audit.log.

Target clang-tidy was attempted on all three changed cpp files using the final
enabled-prev3 compilation database. All exit1: LLVM rejects xesppie and GCC-only
-fno-tree-switch-conversion/-fstrict-volatile-bitfields and cannot find target
cmath. This is incomplete target analysis, not a passing result. No flags/checks
were weakened. Exact diagnostics are retained in tidy-*.log.

### What these checks prove and leave open

Real patched RadioLib tests cover both unicast counter domains, authenticated
commit, failed MIC, independent first-zero state across session restoration,
legacy rejection,16-bit rollover,32-bit exhaustion, fresh OTAA reset and confirmed
ACK MIC input. Crypto uses the real library's own primitives; TTN interoperability
and independent crypto vectors are not established. Host RadioLibTime_t remains
64-bit; target-width elapsed loops and IRQ scheduling are not emulated.

WiFi tests exercise verbatim load/service code and command struct against STA/NVS
transport doubles, including rejected requests with the old link, raw link/IP,
SSID identity, rapid same-SSID change, open networks and NVS failures. They do not
model physical association timing or NVS flash power-loss recovery. One credential
blob replaces the two strings; legacy settings require one-time re-entry.
Amplifier tests exercise actual setup/service statements at GPIO/I2S boundaries.
The manufacturer display configuration comparison passes; it proves numerical
source alignment, not display/touch operation.

The retained ES8311 error-propagation defect is not hidden by audioReady. SD/Hosted
lifecycle concurrency and unbounded nested MAC recursion were explicitly excluded
by the user; the old coexistence/reset and stack risks remain. No host test
emulates ESP32, SDMMC contention, physical codec/amplifier, RF, or a live network.
All previous GPS/shunt/TCXO/power/antenna/connector/C6/P4/display prerequisites,
watermarks/latency and the full24h run remain unexecuted. No flash/RF permission.

Wiring check PASS with unchanged J7 result. Config.h, telemetry/decoder/reference
packet, UI previews, wiring graphics/radio-nets, historical raw reviews and delivery
ZIP match baseline SHA256. The photo and SVG-embedded photo are byte-identical;
photo SHA256 remains `5b2e2bdfefe5a38b7bbee5ef09f37c363579a8628f27df23f5702b5fcd6624ad`.
No preview regeneration was needed: UI geometry and generators did not change.
Formatter/analysis configurations remain unchanged. Project-owned diff whitespace
checks pass; raw verbose logs retain tool-emitted whitespace.

### Accounting and handover

Physical added/removed lines relative to main@1961cfa:

| Category | Added | Removed | Net |
|---|---:|---:|---:|
| Application production | 18 | 19 | −1 |
| Patched dependency source | 17 | 12 | +5 |
| Tests | 372 | 33 | +339 |
| Build/test tooling | 30 | 1 | +29 |

The approved two-bit state accounts for dependency growth. No application growth:
existing worker flags/timers and one record absorb the WiFi correction; amplifier
and display changes are neutral. Tests/tooling add boundary and real-library
regressions without a new framework. Patch-file hunk/context text is not counted
as dependency production; actual before/after sources were compared separately.
The separate accepted_independent_review entry in change-accounting.json preserves
historical entries. Changed files and detailed counts are saved with the evidence.

Evidence was copied and checked before removing this task'sT, including temporary
build trees, isolated dependency trees, caches and negative-test tree. No persistent
build, extra project checkout, archive or global install was created. The pre-existing
rereview-fix/self-review source/evidence collections were reused read-only and left
untouched. SHA256SUMS.json is regenerated in its existing sorted format/scope.
Work remains uncommitted on main@1961cfa; no push performed. Assembly is conditional
on physical identification/voltage checks. Bench flashing requires separate
authorization and those prerequisites. Unattended TTN remains NO-GO until retained
risks have measured acceptable limits or fixes and the full checklist passes.
