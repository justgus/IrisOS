# Active Tasks

Tasks listed here are assigned to a Sprint and actively being implemented.

Currently: **2 active Tasks**

---

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
