---
GitHub-Issue: N/A
---

# AR Guidelines

## Purpose

Architecture Recommendations (ARs) capture durable technical decisions that shape IrisOS structure,
interfaces, or long-lived constraints. An AR is the record for "we considered options and chose
this model."

Use an AR when a change:

- affects multiple subsystems or future implementation work
- introduces a new architectural boundary, invariant, or system model
- needs explicit alternatives and tradeoff discussion
- should remain readable after the original implementation context is gone

Do not use an AR for:

- a single bug fix
- an isolated implementation task
- progress tracking without an architectural choice

## Document Shape

ARs should stay short and decision-focused. The current AR corpus uses this structure:

- title and metadata
- context
- decision or recommendation
- alternatives considered
- consequences
- implementation notes when needed

Write the "why" clearly. The implementation details belong in ERs unless they are necessary to
explain the decision itself.

## Status and Location

AR status is reflected both in the document and in its directory:

- `Proposed`: draft under `docs/AR/proposed/`
- `Accepted`: approved record under `docs/AR/accepted/`
- `Rejected`: keep only if the record is useful for future context

When an AR is accepted, move it from `docs/AR/proposed/` to `docs/AR/accepted/` in the same change
that updates its status.

If an accepted AR is later replaced, keep the old AR in `accepted/` and note that it has been
superseded rather than deleting history.

## Relationship to ERs

ARs describe architecture. ERs implement that architecture.

- Prefer landing the AR before or with the first ER that depends on it.
- If an AR depends on multiple implementation tasks, list them in `ER-Dependencies:`.
- The sync script can auto-close an AR issue after all listed ERs are `Verified`.

An AR should not become a backlog dump. If the "decision" section turns into a work breakdown, split
that work into ERs.

## Review Standard

An AR is ready for acceptance when:

- the problem statement is concrete
- the chosen direction is unambiguous
- major alternatives are named and rejected for explicit reasons
- consequences are honest about tradeoffs and constraints
- downstream ER authors can implement from it without guessing the intended model

## GitHub Tracking

- Use label `ar` for the issue type.
- Use one status label.
- GitHub issue status is coarse-grained:
  - `status:proposed` for proposed ARs
  - `status:accepted` for accepted ARs
  - `status:done` for closed-out or rejected records

The AR doc remains the source of truth if the document status is more precise than the issue label.
