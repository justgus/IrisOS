# Airframe Migration Validation

**Date:** 2026-08-21
**Scope:** Documentation and GitHub mapping validation
**Epic:** EP-001
**Task:** T-0088

## Boundary

This validation covers the documentation migration artifacts and the approved GitHub synchronization recorded in `docs/Airframe-GitHub-Live-Audit.md`. It does not build or change product code.

## Source Coverage

| Source Area | Expected From Local Audit | Represented In Migration Artifacts | Result |
| ----------- | ------------------------- | ---------------------------------- | ------ |
| AR records | 26 | 26 in `docs/Airframe-Migration-Audit.md`; preserved by `docs/Airframe-Architecture-Record-Handling.md` | Pass |
| ER records | 88 | 88 in `docs/Airframe-ER-Task-Migration.md` | Pass |
| DR records | 1 | 1 in `docs/Airframe-DR-Issue-Migration.md` | Pass |

## Status Count Comparison

| Record Type | Source Status Counts | Target Representation | Result |
| ----------- | -------------------- | --------------------- | ------ |
| AR | Accepted: 11; Implemented: 15 | Preserved as AR records; not converted to Epics | Pass |
| ER | Verified: 80; Proposed: 8 | Verified Task records: 80; Backlog Task records: 8 | Pass |
| DR | Verified: 1 | Verified Issue records: 1 | Pass |

## Mapping Coverage

| Mapping | Expected | Actual | Result |
| ------- | -------- | ------ | ------ |
| ER-derived Task IDs | 88 | `T-0089` through `T-0176` | Pass |
| DR-derived Issue IDs | 1 | `I-0001` | Pass |
| GitHub issue mapping rows | 89 | 88 Task mappings plus 1 Issue mapping | Pass |

## Workspace Seed Validation

| Check | Result |
| ----- | ------ |
| `.airframe/airframe-workspace.json` exists | Pass |
| Workspace identifies `justgus/IrisOS` | Pass |
| Workspace active Epic is `EP-001` | Pass |
| Workspace active Sprint is `SP-001` | Pass |
| `docs/Epics`, `docs/Sprints`, `docs/Tasks`, and `docs/Issues` indexes exist | Pass |

## Local Validation Commands

These commands were used for local validation:

```sh
git status --short --branch
find docs/Epics docs/Sprints docs/Tasks docs/Issues -maxdepth 1 -type f | sort
rg -n "^# (AR|ER|DR)-|^- Status:|^GitHub-Issue:|^AR-Dependencies:|^- (ER|DR)-[0-9]" docs/AR docs/ER docs/DR
ruby -e '...local source coverage checks...'
ruby -e '...local ER and DR mapping coverage checks...'
```

## Residual Risks And Follow-Up Work

| Risk | Follow-Up |
| ---- | --------- |
| GitHub state can drift after validation. | Re-run the read-only comparison before future migration-related mutations. |

## Cutover Position

The local canonical documentation migration is ready for System Engineer review. The audit, AR boundary, ER mapping, DR mapping, generated Task records, generated Issue records, local GitHub mapping, local-only synchronization plan, and regenerated indexes are present in the Airframe documentation state.

SP-001 migration closeout is ready for System Engineer review because:

1. The generated Task and Issue records are reviewed.
2. The local duplicate GitHub issue references are approved.
3. Every AR, ER, and DR record has a numeric GitHub issue reference.
4. The post-synchronization audit reports no remaining title, status-label, or required-closure drift.
