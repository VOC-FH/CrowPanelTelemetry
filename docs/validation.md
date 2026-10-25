# Final validation — 2026-10-25

This revision removes the per-physical-transmission airtime reservation policy at
user request. Nested application downlinks and the accepted RadioLib timing and
airtime corrections remain. The patch comment, project marker macro, build guard,
reservation callbacks and airtime NVS reads/writes are removed. DevNonce remains
reserved before joining; it is unrelated to airtime reservations.

Canonical checkout: /home/voc/Documents/Hagenberg/IntMinor/2026/codeing/CrowPanelTelemetry.
Repository: https://github.com/VOC-FH/CrowPanelTelemetry (private).
Task branch: fix/remove-airtime-reservations, based on main at
1e3cb016c8180f681635c0d69b45329ef8e01dd5. No other open PR existed at task start.
The user authorized fixing and pushing; no merge, installation, flash or RF was
performed. No alternate checkout was created.

## Configuration and fresh builds

Arduino CLI1.5.1 (01f3d4f2b); esp32 core3.3.12 / ESP-IDF5.5.5;
Arduino_GFX1.6.9; RadioLib7.8.1 + patches/radiolib-7.8.1.patch;
INA2260.6.6; TinyGPSPlus tagv1.0.3a; JPEGDEC1.8.4; audio-driver0.3.1.
The core/SDK and normal Arduino libraries were not installed, upgraded or edited.
Exact tags were downloaded only into the isolated task dependency collection.

Common FQBN: esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,
PartitionScheme=app3M_fat9M_16MB (3,145,728-byte app). ChipVariant=prev3 or postv3.
Delivered Config.h still contains blank credentials and shunt configuration.
Enabled builds use dummy overrides only:
-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f.

| Final configuration | Flash bytes | Static RAM bytes | Result |
|---|---:|---:|---|
| prev3, blank | 1,202,142 | 39,364 | PASS |
| prev3, dummy enabled | 1,209,106 | 39,436 | PASS |
| postv3, dummy enabled | 1,210,676 | 39,436 | PASS |

These are three configurations of the same sketch. Runtime heap, real task
latency and hardware behavior were not measured. Dynamic display/PSRAM, stacks,
SD, WiFi and audio allocations are additional to static RAM.

## Commands and evidence

B=/tmp/codex/build/CrowPanelTelemetry/remove-reservation. Dependencies are in
$B/deps. Each build uses its own task-local temporary/cache/build directory.
Full commands and fresh output are in validation-logs/{default-prev3,
enabled-prev3,enabled-postv3}.txt. Representative invocation:

```sh
python3 tools/patch_radiolib.py "$B/deps/RadioLib"
TMPDIR="$B/default-prev3-tmp" ARDUINO_BUILD_CACHE_PATH="$B/default-prev3-cache" \
  "$B/arduino-cli" compile --jobs 3 \
  --fqbn 'esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant=prev3' \
  --libraries "$B/deps" --build-path "$B/default-prev3" .
# Enabled configurations add:
# --build-property 'compiler.cpp.extra_flags=-DDEV_EUI=1ULL -DAPP_KEY={0x01} -DNWK_KEY={0x02} -DINA_SHUNT_OHMS=0.1f'
make format
make format-check
RADIOLIB_DIR="$B/deps/RadioLib" make test
python3 tools/check_wiring.py "$B/panel.sch" "$B/panel.brd"
```

- PASS: all independent host checks: encoder/reference/JS decoder, fragments,
  sensor saturation, join DR progression, airtime calculations and SD lifecycle.
- PASS: real patched RadioLib with production node and scheduling code extracted
  verbatim. Six failed joins each account112ms, never cumulative; volatile holds
  expire at the tested boundary. Tests cover wrapping application deadlines, TX
  ending exactly at millis0 with receive windows, immediate nested MAC responses,
  total224ms for outer+nested frames, and aggregate spacing afterward. Both nested
  application downlinks arrive in order, including empty and maximum binary data.
  Fair demo scheduling and7+3 batch fragments remain covered.
- PASS: ambiguous launch-error and reboot tests preserve DevNonce non-reuse;
  failed nonce NVS writes prevent RF. Reboot has no restored airtime debt. The
  old airtime-write failure tests were removed with that rejected mechanism;
  the Preferences test double exposes only nonce writes now.
- PASS: fresh exact-source patch application, idempotence, modified-source
  refusal and integration-test patched hash verification. See patch-check.txt.
- PASS: final C++/headers in all three build trees match canonical bytes after
  excluding Arduino's generated #line prefix. Final ELFs have the downlink
  callback and no beforeUplink/afterUplink or SDMMCFS begin/end symbols.
  See source-and-symbol-audit.txt and build-input-sha256.json.
- PASS: format check and unchanged canonical formatter/analysis configurations.
- PASS: repeated pinned Eagle net audit. Config.h, Application.cpp, Ui.cpp,
  SdStorage.h, decoder, wiring graphics and UI previews match the starting commit.
  Source photo SHA256 remains
  5b2e2bdfefe5a38b7bbee5ef09f37c363579a8628f27df23f5702b5fcd6624ad;
  the SVG-embedded photo matches its exact bytes. No geometry or pin changes.

No physical hardware or RF is involved in the host tests. Host unsigned long
width differs from P4; wrapped HAL values and uint32 application deadlines are
modeled, but ESP IRQ/timer scheduling still needs a device test.
The earlier net audit did not inspect core variant macros; C1 was separately
traced and resolved by removing SD_MMC, with fixed IO_MUX static assertions and
slot-lifecycle tests. It must not be described as proof that stock SD_MMC is safe.

## Accepted patch scope and trace

Line references below are to the hash-checked, patched 7.8.1 dependency:

- LoRaWAN.cpp sendReceive:118/124 and activateOTAA:960/966: wrap-safe deadlines;
  transmitUplink:1508/1511 and receiveClassA:1615 likewise; elapsed TX timeout,
  millis0 receive reference and timeUntilUplink:3783 corrections remain.
- activateOTAA:989 resets lastToA for each join.
- transmitUplink:1522 counts an attempted launch before the potentially ambiguous
  launchMode return, without a hook, NVS write or wait. This counts airtime
  conservatively on errors; it is not a pre-transmission reservation.
- parseDownlink:2146 forwards authenticated application data before recursive
  handling; :2282–2284 retains outer airtime around the nested sendReceive call.
- LoRaWAN.h adds only the protected applicationDownlink callback. No project
  marker macro or patch-brand comment remains. Exact dependency hashes remain
  the patcher's and integration tests' authority.

The application spaces completed operations using aggregate airtime and one
volatile deadline. It does not enforce separate intervals between internal MAC
replies/repetitions and does not preserve airtime debt across resets. Repeated
resets or interruption before accounting can exceed allowances. See airtime.md.

## Static analysis

Fresh target attempt with the actual default-prev3 compilation database:

```sh
clang-tidy -p "$B/default-prev3" "$B/default-prev3/sketch/Workers.cpp"
```

Exit1: installed Debian LLVM21.1.8 rejects xesppie and GCC-only
-fno-tree-switch-conversion/-fstrict-volatile-bitfields and cannot resolve the
GCC target cmath header. Target clang-tidy did not pass. No checks were weakened.

Supplemental runs completed with exit0 using canonical checks:

```sh
clang-tidy tests/radio.cpp -- -std=c++17 -I"$B/deps/RadioLib/src" -I. -I/tmp/codex/build/CrowPanelTelemetry/radio-tests
clang-tidy tests/sd-storage.cpp -- -std=c++17 -Itests/sd-stubs -I.
clang-tidy SdStorage.h -- -x c++ -std=c++17 -Itests/sd-stubs -I.
```

Warnings retained: three canonical MaxSize=-1 values rejected by this Clang,
unnamed test parameters, const/readability suggestions, implicit integer-to-bool,
float suffixes/multiple declarations, redundant host cast, header pragma style,
and queue destination initialization suggestions. Logs preserve all diagnostics.
Supplemental host analysis does not replace target analysis or hardware tests.

## Change accounting and handover

Relative to starting commit1e3cb01, added/removed physical lines:

| Category | Added | Removed | Net |
|---|---:|---:|---:|
| Application production (.ino/.h/.cpp and decoder) | 12 | 31 | -19 |
| Actual patched dependency source | 2 | 11 | -9 |
| Tests | 27 | 43 | -16 |
| Build/test tooling | 0 | 0 | 0 |

This reshapes the existing deadline and deletes the reservation mechanism; no
additional production architecture or net growth was introduced. Historical
repair accounting remains in change-accounting.json, with this task separate.
Historical review records and artifacts/CrowPanelTelemetry-delivery.zip remain
unchanged and explicitly identify the prior snapshot, not this final revision.
SHA256SUMS.json is regenerated for current repository files, excluding itself
and Git internals.

Temporary remove-reservation, radio-tests and local task directories under
/tmp/codex/build/CrowPanelTelemetry/ are removed after evidence is copied and
verified. No alternate checkout or persistent build exists. User changes, pins,
J7 wiring, SD ownership, UI visuals and source photo are preserved.

Assembly and bench flashing remain conditional on unresolved physical checks in
hardware-checklist.md; neither flashing nor RF is authorized or executed here.
TCXO population, RF switch polarity, GPS voltage, shunt/straps, silicon/C6 version,
10.1-inch timing, touch orientation and audio behavior remain unverified.
Unattended TTN operation remains NO-GO until hardware acceptance and real airtime
behavior pass. No display/touch/audio/WiFi/sensor/GPS/RF operation is claimed.
