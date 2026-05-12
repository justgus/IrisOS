---
GitHub-Issue: N/A
---

# AR/ER/DR Integration with GitHub Issues

## Purpose

GitHub Issues act as the index and workflow tracker. AR/ER/DR docs remain the system of record.
An Issue may link to one document or to a small, intentionally grouped set of related documents, and
each document links back to its Issue.

## Mapping

- AR (Architecture Recommendation) -> Issue label `ar`
- ER (Engineering Request) -> Issue label `er`
- DR (Defect Report) -> Issue label `dr`

ERs and DRs should point back to the AR that spawned them through `AR-Dependencies`. ARs should
list their child implementation and defect work through `ER-Dependencies` and `DR-Dependencies`.

Status is tracked by labels:

- `status:proposed`
- `status:accepted`
- `status:in-progress`
- `status:done`

## Workflow

1. Draft the document in the matching folder:
   - AR drafts: `docs/AR/proposed/`
   - Accepted ARs: `docs/AR/accepted/`
   - ERs: `docs/ER/`
   - DRs: `docs/DR/`
2. Use `GitHub-Issue: N/A` while the document does not yet have an Issue number.
3. Create the Issue using the AR/ER/DR template, or run `scripts/issue_sync.sh <doc_path>` to create
   it from the document and update the front matter automatically.
4. Add the Issue number to the document front matter: `GitHub-Issue: #123`.
5. Add the document path, or all related document paths for grouped work, back into the Issue body.
6. When a PR implements the work, reference the Issue with `Fixes #123`.
7. On acceptance, update labels and move ARs from `docs/AR/proposed/` to `docs/AR/accepted/`.
8. When all child ERs and DRs are complete, update the AR to `Implemented` or `Done` and move the
   Issue to `status:done`.

## Automation (Optional)

There is no automatic Issue creation by default. If we want automation later, we can add a small
script or GitHub Action that creates an Issue when a new AR/ER/DR doc appears in a PR. That would
require explicit opt-in and standard metadata in the document front matter.

## Manual Sync Script

For manual sync, use `scripts/issue_sync.sh <doc_path>` to create/update the Issue and keep the
document front matter and labels aligned.

When multiple docs share one Issue, the sync script updates that Issue from the specific document
passed on the command line. The most recently synced document therefore controls the Issue title/body,
so grouped Issues should be reviewed manually after sync.

When `ER-Dependencies` or `DR-Dependencies` is present in an AR, the sync script will close the AR
Issue automatically once the AR is marked `Implemented` or `Done` and all referenced ERs/DRs are
marked `Verified`.
