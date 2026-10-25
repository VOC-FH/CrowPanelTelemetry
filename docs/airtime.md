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

PersistentNode owns one volatile, wrap-safe application deadline. After a completed
join or application sendReceive operation, it uses RadioLib's corrected aggregate
`getLastToA()` to defer the next operation. The total includes repeated frames and
nested MAC replies. The patch resets join totals per attempt, preserves the outer
total around nested calls, and counts a launch attempt even when its result is
ambiguous. These are predicted PHY durations, not measured RF airtime.

The allowance is the smallest of EU868's regional allowance, the current network
duty-cycle allowance, and TTN_AIRTIME_MS_PER_HOUR when nonzero. The interval is
ceil((aggregate_airtime_ms+1)*3600000/allowance), plus the aggregate airtime as a
conservative margin for integer truncation. It starts after the operation returns.
Join retry backoff and error backoff extend the same deadline rather than creating
another gate. Ordinary RadioLib duty-cycle gating remains disabled.

The user rejected reservations before every physical transmission. Consequently,
there are no airtime reservation hooks, airtime NVS writes, or restored airtime
debts. RadioLib's internal MAC replies and repetitions proceed within the call;
the application does not delay individual frames. Their summed airtime defers the
next application operation. This provides aggregate spacing, not enforcement of
every individual frame's regional or network transmission interval.

A reset clears the deadline, including any older saved `hold` value (which is no
longer read or written). Repeated resets or a power cut before post-call accounting
can exceed airtime allowances. DevNonce remains reserved durably before a join;
nonce-write failures still prevent RF. The former airtime-debt reset lockout no
longer applies. At112ms for one completed frame, the running deadline is325552ms;
for two112ms frames it is648224ms. No delay is inserted between those two frames
by the removed hooks. The table above remains an airtime-based lower bound.

Setting the public macro to0 removes only that allowance for an appropriate
private/paid network. This software policy does not certify per-frame duty cycle,
antenna EIRP or exact daily rolling-window usage. Verify actual RF parameters,
frequency error, repetitions, MAC traffic and reset behavior during separately
authorized hardware tests. Unattended TTN operation remains unverified.

## Class A downlinks

Each uplink is followed by RX1 and (if needed) RX2 receive windows. A queued TTN
message reaches this Class A device only in a suitable receive window after an
uplink. It is not a continuously listening receiver. Queue a downlink on an
application port, e.g.10, then wait for a scheduled uplink or press Send demo.
The UI shows its port and text; nonprintable bytes become `\xNN`. Tap the downlink
area to open the entire message. Routine uplinks are unconfirmed to avoid consuming
an acknowledgement downlink for every batch. Keep manual test downlinks within
the network's separate daily allowance.

## Deep-review outcome and accounting limits

A fragment advances only when the existing zero-initialized uplink event has its
application fPort populated by RadioLib after successful local TX completion.
A changed FCntUp is not proof of RF: transmit errors also consume counters.
Outer TX followed by RX/parse failure advances the fragment and reports
"sent; RX error"; stage/launch/ambiguous TX errors retain it. Deduplicate sample
indices as above: unconfirmed local completion is not delivery acknowledgement.
A nested MAC-reply error is discarded by RadioLib parseDownlink; the outer event
still proves the outer local TX, but cannot diagnose that nested error.

RadioLib sendReceive can return before resetting lastToA (null dataDown/lenDown,
not-joined/rekey transition, duty gate or invalid uplink). An app accountAirtime
then conservatively reuses the previous total. Inside recursive MAC handling an
early return can preserve the outer total and add it again. These paths over-defer;
they do not omit a physical frame. Retained rather than expanding the patch again.
After reset the deadline is still volatile; no per-frame reservations or durable
airtime debt have been restored.

The host PHY normally uses112ms to isolate scheduling decisions. A separate join
loop test uses DR-dependent reference airtimes generated by tools/airtime.py for
23-byte JoinRequests: DR5→DR0, per-attempt accounting and durable nonce-write fail
stop. These remain predicted airtimes with simulated RF, not RF measurements.


## Re-review reset and MAC-state qualifications

N1 parser failure and N2 Hosted restart paths can interact with the accepted
volatile startup deadline. Repeated immediate joins after reset remain possible;
the review's 3–5 second loop timing is a scenario, not a measurement. No new boot
delay, persisted debt or per-physical-frame reservation is implemented. N5 can
produce unnecessary immediate MAC replies; the real-library diagnostic reproduced
two PHY submissions where one is expected on a subsequent small request. The approved
flag-lifetime patch now consumes the existing flag after payload selection; the
regression verifies that unnecessary replies stop while required immediate MAC
responses and aggregate accounting remain. No per-frame pacing was introduced. See review/rereview-fix-status.md.
