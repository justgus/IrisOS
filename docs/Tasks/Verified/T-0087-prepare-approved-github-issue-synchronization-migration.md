---
GitHub-Issue: TBD
---

# T-0087 — Prepare Approved GitHub Issue Synchronization Migration

## Task Metadata

- Task ID: T-0087
- Status: Verified
- Epic: EP-001
- Sprint Assigned: SP-001
- GitHub Issue: TBD
- Component: GitHub issue synchronization
- Priority: High
- Date Requested: 2026-07-06
- Date Implemented: 2026-08-17
- Date Verified: 2026-08-17
- Verified By: System Engineer

## Rationale

GitHub issue titles, bodies, and labels should only be changed after the local migration map is reviewed and approved.

## Acceptance Criteria

1. A GitHub mutation plan identifies all proposed title, body, label, and state changes.
2. The plan preserves existing issue numbers.
3. The plan identifies mismatches that need System Engineer approval.
4. No GitHub mutation is executed as part of this Task without explicit approval.
5. The migration can be rerun or reviewed without hidden side effects.

## Planning Notes

- Proposed GitHub changes should be generated as a reviewable command list or table before execution.
- The plan should distinguish safe metadata additions from state-changing operations such as closing issues.
- Legacy labels should remain until the System Engineer approves removing or replacing them.

## Evidence

- Local-only GitHub synchronization plan recorded in `docs/GitHub-Issue-Synchronization-Plan.md`.
- Read-only live audit recorded in `docs/Airframe-GitHub-Live-Audit.md`.
- The System Engineer approved synchronization on 2026-08-17.
- Shared mappings `#181`, `#182`, and `#184` were preserved with complete document lists.
- Forty-four stale status labels were corrected, `#116` was retitled, and `#287` and `#288` were closed as completed.
- Post-synchronization comparison of all 102 numeric references found no remaining title, status-label, or required-closure drift.
- PR #300 Linux CI passed build, unit tests, and `distcheck` after the synchronization evidence was committed.

## Verification

The System Engineer approved T-0087 as Verified on 2026-08-17.
