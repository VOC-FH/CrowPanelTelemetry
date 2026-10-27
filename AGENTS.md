# CrowPanelTelemetry

Standalone Arduino sketch for CrowPanel Advanced ESP32-P4 10.1-inch V1.2.
Canonical repository: https://github.com/VOC-FH/CrowPanelTelemetry (private).
Implement directly on main by default. Create or use another branch only when
the user explicitly instructs it. Do not create duplicate PRs.
The canonical project is this folder; do not modify sibling Basic/S3 sketches.

`Config.h` owns pin assignments and editable credentials. `Application.cpp`
owns the UI model, scheduler, GPS parsing and shared I2C bus. Codec setup runs
before tasks start. UI alone owns the display and live touch/sensor I2C traffic.
Workers exclusively own radio/SPI/NVS, WiFi, SD/JPEG, and audio/I2S respectively.
WiFi alone stores one atomic credentials blob after its accepted request reaches
live association/IP state with the requested SSID.
The hash-checked RadioLib 7.8.1 patch in patches/ fixes Class-A timing and airtime
accounting, authenticated join/counter state and ACK counter selection, and forwards
nested application downlinks. Its MAC queue alone owns pending RekeyInd; MAC-only
packet construction must not clear it or duplicate it in FOpts and FRMPayload.
It has no project marker or
transmission-reservation hooks. PersistentNode owns the sole volatile RF deadline
and NVS nonces; completed operations extend that deadline using total airtime.
That deadline applies join jitter after the stricter backoff/airtime hold and
clamps the result to the signed wrap-safe span; no second retry timer or RF gate.
Preserve pre-RF DevNonce reservation, including ambiguous TX errors. Airtime is
not persisted or reserved before transmissions; internal MAC replies are not
held by the application deadline. Apply the patch only to the selected isolated
dependency. The hash-checked JPEGDEC 1.8.4 patch bounds SOF metadata before writes
and removes unused EXIF interpretation; orientation/thumbnail metadata is ignored.
Use tools/patch_dependency.py as the sole patch-application implementation for both
libraries; the former RadioLib-only tool is replaced. The approved seven-line
JPEGParseInfo guard retains the vendor error-return idiom as a narrow style
exception; no other vendor style exception is implied. Do not edit installed SDK sources.
The media worker uses SdStorage (IDF slot 0, default deinit flags, fixed shared
3.3 V VO4 reference) and POSIX files. It starts after WiFi.mode completes so
application init calls are ordered; Hosted traffic is not serialized with SD mounts. Its cleanup callback must
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
PlatformIO is an alternative build of the same root sketch/C++ and prepared DEPS
library collection. platformio.ini pins pioarduino55.03.312/core3.3.12, the existing
partition and both P4 silicon targets. Its workspace/core packages default to
build/codex/platformio and build/codex/platformio-core; validation overrides both
under the task temporary root. Reuse tools/patch_dependency.py, never add a second
patch authority. No firmware mirror/custom board/partition or PlatformIO test
framework. Use one builder at a time in this working directory because PlatformIO
creates a transient .ino.cpp beside the sketch. Never commit that intermediate.
No extra checkout was created. Do not write credentials into published artifacts.
See docs/wiring.md before changing any pin: generic P4 defaults differ from this board.
Hardware verification is required before claiming panel, audio, WiFi or RF operation.

The two course sources are docs/documentation.md and learning/learning-material.md.
Render their sans-serif PDFs with tools/render_documents.py (Pandoc/XeLaTeX and
Graphviz); output/pdf holds the deliverables. Renderer scratch uses BUILD or the
student-pdfs task temporary directory. docs/wiring.md remains the detailed pin
reference; keep the course connection tables identical when it changes.

The two supplied lecture PDFs in docs/slides are tracked individually with Git
LFS via .gitattributes. Keep their original PDF bytes; do not migrate other PDFs
or firmware into LFS. SHA256SUMS.json records the actual downloaded PDF bytes,
not the LFS pointer text. Firmware builds do not depend on the slide downloads.
