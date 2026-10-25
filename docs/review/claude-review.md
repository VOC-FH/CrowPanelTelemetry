# CrowPanelTelemetry review: functionality and correctness only

This was a read-only review. I built nothing, flashed nothing and sent no RF. Evidence comes from the project sources, the local core 3.3.12 SDK, and upstream RadioLib 7.8.1, ESP-IDF v5.5, INA226 0.6.6 and the Waveshare schematic. I did **not** re-parse the Elecrow Eagle files myself. For the board nets I relied on reviewing `tools/check_wiring.py` and the J7 silkscreen in the photo.

**Verdict: no-go for flashing and running as-is.** At every boot, the SD card mount takes over the I2C data pin (GPIO45). That alone kills touch and the INA226.

---

## Confirmed firmware defects

### CRITICAL

**C1. The SD mount takes over I2C SDA (GPIO45) on every boot, so touch and the INA226 stop working.**
- **Where:** `Workers.cpp:327-329`, against the core files `variants/esp32p4/pins_arduino.h:73-74` and `libraries/SD_MMC/src/SD_MMC.cpp:284-292`.
- **Trigger:** `mediaTask` starts with `Remount`, so it calls `SD_MMC.begin()` at boot whether or not a card is inserted. It does so again on every Retry SD.
- **Mechanism:** The generic "ESP32P4 Dev Module" variant (Function-EV board) defines `BOARD_SDMMC_POWER_PIN 45` with `ON_LEVEL LOW`. `SD_MMC.begin()` therefore runs `pinMode(45, OUTPUT)`. In `esp32-hal-gpio.c:122-124`, `pinMode` calls `perimanClearPinBus(45)`. That fires `i2cDetachBus`, registered in `esp32-hal-i2c-ng.c:104`, which runs `i2cDeinit`. GPIO45 is then driven HIGH for 200 ms and left LOW.
- **Consequence:**
  - The shared I2C bus is gone for the rest of runtime, and SDA is held low.
  - GT911 polling fails silently, so the UI cannot be operated: no WiFi setup, demo, media or downlink view.
  - The INA226 shows "missing/error" and every sample becomes `FFFFFFFF`.
  - The bus is torn down from the media task while the UI task may be mid-transaction.
- **Docs are wrong here too:** `README.md:86-88`, `docs/wiring.md:15` and `docs/sources.md:24` say core VO4/SD_MMC handling is safe. That inspection missed this macro.
- **Smallest correction:** Own the SD mount in the media worker through ESP-IDF `esp_vfs_fat_sdmmc_mount`, using an unmodified `SDMMC_HOST_DEFAULT()` with slot 0 and the LDO channel-4 power control, and no power GPIO. The alternative is a CrowPanel board variant that keeps the VO4 auto-LDO definitions but drops `BOARD_SDMMC_POWER_PIN`. A sketch-level `#undef` cannot work, because `SD_MMC.cpp` is compiled separately. Either fix also resolves H1.

### HIGH

**H1. SD unmount or mount failure calls the SDMMC host deinit with a garbage slot number, which can break WiFi or SD remount.** The mechanism is confirmed; the exact outcome is a hypothesis.
- **Where:** `Workers.cpp:327-329`, against `SD_MMC.cpp:228,258`.
- **Mechanism:**
  - `SD_MMC.cpp` overwrites `host.flags` with `FLAG_4BIT`/`FLAG_1BIT`, which drops `SDMMC_HOST_FLAG_DEINIT_ARG`.
  - In IDF v5.5, `call_host_deinit()` then calls `host->deinit()`. That field is a union with `deinit_p = sdmmc_host_deinit_slot`, so the call passes an indeterminate slot argument.
  - Mount failure (for example, no card) still runs this cleanup. `sdmmc_host_init()` returns `ESP_OK` ("already initialized") once ESP-Hosted owns slot 1, so `host_inited` is true. Retry SD runs it again on unmount.
- **Likely outcomes:** either slot 0 is leaked, so Retry SD never remounts, or the C6 SDIO slot or controller is torn down, so WiFi is lost.
- **Test needed:** Boot without a card and with WiFi connected, then check WiFi. Then mount, press Retry SD three times, and check both WiFi and remount.
- **Correction:** Same as C1. Keep the default host flags.

**H2. After about 49.7 days of uptime, the radio stops for about 49.7 days.**
- **Where:** `Workers.cpp:103,133` gate on `node.timeUntilUplink() == 0`.
- **Evidence:** In RadioLib 7.8.1, `RadioLibTime_t` is `unsigned long` (32-bit). `timeUntilUplink()` does `nextUplink = tUplinkEnd + interval; if (millis() > nextUplink) return 0; return nextUplink - millis() + 1;`. This is not wrap-safe. After `millis()` wraps, a pre-wrap `tUplinkEnd` makes it return about 2³² ms.
  - Uplinks are paced about 5.5 min apart and the duty-cycle interval is about 11 s at SF7. The block therefore happens on about 97 % of wraps.
  - The pre-TX check inside `sendReceive` (`tUplinkEnd + interval > tUplink`) has the same flaw, gated by `dutyCycleEnabled`. The `tUplink` clamp is also non-wrap-safe.
- **Consequence:** No uplinks or joins. Batches overflow and the dropped count rises.
- **Correction:**
  - The app's own pacing (`allowedAt`) is wrap-safe and stricter than 1 %. Stop gating on `timeUntilUplink()` and use `setDutyCycle(false)`, *and* fix or patch the RadioLib time comparisons, since the `tUplink` clamp stays wrong.
  - Interim alternative: a controlled reboot before 49 days. This is safe with the nonce reservation and costs one join.
- **Test:** Use a RadioLibHal whose `millis()` returns `::millis() + 0xFFF00000`.

### MEDIUM

**M1. Join airtime is counted cumulatively, so retry and post-join delays grow without bound.**
- **Where:** `Workers.cpp:116-118`.
- **Mechanism:** In RadioLib, `activateOTAA` adds to `lastToA` with `+=` and never resets it. `clearSession()` does not reset it either; only `sendReceive` does. So the hold after the k-th failed join is 2880 × (sum of all join airtimes so far), and it is persisted to NVS.
- **How fast it grows:**
  - The first join runs at DR5 (about 62 ms). Every later join runs at DR3 (about 206 ms), because `clearSession` resets the datarate to the midpoint.
  - The hold reaches about 1.5 h after 10 attempts, about 3.8 h after 24 and about 7.8 h after 48.
  - After a one-day gateway outage, both rejoin and the first data uplink wait hours.
- **Correction:** Use the airtime of that attempt only: `getLastToA()` after the join minus the value before it.

**M2. A nested MAC-only uplink can under-count airtime.** The mechanism is confirmed; how often it happens is unknown.
- **Mechanism:** `parseDownlink` calls `sendReceive(..., FPORT_MAC_COMMAND)` when the MAC answers exceed the FOpts space or the payload limit. Duty cycle is lifted for that call, and it resets `lastToA`. `Workers.cpp:157` then paces only the nested frame and drops the outer uplink's airtime.
- **Correction:** Derive airtime per call from your own `getTimeOnAir` of the actual frame lengths, or get the fix upstream.

**M3. The demo uplink starves whenever batches are backlogged.**
- **Where:** `Workers.cpp:130-139`.
- **Mechanism:** A queued batch is always taken first (`offset = 0`), and the demo is only considered when `offset == 10`. At SF8 and slower the queue is practically never empty, so the demo is never sent while the UI says "Demo queued".
- **Correction:** Check `demoRequests` before taking a new batch, or alternate between the two.

**M4. A saturated INA226 reading is published as valid.**
- **Where:** `Application.cpp:76-86`.
- **Mechanism:** `INA226_MATH_OVERFLOW_FLAG` only flags arithmetic overflow. It does not flag shunt-ADC clipping at ±81.92 mV.
- **Example:** With 0.1 Ω and a real 2 A load, the reading is about 0.82 A, marked valid. This breaks "never publish failed measurements as valid".
- **Correction:** Read the shunt-voltage register and mark the slot invalid at full scale, or when |I| ≥ the calibrated maximum.

**M5. The join datarate never goes below DR3 (SF9).**
- **Where:** `Workers.cpp:90,126`.
- **Mechanism:** The first join uses DR5. After `clearSession()`, RadioLib picks `(0+5+1)/2 = DR3` every time.
- **Consequence:** A device that needs SF10–12 never joins.
- **Correction:** Step the join datarate down per attempt with `setDatarate()` before `activateOTAA()`.

### LOW
- **L1 – voltage dropped from uplinks:** With `INA_SHUNT_OHMS=0` (the default), valid voltage is discarded on air, because `encodeReading` (`Telemetry.h:17`) needs all three values to be finite. This is documented, but it conflicts with "submit all ten readings".
- **L2 – 48 h lockout after a reset during an uplink:** The 60 s reservation (`Workers.cpp:149`) is documented. A brownout during TX on a weak 5 V supply would trigger it.
- **L3 – misleading GPS status:** If the GPS stops sending after the first data, the UI says "No fresh fix / move antenna outdoors" instead of "no data" (`Application.cpp:63`). The satellite count is shown without a validity check, and no fix quality is shown.
- **L4 – downlink display gaps:**
  - The downlink screen does not refresh when a new downlink arrives.
  - A zero-length downlink on an application port is not shown (`Workers.cpp:172`).
  - Application data received inside the nested MAC call (M2) is lost.
- **L5 – silent touch loss:** GT911 failures after boot are invisible, because `touchAddress` is only evaluated in `beginUi`. With C1, the board looks frozen.

---

## Known documentation error and unresolved hardware prerequisites

**Known documentation error (the false socket/WiFi dependency):**
- **Where:** `docs/wiring.md:26-27,177`; `tools/draw_wiring.py:99,125`; `docs/wiring.svg:150,238`; `docs/wiring.png`.
- **What it says:** "Keep C6 WiFi module installed" and "Do not attach the LoRa module to the occupied WiFi socket".
- **Fix:** Correct the text only. WiFi comes from the onboard C6 (IC1). The J7 wiring stays as it is.

**Additional documentation inaccuracies:**
- `docs/airtime.md:68-71` says pacing uses "actual stack-reported airtime" for every RF operation. That is false for joins (M1) and for nested MAC uplinks (M2).
- `validation.md:43-47` reports a net audit only. Core variant macros (C1) were outside its scope.

**Hardware prerequisites:**
- **TCXO supply:**
  - The Waveshare schematic shows the TCXO VCC fed both through FB1 from DIO3 and through R22 (0 R) from 3V3. Neither is marked NC; only L8 is marked NC.
  - The Waveshare wiki says DIO3 powers the TCXO.
  - If only FB1 is fitted, `RADIO_TCXO_VOLTAGE=0` leaves the radio with no reference clock, so begin or TX will fail.
  - If R22 is fitted, XOSC mode with an injected TCXO signal is not Semtech's documented TCXO mode, so frequency accuracy is unverified.
  - Check: measure the module's components, then look at the frequency offset in TTN metadata.
- **RF switch polarity:** The schematic supports the project's mapping. TX_EN drives CTRL (RF1, PA path) and RX_EN drives CTRL̄ (RF2, LNA path). So the English wiki sentence is reversed, as the docs say. The switch part number is not shown, so confirm with a logic analyser during TX/RX.
- **Wiring and modules:**
  - The J7 nets and orientation look consistent with the photo silkscreen (3V3/5V/GND, IO2…IO54).
  - The DIO1/DIO2 wire endpoints on the module photo cannot be told apart at this resolution. Wire from the table, not the picture.
  - Still open: the GT-U7 VCC and UART levels, the INA226 shunt value and address straps, the P4 silicon revision, and the C6 ESP-Hosted firmware version.
- **Still unverified:**
  - 10.1-inch DSI timing: Arduino_GFX's EK79007 profile is for the *7-inch* CrowPanel P4.
  - GT911 coordinate orientation.
  - NS4263B enable polarity on GPIO30.
  - Whether the ES8311 keeps its configuration across the I2S `end()`/`begin()` clock gap.

---

## Feature status

| Feature | Status |
|---|---|
| Display / dashboard | Unverified (timing comes from the 7-inch profile) |
| Touch | **Broken** after boot (C1) |
| WiFi provisioning and persistence | Logic is fine. **Conditional** on C6 firmware; at risk from H1 |
| JPG browsing | Decode bounds and stride are correct. SD mount works, but at the cost of C1; Retry SD is at risk (H1) |
| WAV playback | Conditional (amplifier polarity and codec clocking unverified) |
| GPS display | Functional, with a misleading status (L3) |
| U/I/P sampling | **Broken** by C1. After that is fixed: saturation (M4); voltage not sent while uncalibrated (L1) |
| LoRaWAN 1.1 OTAA, nonces, version check | Logic is correct against RadioLib 7.8.1: DevNonce is reserved before TX and matches the post-TX increment; 1.0 sessions are rejected via `rev` |
| TTN pacing | Wrong for joins (M1), MAC uplinks (M2) and after 49.7 days (H2) |
| Demo uplink | Starves under backlog (M3) |
| Downlink display (binary, text, long) | Correct for normal cases (L4) |
| Encoder / decoder | Correct and consistent with each other |

## Test coverage
The host tests cover only the codec, the decoder and the airtime arithmetic. None of the defects above would be caught by them.

Meaningful additional checks:
1. Boot with an SD card, then use touch and read the INA226 (catches C1).
2. Boot with no card, and press Retry SD repeatedly, both with WiFi connected (catches H1).
3. Offset the RadioLib HAL `millis()` near the 32-bit wrap (catches H2).
4. Log the stored hold over 5+ forced join failures (catches M1).
5. Press demo while three batches are queued (catches M3).
6. Run an INA226 over-current test (catches M4).
7. Do the TCXO and RF-switch bench checks.
8. Run the power-cut nonce test already listed in the checklist.

## Go/no-go
- **Assembly:** You can wire the J7 radio and the other modules after the listed component checks (TCXO population, GPS supply, shunt).
- **Flashing and operation:** No-go until C1 is fixed. Fix H1–H2 and M1–M5 before any unattended or long-term TTN operation.

Sources:
- [RadioLib 7.8.1 LoRaWAN.cpp](https://raw.githubusercontent.com/jgromes/RadioLib/7.8.1/src/protocols/LoRaWAN/LoRaWAN.cpp), [LoRaWAN.h](https://raw.githubusercontent.com/jgromes/RadioLib/7.8.1/src/protocols/LoRaWAN/LoRaWAN.h), [TypeDef.h](https://raw.githubusercontent.com/jgromes/RadioLib/7.8.1/src/TypeDef.h)
- [ESP-IDF v5.5 vfs_fat_sdmmc.c](https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/components/fatfs/vfs/vfs_fat_sdmmc.c), [sdmmc_host.c](https://raw.githubusercontent.com/espressif/esp-idf/release/v5.5/components/esp_driver_sdmmc/src/sdmmc_host.c), [ESP-IDF issue 16233](https://github.com/espressif/esp-idf/issues/16233), [esp_hosted SD example](https://components.espressif.com/components/espressif/esp_hosted/versions/2.12.1/examples/host_sdcard_with_hosted?language=en)
- [INA226 0.6.6](https://raw.githubusercontent.com/RobTillaart/INA226/0.6.6/INA226.cpp)
- [Waveshare Core1262 wiki](https://www.waveshare.com/wiki/Core1262-868M), [schematic](https://files.waveshare.com/upload/c/c1/CoreSX1262_Sch.pdf)
- [Arduino_GFX v1.6.9 device profiles](https://raw.githubusercontent.com/moononournation/Arduino_GFX/v1.6.9/examples/PDQgraphicstest/Arduino_GFX_dev_device.h)
- [Elecrow 10.1 wiki](https://elecrow.com/wiki/CrowPanel_Advanced_10.1inch_ESP32-P4_HMI_AI_Display_1024x600_IPS.html), [openelab product page](https://openelab.io/a/s/products/crowpanel-advanced-10-1inch-esp32-p4-hmi-ai-display)
