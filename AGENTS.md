# CrowPanelTelemetry

Standalone Arduino sketch for CrowPanel Advanced ESP32-P4 10.1-inch V1.2.
Canonical repository: https://github.com/VOC-FH/CrowPanelTelemetry (private).
The initial repository import uses main; subsequent coherent work uses feature
branches based on the actual remote default branch. Do not create duplicate PRs.
The canonical project is this folder; do not modify sibling Basic/S3 sketches.

`Config.h` owns pin assignments and editable credentials. `Application.cpp`
owns the UI model, scheduler, GPS parsing and shared I2C bus. Codec setup runs
before tasks start. UI alone owns the display and live touch/sensor I2C traffic.
Workers exclusively own radio/SPI/NVS, WiFi, SD/JPEG, and audio/I2S respectively.
The approved RadioLib 7.8.1 patch in patches/ supplies per-physical-frame hooks.
PersistentNode is the sole RF deadline/NVS owner; preserve pre-RF airtime and
DevNonce reservation, including ambiguous TX errors. Apply the hash-checked patch
only to the selected isolated dependency. Do not edit installed SDK sources.
The media worker uses SdStorage (IDF slot 0, default deinit flags, fixed shared
3.3 V VO4 reference) and POSIX files. It starts after WiFi.mode completes so
application shared-controller startup is serialized. Its cleanup callback must
not decrement an uncounted slot after an early IDF initialization failure. Never use SD_MMC or cycle
the shared IO supply. An unmount error latches storage unavailable until restart.
Queues transfer commands and unique ownership of pixel/audio allocations.
`Telemetry.h` is the payload encoder authority; `ttn-decoder.js` is the matching
stateless network decoder. Never synthesize failed measurements as zero.

Use the included canonical clang-format, clang-tidy and clangd files. Only
project-owned C++/Arduino files are formatting targets; third-party dependencies
are excluded. CMake configs/modules are inapplicable to this Make/Arduino project.
`make format`, `make format-check`, `make test`, `make compile` are explicit targets.
For analysis, generate an Arduino compilation database with the same FQBN and
build path and pass it to clang-tidy; analysis is not a formatting operation.

Persistent builds use `build/codex/<configuration>`; temporary checks and dependencies
use `/tmp/codex/build/CrowPanelTelemetry/<task>`. Both build/ and checkouts/ are ignored.
No extra checkout was created. Do not write credentials into published artifacts.
See docs/wiring.md before changing any pin: generic P4 defaults differ from this board.
Hardware verification is required before claiming panel, audio, WiFi or RF operation.
