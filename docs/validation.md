# Deep-review validation — 2026-10-25

Canonical checkout:
`/home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry`.
Private repository: `git@github.com:VOC-FH/CrowPanelTelemetry.git`.
Default/current branch `main`; HEAD remains
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
