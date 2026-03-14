---
GitHub-Issue: N/A
---

# Defect And Regression Closure Policy

## Purpose

This note defines the expected closure posture for verified defects and regressions.

## Policy

- Record a DR for each verified regression or non-trivial user-visible defect unless there is a clear,
  documented reason not to.
- Link the DR to the implementation change that fixes it.
- Add a regression test for the defect when the behavior is testable in CI or stable local checks.
- If a regression test is not practical, state the reason in the DR verification plan or fix PR.
- Review `docs/DR/DR-Status.md` when a major implementation phase is completed to ensure the defect
  record is current.

## Why This Exists

- DRs are the system of record for defect tracking in this repository.
- Regression tests are the primary mechanism for preventing repeat defects.
- A thin DR ledger hides quality risk even when the codebase is evolving quickly.

## Relationship To ARs, ERs, And DRs

- ARs describe architectural direction.
- ERs describe implementation work.
- DRs describe defects and regressions.
- This policy clarifies the expected closure workflow when implementation uncovers or resolves defects.
