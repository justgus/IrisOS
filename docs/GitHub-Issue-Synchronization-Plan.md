# GitHub Issue Synchronization Plan

**Date:** 2026-07-07
**Scope:** Local documentation migration plan
**Epic:** EP-001
**Task:** T-0087

## Boundary

This plan is documentation-only. It does not query GitHub and does not mutate issue titles, labels, bodies, or state.

Because this migration pass is local-only, the plan cannot claim live GitHub drift, current labels, current titles, or current issue state. Those fields require a separate approved GitHub-backed audit.

## Inputs

- `docs/Airframe-Migration-Audit.md`
- `docs/Airframe-ER-Task-Migration.md`
- `docs/Airframe-DR-Issue-Migration.md`
- `docs/GitHub-Issue-Mapping.md`

## Required Review Decisions Before Mutation

| Decision | Reason |
| -------- | ------ |
| Confirm whether shared local GitHub refs remain grouped or split | Multiple ER records share `#181`, `#182`, and `#184`. |
| Resolve or defer AR records with local `#TBD` refs | AR-0018, AR-0019, AR-0020, and AR-0021 have no local issue number. |
| Confirm whether verified ER/DR records should close GitHub issues | Local source status is Verified, but live GitHub state was not queried. |
| Confirm label vocabulary for Airframe Tasks and Issues | Existing GitHub labels were not queried in this local-only pass. |
| Confirm whether legacy AR/ER/DR labels remain during transition | Removing labels is a state-changing operation and needs explicit approval. |

## Proposed Synchronization Phases

1. Run a GitHub-backed read-only audit for mapped issue numbers.
2. Compare live GitHub title, state, labels, and body summary against local mapping docs.
3. Produce a review table of proposed changes.
4. Review shared issue references and `#TBD` architecture records with the System Engineer.
5. After approval, apply only the approved GitHub mutations.
6. Record mutation evidence and any deferred items in the Airframe migration docs.

## Proposed Change Categories

| Category | Proposed Handling |
| -------- | ----------------- |
| Title alignment | Defer until live issue titles are audited. |
| Body alignment | Defer until full migrated Task/Issue bodies are reviewed. |
| Label additions | Defer until current label vocabulary and live labels are audited. |
| Label removals | Defer until System Engineer explicitly approves transition away from legacy labels. |
| State changes | Defer until live GitHub state is audited and verified source-state handling is approved. |
| Issue creation | Defer for AR `#TBD` records; ARs are architecture records, not Airframe Tasks or Issues. |

## Explicit Non-Actions In This Pass

- No GitHub API calls.
- No `gh issue edit` commands.
- No issue creation.
- No issue closure.
- No label creation or removal.
- No title or body changes.

## Local Diagnostics Carried Forward

| Diagnostic | Records |
| ---------- | ------- |
| AR records with local `GitHub-Issue: #TBD` | AR-0018, AR-0019, AR-0020, AR-0021 |
| Multiple ER records sharing one local GitHub issue reference | `#181`: ER-0045, ER-0045.1, ER-0045.2, ER-0045.3, ER-0045.4 |
| Multiple ER records sharing one local GitHub issue reference | `#182`: ER-0046, ER-0047.1, ER-0047.2, ER-0047.3, ER-0047.4 |
| Multiple ER records sharing one local GitHub issue reference | `#184`: ER-0048, ER-0054 |

## Approval Gate

No GitHub mutation should be performed until the System Engineer approves:

1. A live GitHub-backed read-only audit.
2. A concrete mutation table derived from that audit.
3. The exact commands or tool operation to apply the approved changes.
