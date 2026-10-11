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
- Status: Complete
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
- Functional: catalog metadata stores all 24 BIPM SI prefixes by name, symbol, and decimal exponent; eligible SI units are composed from that metadata algorithmically rather than requiring a record for every prefix-unit pair.
- Functional: prefix composition applies to the 29 SI named units except kilogram; gram is the mass composition base. Prefix composition preserves dimension and applies scale through the unprefixed base unit.
- Functional: the finite customary manifest includes the general-use entries retained in the project catalog plus the selected additions asserted by the acceptance manifest: inch, foot, yard, mile, mil thickness, square inch/foot/yard/mile, acre, cubic inch/foot/yard, U.S. liquid minim/fluid dram/fluid ounce/gill/cup/pint/quart/gallon, U.S. dry pint/quart/peck/bushel, avoirdupois ounce/dram/pound/short hundredweight/short ton, psi/mmHg/inH2O, Fahrenheit, ounce-force/pound-force/kip/short-ton force, foot-pound-force per second, and mechanical horsepower. Sources are the relevant current general-use tables in NIST Handbook 44 (2026), Appendix C, Section 4, or the cited NIST SP 811 Appendix B.9 entries for force/power. Exclude survey-chain categories and retired survey-foot equivalents, apothecaries and troy systems, British Imperial measures, acre-foot volume, and entries marked legacy or not in general current use. Keep colliding capacity symbols system-qualified.
- Functional: include the general engineering force entries ounce-force, pound-force, kip, and short-ton force; include foot-pound-force per second and mechanical horsepower for power. Do not include CGS dyne or FPS poundal, specialized boiler/electric/metric/water/U.K. horsepower variants, or force per unit mass.

## Proposed Approach

- Summary: expand the existing Caliper catalog data model and tests before implementing runtime conversion.
- Alternatives considered: implementing conversions first was rejected because conversion behavior depends on catalog completeness.

## Acceptance Criteria

- The 29-unit BIPM SI core manifest is owned by ER-0095 and is a prerequisite to this catalog expansion.
- A checked-in or generated manifest test enumerates every selected customary unit and prefix and asserts its canonical name, symbol, dimension, base-unit link, scale/offset metadata, and authoritative source table or section. Every conversion factor has a cited source. Prefix inventory and representation follow the System Engineer's ruling.
- Prefix behavior matches the selected representation (metadata with algorithmic composition or separately materialized records) and the accepted SI prefix scope.
- The exact customary unit list is finite and asserted by name, symbol, dimension, base symbol, scale, offset, and source table/section. Exact factors use the international foot, exact SI definitions, NIST Handbook 44 tables, or a documented NIST SP 811 Appendix B.9 entry as specified per manifest row.
- Repeated bootstrap and store reload preserve the selected catalog identity/version and produce deterministic serialized catalog payloads and ordering.
- Tests verify duplicate symbols or ambiguous unit definitions are rejected.
- Existing Caliper starter catalog tests continue to pass.

## Risks / Open Questions

- Risk: catalog coverage can become subjective without an explicit source list.
- Resolved during batch planning: use the current BIPM SI Brochure and its SI Prefixes reference for SI definitions, named derived units, and prefix definitions. Use NIST Handbook 44 (2026), Appendix C, for covered U.S. customary conversions; use NIST SP 811 only as a supplemental factor source after checking that the definition remains current. Record the source and table or section for every non-obvious conversion factor.
- Resolved during batch planning (2026-10-10): catalog coverage uses the finite general-engineering list above, includes force and power, preserves project unit names, stores all 24 SI prefixes as metadata, and composes eligible prefixed units algorithmically. Dry and liquid capacity forms use system-qualified symbols where NIST reuses a symbol.

## Dependencies

- Dependency 1: AR-0019 Caliper Unit Catalog.
- Dependency 2: ER-0033 Caliper Units and Quantities.
- Dependency 3: ER-0095 SI Core Catalog Manifest.

## Implementation Notes

- Notes for implementer: keep source references visible for non-obvious conversion constants. Official references:
  - BIPM, [SI Brochure](https://www.bipm.org/en/publications/si-brochure) and [SI prefixes](https://www.bipm.org/en/measurement-units/si-prefixes).
  - NIST, [Handbook 44 (2026), Appendix C](https://www.nist.gov/document/2026-nist-handbook-44-appendix-c), for current U.S. customary conversion tables.
  - NIST, [SP 811](https://www.nist.gov/pml/special-publication-811), as a supplemental source only where the relevant factor is confirmed to remain applicable.
  - NIST SP 811, [Appendix B.9](https://www.nist.gov/pml/special-publication-811/nist-guide-si-appendix-b-conversion-factors/nist-guide-si-appendix-b9), for the selected force and power entries; its factors are cross-checked against exact unit definitions where available.
- Implementation result: catalog v2 stores the exact 24-prefix metadata set and the finite customary manifest with source section strings. Prefix composition is generated by the runtime loader from the prefix records and SI base-unit catalog; existing entries are accepted as collisions only when their dimensions, links, factors, offsets, and systems match the generated definition.
- Validation result: the bootstrap manifest test asserts the full customary symbol set and each entry's name, dimension, base, scale, offset, and exact source label; runtime tests cover generated SI prefixes and a `1000 mg` to `g` conversion. Independent review passed with no open findings.

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
