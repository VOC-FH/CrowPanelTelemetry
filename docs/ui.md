# UI/UX proposal implemented in Ui.cpp

Landscape1024×600, dark navy canvas, three large measurement cards, mint primary
actions and amber diagnostic text. Left: SD image area plus Previous/Next, Play
sound and Retry SD controls. Right: GPS freshness/satellites, LoRaWAN status,
last downlink, and Send demo. Top-right: WiFi setup. Footer: calibration/sensor
status, acquisition minute, batch progress, queue/drop count and resource errors.

Numbers are deliberately blank (`--`) until valid measurements are available;
there are no fake initial readings. GPS coordinates expire after10seconds without
a fresh valid fix. Missing recent UART data is distinguished from no fix, and
satellite counts expire. A held touch triggers once. Each button has a large touch target.

WiFi opens a dedicated screen with SSID/password fields, a masked password and
three keyboard pages for ASCII lower/upper/symbol entry. Connection feedback
continues without blocking the keyboard/dashboard. Connect saves only after
successful association; Close abandons the password edit. Empty or shorter-than8
nonempty passwords are not submitted. Enterprise/captive-portal flows are excluded.

Tap the downlink area for a full message screen (up to the255-byte radio payload,
with printable ASCII and `\xNN` escapes). This avoids hiding a long message behind
the compact dashboard preview. The open screen refreshes on new messages; empty
application messages are labelled. A runtime touch error returns to the existing
dashboard with GT911 lost/restart status. Close returns to the dashboard and cached image.

The preview uses illustrative data and a vector landscape placeholder for an SD
image. It is a layout/color/interaction proposal, not a hardware screenshot or proof
of a GPS fix/TTN connection. The firmware uses Arduino_GFX's built-in raster font;
preview typography is approximate. Image coordinates and control geometry follow
the actual layout. No live data, credentials or generated landscape are embedded
as fake measurements or real SD resources in the firmware.
