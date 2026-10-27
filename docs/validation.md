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


## Protocol/parser correction checkpoint — 2026-10-26 (incomplete)

Baseline/main HEAD: `c86439247b2840ed94099a34bc8c72b722f0976f`; initially clean.
The user requested five fixes after the latest review. WiFi rejection feedback
and requested-versus-derived display-clock documentation are corrected. Join,
JPEG EXIF and RekeyInd changes are not applied pending the quantified growth/
patch-scope choices in [protocol-parser-fix-proposals.md](review/protocol-parser-fix-proposals.md).
This is not completion or a go-ahead for hardware/RF/unattended operation.

Temporary task rootT: `/tmp/codex/build/CrowPanelTelemetry/protocol-parser-fixes`.
Six isolated exact dependency archives matched the previous pinned archive hashes;
current RadioLib/JPEG patches applied and hashes verified. Core3.3.12/IDF5.5.5,
CLI1.5.1 and all library versions unchanged; installed sources are read-only.
No project source copy/checkout, branch change, install, archive, commit or push.

Baseline and current full host suites pass; baseline/current format checks pass.
The new offline rejected-request test failed before and passes after the reshape,
including uint32 rollover. No assertion weakened. Current enabled-prev3 compile
passes: flash1,209,284/3,145,728; static RAM39,436/327,680. No blank-prev3/postv3
build is claimed for this partial checkpoint. Target clang-tidy was attempted on
Workers.cpp using that build's compilation database; it again exits1 for unsupported
ESP xesppie/GCC flags and missing target cmath. No analysis settings weakened.

Commands (logs in validation-logs/protocol-parser-checkpoint/):

```sh
RADIOLIB_DIR="$T/deps/RadioLib" make test BUILD="$T/baseline-tests"
make format-check
BUILD="$T/wifi-before" python3 tools/test_wifi_worker.py
BUILD="$T/wifi-after" python3 tools/test_wifi_worker.py
RADIOLIB_DIR="$T/deps/RadioLib" make test BUILD="$T/current-tests"
TMPDIR="$T/compile-tmp" ARDUINO_BUILD_CACHE_PATH="$T/compile-cache" \
  /tmp/codex/build/CrowPanelTelemetry/rereview-fix/cli/arduino-cli compile --verbose --jobs 3 \
  --fqbn 'esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant=prev3' \
  --libraries "$T/deps" --build-path "$T/partial-enabled-prev3" \
  --build-property 'compiler.cpp.extra_flags=-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f' .
clang-tidy -p "$T/partial-enabled-prev3" "$T/partial-enabled-prev3/sketch/Workers.cpp"
```

Application physical delta2 added/2 removed; tests11 added/0 removed. No dependency
production change and no build/test-tooling change yet. Patch proposals are prose,
not applied code. Protected hashes and canonical build copies match. Evidence is
saved, whileT/deps is retained for continuing the pending approvals. Large temporary
compile products/caches are removed after evidence is copied; there is no persistent
build. No physical test or ESP32 emulation, flashing, RF or TTN operation occurred.

## Protocol/parser corrections — final 2026-10-26

This final revision supersedes the incomplete checkpoint above. All five selected
corrections are implemented following explicit approval of P1–P3. See the
[dispositions](review/protocol-parser-fix-dispositions.md) for source traces,
invariants, scope, individual tests and approved trade-offs. Canonical main HEAD
remains `c86439247b2840ed94099a34bc8c72b722f0976f`; changes are uncommitted.
Remote/default/current: VOC-FH/CrowPanelTelemetry, main/main; locally recorded
origin/main is at HEAD (0 ahead/0 behind). No fetch, branch, commit or push.

Final evidence: [validation-logs/protocol-parser-fixes](validation-logs/protocol-parser-fixes).
Baseline suite and format passed. New WiFi, join-ceiling and RekeyInd regressions
failed against baseline implementations; the old EXIF reader also produced a host
ASan fault. All final independent checks pass without weakened assertions. Real
pinned libraries run against host transport stubs/extracted production decisions;
this is not ESP32 emulation or a physical operation test. The join simulation
covers 60 days and sliding-day airtime windows with two seeded RNG streams.
Target RNG entropy and 32-bit RadioLib elapsed loops are not thereby proven.

Versions unchanged: Arduino CLI1.5.1, esp32 core3.3.12/IDF5.5.5; RadioLib7.8.1,
JPEGDEC1.8.4, INA2260.6.6, TinyGPSPlusv1.0.3a, Arduino_GFX1.6.9,
arduino-audio-driver0.3.1. Six archive hashes matched the exact source provenance
in validation-logs/accepted-review-fixes/dependencies.json. Fresh patch apply,
idempotence and modified-source refusal pass for both updated dependency patches.
The manifests retain pristine before hashes and final after hashes. Isolated
patched source hashes are saved in this task's evidence. No installed source or
library was modified. Exact clock/RNG source hashes were added to
correctness-source-hashes.json; no dependency version changed.

Commands (T is /tmp/codex/build/CrowPanelTelemetry/protocol-parser-fixes):

```sh
RADIOLIB_DIR="$T/deps/RadioLib" make test BUILD="$T/final-tests"
make format-check
python3 "$T/final-builds.py"
clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Workers.cpp"
python3 tools/check_wiring.py /tmp/codex/build/CrowPanelTelemetry/rereview-fix/panel.sch /tmp/codex/build/CrowPanelTelemetry/rereview-fix/panel.brd
python3 "$T/final-audit.py"
```

The evidence includes final-builds.py and final-build-results.json with every exact
CLI invocation. All use --libraries "$T/deps", separate build/TMPDIR/cache paths,
and FQBN esp32:esp32:esp32p4 with FlashSize=16M, PSRAM=enabled,
PartitionScheme=app3M_fat9M_16MB. Enabled variants add only dummy overrides
`-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f`.
All six selected external libraries resolve under task deps; bundled libraries
resolve under core3.3.12. Final production bytes match all three build inputs
(after removing Arduino's initial #line directive).

| Configuration | Result | Flash / 3,145,728 bytes | Static RAM / 327,680 bytes |
| --- | --- | ---: | ---: |
| blank prev3 | PASS | 1,201,852 | 39,364 |
| dummy enabled prev3 | PASS | 1,208,846 | 39,436 |
| dummy enabled postv3 | PASS | 1,210,416 | 39,436 |

Full host test/format/wiring/audit checks exit0. Target clang-tidy exits1:
installed LLVM rejects xesppie, -fno-tree-switch-conversion,
-fstrict-volatile-bitfields and cannot locate target cmath. No settings weakened;
no clean target static-analysis result is claimed. git diff --check reports only
four literal deleted vendor whitespace lines inside unified patch files; these
must remain exact for pristine hash-checked application. No project-source
whitespace error was reported. TinyGPSPlus emits its existing fallthrough warning.

Enabled ELF symbols include the application downlink callback; no beforeUplink,
afterUplink or SDMMCFS begin/end symbols. No marker macro or patch-branding comment.
Protected-hash audit passes for Config/pins/J7, Application.cpp, Ui.cpp, protocol/
reference packet, source photo, wiring/UI previews, canonical formatter settings,
raw historical reviews and the historical delivery ZIP. SVG-embedded photo bytes
match the source photo. Wiring check passes with unchanged J7 routing. Generators
and visual assets were not changed or regenerated because their inputs are unchanged.

| Physical lines relative to baseline | Added | Removed |
| --- | ---: | ---: |
| Application production | 8 | 7 |
| Patched dependency source | 60 | 199 |
| Tests | 387 | 30 |
| Build/test tooling | 26 | 2 |

The approved net +1 application line supplies the RNG include; existing retry and
deadline state absorb the policy. JPEG deletes126 lines; RadioLib adds60/removes73,
including the queue's paired buffer/length reset and shared deletion authority.
The final entry in change-accounting.json is cumulative and supersedes the partial
entry. Patch hunk/context text is excluded. Files/counts are recorded in
final-accounting.json; protected hashes in final-audit.json.

Evidence was copied before removing this task's temporary root, including its
isolated dependencies, build trees and caches. No persistent build, project copy,
checkout, global install or new archive was created. Pre-existing rereview-fix and
self-review evidence/source collections were left untouched. SHA256SUMS.json was
regenerated in its existing sorted scope, excluding itself and Git/ignored files.
All changes remain local on main; no commit or push performed.

Assembly is conditional on physical prerequisite checks. Bench flashing needs
separate authorization. Unattended TTN stays NO-GO until the retained risks and
full hardware checklist (including 24 h run) pass. No physical/RF tests were run.

## Focused session-restart join-budget fix — 2026-10-26

Baseline main@b80bf07c028729f12089f1b27d4c5d032107150c. The user authorized only
this finding's correction, push and a scoped Claude follow-up. Existing uncommitted
b80bf07 review files and their provenance were preserved. See
[trace and regression](review/join-restart-fix.md). All other findings are unchanged.

Production delta is one constant replacement in Workers.cpp: after successful
OTAA, joinRetry becomes3840000ms instead of60000ms. Cold boot still uses60000.
The next in-run session revert consequently rejoins at DR0, then doubles failure
backoff toward the existing24h cap. No new state, timer, patch or ownership change.
Test delta46added/16removed physical lines; application1/1; dependency/tooling0/0.

Evidence: validation-logs/join-restart-fix/. Baseline real-library radio tests pass.
The new regression fails before correction at the11h-window8.7s assertion; final
real-library and full independent host suites pass. Tests perform an authenticated
successful join,64 service uplinks and actual ADR-limit deactivation before each
failed-rejoin series, at10h and30h, for two RNG streams and60days. Existing cold-boot
windows, signed-wrap/jitter/nonce/fragment/other dependency checks remain passing.
No RF, hardware, entropy or target-width RadioLib timing claim follows.

Commands (T=/tmp/codex/build/CrowPanelTelemetry/join-restart-fix):

```sh
BUILD="$T/baseline" python3 tools/test_radio.py "$T/deps/RadioLib"
BUILD="$T/before" python3 tools/test_radio.py "$T/deps/RadioLib" # expected failure, new regression/old worker
BUILD="$T/after" python3 tools/test_radio.py "$T/deps/RadioLib"
make format
make format-check
RADIOLIB_DIR="$T/deps/RadioLib" make test BUILD="$T/tests"
python3 "$T/builds.py"
clang-tidy -p "$T/enabled-prev3" "$T/enabled-prev3/sketch/Workers.cpp"
python3 "$T/audit.py"
```

Six exact pinned dependency archives matched saved provenance; both current
patches applied and hashes verified. CLI1.5.1, core3.3.12/IDF5.5.5 and all library
versions are unchanged; installed sources are read-only. The official1.1 PDF
matched the previous published hash; spec-source.json records its URL/hash.
Build scripts/results include every exact CLI command, --libraries "$T/deps",
separate TMPDIR/cache/build paths, common16M/PSRAM/app3M_fat9M_16MB options and
only the established dummy credentials/shunt overrides on enabled builds.

| Configuration | Result | Flash / 3,145,728 bytes | Static RAM / 327,680 bytes |
| --- | --- | ---: | ---: |
| blank prev3 | PASS | 1,201,796 | 39,364 |
| dummy enabled prev3 | PASS | 1,208,800 | 39,436 |
| dummy enabled postv3 | PASS | 1,210,362 | 39,436 |

Full host suite, format and source/symbol/library/protected-file audits exit0.
Target clang-tidy was attempted on the same build compilation database; exit1
again reports unsupported ESP xesppie/GCC flags and missing target cmath. Analysis
settings were not weakened. C++/headers match all three build inputs after Arduino's
#line insertion. External dependencies resolve under task deps; enabled callback
is linked; no reservation hooks or SDMMCFS begin/end symbols. Pins/Config, J7,
Application/Ui, visual/source-photo assets, protocol/reference, both patch profiles,
formatting files, historical ZIP and pre-existing raw review records match baseline
hashes. No unrelated source changes or global installation; no new archive/checkouts.

Evidence is saved before removing this task's temporary dependencies/builds/caches
following Claude's focused read-only follow-up. No persistent build was created.
The existing review history remains separate from the implementation disposition.
All assembly, supervised flashing and unattended-operation conditions remain as
previously documented; no hardware/RF operation was executed or authorized.

## Student documentation and separate learning material — 2026-10-26

Baseline main@f59fd3f, the local commit recording Claude's focused review. The user
requested that commit and then detailed documentation plus separate learning
material. The reference manual is in docs/manual/; the six-lesson learning path,
worksheet and instructor guide are in learning/. README links both and clarifies
that both patches are required and replacement-session uplinks initially use DR0.
No application, test, build/test-tooling, dependency patch or visual changes.

Evidence is in validation-logs/student-material/. A task-local Python check verified
17 Markdown files (including the root README),92 local links/anchors and balanced
code fences. The two caption/threshold C++ blocks were extracted verbatim and
compiled with the real Reading type; NaN, zero, below/exact/above threshold cases
pass. This checks the documented expression, not touchscreen rendering. Existing
codec/reference and Node decoder tests pass; teacher answer values/indices and
packet metadata were checked against the actual decoder output. The existing
airtime calculator passes and agrees with the teaching calculations. make
format-check passes without modifying source or style settings.

Exact source-derived checks used c++ -std=c++17 -Wall -Wextra -Werror -I. on the
extracted caption harness, c++ -std=c++17 -I. tests/codec.cpp, node tests/decoder.js
tests/reference-packet.json, the documented Node decode invocation, and python3
tools/airtime.py. Compilations and generated checks lived only under
/tmp/codex/build/CrowPanelTelemetry/student-material. After evidence was copied,
that task directory was removed. No new global package, archive, checkout or
persistent build was created. Prior source/evidence collections remain untouched.

Firmware, tests/tools, patches, Config/pins, UI/wiring assets/source photo,
formatting settings, historical ZIP and committed raw review records match baseline
hashes. Application/test/tooling physical delta0added/0removed. No Arduino rebuild
was needed for prose changes; no new firmware-build or physical-operation result
is claimed. SHA256SUMS.json was regenerated in its existing scope. Documentation
changes remain local/uncommitted; review commit f59fd3f has not been pushed by this
task. All prior physical/RF/unattended-operation limitations remain in force.

## PlatformIO alongside Arduino IDE/CLI — 2026-10-26

The user requested both build workflows. Canonical main remains at f59fd3f;
pre-existing uncommitted student documentation was preserved and linked to the
new PlatformIO chapter. No firmware, tests, library patch, pin or UI asset changed.
The new platformio.ini uses the root sketch/C++ sources and symlink package
references to the same DEPS collection supplied to Arduino CLI. No alternate
firmware tree, build hook, custom board variant or partition copy was added.

Versions: PlatformIO Core6.2.0 (requirements-platformio.txt), pioarduino55.03.312,
Arduino3.3.12/IDF5.5.5, original six external release tags and both existing patches.
Both workflows use16MB, PSRAM, QIO/80MHz, the packaged app3M_fat9M_16MB partition,
USB CDC disabled and the corresponding prev3 360MHz or postv3 400MHz SDK.
Core6.2/toolchain packages were isolated under the task directory; the user's
installed Arduino sources were read-only. No global install/upgrade/downgrade.

Final build results (flash maximum3,145,728 bytes; reported static-RAM maximum327,680):

| Workflow | Configuration | Result | Flash bytes | Static RAM bytes |
| --- | --- | --- | ---: | ---: |
| Arduino CLI1.5.1 | blank prev3 | PASS | 1,201,828 | 39,364 |
| Arduino CLI1.5.1 | dummy enabled prev3 | PASS | 1,208,824 | 39,436 |
| Arduino CLI1.5.1 | dummy enabled postv3 | PASS | 1,210,394 | 39,436 |
| PlatformIO Core6.2.0 | blank prev3 | PASS | 1,188,942 | 39,636 |
| PlatformIO Core6.2.0 | dummy enabled prev3 | PASS | 1,195,090 | 39,716 |
| PlatformIO Core6.2.0 | dummy enabled postv3 | PASS | 1,196,652 | 39,716 |

Evidence is in validation-logs/platformio-support/. The JSON/scripts record exact
commands and dummy overrides; only delivered blank configuration or
`-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f` were used.
No real credentials, upload or RF. Final PlatformIO commands are ordinary pio run;
ELF generation followed by a failed binary step was not counted as a pass.

Diagnostics retained: Core6.2/SCons4.11.1 with pio run -v failed in the framework's
binary-generation print action (`_Null` + str). A task-local Core6.1.19 probe was
rejected by the platform's >=6.2.0 engine constraint. The supported normal-mode
Core6.2 commands finish binary generation in all three configurations. No framework
or patch-scope change was made. The docs identify this verbose-mode limitation.
The default platform manager's IDE extension UI was not exercised; its documented
Core/path/environment setup is supplied. Arduino IDE GUI operation was not rerun;
its equivalent pinned CLI configurations above were compiled.

Commands used, with T=/tmp/codex/build/CrowPanelTelemetry/platformio-support:

```sh
python3 "$T/arduino-builds.py"
python3 "$T/final-pio-builds.py"
RADIOLIB_DIR="$T/deps/RadioLib" make test BUILD="$T/host-tests"
make format-check
DEPS="$T/deps" PLATFORMIO_CORE_DIR="$T/core" PLATFORMIO_WORKSPACE_DIR="$T/workspace" \
  pio project config --json-output
DEPS="$T/deps" PLATFORMIO_CORE_DIR="$T/core" PLATFORMIO_WORKSPACE_DIR="$T/workspace" \
  pio project metadata -e crowpanel-prev3 --json-output
python3 "$T/audit.py"
```

PIO metadata was also captured for postv3, with the same dummy environment flags
as its enabled build. Production-SDK/CPU/PSRAM/USB flags and both environments'
.pio-link sources were checked. PlatformIO compiled only the four shared application
translation units; tests/tools/docs/learning/build/checkouts were excluded.
All canonical C++/headers match Arduino build copies after the initial #line.
Partition binaries match across corresponding workflows. Enabled callbacks are
linked; no beforeUplink/afterUplink or SDMMCFS begin/end symbols. Both patches'
after hashes remain exact. The host suite and format check pass. Documentation
checks cover18 Markdown files,101 local links/anchors and balanced code fences.

Framework/core/variant/WiFi/partition sources, both SDK configurations and clock
headers were compared. Additionally3155 shared SDK header/archive/linker/flag files
per target and the C++ compiler executable match the installed Arduino baseline.
No binary identity is claimed; build-system board/version macros differ and sizes
are reported separately. Actual hardware, radio timing, SDK concurrency and stack
limits are unchanged/unverified. Existing target LLVM limitations remain; no C++
source changed to require a new target-analysis claim.

PlatformIO creates a transient .ino.cpp beside the sketch, then deletes it on exit.
The docs require one active builder at a time in this working directory; this
support means alternative workflows alongside each other, not a guarantee of
simultaneous source scans. No generated .ino.cpp remains or is committed.

Physical line accounting for this task: application0added/0removed, tests0/0,
dependency source0/0, build/test scripts0/0; new build configuration52added/0removed.
AGENTS.md records the build layout and common patch authority. Existing student
material, Config/pins/J7, wire protocol, firmware, tests/tools, patches, formatting,
raw reviews, UI/wiring images/source photo and historical ZIP remain preserved.
Only relevant documentation links/setup additions were made to the student material.

After evidence was copied, this task's temporary builds, platform/tool packages,
experimental Core virtual environment and caches were removed. No persistent build,
project copy, branch change or global installation remains from validation. The
pre-existing rereview/self-review source collections were left untouched. No commit
or push was performed for PlatformIO or the uncommitted learning material;
SHA256SUMS.json was regenerated. All changes remain in the canonical checkout.


## Student compilation walkthrough (2026-10-26)

Added learning/compiling-the-template.md as the student-facing first-build
procedure. Shared source/library/patch preparation precedes three alternative
routes: Arduino IDE, Arduino CLI (Makefile or direct command), and PlatformIO
(terminal or VS Code). Lesson 1, worksheet, instructor guide, manual navigation
and the root README now link the procedure. Reference documentation and learning
material remain separate.

Validation used a task-local Python check under
/tmp/codex/build/CrowPanelTelemetry/student-build-guide, invoked from the canonical
repository with `python3 "$T/check.py"` (T is that temporary directory). It checked
19 Markdown files, 122 local links/anchors, balanced fences and ten new Bash
blocks using `bash -n`. Result: PASS. It also compared the guide's FQBN, environment
names, partition and platform/Core pins with Makefile, platformio.ini and
requirements-platformio.txt. Installed core3.3.12 boards.txt confirms the actual
USB-OTG menu label, both silicon/CPU selections and partition menu text.
Results are saved in docs/validation-logs/student-build-guide/checks.json.

The new commands were syntax/static checked, not executed as package installation,
GUI operation or another firmware build. Existing six-configuration build evidence
in the preceding section remains the compile evidence; no production/configuration
change warrants rebuilding it for this prose-only task. No physical operation,
flashing or RF was performed. The IDE/CLI setup links were checked against the
linked official Arduino installation and Boards Manager documentation.

Application, dependency source, tests, build configuration and build/test tooling:
0 physical lines added, 0 removed in each category. Baseline byte comparisons
confirm all pre-existing non-documentation files, Config/pins/J7, patches, wire
protocol, source photo, UI/wiring assets, historical archive and raw reviews remain
unchanged. Pre-existing uncommitted documentation/PlatformIO work is preserved.
SHA256SUMS.json was regenerated over the existing manifest scope. Work remains
uncommitted on main at f59fd3f; no commit, push or installation was performed.
The task's temporary checks were removed after copying their result; no build,
checkout or globally installed package was created for this task.


## Two-document student edition (2026-10-26)

Supersedes the split document layout described in the preceding student-material
entries. There are now exactly two course documents: docs/documentation.md and
learning/learning-material.md. The first contains the shared preparation and
complete Arduino IDE/CLI and PlatformIO compile routes, hardware/configuration
reference, code guide, customization examples, data formats, troubleshooting and
glossary. The second contains the six lessons, project worksheet and teacher
notes/answer guidance. The former docs/manual directory and split learning files
were removed after merging; they were uncommitted additions from this task.
Historical review records and validation evidence were left intact.

The prose now introduces background jobs as jobs with specific responsibilities,
queues as waiting lists, and the Model as the readings/status used by the screen.
It explains specialist terms before their detailed reference tables and marks the
binary-packing lesson as optional. Technical identifiers, commands, worked C++
examples, electrical requirements and known verification limits are retained.
Duplicate build procedures were consolidated rather than repeated in lessons.
README exposes only the two course-document entry points.

Ran `python3 "$T/check.py"` with
T=/tmp/codex/build/CrowPanelTelemetry/consolidate-student-docs from the canonical
repository. PASS: two course files, root README plus both documents checked,
62 local links/anchors, balanced fences and 12 Bash blocks checked with `bash -n`.
All 295 pre-existing non-documentation files compared byte-identical against the
start-of-task hashes, including production, tests/tools, dependency patches,
PlatformIO configuration, pins/protocol and image/archive assets. An initial
checker expectation incorrectly counted four C++ examples; inspection confirmed
three (threshold, caption and queue command), and the checker was corrected.
The three examples remain present. Final result is saved in
docs/validation-logs/student-document-consolidation/checks.json.

No firmware build or hardware test was repeated for this prose-only change.
Application, tests, dependency source, build configuration and build/test tooling:
0 physical lines added and 0 removed in every category. Pre-existing working-tree
changes are preserved except for the explicitly requested consolidation of the
uncommitted course documents. SHA256SUMS.json was regenerated using its existing
scope. The temporary checker and scratch files were removed after saving evidence.
No package installation, source copy, branch change, commit or push was performed;
canonical main remains at f59fd3f with documentation/PlatformIO work uncommitted.


## Student wiring integration and sans-serif PDFs (2026-10-26)

The two course documents now include panel/module wiring: documentation contains
the connection tables, complete J7 orientation, existing annotated photo and the
conditional GPS/sensor/radio prerequisites. Learning material includes a paired,
power-off wiring activity and a worksheet check. All 56 rows from the existing
wiring-reference tables are present unchanged. docs/wiring.md remains the detailed
engineering reference. No pin assignment or physical verification status changed.

Rendered output/pdf/documentation.pdf (35 pages) and
output/pdf/learning-material.pdf (18 pages) using Pandoc3.11, XeTeX from TeX Live
2026/Debian and Graphviz14.1.2. Body/headings use DejaVu Sans at11pt; command/code
text uses DejaVu Sans Mono. A symbol-only AMS font appears in the documentation,
but there is no serif body/heading font. Both PDFs contain clickable contents,
named destinations and links to the companion PDF. Keep them together.
The existing annotated wiring PNG is embedded without editing its bytes. The
Mermaid data-flow source is rendered through Graphviz for the LaTeX edition.

Reproduction from the canonical repository root:

```sh
python3 tools/render_documents.py
python3 tools/check_wiring.py /tmp/codex/build/CrowPanelTelemetry/rereview-fix/panel.sch /tmp/codex/build/CrowPanelTelemetry/rereview-fix/panel.brd
make format-check
```

The second command uses the pre-existing pinned schematic/board source collection;
source hashes and the unchanged nine-radio-GPIO result are saved with the evidence.
The renderer runs Pandoc to generate LaTeX and XeLaTeX three times for contents and
references, using scratch under /tmp/codex/build/CrowPanelTelemetry/student-pdfs.
Dependencies were already installed; no install/upgrade, firmware compile, upload,
RF operation or physical check occurred in this task.

Quality checks: all53 final pages rendered with pdftoppm and visually inspected in
contact sheets. The final anchor-only correction was rendered again; all53 page
images were pixel-identical to those inspected. Final XeLaTeX logs contain no
Overfull boxes, missing characters or undefined references. The initial passes
found a heading/HTML spacing issue, an image path issue, unwrapped code and PDF
cross-document destinations; all were corrected before delivery. Current PDF
links were checked against actual named destinations in both PDF files.

A task-local checker, invoked as `python3 "$T/check_final.py"`, passed74 local
Markdown links,12 Bash syntax blocks,56 exact wiring-table rows, page text bounds,
PDF fonts and destinations.17 protected source/assets/patch files remain
byte-identical to HEAD, including Config/pins/J7, protocol and original photo.
Existing firmware/tests/dependency patches and historical ZIP were preserved.
make format-check and git diff --check passed. The two documents' Markdown needed
blank lines around explicit HTML anchors for correct Pandoc heading recognition;
those formatting corrections are in the editable source too.

Application production0added/0removed; tests0/0; dependency source0/0.
New rendering tooling144added/0removed physical lines. It generates rendering-only
LaTeX/filter files under the temporary task directory and does not change firmware
or library installation. PlatformIO's52 configuration lines and earlier six-build
evidence are accounted for in the preceding entry; they were not rebuilt for prose.
Evidence is in docs/validation-logs/student-pdfs. The temporary rendering files and
page images were removed after copying final evidence. The two PDFs and renderer
are retained. No additional project checkout or branch was created. Publication
of the pending student documentation/PlatformIO work on main is authorized by the
user's push request; the final handover reports the resulting commit/push state.


Publication validation note: the final staged diff includes raw compiler and
XeLaTeX logs with trailing spaces emitted by those tools. Full staged
`git diff --cached --check` therefore reported whitespace warnings in logs;
the earlier unstaged check had not included new log files. The initial commit
proceeded because the shell command did not stop at that failed check. The raw
logs remain unchanged as evidence. `git diff HEAD^ HEAD --check -- .
':!docs/validation-logs/**'` passed for all authored files. This is a recorded
raw-log whitespace exception, not a clean full-diff result. PDF rendering,
font/link/bounds checks, wiring checks and format-check remain passing.


## Included manufacturer design files (2026-10-26)

Downloaded Elecrow V1.2 EAGLE .sch/.brd and the schematic PDF directly from
manufacturer revision cfcbfdbb1fd05997bc5e24dfeaece7fc44b6f61e into
`docs/hardware/elecrow-v1.2/`. provenance.json records exact immutable URLs,
original filenames, upstream Git blob IDs, bytes and SHA-256 values. The copies
are unedited. Their Git blob IDs were independently recomputed from the bytes;
the schematic and board SHA-256 values match the earlier wiring audit. The vendor
PDF opens as one landscape page. The supplied format is EAGLE CAD, not SPICE.

Added local links and a plain-language explanation in the existing documentation,
a connector-tracing exercise in the learning material, and links from the wiring
page/README. The two-document student structure is unchanged. Also replaced the
misleading phrase battery charging telemetry: charging is onboard hardware;
the template does not read battery/charger status. No battery feature was added.

Commands run from the canonical repository:

```sh
python3 tools/check_wiring.py docs/hardware/elecrow-v1.2/panel-v1.2.sch docs/hardware/elecrow-v1.2/panel-v1.2.brd
BUILD=/tmp/codex/build/CrowPanelTelemetry/hardware-files/render python3 tools/render_documents.py
make format-check
```

All passed. Task-local `python3 "$T/check.py"` checked95 local Markdown links,
all three file hashes/Git blob IDs, XML parsing, vendor PDF opening,56 unchanged
wiring-table rows, PDF destinations/fonts/text bounds and protected-file hashes.
Result PASS. The regenerated course PDFs remain35 and18 pages. All53 pages were
rendered with pdftoppm and visually reviewed in contact sheets; the downloaded
vendor schematic was rendered and inspected too. No clipping, overlapping content,
missing glyphs, overfull boxes or unresolved references were found. The PDFs keep
DejaVu Sans body/headings and DejaVu Sans Mono code. Evidence is retained in
`docs/validation-logs/hardware-files/`; raw renderer logs retain tool whitespace.
Authored-file diff whitespace checks exclude those raw logs and the unedited
manufacturer CAD files. The latter also contain upstream trailing whitespace;
these bytes are deliberately preserved so their source hashes still match.

No firmware, test, tool, dependency patch, board setting, pin, radio route, image
or historical ZIP changed. Application/test/dependency/build-configuration/tooling
physical line changes are0added/0removed in each category. No firmware rebuild,
installation, flashing, RF or hardware operation was performed for this document
and vendor-asset addition. SHA256SUMS.json was refreshed. The task temporary
rendering/check directories were removed after evidence was saved; no extra
checkout or branch was created. Commit/push on main is explicitly authorized.


## Supplied lecture PDFs (2026-10-27)

Added docs/slides/PCO.pdf and docs/slides/IntMinor-2026.pdf, copied unchanged
from the two user-supplied files. PCO is193425086 bytes/389 pages; IntMinor-2026
is103142941 bytes/111 pages. Both open with pdfinfo. SHA-256 comparison against
the supplied files passed. No PDF was rendered, recompressed or edited.

PCO exceeds GitHub's100MiB regular-file limit. Both slides are tracked individually
using the already-installed Git LFS3.8.0; no other PDF was migrated. `git lfs install
--local` configured only this repository and its previously absent pre-push hook;
no global software/settings were installed or upgraded. README links both slides
and gives the selective LFS download command. The checksum manifest hashes real
PDF contents. Firmware, pins, tests, build configuration and the two authored
student documents are unchanged; no compile/hardware/RF test applies to this
binary-document addition. Production0added/0removed; tests0/0. No extra checkout
or branch was created. Publication on main is authorized by the user's request.
