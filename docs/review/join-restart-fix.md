# Focused session-restart join-budget correction

2026-10-26; baseline main@b80bf07c028729f12089f1b27d4c5d032107150c.
Scope is only finding1 of Claude's b80bf07 review. The user authorized fixing,
pushing and asking Claude for a review restricted to this correction. No other
review finding is implemented or implicitly approved.

Invariant: entering a join retry sequence after an in-run session revert must not
restart the faster power-up-only join budget. LoRaWAN1.1 section7/Table17 uses
power-up/reset as T0. The radio worker owns joinRetry; PersistentNode owns the
single volatile deadline. The pinned RadioLib7.8.1 sendReceive:103–108 clears its
session at the pending-RekeyInd ADR limit. Workers.cpp's next iteration detects
!isActivated and re-enters the existing join branch. A successful join previously
assigned60000 to the state reused there. No persistent uptime or new scheduler is
necessary: the existing post-success assignment is the faulty policy boundary.

Workers.cpp:167 now assigns3840000 instead (64minutes). Cold-boot initialization
at:156 stays60000. The same joinDatarate in Telemetry.h selects DR0 on a session
restart; the existing failure branch doubles the base to24h, and the existing
max(airtime hold,retry)+jitter deadline remains authoritative. Normal joined
traffic, initial join behavior, nonce reservation/fail-stop, all pins/J7, UI,
protocol and both library patches are unchanged. Trade-off: slower restart
recovery/SF12 joins. No production addition, helper, state or architecture change.

Regression: tests/radio.cpp's existing joinRetryTrace is extended with an optional
restart scenario. It accepts a real authenticated1.1 JoinAccept via the extracted
join block, sends64 demos via the extracted service block and real RadioLib,
then proves the next service call clears activation without RF. At10h and30h,
for each of two seeded RNG streams, failed joins are simulated for60days using
independent reference DR airtimes. All JoinRequests (including the boot success)
are integrated in microseconds; every relevant later24h sliding extremum plus
the first1h/next10h windows is checked, as are jitter bounds,24h cap and application
millis rollover. Other data frames are not included in a JoinRequest budget.
No manually forced counter, retyped worker policy or new mock MAC implementation.

The regression fails before the fix at the existing11h-window8.7s assertion and
passes afterward. The fixed failed-rejoin progression fits at most five1.482s
DR0 requests per24h (7.41s) under these pinned/default settings. This is a source/
host result, not a physical transmission or general regulatory certification.
Host RadioLibTime_t remains64bit; stubs do not prove RF timing/entropy or network
interoperability. Reboot policy, aggregate internal reply limits, all unrelated
review findings and hardware prerequisites are retained.

Final commands/results, three firmware sizes and evidence are in validation.md
and validation-logs/join-restart-fix/. Application physical delta1added/1removed;
tests46added/16removed; dependency source and build/test tooling0/0. Earlier raw
reviews are unchanged. A fresh scoped Claude response will be recorded separately
and cannot serve as a new overall system approval.
