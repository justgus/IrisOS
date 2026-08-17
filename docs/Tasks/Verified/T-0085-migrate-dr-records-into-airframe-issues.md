---
GitHub-Issue: TBD
---

# T-0085 — Migrate DR Records into Airframe Issues

## Task Metadata

- Task ID: T-0085
- Status: Verified
- Epic: EP-001
- Sprint Assigned: SP-001
- GitHub Issue: TBD
- Component: DR to Issue migration
- Priority: High
- Date Requested: 2026-07-06
- Date Implemented: 2026-08-17
- Date Verified: 2026-08-17
- Verified By: System Engineer

## Rationale

DRs are defect records and should become Airframe Issues without losing diagnosis or verification context.

## Acceptance Criteria

1. Every DR document has exactly one migrated Issue record or an explicit migration diagnostic.
2. Issue records preserve source DR ID, title, status, GitHub issue number, severity, priority, environment, reproduction steps, expected behavior, actual behavior, impact, fix plan, and verification plan.
3. Verified DRs are not downgraded.
4. The migration distinguishes planned work Tasks from defect Issues.

## Evidence

- The DR-to-Issue migration map is recorded in `docs/Airframe-DR-Issue-Migration.md`.
- The canonical Issue record exists under `docs/Issues/Verified`.
- The sole local DR record is represented by Issue ID `I-0001`.
- Legacy ID `DR-0001`, GitHub issue `#116`, status, diagnosis, reproduction, impact, fix, and verification details are preserved.
- The migrated Issue retains the approved Verified source status.
- Airframe canonical diagnostics passed after the verified transition.

## Verification

The System Engineer approved T-0085 as Verified on 2026-08-17.
