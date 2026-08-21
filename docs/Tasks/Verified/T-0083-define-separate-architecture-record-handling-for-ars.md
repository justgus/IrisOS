---
GitHub-Issue: TBD
---

# T-0083 — Define Separate Architecture-Record Handling for ARs

## Task Metadata

- Task ID: T-0083
- Status: Verified
- Epic: EP-001
- Sprint Assigned: SP-001
- GitHub Issue: TBD
- Component: Architecture documentation migration
- Priority: High
- Date Requested: 2026-07-06
- Date Implemented: 2026-08-17
- Date Verified: 2026-08-17
- Verified By: System Engineer

## Rationale

The System Engineer directed that ARs remain separate architecture records. The Airframe migration must not treat accepted ARs as delivery Epics.

## Acceptance Criteria

1. The migration defines where architecture records live after cutover.
2. Existing AR IDs, statuses, GitHub issue references, dependencies, and content are preserved.
3. Airframe Epics are limited to delivery planning and do not replace ARs.
4. The master index clearly distinguishes architecture records from Epics.

## Evidence

- The AR handling boundary is recorded in `docs/Airframe-Architecture-Record-Handling.md`.
- `docs/governance/Master-Index.md` identifies ARs as the architecture record system of record and distinguishes them from Airframe Epics.
- The migration audit preserves all 26 AR IDs, statuses, GitHub references, dependencies, titles, and source paths.
- ER and DR records retain their AR dependency trace links.
- Airframe canonical diagnostics passed after the verified transition.

## Verification

The System Engineer approved T-0083 as Verified on 2026-08-17.
