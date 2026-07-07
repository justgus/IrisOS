# Active Tasks

Tasks listed here are assigned to a Sprint and actively being implemented.

Currently: **8 active Tasks**

---

## T-0081: Audit current AR/ER/DR documentation and GitHub issue state

**Status:** Active
**GitHub Issue:** TBD
**Component:** Workflow documentation audit
**Priority:** High
**Epic:** EP-001
**Sprint Assigned:** SP-001
**Date Requested:** 2026-07-06
**Date Implemented:** TBD
**Date Verified:** TBD

**Rationale:**
The migration must preserve the current manual documentation state and identify local/GitHub drift before any state is rewritten.

**Acceptance Criteria:**
1. All current AR, ER, and DR documents are inventoried.
2. Current local status for each AR, ER, and DR is captured.
3. Current GitHub issue number, title, state, and labels are captured where available.
4. Local/GitHub mismatches are listed explicitly.
5. The audit does not mutate GitHub state.

**Planning Notes:**
- Capture source path, source ID, title, local status, GitHub issue number, GitHub issue state, GitHub labels, and target Airframe ID.
- Treat local documentation as authoritative unless the System Engineer approves a GitHub-derived correction.
- Record mismatches as migration diagnostics rather than silently normalizing them.

**Evidence:**
- Local-only audit recorded in `docs/Airframe-Migration-Audit.md`.
- Live GitHub issue state, titles, and labels were intentionally not queried in this pass.
- System Engineer approved the local audit on 2026-07-07.

## T-0082: Install IrisOS Airframe workspace configuration and canonical planning structure

**Status:** Active
**GitHub Issue:** TBD
**Component:** Airframe workspace configuration
**Priority:** High
**Epic:** EP-001
**Sprint Assigned:** SP-001
**Date Requested:** 2026-07-06
**Date Implemented:** TBD
**Date Verified:** TBD

**Rationale:**
IrisOS needs a repo-local Agile Airframe configuration and canonical planning folders before detailed migration records can be introduced.

**Acceptance Criteria:**
1. `.airframe/airframe-workspace.json` identifies the IrisOS workspace and `justgus/IrisOS` backend.
2. `docs/Epics`, `docs/Sprints`, `docs/Tasks`, and `docs/Issues` exist with initial index files.
3. The active project references `EP-001` and `SP-001`.
4. Existing manual AR/ER/DR documents remain in place during this seed step.

**Planning Notes:**
- Keep generated build artifacts out of Airframe planning commits.
- Do not move or archive legacy docs as part of the seed structure.
- Any later legacy-doc relocation needs its own approved cutover Task.

**Evidence:**
- ER-to-Task migration map recorded in `docs/Airframe-ER-Task-Migration.md`.
- The map covers all 88 local ER records and assigns deterministic target Task IDs `T-0089` through `T-0176`.
- Full Task body generation remains a follow-up migration step after mapping review.

## T-0083: Define separate architecture-record handling for ARs

**Status:** Active
**GitHub Issue:** TBD
**Component:** Architecture documentation migration
**Priority:** High
**Epic:** EP-001
**Sprint Assigned:** SP-001
**Date Requested:** 2026-07-06
**Date Implemented:** TBD
**Date Verified:** TBD

**Rationale:**
The System Engineer directed that ARs remain separate architecture records. The Airframe migration must not treat accepted ARs as delivery Epics.

**Acceptance Criteria:**
1. The migration defines where architecture records live after cutover.
2. Existing AR IDs, statuses, GitHub issue references, dependencies, and content are preserved.
3. Airframe Epics are limited to delivery planning and do not replace ARs.
4. The master index clearly distinguishes architecture records from Epics.

**Planning Notes:**
- Candidate target is an architecture-record area that preserves the current AR namespace.
- AR GitHub issues remain architecture-tracking issues, not Airframe Task or Issue records.
- ER and DR dependencies on ARs should remain trace links after migration.

**Evidence:**
- AR handling boundary recorded in `docs/Airframe-Architecture-Record-Handling.md`.
- `docs/governance/Master-Index.md` now identifies ARs as the architecture record system of record and explicitly distinguishes them from Agile Airframe Epics.

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
- DR-to-Issue migration map recorded in `docs/Airframe-DR-Issue-Migration.md`.
- The map covers the local DR record and assigns target Issue ID `I-0001`.
- Full Issue body generation remains a follow-up migration step after mapping review.

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
- Local GitHub issue mapping recorded in `docs/GitHub-Issue-Mapping.md`.
- The mapping covers 88 ER-derived Task mappings and 1 DR-derived Issue mapping.
- Canonical Task and Issue index projection remains pending full migrated record body review.

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
- Local-only GitHub synchronization plan recorded in `docs/GitHub-Issue-Synchronization-Plan.md`.
- The plan identifies required review decisions and explicitly defers all GitHub mutation until a separate approved GitHub-backed audit.
- No GitHub commands were run.

## T-0087: Prepare approved GitHub issue synchronization migration

**Status:** Active
**GitHub Issue:** TBD
**Component:** GitHub issue synchronization
**Priority:** High
**Epic:** EP-001
**Sprint Assigned:** SP-001
**Date Requested:** 2026-07-06
**Date Implemented:** TBD
**Date Verified:** TBD

**Rationale:**
GitHub issue titles, bodies, and labels should only be changed after the local migration map is reviewed and approved.

**Acceptance Criteria:**
1. A GitHub mutation plan identifies all proposed title, body, label, and state changes.
2. The plan preserves existing issue numbers.
3. The plan identifies mismatches that need System Engineer approval.
4. No GitHub mutation is executed as part of this Task without explicit approval.
5. The migration can be rerun or reviewed without hidden side effects.

**Planning Notes:**
- Proposed GitHub changes should be generated as a reviewable command list or table before execution.
- The plan should distinguish safe metadata additions from state-changing operations such as closing issues.
- Legacy labels should remain until the System Engineer approves removing or replacing them.

**Evidence:**
- Workspace configuration exists at `.airframe/airframe-workspace.json`.
- Canonical Airframe planning indexes exist under `docs/Epics`, `docs/Sprints`, `docs/Tasks`, and `docs/Issues`.
- Workspace seed checks are recorded in `docs/Airframe-Migration-Validation.md`.

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
