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
