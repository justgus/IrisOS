---
GitHub-Issue: N/A
---

# DR Guidelines

## Purpose

Defect Reports (DRs) record bugs, regressions, discrepancies, or corrective tasks that need clear
triage and verification. A DR should make the defect reproducible, explain impact, and define a
minimal fix plan and verification path.

## Roles

- Implementation Engineer: drafts the DR, performs triage, implements the fix, and updates status.
- System Engineer: reviews the defect resolution and may mark the DR as `Verified`.

Only the System Engineer may mark a DR as `Verified`.

## Document Location

- DR documents live in `docs/DR/`.
- The DR status ledger lives in `docs/DR/DR-Status.md`.

## Source Template

Use `docs/DR/DR-Template.md` as the source format for new DRs.

## Required Metadata

Each DR should include:

- `GitHub-Issue: #<number>` in front matter
- `DR ID`
- `Title`
- `Status`
- `Date`
- `Owners`
- `Severity`
- `Priority`

## Status Guidance

Use the template status values:

- `New`
- `Triaged`
- `Proposed`
- `In Progress`
- `Fixed`
- `Complete`
- `Verified`
- `Deferred`
- `Rejected`

In general:

- `New` means the issue has been recorded.
- `Triaged` means severity, scope, and likely area are understood.
- `Proposed` means a fix approach has been selected.
- `In Progress` means implementation is underway.
- `Fixed` or `Complete` means the change landed but has not yet been verified.
- `Verified` is reserved for the System Engineer.
- `Deferred` or `Rejected` should explain why the work is not proceeding.

Keep `docs/DR/DR-Status.md` aligned with the current document status.

## Writing Guidance

- Make reproduction steps short, deterministic, and specific.
- Separate expected behavior from actual behavior.
- State user impact and system impact directly.
- Capture available evidence, even if it is only shell output or a short log excerpt.
- Keep the proposed fix narrow and note alternatives only when they affect triage.
- Include a verification plan that checks both the original failure and regression risk.

## Workflow

1. Create a GitHub Issue using the DR issue template.
2. Draft the DR in `docs/DR/` from `docs/DR/DR-Template.md`.
3. Add the Issue number to the front matter and the doc path to the Issue body.
4. Triage the defect before broad implementation changes.
5. Implement the fix on a branch and reference the Issue in the PR.
6. Update the DR status in the same commit as the implementation progress it describes.
7. Update `docs/DR/DR-Status.md` when the DR status changes.
8. System Engineer reviews, reproduces as needed, and marks the DR `Verified`.

## GitHub Workflow

- Use the `dr` label plus one status label.
- Add area labels only when they improve triage.
- `scripts/issue_sync.sh <doc_path>` can synchronize title, body, and labels from the DR document.

The sync script maps `New` and `Proposed` to `status:proposed`, `In Progress` to
`status:in-progress`, and `Fixed`, `Complete`, or `Verified` to `status:done`. `Triaged` and
`Deferred` currently fall through to `status:in-progress`, so update labels manually if a different
presentation is needed.

## Relationship To ARs And ERs

- Use a DR when the main job is to describe and fix a defect.
- Link an ER only when the defect fix grows into a larger implementation effort.
- Link an AR only when the defect reveals an architectural gap or forces a design decision.
