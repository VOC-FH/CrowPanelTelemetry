# Hardware acceptance checklist — not yet executed

Software corrections have passed local tests/builds; this checklist remains
unexecuted. Inspect all physical prerequisites before assembly/operation. No
upload/RF action is authorized by the repair task itself. See correctness-status.md.

Use the final wiring.png/wiring.md. The earlier AI draft is rejected.
Record actual board, module revisions, P4 silicon revision, C6 slave firmware,
shunt value and supply/load/antenna configuration with each test run.

1. With power disconnected, identify J7/J2/J13 orientation, common ground and
   rails. Confirm the J7 even/odd rows against the actual connector. Inspect for
   solder bridges. Check GT-U7 input-voltage rating, UART TX voltage specification,
   its labelled pads and regulator. Leave GPS power disconnected until confirmed.
2. Inspect the INA226 fitted shunt/straps; set the real resistance and calibration
   current. Verify its VBS connection to load-side IN− and measurement common
   ground. Start with a low-voltage, current-limited DC test load.
3. Confirm radio supply3.3V only, local100nF/10µF decoupling, all GND connections,
   separate RXEN/TXEN and unconnected DIO2. Confirm TCXO supply component population
   before using Config.h's fixed-supply default. Check whether FB1 or R22 (or both)
   is fitted; the schematic alone does not establish population. After separately
   authorized RF testing, inspect measured frequency offset/TTN metadata. Connect
   a correct868MHz antenna.
4. Read P4 silicon revision with esptool, choose the matching IDE Chip Variant,
   compile that variant and upload through UART0. Do not erase nonce NVS during
   ordinary uploads. Verify stable panel supply voltage during backlight/audio/RF.
5. Boot without SD or configured TTN credentials. Verify the dashboard appears,
   errors are visible, touch aligns at all four corners, and a held finger activates
   a control once. Test taps outside controls. Record any rotation/offset mismatch.
6. Verify WiFi startup through C6 with host/slave version logs. Enter a valid SSID
   and password, check IP/association, reboot and check saved credentials. Test bad
   password, absent AP, open AP if needed, connection loss and configuration changes.
   During failed WiFi connection the UI/GPS must keep updating. Check SD operations
   do not disconnect WiFi and WiFi startup does not disrupt the shared I2C devices.
   Boot without a card while WiFi is connected; insert one, then retry at least
   three times while checking touch, INA226 and WiFi each time.
7. Insert a prepared FAT32 SD. Browse several JPGs, previous from first wraps to
   last, next from last wraps to first. Test missing `/images`, an empty folder,
   baseline and progressive JPGs (progressive must be rejected), corrupt/oversized JPGs, removal while reading, insertion and Retry SD.
   A missing/replaced card must not be reported as a successfully decoded new image.
   If unmount reports an error, verify storage stays unavailable until restart.
8. Play canonical WAV on the physical speakers at low volume. Check both outputs,
   sample rate, distortion/pops, playback completion and amplifier shutdown. Browse
   JPGs, configure WiFi and send LoRa while sound plays; check for gaps. Test missing,
   corrupt, wrong-format, oversized WAV and repeated taps/queue-full feedback.
9. With GPS power/logic verified, connect TXD→J2.1 and GND. Observe valid NMEA at
   configured baud, obtain an outdoor fix, verify coordinates independently, then
   obstruct/unplug the antenna/module: stale fixes must disappear within10seconds.
10. Measure a known DC supply/load with an independent meter. Wait for minute1,
    compare U/I/P and shunt drop within component/calibration tolerance. Test zero
    load, then several safe currents. Never exceed chip/shunt/connector ratings.
    Check clipping/current-limit rejection with a safely limited shunt test setup;
    never use dangerous overcurrent as a test. Disconnect INA226 or disturb I2C: invalid measurements must appear as unavailable,
    not zero. Reconnect and verify reconfiguration plus completed conversion.
11. Register TTN MAC1.1.0/RP1.0.4/ClassA with matching EUIs/keys. Observe JoinRequest/
    JoinAccept and a successful1.1 session. Check SPI with a logic analyzer if radio
    init fails. Check RF switch logic during TX/RX and oscillator state if join fails.
    Under separately authorized RF testing measure conducted power at TXPower0,
    antenna gain and cable/connector loss; calculate EIRP and ERP. Compare with
    RP002 default16dBm EIRP and 868.0–868.6MHz25mW ERP (about16.13dBm EIRP); the initial
    radio.begin power is not the maximum LoRaWAN will request through ADR.
12. Tap Send demo and verify FPort2 `DEMO`. After minute10, verify FPort1 with ten
    samples and correct sample uptime indices. Compare decoded quantized values
    with the panel/logs. Exercise MAC-command payload reduction and verify fragments
    preserve every sample index; avoid shortening sampling/transmission intervals
    on the public network just to accelerate the test.
13. Queue a short FPort10 downlink in TTN, then send/wait for an uplink. Verify the
    panel receives it after RX1/RX2, displays its port/text, and opens the full message
    on tapping the downlink area. Leave that screen open and receive another
    message: it must refresh. Test empty, binary, maximum-size and nested MAC
    application replies, and confirm fresh data replaces older data in order.
    Respect TTN's separate downlink allowance; use a controlled private setup for
    extensive testing. Do not expect unsolicited delivery between receive windows.
14. Make a gateway unavailable. Confirm join retry pacing, responsive UI, continued
    minute sampling and bounded queue/drop reporting. Restore coverage and inspect
    queued sample timing and oldest-first transmission. At slow ADR rates expect
    paced delivery rather than an unconditional10-minute schedule. Verify joins
    progress from DR5 to DR0 under repeated failure. Queue demos under sustained
    batch backlog and check both demos and batch fragments make progress.
15. Test resets and controlled power cuts before join, during join, during uplink
    and after downlink. Verify the next DevNonce is greater and no old session frame
    counters are reused. Verify nonce NVS-write failure handling. Ambiguous TX
    errors must not reuse the old DevNonce. Airtime deadlines now exist only in
    RAM and reset on reboot; no airtime debt is persisted or restored. Verify
    aggregate accounting includes automatic MAC replies and repetitions without
    delaying those individual frames. Measure actual duty-cycle/network behavior,
    including repeated resets; software spacing does not prove per-frame compliance.
16. Run for at least24hours. Compare TTN airtime, downlinks, sampling timing, queue
    depth, dropped counts, WiFi stability, SD behavior, PSRAM usage and touch/audio
    responsiveness. These are hardware acceptance/performance tests; none were
    replaced by the compile or host codec tests.

17. Unplug GPS after receiving data: within10seconds status must distinguish no
    recent checksum-valid NMEA from no fresh fix; satellite counts must expire. Induce a GT911 I2C
    error from both WiFi and downlink screens: the dashboard must display
    GT911 lost/restart instead of silently freezing. Restart after repair.
18. Exercise millis wrap in a controlled accelerated-HAL build on the actual P4,
    including TX ending at zero, both receive windows, join and nested frames.
    Host tests model this; they do not prove ESP timer/IRQ scheduling on hardware.

## Deep-review bench checks — all unexecuted

- F1/F2: only in a controlled private network, inject malformed/foreign/bad-MIC
  JoinAccept and downlink frames, then genuine1.1 replies; check retries, counters,
  fragment advancement and received text. Include local TX timeout and nested reply
  failure. A local TX-done event never proves gateway receipt.
- F3: calibration-register write NACK must show sensor error and invalid triplets;
  true zero load remains valid. Inspect shunt/address straps and I2C pull-ups first.
  INA_SHUNT_OHMS=0 permits on-screen voltage but version1 sends invalid triplets.
- F4: continuous WiFi traffic during at least50 Retry SD cycles, with/without card,
  plus cold boots, failed mounts, removal, C6 startup failure and Hosted recovery.
  Count H_SDIO_DRV/sdmmc errors and recovery events. Init-call ordering is not bus
  exclusion; no app mutex or startup delay is claimed to fix this.
- F5: inspect TCXO R22/FB1 population, RXEN/TXEN polarity with a logic analyser,
  antenna/cable and measured radiated power. FB1-only with TCXO voltage0 may have
  no clock; R22 fitting does not prove frequency accuracy. Record frequency offset.
- F6: measure UI/touch/GPS responsiveness during SF12 RX windows with WiFi/audio
  active. No device latency baseline has been measured by this task.
- F7: transient/persistent GT911 faults, including clear-status-write NACK, must
  visibly latch lost/restart. Repair and restart; no automatic recovery is promised.
- F8: wrong baud/garbage, valid no-fix NMEA, silence and stale satellites/fix.
- F9/F10: NVS/radio failures must stop RF; inspect serial for the specific init
  diagnostic. Empty SSID and1–7 character passwords must show the WiFi rejection;
  exercise open, valid WPA and64-hex PSK input, including periodic status updates.
- F12: controlled display-init failure is serial-only panic/reboot diagnosis;
  there is no graceful on-screen DSI fallback. Confirm no worker/RF starts.
- F13: progressive JPEG visibly rejected; baseline browsing still works.
- F14: measure radio AND media stack high-water marks with maximum binary downlink,
  nested MAC replies, full-size baseline JPEG, directory sorting, simultaneous
  WiFi/audio and the24h run. Media stack increased to24576 after a19120-byte
  loadImage frame was found; static evidence is not a whole-call-chain guarantee.
- Pinned panel.sch re-verifies GPIO30→R147(0Ω)→NS4263B U4 pin1 and R71(10kΩ)
  pulling AUDIO_OUT_SD toward3.3V. Measure enable polarity and reset-to-setup pop;
  neither component population nor the actual amplifier behavior is verified.
- Still inspect GPS VCC/TXD levels, J2/J13 orientation, INA226 shunt/rating/straps,
  actual P4 revision/Chip Variant and C6 firmware compatibility with Hosted2.12.13.
  Verify10.1-inch DSI timing, GT911 orientation and ES8311 retention across MCLK gap.
  Wire DIO1/DIO2 from the table, not from the photographed pad placement.
