# Final validation — 2026-10-25

All three firmware builds below use the final canonical C++/headers, exact pinned
dependencies and the included RadioLib patch. Arduino-generated #line prefixes
were excluded when byte-comparing build inputs with the canonical files. Exact
input hashes and logs are in validation-logs/. Earlier build results were replaced.
No hardware, RF or TTN access was used.

## Configuration and builds

Arduino CLI1.5.1 (01f3d4f2b); esp32 core3.3.12 / ESP-IDF5.5.5;
Arduino_GFX1.6.9; RadioLib7.8.1 + patches/radiolib-7.8.1.patch;
INA2260.6.6; TinyGPSPlus tagv1.0.3a; JPEGDEC1.8.4; audio-driver0.3.1.
Core-bundled transports and IDF FAT/SDMMC supply the remaining interfaces.
The core/SDK and normal Arduino library installations were not edited.

Common FQBN: esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,
PartitionScheme=app3M_fat9M_16MB (3,145,728-byte app). ChipVariant=prev3 or postv3.
Credentials/shunt remain blank in Config.h. Enabled compiler overrides:
-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f.
These dummies were never flashed and cannot operate a real TTN registration.

| Final configuration | Flash bytes | Static RAM bytes | Result |
|---|---:|---:|---|
| prev3, blank | 1,202,652 | 39,364 | PASS |
| prev3, dummy enabled | 1,209,852 | 39,436 | PASS |
| postv3, dummy enabled | 1,211,422 | 39,436 | PASS |

Linker/static allocations exclude dynamic DSI/PSRAM, stacks, SD, WiFi and audio
allocations. Runtime heap, latency and performance were not measured. Compared
with the original builds, removal of the unused board catalog reduces linked
flash/static allocations; this is not a runtime performance claim.

## Commands and checks

Build root B=/tmp/codex/build/CrowPanelTelemetry/sd-repair; CLI=$B/arduino-cli;
dependency collection=$B/deps. Each configuration uses its own TMPDIR and build
cache. Exact CLI invocation pattern (substitute the configuration and variant):

```sh
python3 tools/patch_radiolib.py "$B/deps/RadioLib"
TMPDIR="$B/blank-tmp" ARDUINO_BUILD_CACHE_PATH="$B/blank-cache" \
  "$B/arduino-cli" compile --jobs 3 \
  --fqbn 'esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant=prev3' \
  --libraries "$B/deps" --build-path "$B/blank" .
# Enabled builds add exactly:
# --build-property 'compiler.cpp.extra_flags=-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f'
RADIOLIB_DIR="$B/deps/RadioLib" make test
make format-check
python3 tools/check_wiring.py "$B/panel.sch" "$B/panel.brd"
```

- PASS: encoder/reference/decoder, every fragment combination, valid zeros and
  invalid samples, range boundaries, DR5→DR0 progression and airtime arithmetic.
- PASS: actual SdStorage API-boundary tests with C6 slot1 pre-owned: power failure,
  early uncounted-slot failure, counted no-card failure, insertion/removal,
  three remounts, fixed shared3.3V, IO_MUX-only pins and default host flags,
  no formatting, and no second cleanup after ambiguous unmount failure.
- PASS: production PersistentNode and actual scheduling block compiled verbatim
  against real patched RadioLib. Simulated clock/RF/NVS/queues exercise TX ending
  exactly at millis0 and valid RX windows, wrapped deadlines, six failed joins,
  non-cumulative airtime, nested MAC frame pacing/counters, chronological nested
  downlinks, empty/max binary downlinks, TX-error nonce non-reuse, reboot recovery,
  failed nonce/airtime writes before RF, demo fairness and7+3 fragment ordering.
  A generic host's unsigned long differs from P4 width; uint32 application
  deadlines and wrapped HAL values are modeled, but this does not prove actual
  ESP IRQ/timer scheduling. No RF is emitted by these tests.
- PASS: fresh patch application, idempotent second invocation, tampered-source
  refusal, and verification of exact dependency hashes before integration tests.
- PASS: final Eagle audit of the pinned V1.2 .sch/.brd; all nine radio nets, all
  Config pins, rails, UART/I2C and J7 orientation remain unchanged.
- PASS: final linked ELF has no SDMMCFS::begin/end, while the production frame/
  downlink hooks are linked. No generic SD power-pin code can be reached through
  the removed SD wrapper. IDF source trace verifies width1 selects CLK43/CMD44/D039
  and does not configure GPIO45/46. SDK-pin static_asserts compile in all builds.
- PASS: canonical format check; all four applicable baseline configuration files
  remain byte-identical. Original photo and UI preview files are byte-identical;
  UI control geometry and colors are unchanged. Wiring text-only correction was
  regenerated from its script and visually inspected in the prior checkpoint.

## Analysis limitations and retained warnings

Target analysis was attempted with the actual enabled Arduino compilation DB:

```sh
clang-tidy -p "$B/enabled" "$B/enabled/sketch/Workers.cpp"
```

Debian LLVM21.1.8 rejects the Espressif xesppie RISC-V extension, GCC-only
-fno-tree-switch-conversion/-fstrict-volatile-bitfields, and does not resolve that
GCC target's C++ headers (cmath). Exit1; target analysis did not pass. The raw
log is clang-tidy-target.txt. No baseline checks were disabled or rewritten.

Supplemental host analysis used the same canonical checks on SdStorage.h
(as a standalone host translation unit), tests/sd-storage.cpp
with -Itests/sd-stubs -I., and tests/radio.cpp with the generated production class/
scheduler headers and real RadioLib includes. These complete, but are not a target
analysis replacement. Retained warnings: three canonical MaxSize=-1 option values
are rejected by this Clang version; test-double unnamed parameters/redundant void,
const/readability suggestions, float suffix/multiple declarations, pragma-once
style/standalone-header warnings, storage pointer-to-bool suggestions, and production
suggestions for const locals, explicit integer-to-bool, an uninitialized queue
receive destination (not read before successful receive), and a redundant cast in
the host queue stub. Full diagnostics are preserved. No correctness diagnostic was
reported by these supplemental runs; they are not clean-warning claims.

## Scope, accounting and remaining work

See correctness-status.md for every C1/H1–H2/M1–M5/L1–L5 finding and additional N1.
L1 retains protocol version1 and documents voltage-only local display. The old L2
reservation is replaced by the approved per-frame debt; legacy debt is respected.
Hardware-only UI freshness/touch faults and all electrical prerequisites remain
on hardware-checklist.md. No upload, actual panel/codec/SD/GPS/INA/RF operation,
C6 firmware compatibility, legal RF certification or TTN delivery is claimed.

Physical line deltas versus the delivered firmware / pre-SD checkpoint:

- Application production: +258 / −104 (net +154).
- Dependency patch: +26 / −12 (net +14); unmodified vendor sources are not shipped.
- Tests/fixtures: +548 / −0.
- Build/test/patch tooling: +69 / −4.

SD replacement accounts for70 net application lines (within approved40–80).
Remaining application corrections account for84; dependency net14 gives98
additional production lines (within the approved40–100 estimate). Growth is the
necessary explicit SD cleanup/resource ownership, single per-frame pacing/nonce
boundary and failure/freshness validation. It replaces the old SD wrapper and
post-call RF reservations; no old/new implementation remains active in parallel.
The wire format, pin assignments, credentials and UI visuals are preserved.
See change-accounting.json for categories and changed files.

Canonical location: /home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry.
The build/repair stage was outside Git. The user subsequently authorized creating
and pushing https://github.com/VOC-FH/CrowPanelTelemetry as a private repository,
with main as the initial branch. The canonical checkout remains this folder.
Publication adds repository metadata and historical review/delivery records only;
validated firmware sources remain unchanged. SHA256SUMS.json covers repository
files, excluding itself and Git internals. The original delivery snapshot remains
unchanged under artifacts/. No additional project checkout or persistent build exists.
Temporary sd-repair, radio-tests and local directories under
/tmp/codex/build/CrowPanelTelemetry/ hold only downloads/dependencies/builds/test
fixtures. Cleanup condition: final validation evidence copied and package verified.
They are removed on completion; all task changes remain in the canonical folder.
Unattended operation remains NO-GO until physical acceptance passes; separately
authorized bench bring-up is conditional on the unresolved component checks.
