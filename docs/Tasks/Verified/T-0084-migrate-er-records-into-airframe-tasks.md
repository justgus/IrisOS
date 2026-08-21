---
GitHub-Issue: TBD
---

# T-0084 — Migrate ER Records into Airframe Tasks

## Task Metadata

- Task ID: T-0084
- Status: Verified
- Epic: EP-001
- Sprint Assigned: SP-001
- GitHub Issue: TBD
- Component: ER to Task migration
- Priority: Critical
- Date Requested: 2026-07-06
- Date Implemented: 2026-08-17
- Date Verified: 2026-08-17
- Verified By: System Engineer

## Rationale

ERs are the current IrisOS implementation work records and should become Airframe Tasks while preserving their existing state.

## Acceptance Criteria

1. Every ER document has exactly one migrated Task record or an explicit migration diagnostic.
2. Task records preserve source ER ID, title, status, GitHub issue number, dependencies, requirements, acceptance criteria, implementation notes, and verification plan.
3. Verified ERs are not downgraded.
4. Proposed ERs remain proposed/backlog unless explicitly assigned to active work.
5. Task numbering continues from the ER range and does not collide with existing ER IDs.

## Evidence

- The ER-to-Task migration map is recorded in `docs/Airframe-ER-Task-Migration.md`.
- Canonical Task files exist under `docs/Tasks/Verified` and `docs/Tasks/Backlog`.
- All 88 local ER records are represented by deterministic Task IDs `T-0089` through `T-0176`.
- Source status counts are preserved as 80 Verified Tasks and 8 Backlog Tasks.
- Legacy ER IDs, GitHub references, dependencies, requirements, acceptance criteria, implementation notes, and verification plans are preserved.
- Airframe canonical diagnostics passed after the verified transition.

## Verification

The System Engineer approved T-0084 as Verified on 2026-08-17.
