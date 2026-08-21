# Airframe DR to Issue Migration

**Date:** 2026-07-07
**Scope:** Local documentation migration
**Epic:** EP-001
**Task:** T-0085

## Boundary

This document maps existing local Defect Report records to Agile Airframe Issue records. It does not move, delete, or rewrite legacy DR files, and it does not query or mutate GitHub.

## Mapping Rules

| Rule | Direction |
| ---- | --------- |
| Source records | Every non-template `docs/DR/DR-*.md` file is treated as one DR source record. |
| Target IDs | Target Issue IDs continue from the current Airframe Issue index. |
| Legacy identity | Each target Issue preserves its source DR ID as the legacy identifier. |
| Verified DRs | Existing Verified DRs map to Verified Issue records as representation of approved source state. |
| GitHub references | Local `GitHub-Issue:` values are preserved as local references only. |
| AR dependencies | Local `AR-Dependencies:` values are preserved as architecture trace links. |

## Summary

| Source Status | Count | Target Handling |
| ------------- | ----- | --------------- |
| Verified | 1 | Verified Issue record |

## Migration Map

| Target Issue | Legacy DR | Source Status | Target Status | Local GitHub Ref | Severity | Priority | AR Dependencies | Title | Source Path |
| ------------ | --------- | ------------- | ------------- | ---------------- | -------- | -------- | --------------- | ----- | ----------- |
| I-0001 | DR-0001 | Verified | Verified | #116 | Medium | P1 | AR-0012, AR-0016 | Conch Compare Alias Resolution | docs/DR/DR-0001-Conch-Compare-Alias-Resolution.md |

## Preserved Defect Context

`I-0001` preserves these source DR sections by reference:

- Summary
- Environment
- Steps to Reproduce
- Expected Behavior
- Actual Behavior
- Impact
- Evidence
- Triage Notes
- Fix Plan
- Verification Plan

## Validation Commands

The migration map used these local read-only commands:

```sh
rg -n "^# DR-|^- Status:|^GitHub-Issue:|^AR-Dependencies:|^- Severity:|^- Priority:" docs/DR
```

## Generated Canonical Records

Canonical Issue records were generated from this map:

- Verified Issue records: `docs/Issues/Verified/`

The generated Issue records preserve the complete legacy DR body in a `Preserved Legacy DR Content` section.

## Caveats

- The Verified target status represents existing System Engineer-approved DR source state; it is not a new verification action.
- Live GitHub issue state, title, and labels are not included.
