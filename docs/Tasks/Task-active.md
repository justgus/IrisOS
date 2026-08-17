# Active Tasks

Tasks listed here are assigned to a Sprint and actively being implemented.

Currently: **4 active Tasks**

---

## T-0084: Migrate ER records into Airframe Tasks

**Status:** Active
**GitHub Issue:** TBD
**Component:** ER to Task migration
**Priority:** Critical
**Epic:** EP-001
**Sprint Assigned:** SP-001
**Date Requested:** 2026-07-06
**Date Implemented:** TBD
**Date Verified:** TBD

**Rationale:**
ERs are the current IrisOS implementation work records and should become Airframe Tasks while preserving their existing state.

**Acceptance Criteria:**
1. Every ER document has exactly one migrated Task record or an explicit migration diagnostic.
2. Task records preserve source ER ID, title, status, GitHub issue number, dependencies, requirements, acceptance criteria, implementation notes, and verification plan.
3. Verified ERs are not downgraded.
4. Proposed ERs remain proposed/backlog unless explicitly assigned to active work.
5. Task numbering continues from the ER range and does not collide with existing ER IDs.

**Planning Notes:**
- ER-derived Task records should include `Legacy ID: ER-XXXX`.
- Existing GitHub issue numbers should be preserved even when later titles or labels are updated.
- Verified ERs should migrate to verified Task records only as a representation of existing approved state, not as new verification action.
- Proposed ERs should migrate to backlog Tasks unless the System Engineer explicitly assigns them to SP-001 or another active Sprint.

**Evidence:**
- ER-to-Task migration map recorded in `docs/Airframe-ER-Task-Migration.md`.
- The canonical Task files were generated under `docs/Tasks/Verified` and `docs/Tasks/Backlog`.
- The migration covers all 88 local ER records and assigns deterministic target Task IDs `T-0089` through `T-0176`.

## T-0085: Migrate DR records into Airframe Issues

**Status:** Active
**GitHub Issue:** TBD
**Component:** DR to Issue migration
**Priority:** High
**Epic:** EP-001
**Sprint Assigned:** SP-001
**Date Requested:** 2026-07-06
**Date Implemented:** TBD
**Date Verified:** TBD

**Rationale:**
DRs are defect records and should become Airframe Issues without losing diagnosis or verification context.

**Acceptance Criteria:**
1. Every DR document has exactly one migrated Issue record or an explicit migration diagnostic.
2. Issue records preserve source DR ID, title, status, GitHub issue number, severity, priority, environment, reproduction steps, expected behavior, actual behavior, impact, fix plan, and verification plan.
3. Verified DRs are not downgraded.
4. The migration distinguishes planned work Tasks from defect Issues.

**Planning Notes:**
- DR-derived Issue records should include `Legacy ID: DR-XXXX`.
- Verified DRs should migrate as resolved and verified only when that status is already present in the source ledger.
- New defects found during migration should become new Airframe Issues, not edits to the migrated historical DR.

**Evidence:**
- DR-to-Issue migration map recorded in `docs/Airframe-DR-Issue-Migration.md`.
- The canonical Issue file was generated under `docs/Issues/Verified`.
- The migration covers the local DR record and assigns target Issue ID `I-0001`.

## T-0086: Generate Airframe indexes and GitHub issue mapping

**Status:** Active
**GitHub Issue:** TBD
**Component:** Airframe indexes and issue mapping
**Priority:** Critical
**Epic:** EP-001
**Sprint Assigned:** SP-001
**Date Requested:** 2026-07-06
**Date Implemented:** TBD
**Date Verified:** TBD

**Rationale:**
The migrated records need deterministic indexes and an auditable mapping to existing GitHub issues before GitHub state can be synchronized.

**Acceptance Criteria:**
1. Task, Issue, Epic, and Sprint indexes reflect the migrated canonical state.
2. `docs/GitHub-Issue-Mapping.md` maps each migrated Task and Issue to its GitHub issue number where available.
3. Mapping entries identify source IDs such as `ER-0070` or `DR-0001`.
4. Known local/GitHub status drift is listed before any mutation.
5. Index counts match the migrated record counts.

**Planning Notes:**
- Index generation should be deterministic and sorted by Airframe ID.
- Counts should be derived from migrated records, not hand-maintained independently.
- Mapping should preserve both Airframe ID and legacy ID until the migration is closed.

**Evidence:**
- Local GitHub issue mapping recorded in `docs/GitHub-Issue-Mapping.md`.
- Canonical Task and Issue indexes were regenerated from the migrated records.
- The indexes cover 8 active Tasks, 8 backlog Tasks, 80 verified Tasks, and 1 verified Issue.

## T-0088: Validate migration equivalence and document cutover

**Status:** Active
**GitHub Issue:** TBD
**Component:** Migration validation and cutover
**Priority:** Critical
**Epic:** EP-001
**Sprint Assigned:** SP-001
**Date Requested:** 2026-07-06
**Date Implemented:** TBD
**Date Verified:** TBD

**Rationale:**
The migration should not be considered complete until the new canonical documentation is shown to represent the same state as the current manual documentation.

**Acceptance Criteria:**
1. Every source AR, ER, and DR is accounted for in the migrated state or diagnostics.
2. Status counts match the approved source of truth.
3. Legacy manual docs are either preserved or marked legacy according to the approved cutover path.
4. Build and documentation validation commands are recorded.
5. Residual risks and follow-up Tasks are documented before EP-001 closeout.

**Planning Notes:**
- Validation should include source coverage, status count comparison, GitHub mapping coverage, and deterministic index checks.
- Product build/test commands should be recorded as migration evidence, but product behavior changes are out of scope.
- Any unresolved mismatch should become a follow-up Task or Issue before EP-001 is proposed as Complete.

**Evidence:**
- Local migration validation recorded in `docs/Airframe-Migration-Validation.md`.
- Validation covers source coverage, status count comparison, mapping coverage, workspace seed checks, and residual follow-up work.
- Product build/tests were not run because this pass changed documentation only.

---

*Last Updated: 2026-07-07 (SP-001 task planning notes added)*
