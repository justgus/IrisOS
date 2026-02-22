---
GitHub-Issue: N/A
---

# AR/ER/DR Integration with GitHub Issues

## Purpose

GitHub Issues act as the index and workflow tracker. AR/ER/DR docs remain the system of record.
Each Issue links to its document, and each document links back to its Issue.

## Mapping

- AR (Architecture Recommendation) -> Issue label `ar`
- ER (Engineering Request) -> Issue label `er`
- DR (Defect Report) -> Issue label `dr`

Status is tracked by labels:

- `status:proposed`
- `status:accepted`
- `status:in-progress`
- `status:done`

## Workflow

1. Create the Issue using the AR/ER/DR template.
2. Draft the document in the matching folder:
   - `docs/AR/proposed/`
   - `docs/ER/`
   - `docs/DR/`
3. Add the Issue number to the document front matter: `GitHub-Issue: #123`.
4. Add the document path back into the Issue body.
5. When a PR implements the work, reference the Issue with `Fixes #123`.
6. On acceptance, update labels and move ARs from `proposed` to `accepted`.

## Automation (Optional)

There is no automatic Issue creation by default. If we want automation later, we can add a small
script or GitHub Action that creates an Issue when a new AR/ER/DR doc appears in a PR. That would
require explicit opt-in and standard metadata in the document front matter.

## Manual Sync Script

For manual sync, use `scripts/issue_sync.sh <doc_path>` to create/update the Issue and keep the
document front matter and labels aligned.

When `ER-Dependencies` is present in an AR, the sync script will close the AR Issue automatically
once all referenced ERs are marked Verified.
