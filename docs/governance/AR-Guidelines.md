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
GitHub-Issue: N/A | #123
ER-Dependencies: ER-XXXX, ER-YYYY
DR-Dependencies: DR-XXXX
---

# AR-XXXX — Short Title

- Status: Proposed | Accepted | Implemented | Done | Rejected
- Date: YYYY-MM-DD
- Owners:
```

If the AR drives implementation work, include the ERs and DRs that realize or correct the AR:

```md
ER-Dependencies: ER-XXXX, ER-YYYY
DR-Dependencies: DR-XXXX
```

Use `GitHub-Issue: N/A` while drafting a new AR before an Issue exists. `scripts/issue_sync.sh <doc_path>`
can create the Issue and replace `N/A` with the assigned number.

Use `GitHub-Issue: N/A` permanently only for governance/supporting docs that are not tracked as
AR/ER/DR work.

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
- Add `DR-Dependencies` only for DRs whose fix is required for the AR to be considered done.

## Lifecycle

1. Draft the AR under `docs/AR/proposed/`. New drafts should start with `GitHub-Issue: N/A` until an
   Issue exists.
2. Create or synchronize the Issue by either:
   - creating it from the AR issue template and then updating `GitHub-Issue:` to `#<number>`, or
   - running `scripts/issue_sync.sh <doc_path>` to create the Issue from the document and update the
     front matter automatically.
3. Refine the recommendation while it remains in `docs/AR/proposed/` until the System Engineer
   accepts or rejects it.
4. On acceptance, update `- Status:` and move the file to `docs/AR/accepted/`.
5. When implementation is needed, create or link the dependent ERs.
6. When all dependent ERs and DRs are complete, update the AR status to `Implemented` or `Done`.

## GitHub Workflow

- Use the `ar` label plus one status label.
- Link the document path in the Issue body. When multiple docs intentionally share one Issue, list all
  related doc paths there.
- Reference the Issue in implementation or follow-up PRs with `Fixes #<issue>`.
- `scripts/issue_sync.sh <doc_path>` may be used to keep the Issue title, body, and labels aligned.
  For shared Issues, the most recently synced document becomes the Issue title/body source of truth,
  so review grouped Issues manually after syncing.

If `ER-Dependencies` or `DR-Dependencies` is present, the sync script can close the AR Issue
automatically when the AR is marked `Implemented` or `Done` and all listed ERs/DRs are marked
`Verified`.

## Relationship To ERs And DRs

- ARs define direction and group the ERs/DRs that realize that direction.
- ERs define implementation work that realizes approved direction.
- DRs record defects, regressions, or discrepancy-driven fixes.

Do not use an AR to track implementation progress beyond the dependency list and final
implementation status. Once implementation planning is needed, create or update ERs and DRs.
