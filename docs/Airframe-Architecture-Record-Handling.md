# Airframe Architecture Record Handling

**Date:** 2026-07-07
**Scope:** Local documentation migration
**Epic:** EP-001
**Task:** T-0083

## Decision

Architecture Recommendations remain architecture records after the Agile Airframe migration. They are not converted into Airframe Epics, Tasks, or Issues.

The canonical AR locations remain:

- Proposed ARs: `docs/AR/proposed/`
- Accepted ARs: `docs/AR/accepted/`
- AR guidance: `docs/governance/AR-Guidelines.md`

Airframe Epics describe delivery outcomes. They may reference ARs as architectural context, but they do not replace the AR namespace or AR lifecycle.

## Migration Rules

| Source Record | Migration Handling |
| ------------- | ------------------ |
| Proposed AR | Preserve as an AR under `docs/AR/proposed/`. |
| Accepted AR | Preserve as an AR under `docs/AR/accepted/`. |
| Implemented AR | Preserve as an AR with its current status and dependency links. |
| AR GitHub reference | Preserve the local `GitHub-Issue:` value as an architecture-tracking reference. |
| AR dependencies | Preserve `ER-Dependencies` and `DR-Dependencies` as AR trace links. |

## Airframe Relationship

- Airframe Epics may cite ARs in rationale, scope, or dependency notes.
- ER-derived Airframe Tasks may keep AR references as legacy architectural trace links.
- DR-derived Airframe Issues may keep AR references as legacy architectural trace links.
- AR GitHub issues remain architecture-tracking issues unless the System Engineer approves a separate issue synchronization change.

## Non-Goals

- Do not reclassify accepted ARs as Airframe Epics.
- Do not invent historical Epics from existing ARs.
- Do not move AR files as part of the Airframe seed migration.
- Do not normalize AR GitHub issue references without a reviewed GitHub mapping plan.
- Do not change AR status values as part of this handling decision.

## Local Inventory Result

The local audit found 26 AR records:

| Status | Count |
| ------ | ----- |
| Accepted | 11 |
| Implemented | 15 |

Four ARs currently have local `GitHub-Issue: #TBD` references: AR-0018, AR-0019, AR-0020, and AR-0021.

These are migration diagnostics for later mapping review, not blockers for preserving the AR namespace.

## Cutover Expectation

After Airframe cutover:

1. `docs/AR/` remains the architecture record area.
2. `docs/Epics/` contains delivery-planning Epics only.
3. Airframe projections preserve links back to AR IDs where implementation or defect records depend on architecture direction.
4. The governance master index distinguishes ARs from Airframe Epics.
