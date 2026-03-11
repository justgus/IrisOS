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

Only the System Engineer may mark an ER as `Verified`.

## Document Location

- ER documents live in `docs/ER/`.
- The ER status ledger lives in `docs/ER/ER-Status.md`.

## Source Template

Use `docs/ER/ER-Template.md` as the source format for new ERs.

## Required Metadata

Each ER should include:

- `GitHub-Issue: #<number>` in front matter
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
- `Verified` is reserved for the System Engineer.

Keep `docs/ER/ER-Status.md` aligned with the current document status.

## Writing Guidance

- State the problem and constraints concretely.
- Keep goals, non-goals, and scope separate.
- Write acceptance criteria that can actually be checked.
- List real dependencies, including prerequisite ARs or ERs.
- Add implementation notes only when they reduce ambiguity for the implementer.
- Include a verification plan with exact tests and manual checks where practical.

## Workflow

1. Create a GitHub Issue using the ER issue template.
2. Draft the ER in `docs/ER/` from `docs/ER/ER-Template.md`.
3. Add the Issue number to the front matter and the doc path to the Issue body.
4. Implement the work on a branch and reference the Issue in the PR.
5. Update the ER status in the same commit as the implementation progress it describes.
6. Update `docs/ER/ER-Status.md` when the ER status changes.
7. System Engineer reviews, tests, and marks the ER `Verified` when complete.

## GitHub Workflow

- Use the `er` label plus one status label.
- Add area labels only when they improve triage.
- `scripts/issue_sync.sh <doc_path>` can synchronize title, body, and labels from the ER document.

The sync script maps `Draft` and `Proposed` to `status:proposed`, `Approved` to
`status:accepted`, `In Progress` to `status:in-progress`, and `Implemented`, `Complete`, or
`Verified` to `status:done`.

## Relationship To ARs And DRs

- Link ERs to the ARs they implement when applicable.
- Use a DR instead of an ER when the primary purpose is defect reporting, reproduction, and fix
  tracking for a bug or regression.
