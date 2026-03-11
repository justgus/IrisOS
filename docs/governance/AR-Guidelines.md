---
GitHub-Issue: N/A
---

# AR Guidelines

## Purpose

Architecture Recommendations (ARs) capture technical direction before implementation is broken into
Engineering Requests (ERs). An AR explains the decision, scope, rationale, and open questions so
follow-on ERs can implement a stable direction.

## Roles

- Implementation Engineer: drafts and updates AR documents when architecture needs to be proposed or refined.
- System Engineer: reviews, accepts, rejects, or redirects the recommendation.

## Document Location

- Proposed ARs live in `docs/AR/proposed/`.
- Accepted ARs live in `docs/AR/accepted/`.

Move an AR from `proposed` to `accepted` only when the recommendation has been approved.

## Required Header

Each AR should start with front matter and a concise title:

```md
---
GitHub-Issue: #TBD
---

# AR-XXXX — Short Title

- Status: Proposed | Accepted | Rejected
- Date: YYYY-MM-DD
- Owners:
```

If the AR drives implementation work, include:

```md
ER-Dependencies: ER-XXXX, ER-YYYY
```

Use `GitHub-Issue: N/A` only for governance/supporting docs that are not tracked as AR/ER/DR work.

## Recommended Sections

- `## Context`
- `## Recommendation`
- `## Naming` when a subsystem or concept needs a canonical name
- `## Goals`
- `## Non-Goals (v1)` or equivalent scope limits
- `## Proposed Model` or another section that explains the design in enough detail for ER planning
- `## Open Questions`
- `## Next Steps`

The AR format is intentionally lighter than ER/DR templates. It should explain the decision clearly
without expanding into implementation task detail.

## Writing Guidance

- Focus on the architectural decision and why it is needed.
- Keep implementation specifics at the level needed to plan ERs, not full coding tasks.
- Name invariants, interfaces, boundaries, and tradeoffs explicitly.
- Record non-goals so later ERs do not drift.
- When alternatives were considered, summarize the rejection reason briefly.
- Add `ER-Dependencies` only for ERs that directly implement or complete the AR.

## Lifecycle

1. Create a GitHub Issue using the AR issue template.
2. Draft the AR under `docs/AR/proposed/`.
3. Add the Issue number to `GitHub-Issue:`.
4. Refine the recommendation until the System Engineer accepts or rejects it.
5. On acceptance, update `- Status:` and move the file to `docs/AR/accepted/`.
6. When implementation is needed, create or link the dependent ERs.

## GitHub Workflow

- Use the `ar` label plus one status label.
- Link the document path in the Issue body.
- Reference the Issue in implementation or follow-up PRs with `Fixes #<issue>`.
- `scripts/issue_sync.sh <doc_path>` may be used to keep the Issue title, body, and labels aligned.

If `ER-Dependencies` is present, the sync script can close the AR Issue automatically when all
listed ERs are marked `Verified`.

## Relationship To ERs And DRs

- ARs define direction.
- ERs define implementation work that realizes approved direction.
- DRs record defects, regressions, or discrepancy-driven fixes.

Do not use an AR to track implementation progress. Once implementation planning is needed, create
or update ERs.
