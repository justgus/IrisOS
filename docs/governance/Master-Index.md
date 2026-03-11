---
GitHub-Issue: N/A
---

# Governance Master Index

## Purpose

This index points to the governance and workflow documents used to manage Architecture
Recommendations (ARs), Engineering Requests (ERs), Defect Reports (DRs), and their GitHub Issue
tracking.

## Governance Docs

- `docs/governance/AR-Guidelines.md`
- `docs/governance/ER-Guidelines.md`
- `docs/governance/DR-Guidelines.md`
- `docs/governance/AR-ER-DR-GitHub-Issues.md`
- `docs/governance/GitHub-Issue-Labels.md`

## AR Docs

- Proposed ARs: `docs/AR/proposed/`
- Accepted ARs: `docs/AR/accepted/`
- Authoring guidance: `docs/governance/AR-Guidelines.md`
- GitHub issue template: `.github/ISSUE_TEMPLATE/ar.md`

## ER Docs

- ER documents: `docs/ER/`
- ER template: `docs/ER/ER-Template.md`
- ER status ledger: `docs/ER/ER-Status.md`
- Authoring guidance: `docs/governance/ER-Guidelines.md`
- GitHub issue template: `.github/ISSUE_TEMPLATE/er.md`

## DR Docs

- DR documents: `docs/DR/`
- DR template: `docs/DR/DR-Template.md`
- DR status ledger: `docs/DR/DR-Status.md`
- Authoring guidance: `docs/governance/DR-Guidelines.md`
- GitHub issue template: `.github/ISSUE_TEMPLATE/dr.md`

## Supporting Workflow

- Issue sync script: `scripts/issue_sync.sh`
- Issue template config: `.github/ISSUE_TEMPLATE/config.yml`

## Quick Start

1. Decide whether the work is architectural (`AR`), implementation planning/tracking (`ER`), or a defect (`DR`).
2. Create a GitHub Issue from the matching template.
3. Draft or update the matching document in `docs/`.
4. Add the Issue number to the document front matter.
5. Keep status, labels, and linked paths aligned through implementation and review.

## Notes

- ARs are the architectural system of record for design direction.
- ERs are the implementation system of record for planned work.
- DRs are the defect system of record for bugs and regressions.
- GitHub Issues are the tracker/index layer, not the canonical document body.
