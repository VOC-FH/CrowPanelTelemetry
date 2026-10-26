# Review and repair provenance

The original review/repair files are historical, unedited records from before and during the correctness
repairs: Claude's expert review, implementation prompt, requests and baseline
hash/line-count records. Their descriptions of defects, local paths and the lack
of Git apply to those checkpoints. They do not describe the final firmware.

Use ../correctness-status.md and ../validation.md for the current dispositions
and validation limits. The raw Claude response is preserved alongside the cleaned
prompt.

A new deep review of main@6fada32 was explicitly requested after the fixes:

- [Exact review brief](deep-review-request.md)
- [Claude deep review](claude-deep-review.md)
- [Source checks and qualifications](deep-review-assessment.md)

The new review contains claims and proposals, not implemented corrections. In
particular, its proposed frame-counter fix needs the qualification in the
assessment before implementation. UI styling and the rejected per-transmission
airtime reservation policy were excluded.

The user then requested a complete fix prompt from Claude (drafting only):

- [Drafting request and constraints](deep-fix-prompt-request.md)
- [Claude implementation prompt covering F1–F14](claude-deep-fix-prompt.md)

Claude CLI produced the prompt read-only, with 32 source reads/searches over
357 seconds. The saved prompt is the exact returned text. It is a draft for the
user to invoke later, not authorization to implement in this drafting task.
No source/test/tool changes, builds, commits or pushes were made. Existing local
review documents remain preserved on main. The drafting temporary directory was
removed after the prompt was saved and checked.


Implementation was then invoked by the user on main with separate approvals for
path-only baseline setup, the F1 dependency-patch expansion, F3(+2 application
lines), F9(+2), F10(+8 and its helper), and the additional media-stack increase
from12288 to24576 bytes (net0 lines). See
[implementation dispositions](deep-fix-dispositions.md) and
[final validation](../validation.md). The raw review, assessment, requests and
saved prompt above remain byte-identical to the implementation baseline. This
appendix records later provenance without rewriting their historical statements.
Implementation remains local/uncommitted; no push or archive refresh authorized.

The user subsequently authorized pushing the corrections on main and another
independent deep expert review. [Re-review request](deep-rereview-request.md)
sets the correctness-only scope, identifies approved changes and retained risks,
and asks Claude to verify current source independently. Earlier statements about
uncommitted status/no push describe the implementation checkpoint, not this later
publication authorization. The historical delivery ZIP remains unchanged.


The corrections and review brief were committed/pushed on main as
[90e811bc38c448bc1f23d1f76d80cce7155c417b](https://github.com/VOC-FH/CrowPanelTelemetry/commit/90e811bc38c448bc1f23d1f76d80cce7155c417b).
Claude completed the [full independent re-review](claude-deep-rereview.md), using
Read/Glob/Grep only, with exact-tag sources provided in a temporary dependency
collection. [Run provenance](deep-rereview-run.json) records the reviewed commit,
model, duration, tool calls, report hash, read limitations and caller spot checks.
The raw response is saved without editorial changes. The reviewer explicitly
lists files/evidence it did not inspect in detail; this is not a claim that every
source or assertion was exhaustively verified.

The report confirms several corrections, but records N1–N9 and residual F8/F9
issues, including an unchecked JPEG component count, Hosted reset escalation,
downlink replay/state concerns and error-visibility gaps. Findings and proposed
remedies are review claims to verify, not approval to implement. No new patch,
boot-delay policy, WiFi/SD lifecycle reduction or electrical default was adopted.
No UI visual changes were requested or made. Hardware/RF checks remain unexecuted;
unattended TTN operation remains NO-GO. This follow-up changes review documents
and the manifest only; the application remains exactly the reviewed90e811b source.
The historical delivery ZIP is unchanged. Temporary review sources are removed
after evidence is saved and published.


The user next requested an implementation prompt for the re-review findings,
emphasizing use of ordinary Arduino APIs and avoiding unnecessary complexity.
The [drafting brief](rereview-fix-prompt-request.md) covers N1–N9, residual F8/F9,
source-verification qualifications, required approval gates and unchanged user
policies. It was submitted to Claude read-only, but Claude returned
`usage_limit_reached` (HTTP429), reporting a21:10 Europe/Vienna reset on2026-10-25.
[Failure provenance](rereview-fix-prompt-run.json) preserves the response and exact
request hash. **No Claude implementation prompt was generated.** The saved file
is the caller's request to Claude, not Claude's answer or an invoked implementation
task. No application/tests/tools changes, builds, commits or pushes were made in
this drafting attempt. Temporary drafting files were removed after saving evidence.


On 2026-10-25 the user explicitly requested editorial changes to the latest
[re-review](claude-deep-rereview.md). Its wording now emphasizes defensive input
validation and protocol correctness. This file is therefore no longer the raw
Claude response. The original remains in Git at `c6fe271124666c82f92733a8c0f161697761367d`;
its original hash and run provenance remain unchanged in deep-rereview-run.json.
The editorial note distinguishes the revised presentation from a new review.
No implementation changes or filtering-outcome claims accompany this edit.


The user then invoked local verification/correction of the re-review findings.
[Current checkpoint](rereview-fix-status.md) records reproduced findings, a
non-growing WiFi error-visibility correction and pending source-patch proposals.
This is incomplete work; proposed N1/N5/N6 diffs are reviewable but unapplied.
Baseline/current host and firmware evidence is in ../validation-logs/rereview-fix/.
No commit/push or hardware/RF operation was performed in this stage.


The user approved the concrete N1/N5/N6 proposals, including the JPEG vendor-style
exception and two additional application lines. Those corrections are implemented
locally with regression, sanitizer, patch-integrity and three-configuration build
evidence. `tools/patch_dependency.py` replaces the old RadioLib-only script as the
single patcher for the two pinned profiles. No shim or parallel implementation
remains. N4/N7/F8 are separately proposed, pending approval; N2/N3 policies and
physical prerequisites remain explicitly unresolved/retained. No publication or
hardware/RF operation was authorized or performed in this continuation.


The user subsequently authorized publication with “Push”. The publication includes
the completed N1/N5/N6 fixes, the prior WiFi diagnostic correction, validation
evidence and editorial/provenance records. N4/N7/F8 remain clearly labelled
unapplied proposals; publishing those documents is not approval to implement
them. Earlier local-only/no-push statements describe their respective checkpoints.
The historical delivery ZIP is unchanged; no hardware/RF operation is included.


The user then requested an independent review by Codex and selected the amplifier,
ACK counter, zero-counter, WiFi credential and display-timing corrections. Two
concrete gates (counter-state/session metadata and one-blob WiFi storage with
one-time re-entry) were approved. [Accepted corrections](accepted-review-fixes.md)
records current exact-source traces, regressions and deliberately retained codec,
SD/Hosted and nested-recursion limits. This is a new implementation disposition;
raw historical reviews and their original proposals remain unchanged. Work stays
on main, local/uncommitted; no new publication, archive or hardware/RF action is
implied by this task.


On2026-10-26 the user authorized pushing the accepted corrections and requesting
another independent deep IoT/ESP32/LoRa/LoRaWAN/display review. The implementation
and its saved validation were committed and pushed directly on main as
[f23e3932fc699ba9fb5080ec77a21d8db35f8a8d](https://github.com/VOC-FH/CrowPanelTelemetry/commit/f23e3932fc699ba9fb5080ec77a21d8db35f8a8d).
This supersedes the previous local-only handover status; historical records remain
unchanged. No firmware source changed during the publication/review task.

- [Exact review request](claude-review-f23e393-request.md)
- [Claude's complete, unedited review](claude-review-f23e393.md)
- [Run/tool/source provenance](claude-review-f23e393-run.json)
- [Exact dependency/document provenance](claude-review-f23e393-sources.json)
- [Separate caller spot checks and qualifications](claude-review-f23e393-assessment.md)

Claude completed123 Read/Glob/Grep calls in about1003seconds; only read/search
capabilities were available. It did not run tests or inspect all saved logs.
It confirms the counter corrections and amplifier polarity, qualifies the requested
versus realized display clock, and records new/residual join-backoff, EXIF, WiFi
status/timing and RekeyInd concerns. The caller's qualifications are separate from
its raw response, particularly the unproven same-SSID race and physical fault
consequences. Findings are not approval to add fixes. UI redesign was excluded;
retained policies were preserved. No flashing/RF/TTN operation or archive change.


The user then requested the five selected corrections. The
[protocol/parser checkpoint and quantified proposals](protocol-parser-fix-proposals.md)
records the completed non-growing WiFi-status reshape and display documentation
correction, with join/EXIF/Rekey changes still unapplied pending the user's required
scope/growth decisions. This is an incomplete local checkpoint; the raw review
and prior publication records are unchanged. No additional push is authorized.



The user then explicitly approved all three quantified proposals. The
[completed five-correction dispositions](protocol-parser-fix-dispositions.md)
record the existing-deadline join reshape,126-line EXIF reduction and MAC-queue
lifetime correction (including reset and erase/prune defects found by regression).
Final host/patch/three-build evidence is recorded separately from the initial
checkpoint. No new application rekey state, pacing authority, UI redesign,
publication, archive or physical/RF operation was introduced.


On 2026-10-26 the user authorized pushing the five selected corrections and then
requested another fresh expert review. The implementation was pushed directly on
main as [b80bf07c028729f12089f1b27d4c5d032107150c](https://github.com/VOC-FH/CrowPanelTelemetry/commit/b80bf07c028729f12089f1b27d4c5d032107150c).
Earlier uncommitted/no-publication statements describe their historical stage.

- [Exact fresh expert-review request](claude-review-b80bf07-request.md)
- [Claude's complete, unedited response](claude-review-b80bf07.md)
- [Run, model, request/report hashes and read-only tool provenance](claude-review-b80bf07-run.json)
- [Exact pinned dependency, patch and document provenance](claude-review-b80bf07-sources.json)

Claude completed 95 Read/Glob/Grep calls in about 998 seconds using read-only
capabilities. It ran no tests or builds, changed no files, and performed no
hardware/RF/network operation. It confirms the JPEG, WiFi, RekeyInd queue/wire and display-documentation
corrections, while classifying join retry as partial because of an
in-run session-restart path. It also flags retained-answer capacity, patch setup
instructions, RNG qualification, ADR-limit equality and further JPEG header
bounds concerns. These remain reviewer claims, not newly authorized fixes or
independent caller reproductions. Its explicit coverage limits matter: several
libraries, schematics, electrical PDFs, tests and logs were not inspected.
UI redesign was excluded. No implementation change, new commit or push accompanies
this review request. The request, response and provenance are saved locally;
the historical delivery archive is untouched.


The user then authorized fixing only the session-restart join-budget finding,
pushing it and requesting a Claude follow-up restricted to that correction.
[Focused fix trace](join-restart-fix.md) records the one-constant policy correction
and before/after real-library regression through both worker blocks. The previous
b80bf07 report is unchanged; the other findings are not implemented. New source/
test/build evidence is in ../validation-logs/join-restart-fix/. The forthcoming
Claude follow-up is explicitly not an overall review or system approval.


The focused correction and its evidence were committed/pushed on main as
[f936218e11cd53a023b210c198cf1e22651be33f](https://github.com/VOC-FH/CrowPanelTelemetry/commit/f936218e11cd53a023b210c198cf1e22651be33f).
The subsequent [request](claude-join-restart-request.md) explicitly restricts Claude
to this join-budget fix and its regression, excluding a new overall review.
Claude could not begin: HTTP429 usage_limit_reached, with a stated reset at14:00
Europe/Vienna. [Attempt provenance](claude-join-restart-attempt.json) records the
response and zero read/search calls. There is no Claude review or approval for
this commit yet. These request/attempt records remain local after the fix push.
Only the task's temporary dependency/build/cache directory was removed after
saving evidence; no historical source collections or archive were changed.


At13:02 Europe/Vienna on2026-10-26 the user asked for the focused review and
a retry was made using the identical request, exact patched RadioLib and verified
LoRaWAN1.1 specification. Claude again returned HTTP429 usage_limit_reached before
any read/search call, with reset still stated as14:00 Europe/Vienna. The
[retry record](claude-join-restart-retry-attempt.json) is separate from the first
attempt. No review result exists yet. Only this retry's temporary sources were
removed; no code, commit, push or hardware operation accompanied the retry.


The user retried again after the stated limit reset. The same focused request
completed successfully on2026-10-26 for main@f936218, using only29 Read/Glob/Grep
calls over about263seconds. [Claude's complete, unedited scoped review](claude-join-restart-review.md)
and [run/source/request/report provenance](claude-join-restart-run.json) are saved.
Verdict: PASS for the pinned/default session-restart correction, with no blocking
defect established in this restricted scope. This is explicitly not an overall
system verdict or hardware test. Claude notes nonblocking limits: post-restart
uplinks also begin atDR0/SF12 until ADR adjusts them, repeated reverts driven by
a nonconforming server are outside current coverage, cold-boot initialization is
retyped in the test, and the focused after log predates only the final test-output
wording (the final full-suite log matches current source). These are review claims,
not new implementation changes. Earlier rate-limited attempts remain separate.
The review files/provenance remain local; no additional commit/push was performed.
Only this retry's temporary dependency/specification directory was removed after
preserving the report and provenance. Existing source/evidence collections remain.
