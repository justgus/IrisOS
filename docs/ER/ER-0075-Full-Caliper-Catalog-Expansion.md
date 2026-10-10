---
GitHub-Issue: #283
AR-Dependencies: AR-0019
ER-Dependencies: ER-0033, ER-0095
---

# ER-0075 — Full Caliper Catalog Expansion

## Roles

- Implementation Engineer: drafts and implements changes
- System Engineer: reviews, tests, and verifies
- Note: Outside an Autonomous AR Batch, only the System Engineer may mark an ER as Verified. In this batch, Codex may do so only after the workflow's independent review and validation gates pass.

## ER Metadata

- ER ID: ER-0075
- Title: Full Caliper Catalog Expansion
- Status: Proposed
- Date: 2026-05-28
- Owners: Mike
- Type: Enhancement

## Engineering Guidelines

- Implementation language baseline: C++20 or C++24.
- Avoid line compaction or formatting changes that risk obscuring or losing content.
- Keep source files reasonably small. If a file grows too large to be fully replaced in a change, split it into smaller local files.

## Context

- Problem statement: ER-0033 added a starter Caliper catalog, but AR-0019 calls for a complete canonical SI and imperial catalog.
- Background / constraints: Catalog data must remain deterministic and based on authoritative definitions.

## Goals

- Expand the canonical SI base, derived, and prefixed unit catalog.
- Add common imperial and US customary units.
- Record canonical names, symbols, dimensions, and conversion metadata placeholders.

## Non-Goals

- Runtime conversion execution.
- Conch conversion commands.
- Domain-specific catalogs outside core SI and imperial units.

## Scope

- In scope: catalog entries, dimensions, symbols, and coverage tests.
- Out of scope: conversion engine behavior and user-defined override precedence.

## Requirements

- Functional: canonical catalog includes SI base and common derived units.
- Functional: canonical catalog includes common imperial/US customary units.
- Functional: duplicate symbols or ambiguous unit definitions are rejected.
- Non-functional: catalog ordering and serialization are deterministic.

## Proposed Approach

- Summary: expand the existing Caliper catalog data model and tests before implementing runtime conversion.
- Alternatives considered: implementing conversions first was rejected because conversion behavior depends on catalog completeness.

## Acceptance Criteria

- The 29-unit BIPM SI core manifest is owned by ER-0095 and is a prerequisite to this catalog expansion.
- A checked-in or generated manifest test enumerates every selected customary unit and prefix and asserts its canonical name, symbol, dimension, base-unit link, scale/offset metadata, and authoritative source table or section. Every conversion factor has a cited source. Prefix inventory and representation follow the System Engineer's ruling.
- Prefix behavior matches the selected representation (metadata with algorithmic composition or separately materialized records) and the accepted SI prefix scope.
- Repeated bootstrap and store reload preserve the selected catalog identity/version and produce deterministic serialized catalog payloads and ordering.
- Tests verify duplicate symbols or ambiguous unit definitions are rejected.
- Existing Caliper starter catalog tests continue to pass.

## Risks / Open Questions

- Risk: catalog coverage can become subjective without an explicit source list.
- Resolved during batch planning: use the current BIPM SI Brochure and its SI Prefixes reference for SI definitions, named derived units, and prefix definitions. Use NIST Handbook 44 (2026), Appendix C, for covered U.S. customary conversions; use NIST SP 811 only as a supplemental factor source after checking that the definition remains current. Record the source and table or section for every non-obvious conversion factor.

## Dependencies

- Dependency 1: AR-0019 Caliper Unit Catalog.
- Dependency 2: ER-0033 Caliper Units and Quantities.
- Dependency 3: ER-0095 SI Core Catalog Manifest.

## Implementation Notes

- Notes for implementer: keep source references visible for non-obvious conversion constants. Official references:
  - BIPM, [SI Brochure](https://www.bipm.org/en/publications/si-brochure) and [SI prefixes](https://www.bipm.org/en/measurement-units/si-prefixes).
  - NIST, [Handbook 44 (2026), Appendix C](https://www.nist.gov/document/2026-nist-handbook-44-appendix-c), for current U.S. customary conversion tables.
  - NIST, [SP 811](https://www.nist.gov/pml/special-publication-811), as a supplemental source only where the relevant factor is confirmed to remain applicable.

## Verification Plan

- Tests to run:
  - `./bootstrap.sh`
  - `./configure`
  - `make -j`
  - `make -C tests check TESTS='test_refract_bootstrap'`
  - `make check`
  - `git diff --check`
- Manual checks:
  - inspect catalog coverage for SI base, SI derived, metric prefixes, and common imperial units.

### Live Pass (Human)

- Required: No
- If no, reason: The exact catalog manifest, metadata, serialization, and conversions are observable in automated bootstrap and catalog tests; no target-specific environment or interaction is required.
- Build / environment / target: Not applicable.
- Steps: Not applicable.
- Expected observations: Not applicable.
- Observed results: Not applicable.
- Result: Not Required
- Performed by: Not applicable
- Date: Not applicable
