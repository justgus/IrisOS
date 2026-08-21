# SP-001: Agile Airframe Migration Planning and Seed Implementation

**Status:** Closed
**Epic:** EP-001
**Goal:** Plan and implement the Agile Airframe documentation migration seed, audit, canonical conversion, GitHub mapping plan, and cutover validation.
**Start Date:** 2026-07-06
**End Date:** 2026-08-21
**Capacity:** TBD

### Sprint Planning

**Sprint Objective:**
Produce the reviewed canonical migration plan and first migration-ready Airframe documentation state for IrisOS without changing product behavior, reconstructing historical Sprints, or mutating GitHub before approval.

**Execution Order:**

1. Complete the source audit and identify local/GitHub drift.
2. Finalize the AR architecture-record boundary.
3. Convert ER and DR records into reviewable Airframe Task and Issue records.
4. Generate deterministic indexes and `docs/GitHub-Issue-Mapping.md`.
5. Prepare the GitHub mutation plan as a proposed operation, not an executed operation.
6. Validate equivalence between the current manual docs and the migrated Airframe state.

**Review Gates:**

| Gate | Required Before Proceeding |
| ---- | -------------------------- |
| Audit gate | All AR, ER, and DR records inventoried with source path, local status, and GitHub issue reference. |
| Schema gate | AR, ER, and DR target mappings reviewed for loss of status, dependency, or verification data. |
| Projection gate | Airframe indexes and migrated records match approved source counts and statuses. |
| GitHub gate | Mutation plan reviewed before any issue title, label, body, or state change. |
| Closeout gate | Validation evidence recorded and residual risks converted into follow-up Tasks. |

**Sprint Deliverables:**

- Migration audit.
- AR architecture-record handling decision.
- ER-to-Task migration output.
- DR-to-Issue migration output.
- GitHub issue mapping.
- GitHub mutation plan.
- Cutover validation notes.

### Assigned Tasks

| Task | Title | Priority | Status |
| ---- | ----- | -------- | ------ |
| T-0081 | Audit current AR/ER/DR documentation and GitHub issue state | High | Verified |
| T-0082 | Install IrisOS Airframe workspace configuration and canonical planning structure | High | Verified |
| T-0083 | Define separate architecture-record handling for ARs | High | Verified |
| T-0084 | Migrate ER records into Airframe Tasks | Critical | Verified |
| T-0085 | Migrate DR records into Airframe Issues | High | Verified |
| T-0086 | Generate Airframe indexes and GitHub issue mapping | Critical | Verified |
| T-0087 | Prepare approved GitHub issue synchronization migration | High | Verified |
| T-0088 | Validate migration equivalence and document cutover | Critical | Verified |

### Assigned Issues

None.

### Sprint Notes

- SP-001 is the first IrisOS Airframe Sprint.
- No historical Sprint records were reconstructed.
- ARs remain separate architecture records and were not migrated into Epics.
- GitHub synchronization was reviewed and completed before Sprint closeout.
- Local generated Autotools/build artifacts were not part of the migration planning scope.

### Retrospective

**Completed:**
- All eight assigned Tasks were verified.
- The Airframe migration, GitHub synchronization, and cutover validation were completed.

**Returned to Backlog:**
- None.

**What went well:**
- Source records and GitHub mappings were preserved and validated before closeout.

**What to improve:**
- Agile Cockpit needs branch-aware mutation support so workflow changes are not written directly to a protected branch; tracked as AgileAirframe issue #177.

**Carry-forward notes:**
- Product backlog Tasks were reassigned to EP-002 and EP-003 for future Sprint planning.
