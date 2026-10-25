# Payload, airtime and delivery policy

Sampling occurs at uptime minutes 1, 2, …; the first batch is due at minute 10.
The UI shows full floating-point sensor values. TTN receives all ten U/I/P triplets
using deliberately documented quantization, not an average or just the last sample.

FPort **1** is a binary batch (47 bytes normally). FPort **2** contains `DEMO`.
All multi-byte application fields use big-endian/network byte order.

| Offset | Bytes | Meaning |
|---|---|---|
| 0 | 1 | Protocol version 1 |
| 1 | 2 | NVS-persisted boot ID (wraps after 65535) |
| 3 | 3 | First sample's uptime minute in the ten-sample batch |
| 6 | 1 | High nibble: first sample index 0–9; low nibble: sample count 1–10 |
| 7… | 4 each | Packed U/I/P samples, oldest first |

Each 32-bit sample has bits31–21 = voltage/0.02V (unsigned11), bits20–10 =
current/0.01A (signed11, two's complement), bits9–0 = power/0.1W (unsigned10).
Rounding is to nearest. Half-step errors are at most 10mV, 5mA, 50mW apart from
sensor/calibration uncertainty. Valid transmitted ranges are 0–36V,
−10.23…+10.23A, and 0–102.2W. These encoding ranges do not authorize these currents
through your particular shunt/PCB. Any missing, non-finite, failed, overflowing,
or out-of-encoding-range triplet is `FFFFFFFF`. A valid zero remains a valid zero.

Uptime minutes identify actual sample slots even after delayed delivery. The decoder
does not invent UTC timestamps or assume reception equals acquisition time.
Reassemble fragments by device ID + boot ID + first_sample_minute, and deduplicate
by sample index. The stateless TTN decoder returns only the received fragment;
your database/application can combine fragments into ten samples.

The minimum EU868 LoRa application allowance is 51 bytes before FOpts reductions.
A 47-byte batch fits with up to four FOpts bytes. RadioLib's current
`getMaxPayloadLen()` subtracts queued MAC commands. When fewer than 47 bytes are
available, the sketch sends as many whole samples as fit and retains the remainder.
With all 15 FOpts bytes at DR0, the limit is 36 bytes: seven samples take35 bytes
and the remaining three take19 bytes. No measurement is silently removed to fit.
Different fragments open additional receive windows and consume more airtime.

## Public TTN tradeoff

The public network allowance is 30 seconds uplink airtime/day/device and at most
10 downlinks/day/device, including acknowledgements. See
[TTN duty cycle and fair use](https://www.thethingsnetwork.org/docs/lorawan/duty-cycle/).
The table is calculated with `tools/airtime.py` using the Semtech LoRa airtime
formula, 47 application bytes + 13 LoRaWAN header/MIC bytes, **no FOpts**, BW125kHz,
CR4/5, eight-symbol preamble, explicit header, payload CRC, low-data-rate optimization
for SF11/12. It is a calculation, not an RF measurement. MAC traffic, retries,
joins and demonstration uplinks are additional.

| DR | Spreading factor | Airtime/packet | 144 batches/day | Conservative pacing interval |
|---|---|---|---|---|
| 5 | SF7 | 112.896ms | 16.257s | ≥326s |
| 4 | SF8 | 205.312ms | 29.565s | ≥592s |
| 3 | SF9 | 369.664ms | 53.232s | ≥1065s |
| 2 | SF10 | 698.368ms | 100.565s | ≥2012s |
| 1 | SF11 | 1478.656ms | 212.926s | ≥4259s |
| 0 | SF12 | 2629.632ms | 378.667s | ≥7574s |

At SF8 there is practically no margin for joins, MAC traffic or demos. At SF9 and
slower, continuous ten-minute delivery is incompatible with public TTN policy.
The template preserves one-minute acquisition and slows RF delivery instead.
Six complete batches can wait in RAM, with one additional batch held by the radio
worker once joined. On overflow, the new batch is dropped and a cumulative dropped
batch count is visible. Power loss clears queued measurements. This is a bounded
teaching template, not a durable data logger or a lossless outage store.

PersistentNode is the sole pacing owner through the included RadioLib patch's
per-physical-frame hooks. RadioLib's ordinary duty-cycle gate is disabled. Before
each join, repeated uplink or nested MAC-only transmission, the hook waits for the
previous deadline, derives airtime from RadioLib's selected PHY/frame length, and
persists the next delay debt before launch. NVS failure prevents that launch and
all later RF. This is predicted PHY airtime, not an RF measurement. Failed or
ambiguous launches retain the full reservation conservatively.

The allowance is the smallest of EU868's regional allowance, the current network
duty-cycle allowance, and TTN_AIRTIME_MS_PER_HOUR when nonzero. The full interval
is ceil((floor(PHY_airtime_ms)+1)*3600000/allowance). One extra frame duration is
added as a conservative margin, and the deadline starts after the launch call.
The same single deadline applies to internal repetitions and nested MAC replies;
there is no unpaced hidden RF path within these Class-A operations. A nested
reply can therefore wait minutes or hours; the UI/GPS/sampling continue on other
tasks. Network behavior under such delays still requires integration testing.

On reset, the full saved debt is observed again without assuming a wall clock.
The old arbitrary 60-second airtime reservation has been replaced by the known
individual frame's reservation. At a predicted112ms and the public allowance,
the saved debt is325552ms (325.552s), not48hours. Existing older debt is respected
conservatively rather than erased. The corrected diagnostic counter resets for
each join and includes outer+nested frames, but it is not the pacing authority.
The table above gives airtime-based lower bounds; actual conservative hook delays
can be slightly longer because of integer rounding and the extra frame margin.

Setting the public macro to0 removes only that allowance for an appropriate
private/paid network. The same hook still enforces regional/network pacing.
This is a software policy, not certification of antenna EIRP, regional legal
compliance or exact daily rolling-window usage. Verify actual RF parameters,
frequency error and airtime against TTN during separately authorized hardware tests.

## Class A downlinks

Each uplink is followed by RX1 and (if needed) RX2 receive windows. A queued TTN
message reaches this Class A device only in a suitable receive window after an
uplink. It is not a continuously listening receiver. Queue a downlink on an
application port, e.g.10, then wait for a scheduled uplink or press Send demo.
The UI shows its port and text; nonprintable bytes become `\xNN`. Tap the downlink
area to open the entire message. Routine uplinks are unconfirmed to avoid consuming
an acknowledgement downlink for every batch. Keep manual test downlinks within
the network's separate daily allowance.
