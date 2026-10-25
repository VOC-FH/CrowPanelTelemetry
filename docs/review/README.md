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
