# CrowPanelTelemetry

**Correctness fixes implemented and locally tested; hardware acceptance remains
unexecuted.** Use RadioLib 7.8.1 with the included version-checked patch. See
[repair status](docs/correctness-status.md) and [validation](docs/validation.md).
Target clang-tidy is blocked by the installed tool's Espressif GCC incompatibility.

Original Arduino template for **CrowPanel Advanced ESP32-P4 10.1-inch V1.2**, with
GT-U7 GPS, INA226 and Waveshare Core1262-868M. It provides a touch dashboard, WiFi
credentials keyboard, JPG browsing, WAV playback, GPS fix status, LoRaWAN1.1 Class A
OTAA, demo uplinks, downlink viewing, one-minute measurements and ten-sample batches.

**Read [the wiring guide](docs/wiring.md) before connecting hardware.** The board
mapping is traced against the supplied Eagle files. The exact GT-U7 breakout's
power/UART ratings, the INA226 shunt and radio TCXO population remain unconfirmed.
The code compiles; physical hardware operation has not been tested.

- [Annotated original photo](docs/wiring.png), [editable/self-contained SVG](docs/wiring.svg)
- [UI layout preview](docs/ui-preview.png), [UI behavior](docs/ui.md)
- [Wiring table and electrical conditions](docs/wiring.md)
- [Payload, airtime and delivery policy](docs/airtime.md)
- [Hardware acceptance checklist](docs/hardware-checklist.md)
- [Primary sources](docs/sources.md), [validation record](docs/validation.md)

## Dependencies

These were the latest stable release tags checked on2026-10-25. Exact versions
are pinned for reproducibility; bundled libraries come from the selected core.
Install through Library Manager where available or use Sketch → Include Library →
Add .ZIP Library with the linked release archive. Do not install the large library
snapshots bundled with the CrowPanel demonstrations.

| Package / library name | Version / tag | Purpose |
|---|---|---|
| esp32 by Espressif Systems | [3.3.12](https://github.com/espressif/arduino-esp32/releases/tag/3.3.12) | P4, FreeRTOS, WiFi/ESP-Hosted, I2S, SDMMC, NVS |
| GFX Library for Arduino | [1.6.9](https://github.com/moononournation/Arduino_GFX/archive/refs/tags/v1.6.9.zip) | MIPI DSI / EK79007, drawing |
| RadioLib + project patch | [7.8.1](https://github.com/jgromes/RadioLib/archive/refs/tags/7.8.1.zip) | SX1262, LoRaWAN1.1 Class A / EU868 |
| INA226 by Rob Tillaart | [0.6.6](https://github.com/RobTillaart/INA226/archive/refs/tags/0.6.6.zip) | Sensor conversion/calibration |
| TinyGPSPlus | [v1.0.3a](https://github.com/mikalhart/TinyGPSPlus/archive/refs/tags/v1.0.3a.zip) | NMEA parsing |
| JPEGDEC | [1.8.4](https://github.com/bitbank2/JPEGDEC/archive/refs/tags/1.8.4.zip) | Baseline JPG decoding |
| audio-driver | [0.3.1](https://github.com/pschatzmann/arduino-audio-driver/archive/refs/tags/v0.3.1.zip) | V1.2 ES8311 codec |
| Wire, SPI, WiFi, Preferences, ESP_I2S; IDF FAT/SDMMC | core3.3.12 bundled | Peripheral transports |

Touch is handled by a small original GT911 register reader in `Ui.cpp`; no second
touch stack or display framework is installed. No LVGL or CrowPanel demo is required.
Third-party libraries retain their licenses; audio-driver includes GPL-licensed
wrappers. Review dependency licenses before redistributing an application binary.

## Arduino IDE settings

1. Add `https://espressif.github.io/arduino-esp32/package_esp32_index.json` to Boards
   Manager URLs; install esp32 **3.3.12**.
2. Install the six pinned libraries above. Apply the RadioLib patch described below
   to the chosen dependency folder before Verify; unpatched RadioLib is rejected.
   Keep all project headers beside the sketch, including SdStorage.h.
3. Open `CrowPanelTelemetry.ino`; keep every `.h` and `.cpp` in this folder beside it.
4. Select **ESP32P4 Dev Module**. Use **16MB flash**, **PSRAM Enabled**,
   **QIO**, **80MHz flash**, and **16M Flash (3MB APP/9.9MB FATFS)** partition scheme
   (`app3M_fat9M_16MB`). No extra partition CSV is required; the core's committed
   scheme is used. Flash FATFS is reserved but unused; media is on the removable SD.
5. **Chip Variant must match actual P4 silicon**, not the PCB version. The reference
   configuration is **Before v3.00** (360MHz). Newer silicon needs **v3.00 or newer**
   (400MHz). Determine the silicon revision with Espressif's `esptool chip-id` on
   the board's UART0 USB connection before choosing. Never infer it from V1.2 printed
   on the PCB. Both compile configurations are recorded in the validation report.
6. Use UART0 upload, **USB CDC On Boot Disabled**, erase-all-flash **Disabled**, and
   initially115200 upload speed. Serial Monitor115200 baud. The UART0 Type-C is the
   upload/serial connection; USB2.0 is a separate native USB interface.
7. Inspect the wiring conditions, fill `Config.h`, Verify, then upload on your
   selected port. No upload/installation has been performed by this task.

Exact reference FQBN:

```
esp32:esp32:esp32p4:FlashSize=16M,PSRAM=enabled,PartitionScheme=app3M_fat9M_16MB,ChipVariant=prev3
```

For production-revision silicon substitute `ChipVariant=postv3` and rebuild.

## C6 WiFi firmware prerequisite

The P4 has no built-in WiFi radio. Its fitted C6 must run compatible **ESP-Hosted
SDIO slave firmware**; ESP-AT or an arbitrary C6 Arduino sketch cannot provide the
required service. The core3.3.12 host reports ESP-Hosted**2.12.13** (inspected in
its installed host-version header). The board's current C6 firmware/version cannot
be determined without the hardware and is not claimed compatible merely because
the P4 sketch compiles. First test the shipped firmware. Use its host/slave startup
logs to check compatibility; if mismatched, obtain the matching Espressif slave
release and board-specific configuration before changing that separate module.
This delivery does not flash or replace the C6 firmware.

The sketch overrides the generic P4 EV-board defaults with V1.2 pins:
CLK18/CMD19/D0=17/D1=16/D2=15/D3=14/reset32. WiFi uses SDMMC host slot1; the card
uses slot0. The media worker mounts through ESP-IDF, with the default slot-specific
deinit flag, one-bit IO_MUX pins and a fixed shared 3.3V VO4 reference. No SD power
GPIO or adjustable SD power driver is used. Cleanup checks that slot0 was counted
before deinitializing it, including failed mounts. It starts after WiFi.mode completes
to order application init calls; this does not exclude concurrent Hosted traffic. Generic SD_MMC.begin would drive GPIO45
and break I2C; that wrapper and unused audio board catalog have been removed
from the application. The ES8311 codec uses its driver directly. A failed SD
unmount latches storage unavailable until restart; ordinary no-card mount
failures remain retryable. Failed C6 startup requires a restart; entering WiFi
credentials does not implicitly reinitialize the shared controller in that state.
Real SD/C6 coexistence still needs hardware testing.

Tap **WiFi setup**, type SSID and password, then Connect. The keyboard supports
lowercase, uppercase, digits, printable ASCII punctuation and Space/Delete.
Passwords are masked and saved in NVS only after a successful connection. The
password edit buffer is cleared on Connect/Close. NVS and firmware credentials
are not encrypted by this template. Open networks and Personal authentication are
supported; Enterprise authentication/captive-portal login is outside scope.
WiFi is independent of TTN: LoRaWAN needs an actual LoRaWAN gateway/network path,
not the panel's WiFi connection.

## Fill Config.h

- `DEV_EUI` and `JOIN_EUI`:64-bit hexadecimal integer values, with the displayed
  TTN order; e.g. `UINT64_C(0x0123456789ABCDEF)`. Do not byte-reverse them.
- `APP_KEY` and `NWK_KEY`:16-byte initializer lists in TTN's displayed byte order.
  All-zero example keys disable the radio and show a configuration message.
  JoinEUI0 is allowed if it is what you registered in TTN; DevEUI must be real.
- `INA_SHUNT_OHMS`: actual installed resistor value;0 intentionally disables
  calibrated current/power. Set `INA_MAX_CURRENT_A` within both the shunt and PCB
  limits, and `INA_ADDRESS` for the actual address straps.
- `GPS_BAUD`: default9600; determine actual configured module baud if needed.
- `RADIO_TCXO_VOLTAGE`: fixed-supply default0; read the specific warning in wiring.md.
- `AUDIO_FILE`: default `/audio/demo.wav`.
- Public TTN pacing remains1250ms/hour. See airtime.md before changing it.

## TTN setup

Register a custom device manually: EU863–870MHz / TTN recommended EU868 frequency
plan, **LoRaWAN MAC1.1.0**, **Regional Parameters1.0.4**, **Class A**, **OTAA**.
RadioLib explicitly documents this MAC/regional combination. Match the two EUIs
and both root keys to Config.h. No ClientId or session keys are needed on the radio.
Keep frame-counter and nonce checks enabled; do not enable a development option
that resets/reuses join nonces. The application persists/reserves DevNonce itself.

Paste `ttn-decoder.js` into the application's uplink JavaScript formatter. Test
FPort2/hex `44454D4F` → `DEMO`. The normal binary packet specification and supported
ranges are in airtime.md.

A reboot starts a **fresh OTAA session** using the next reserved nonce. This avoids
restoring an uplink counter that might have already been transmitted before a
power failure. Active session state is intentionally not restored. Both root keys
are needed for1.1; a negotiated1.0 session is rejected. Joining and transmission
run on the radio worker while the UI remains operational.

NVS keys live in `crow-lora` (nonce buffer and boot ID; no airtime debt)
and `crow-wifi` (SSID/password). A nonce write failure stops radio operations.
Changing credentials after an earlier enrollment can make the stored nonce buffer
incompatible; the UI reports failure. **Do not erase/reuse old nonce state with the
same credentials on a live network.** For recovery after lost/corrupt state, provision
fresh root keys/device registration with a coordinated TTN setup, then clear only
this application's old LoRa namespace using an intentional maintenance procedure.
A whole-flash erase also removes WiFi settings; it is not part of normal uploading.

## SD card and resources

Use a FAT32 card (for initial bring-up, a known-good8–32GB card). Create:

```
/images/001.jpg
/images/002.jpg
/audio/demo.wav
```

Up to128 JPG/JPEG files in `/images` are sorted by filename. Use numbered ASCII
filenames for predictable order. Baseline JPEGs up to4MiB are accepted; progressive
or unsupported JPEGs show an error. The decoder selects1,1/2,1/4 or1/8 scaling to
fit600×270 pixels; larger images that still do not fit at1/8 are rejected. A600×270
baseline JPG is the most predictable template resource. Images are decoded to
PSRAM outside the UI task, then ownership transfers to the renderer.

Audio is deliberately a documented **canonical44-byte-header PCM WAV**, stereo,
16-bit little-endian,48000Hz, with a `fmt ` chunk of16 bytes and the `data` chunk
immediately following. Data must be frame-aligned and no larger than4MiB (about
21.8seconds). WAVs with metadata/extra chunks, MP3/AAC and alternative sample
formats are rejected visibly. Convert a resource with ffmpeg, for example:

```
ffmpeg -i input.mp3 -map_metadata -1 -vn -ac 2 -ar 48000 -c:a pcm_s16le -fflags +bitexact -flags:a +bitexact demo.wav
```

Verify the result has the canonical header described above. Audio is loaded into
PSRAM before playback, so browsing JPGs cannot interrupt playback with SD contention.
One additional sound can be queued. Initial volume is35% through the ES8311 driver.
The ES8311 is a mono codec; stereo WAV framing does not promise independently
separated left/right speaker content. Use the board's proper speaker outputs and suitable speaker impedance/rating.
Speaker output pins are differential amplifier outputs; neither speaker terminal
is a module logic ground. Retry SD remounts manually without formatting the card.

## Build and tests

Use an isolated dependency collection with the six pinned libraries, each in
its own subfolder. The RadioLib folder must be named `RadioLib`. Apply the
project patch once (Python 3 and the standard `patch` command are required):

```
python3 tools/patch_radiolib.py /absolute/path/to/dependencies/RadioLib
RADIOLIB_DIR=/absolute/path/to/dependencies/RadioLib make -k format-check test
make compile ARDUINO_CLI=/path/to/arduino-cli LIBRARIES=/absolute/path/to/dependencies
```

`make patch-radio LIBRARIES=/absolute/path/to/dependencies` is an equivalent
explicit patch command. The patcher checks both original 7.8.1 source hashes,
verifies the result, accepts an already-patched copy, and refuses unknown/mixed
sources. No global dependency or SDK is modified automatically. If using Arduino
IDE, patch the exact RadioLib folder selected by its verbose build output before
Verify, or use an isolated CLI dependency folder. A dependency change requires
reviewing/rebasing the patch and repeating validation; do not bypass the checked
patch or apply it blindly to another release.

The patch forwards authenticated application downlinks, including nested and empty
messages, corrects reachable Class-A time comparisons and airtime aggregation,
and commits JoinAccept state only after authentication.
It adds no project marker macro or before/after-transmission hooks. PersistentNode
spaces completed join/uplink operations using their aggregate predicted airtime,
including repetitions and nested replies. RadioLib's ordinary duty-cycle gate is
disabled to keep one application deadline. There are no airtime NVS writes or
pre-transmission airtime reservations; automatic MAC replies proceed normally.
DevNonce persistence before joining remains required to prevent nonce reuse.
Reset clears the volatile airtime deadline; repeated resets can exceed network
allowances. See [airtime policy and limitations](docs/airtime.md).

`make test` runs independent encoder/decoder, sensor-range, join-DR, SD lifecycle,
airtime, real-library calibration/GPS/JPEG, WiFi input/status and patched RadioLib
integration checks. Set RADIOLIB_DIR explicitly;
its absence is a failed prerequisite, not a skipped passing check. The integration
runner compiles the actual production node and scheduling block verbatim against
real RadioLib, with simulated RF, time, queues and NVS. It sends no RF. `make compile`
compiles only, never uploads. Override ARDUINO_CLI, LIBRARIES and FQBN as needed.
Temporary artifacts stay under /tmp/codex/build/CrowPanelTelemetry/; persistent
builds use build/codex/<configuration>. No alternate checkout is used.

A source/net audit can be repeated after downloading the manufacturer's files:

```
python3 tools/check_wiring.py panel.sch panel.brd
```

Repository: [VOC-FH/CrowPanelTelemetry](https://github.com/VOC-FH/CrowPanelTelemetry),
created as a private repository with main as its initial branch. The canonical
local checkout remains this project folder; sibling sketches are excluded.
[Review provenance](docs/review/README.md) and the original
[delivery snapshot](artifacts/README.md) are included. Existing installed Arduino
libraries, board/C6 firmware and TTN services were not modified or deployed.

## Deep-review validation limits

See [F1–F14 dispositions](docs/review/deep-fix-dispositions.md) and
[validation](docs/validation.md). JoinAccept authentication, fragment outcome,
calibration NACK, NMEA freshness, WiFi validation/init diagnostics and progressive
JPEG rejection were corrected. Only init calls are ordered for SD/Hosted; runtime
mount/clock/deinit exclusion is not established. GT911 still intentionally latches
an error until restart. DSI ESP_ERROR_CHECK failures panic and reboot with serial
output, before workers start, rather than returning a graceful UI error.
Media stack is24576 bytes following a measured compiler frame of19120 bytes;
radio/media stack margins and runtime responsiveness still need bench measurements.
No hardware or RF operation is claimed. Unattended TTN operation remains NO-GO.
