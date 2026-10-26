# UI/UX proposal implemented in Ui.cpp

Landscape1024×600, dark navy canvas, three large measurement cards, mint primary
actions and amber diagnostic text. Left: SD image area plus Previous/Next, Play
sound and Retry SD controls. Right: GPS freshness/satellites, LoRaWAN status,
last downlink, and Send demo. Top-right: WiFi setup. Footer: calibration/sensor
status, acquisition minute, batch progress, queue/drop count and resource errors.

Numbers are deliberately blank (`--`) until valid measurements are available;
there are no fake initial readings. GPS coordinates expire after10seconds without
a fresh valid fix. Missing recent checksum-valid NMEA is distinguished from no fix, and
satellite counts expire. A held touch triggers once. Each button has a large touch target.

WiFi opens a dedicated screen with SSID/password fields, a masked password and
three keyboard pages for ASCII lower/upper/symbol entry. Connection feedback
continues without blocking the keyboard/dashboard. Connect saves one credential record only after an accepted request reaches
live association/IP state with the requested SSID; Close abandons the password edit. An empty SSID or invalid
password shows an inline reason that periodic WiFi status cannot overwrite.
Open networks use an empty password; accept 8–63 characters or exactly64 hex digits. Enterprise/captive-portal flows are excluded.

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

Saved WiFi settings from the earlier separate-key format must be entered once
after this update. Rejected requests and credential-save failures use the existing
status field; no screen geometry or visual styling changed.

A rejected WiFi request keeps its specific error for the existing report interval.
If the interface remains offline, later reports say Connection failed rather than
Connecting. The existing connection timer represents a pending accepted request
or an already-expired rejected attempt; no second pending/error state was added.
