---
GitHub-Issue: N/A
---

# ER Guidelines

## Purpose

Engineering Requests (ERs) define implementation-scoped work. They are the planning and acceptance
record for new functionality, integration milestones, or grouped delivery work such as epics and
sprints.

Use an ER when work needs:

- clear scope and non-goals
- acceptance criteria
- implementation and verification notes
- a durable record of what "done" means

Do not use an ER for:

- architecture-only decisions with no immediate implementation scope
- one-off bug triage where the primary artifact should be a DR

## Document Shape

ERs should follow `docs/ER/ER-Template.md`. In practice, the important sections are:

- metadata
- context
- goals and non-goals
- scope
- requirements
- proposed approach
- acceptance criteria
- dependencies
- implementation notes
- verification plan

If an ER grows into a roadmap or umbrella item, keep the parent ER short and split concrete work
into sub-ERs.

## Status Lifecycle

ER documents may use:

- `Draft`
- `Proposed`
- `Approved`
- `In Progress`
- `Implemented`
- `Complete`
- `Verified`
- `Rejected`

Recommended usage:

- `Draft` / `Proposed`: the work is being shaped
- `Approved`: direction accepted, implementation not yet started
- `In Progress`: implementation underway
- `Implemented` / `Complete`: implementation landed, verification still pending
- `Verified`: system engineer has confirmed the result

Only the System Engineer marks an ER as `Verified`.

## ER Ledger and Verification

- Keep the ER document status current.
- Update `docs/ER/ER-Status.md` when ER status changes.
- When implementing an ER, update its status in the same commit as the implementation.
- Do not mark an ER `Verified` from an implementation-only pass.

## Relationship to ARs and DRs

- An ER may implement one or more AR decisions.
- An ER may also be the planned fix vehicle for a DR.
- If a DR exposes a defect but the fix requires substantial scoped work, the DR can point to the
  implementing ER.

ERs should stay implementation-oriented. If the core question is "what architecture should we use,"
write an AR first.

## GitHub Tracking

- Use label `er` for the issue type.
- Use one coarse-grained status label on the issue.

Issue label mapping is intentionally broader than document status:

- `status:proposed` maps to `Draft` and `Proposed`
- `status:accepted` maps to `Approved`
- `status:in-progress` maps to `In Progress`
- `status:done` maps to `Implemented`, `Complete`, `Verified`, and `Rejected`

The ER document is the source of truth for the precise status.

## Review Standard

An ER is ready for implementation when:

- scope is small enough to review coherently
- dependencies are explicit
- acceptance criteria are testable
- verification steps are concrete
- out-of-scope items are clearly excluded

An ER is ready for `Complete` when implementation and intended validation are in place.
