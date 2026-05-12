---
GitHub-Issue: #116
AR-Dependencies: AR-0012, AR-0016
---

# DR-0001 — Conch Compare Alias Resolution

## Roles

- Implementation Engineer: drafts and implements fixes
- System Engineer: reviews, tests, and verifies
- Note: Only the System Engineer may mark a DR as Verified.

## DR Metadata

- DR ID: DR-0001
- Title: Conch Compare Alias Resolution
- Status: Verified
- Date: 2026-03-08
- Owners: Mike
- Severity: Medium
- Priority: P1

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Summary

- One-sentence description of the issue: `call <alias> compare <alias>` compared against a literal token instead of resolving the alias/object, producing incorrect ordering results.

## Environment

- OS / distro: Linux (CI) and local dev
- Compiler / toolchain: g++ (C++20)
- Build flags / config: default Autotools build

## Steps to Reproduce

1. Create two distinct String objects and assign aliases `a` and `b`.
2. Run `call a compare b`.
3. Run `call b compare a` and `call b compare b`.

## Expected Behavior

- Expected: object-to-object comparisons return 1/-1 based on lexicographic ordering and 0 for identical strings.

## Actual Behavior

- Actual: compare treated the argument as a literal token, returning incorrect results for alias/object comparisons.

## Impact

- User impact: incorrect comparisons when using aliases or object IDs.
- System impact: inconsistent ordering logic for core `compare` usage.

## Evidence

- Logs: reported via shell usage, e.g. `call a compare b` and `call b compare b` returning `-1`.
- Screenshots or dumps: not captured.

## Triage Notes

- Suspected cause: core `compare` logic used raw argument tokens instead of resolving to object IDs/aliases.
- Related modules: `src/conch_shell/conch.cc` core operation handling.

## Fix Plan

- Proposed fix: resolve `compare` arguments to object IDs or aliases when possible, otherwise fall back to literal comparison for primitives.
- Alternatives: add explicit flags to force literal vs object comparison (deferred).

## Verification Plan

- Tests to run: Conch compare regression tests (if/when available).
- Manual checks: compare two aliased String objects and compare against a quoted literal.
