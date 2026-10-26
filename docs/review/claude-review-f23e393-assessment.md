# Caller spot checks of Claude's f23e393 review — 2026-10-26

The companion Claude report is preserved verbatim. Its findings are claims to
verify, not instructions or approval for additional implementation. These are
limited source/arithmetic spot checks by the caller, not a second full review.
No test, compile, hardware, RF or network experiment was run in this publication/
review task. Previously saved host/build results remain evidence for f23e393.

- **Finding1, failed-join backoff:** confirmed source mechanism and normative
  basis. Workers.cpp:155–174 caps the retry interval at1h, while:73–79 accounts
  predicted airtime against the configured hourly allowance. Telemetry.h:46–63
  reaches DR0 and calculates that allowance. The official LoRaWAN1.1 PDF section7,
  printed page65, Table17 specifies staged aggregate join limits, including8.7s
  per24h after11h, and randomized retransmission intervals. The current path
  schedules from millis without such interval randomization. Claude's steady-state
  arithmetic exceeds the later budget; no48h simulation or physical rate was
  measured. This is distinct from the separately retained reboot/pacing policy.
- **Finding2, JPEG EXIF:** confirmed missing-boundary mechanism in exact patched
  JPEGDEC1.8.4 `jpeg.inl:1345–1374,1653–1674`. The first directory read and tag
  traversal use file-derived offsets without bounding the addressed bytes; the
  later upper-only thumbnail check does not establish a safe lower bound. The
  SOF guard does not cover this separate APP1 path. No new fixture, sanitizer run,
  fault or board reboot was demonstrated here. The report's fault/reset wording
  describes a possible consequence, not observed target behavior. Its proposed
  guard is an outline, not a verified complete patch.
- **Finding3, display clock:** the requested configuration still matches Elecrow's
  51MHz setting. IDF5.5.5 `mipi_dsi_hal.c:266–273` rounds the integer divider;
  `esp_lcd_panel_dpi.c:265–282` selects/programs it. Both installed P4 headers
  `clk_tree_defs.h:433` select PLL_F240M. Thus the source-derived nominal clock
  is240/round(240/51)=48MHz. This is not a physical clock measurement or evidence
  that Elecrow's requested timing fails. The missing comparison driver/init
  sequence remains an evidence gap; do not infer that DISPON is required without
  its controller documentation or a physical test.
- **Finding4, rejected WiFi status:** confirmed source presentation path.
  Workers.cpp:260–263 preserves the specific rejection for its report interval,
  but:277–283 then uses the fresh connectAt timestamp to display Connecting when
  the interface is offline. The current rejected-request test starts with an old
  connected link; it does not cover this offline follow-up. No change was made.
- **Finding5, in-flight same-SSID credential switch:** remains an unproven timing
  hypothesis. In particular the installed IDF5.5.5 `esp_wifi.h:1005–1028` documents
  ESP_ERR_WIFI_STATE when set_config is called while still connecting. Core3.3.12
  STA.cpp:414–419 returns false on that error, so Workers.cpp:258 does not arm a
  save. This contradicts treating an ordinary still-in-flight set_config as
  necessarily accepted. A narrowly timed state transition/Hosted-slave behavior
  still needs an integration trace or bench evidence; no new state is justified
  from the report alone. C6 firmware identity is still unverified.
- **Finding6, startup pulse:** output-enable before the HIGH write is visible in
  Application.cpp:21–22. Whether the output latch is low in the relevant reset
  path, pulse duration, fitted amplifier response and audible pop are unmeasured.
  No source or electrical change was made.
- **Finding7, RekeyInd lifetime:** confirmed the relevant unconditional ClassA
  fOptsUp clear in patched RadioLib7.8.1 LoRaWAN.cpp:2117–2119 and the queue-dependent
  rejoin check at:103–108. Claude's proposed no-RekeyConf sequence was not executed;
  a real-library regression is still needed before accepting a correction.

The report explicitly lists substantial unread test/log/source areas and did not
recompute hashes. Supplied official-source availability is not a claim that every
document was read. The original Renesas GT911 guide retrieval failed403; an
alternate manufacturer-authored Waveshare-hosted datasheet was then supplied and
read. The LoRa Alliance PDF was supplied after the browser-viewer retrieval failed
and was read locally. Exact URLs/hashes and failures are in the sources provenance.

The five prior corrections are therefore confirmed/qualified as described by the
review, with unresolved claims above. Retained codec error propagation, SD/Hosted
sharing, nested recursion, RF-power/TCXO prerequisites and physical acceptance are
not silently reclassified as fixes. No UI redesign or new policy was implemented.
