# DR Template — Discrepancy Report / Bug / Issue / Task

## Roles

- Implementation Engineer: drafts and implements fixes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark a DR as Verified.

## DR Metadata

- DR ID:
- Title:
- Status: New | Triaged | Proposed | In Progress | Fixed | Complete | Verified | Deferred | Rejected
- Date:
- Owners:
- Severity: Critical | High | Medium | Low
- Priority: P0 | P1 | P2 | P3

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Summary

- One-sentence description of the issue:

## Environment

- OS / distro:
- Compiler / toolchain:
- Build flags / config:

## Steps to Reproduce

1.
2.
3.

## Expected Behavior

- Expected:

## Actual Behavior

- Actual:

## Impact

- User impact:
- System impact:

## Evidence

- Logs:
- Screenshots or dumps:

## Triage Notes

- Suspected cause:
- Related modules:

## Fix Plan

- Proposed fix:
- Alternatives:

## Verification Plan

- Tests to run:
- Manual checks:
