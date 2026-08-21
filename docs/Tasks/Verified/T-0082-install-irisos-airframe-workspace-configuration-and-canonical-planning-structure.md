---
GitHub-Issue: TBD
---

# T-0082 — Install IrisOS Airframe Workspace Configuration and Canonical Planning Structure

## Task Metadata

- Task ID: T-0082
- Status: Verified
- Epic: EP-001
- Sprint Assigned: SP-001
- GitHub Issue: TBD
- Component: Airframe workspace configuration
- Priority: High
- Date Requested: 2026-07-06
- Date Implemented: 2026-08-17
- Date Verified: 2026-08-17
- Verified By: System Engineer

## Rationale

IrisOS needs a repo-local Agile Airframe configuration and canonical planning folders before detailed migration records can be introduced.

## Acceptance Criteria

1. `.airframe/airframe-workspace.json` identifies the IrisOS workspace and `justgus/IrisOS` backend.
2. `docs/Epics`, `docs/Sprints`, `docs/Tasks`, and `docs/Issues` exist with initial index files.
3. The active project references `EP-001` and `SP-001`.
4. Existing manual AR/ER/DR documents remain in place during this seed step.

## Evidence

- Workspace configuration exists at `.airframe/airframe-workspace.json`.
- Canonical Airframe planning indexes exist under `docs/Epics`, `docs/Sprints`, `docs/Tasks`, and `docs/Issues`.
- Workspace seed checks are recorded in `docs/Airframe-Migration-Validation.md`.
- Existing AR, ER, and DR documents remain in their legacy documentation locations.
- Airframe canonical diagnostics passed on 2026-08-17.

## Verification

The System Engineer approved T-0082 as Verified on 2026-08-17.
