# Airframe Migration Validation

**Date:** 2026-07-07
**Scope:** Local documentation validation
**Epic:** EP-001
**Task:** T-0088

## Boundary

This validation covers local documentation migration artifacts only. It does not build product code, run product tests, query GitHub, or mutate GitHub issue state.

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
| Live GitHub issue state, titles, and labels were not queried. | Run a separate approved GitHub-backed read-only audit before mutation. |
| Shared local GitHub references may be intentional grouping or drift. | System Engineer review needed for `#181`, `#182`, and `#184` grouped ER mappings. |
| Four ARs have local `GitHub-Issue: #TBD`. | Resolve or defer AR issue references during GitHub synchronization planning. |
| Live GitHub synchronization has not been performed. | Review and approve `docs/GitHub-Issue-Synchronization-Plan.md` before any GitHub mutation. |

## Cutover Position

The local canonical documentation migration is ready for System Engineer review. The audit, AR boundary, ER mapping, DR mapping, generated Task records, generated Issue records, local GitHub mapping, local-only synchronization plan, and regenerated indexes are present in the Airframe documentation state.

Final GitHub synchronization should wait until:

1. The generated Task and Issue records are reviewed.
2. The local duplicate GitHub issue references are approved or corrected.
3. A GitHub-backed read-only audit is approved and performed.
4. GitHub synchronization is approved and performed, or explicitly deferred.
