---
GitHub-Issue: N/A
---

# ER Guidelines

## Purpose

Engineering Requests (ERs) break approved work into reviewable implementation units. They define the
problem, scope, requirements, approach, dependencies, and verification plan for a specific feature,
milestone, or integration task.

## Roles

- Implementation Engineer: drafts ERs, implements them, and updates status through implementation.
- System Engineer: reviews, tests, and may mark an ER as `Verified`.

Only the System Engineer may mark an ER as `Verified`, except when the user explicitly starts an
Autonomous AR Batch. For the accepted ARs named in that batch, Codex is delegated the verification
decision and may mark an ER `Verified` only after the independent review, local validation, required
CI, and merge gates in `docs/Plans/Autonomous-AR-Batch-Workflow.md` pass. This exception does not
apply to DRs or work outside the selected AR range.

## Document Location

- ER documents live in `docs/ER/`.
- The ER status ledger lives in `docs/ER/ER-Status.md`.

## Source Template

Use `docs/ER/ER-Template.md` as the source format for new ERs.
The checked-in template is a source document and should remain `GitHub-Issue: N/A`.

## Required Metadata

Each ER should include:

- `GitHub-Issue: N/A` or `GitHub-Issue: #<number>` in front matter
- `AR-Dependencies: AR-XXXX` in front matter when the ER implements approved architecture
- `ER ID`
- `Title`
- `Status`
- `Date`
- `Owners`
- `Type`

Use the existing section structure from the template unless there is a clear reason to add a small,
task-specific section.

## Status Guidance

Use the template status values:

- `Draft`
- `Proposed`
- `Approved`
- `In Progress`
- `Implemented`
- `Complete`
- `Verified`
- `Rejected`

In general:

- `Draft` or `Proposed` means the ER is being defined.
- `Approved` means the work is authorized to proceed.
- `In Progress` means implementation is active.
- `Implemented` or `Complete` means the code/doc change has landed but has not been marked `Verified`.
- `Verified` is reserved for the System Engineer, except for an explicitly authorized Autonomous
  AR Batch as described above.

Keep `docs/ER/ER-Status.md` aligned with the current document status.

## Writing Guidance

- State the problem and constraints concretely.
- Keep goals, non-goals, and scope separate.
- Write acceptance criteria that can actually be checked.
- List real dependencies, including prerequisite ARs or ERs.
- Tie each implementation ER to the AR that spawned it unless the work is explicitly standalone.
- Add implementation notes only when they reduce ambiguity for the implementer.
- Include a verification plan with exact tests and manual checks where practical.
- Include a `Live Pass (Human)` section in every ER. Require it for behavior whose correctness
  depends on user-visible interaction, rendering, sound, hardware, or a target environment that
  Codex cannot faithfully validate. Give reproducible steps and observable expected results. Use
  `Not Required` only with a short reason when automated evidence fully covers the acceptance
  criteria.
- Only the human tester may record a required live pass as `Pass` or `Fail`. Codex may prepare the
  checklist and record a result the human reports, but may not infer a live pass from code, logs,
  screenshots, or automated checks.

## Workflow

1. Draft the ER in `docs/ER/` from `docs/ER/ER-Template.md`. New drafts may start with
   `GitHub-Issue: N/A` until an Issue exists.
2. Create or synchronize the Issue by either:
   - creating it from the ER issue template and then updating `GitHub-Issue:` to `#<number>`, or
   - running `scripts/issue_sync.sh <doc_path>` to create the Issue from the document and update the
     front matter automatically.
3. Add the doc path to the Issue body. When multiple ER docs intentionally share one Issue, list all
   related doc paths together.
4. Implement the work on a branch and reference the Issue in the PR.
5. Update implementation progress in the same commit as it describes. In an Autonomous AR Batch,
   record final `Verified` status only through the post-merge reconciliation procedure.
6. Update `docs/ER/ER-Status.md` when the ER status changes.
7. The System Engineer normally reviews, tests, and marks the ER `Verified`. For an explicitly
   authorized Autonomous AR Batch, Codex completes this step using the independent review and
   validation gates in `docs/Plans/Autonomous-AR-Batch-Workflow.md`. A required human live pass must
   be completed by the human before either path may mark the ER `Verified`.

## GitHub Workflow

- Use the `er` label plus one status label.
- Add area labels only when they improve triage.
- `scripts/issue_sync.sh <doc_path>` can synchronize title, body, and labels from the ER document.
  When an Issue is shared across multiple ER docs, the most recently synced document controls the
  Issue title/body, so grouped Issues require manual review after sync.

The sync script maps `Draft` and `Proposed` to `status:proposed`, `Approved` to
`status:accepted`, `In Progress` to `status:in-progress`, and `Implemented`, `Complete`, or
`Verified` to `status:done`.

## Relationship To ARs And DRs

- Link ERs to the ARs they implement. The AR should also list the ER in `ER-Dependencies` so the
  parent AR can be marked `Implemented` or `Done` when all child work is complete.
- Use a DR instead of an ER when the primary purpose is defect reporting, reproduction, and fix
  tracking for a bug or regression.
