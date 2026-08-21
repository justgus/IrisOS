---
GitHub-Issue: TBD
---

# T-0086 — Generate Airframe Indexes and GitHub Issue Mapping

## Task Metadata

- Task ID: T-0086
- Status: Verified
- Epic: EP-001
- Sprint Assigned: SP-001
- GitHub Issue: TBD
- Component: Airframe indexes and issue mapping
- Priority: Critical
- Date Requested: 2026-07-06
- Date Implemented: 2026-08-17
- Date Verified: 2026-08-17
- Verified By: System Engineer

## Rationale

The migrated records need deterministic indexes and an auditable mapping to existing GitHub issues before GitHub state can be synchronized.

## Acceptance Criteria

1. Task, Issue, Epic, and Sprint indexes reflect the migrated canonical state.
2. `docs/GitHub-Issue-Mapping.md` maps each migrated Task and Issue to its GitHub issue number where available.
3. Mapping entries identify source IDs such as `ER-0070` or `DR-0001`.
4. Known local/GitHub status drift is listed before any mutation.
5. Index counts match the migrated record counts.

## Evidence

- The local GitHub issue mapping is recorded in `docs/GitHub-Issue-Mapping.md`.
- The mapping contains 88 ER-derived Task mappings and one DR-derived Issue mapping.
- Mapping entries preserve Airframe IDs, legacy IDs, existing GitHub issue numbers, and titles.
- Pre-mutation local/GitHub drift is recorded in `docs/Airframe-GitHub-Live-Audit.md`.
- Task, Issue, Epic, and Sprint indexes reflect the current canonical state.
- After this transition the Task index contains 1 active, 8 backlog, and 87 verified Tasks; the Issue index contains 1 verified Issue.
- Airframe canonical diagnostics passed after the verified transition.

## Verification

The System Engineer approved T-0086 as Verified on 2026-08-17.
