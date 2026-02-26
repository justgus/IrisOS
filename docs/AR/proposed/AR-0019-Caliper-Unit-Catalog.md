---
GitHub-Issue: #TBD
---

# AR-0019 — Caliper Unit Catalog

- Status: Proposed
- Date: 2026-02-26
- Owners: Mike

## Context

ER-0033 introduces Caliper schemas and a starter unit catalog. IrisOS needs a complete and
authoritative catalog of metric and imperial units, including conversions and derived units, to
support scientific and engineering workloads across Refract, Astra, and Crate types.

## Recommendation

Define and ship a full Caliper unit catalog that:

- Covers SI base + derived units and common metric prefixes.
- Covers common imperial/US customary units and their relations to SI.
- Provides canonical symbols, names, and conversion metadata.
- Defines dimension metadata and conversion chains that are deterministic.

## Goals

- Provide a complete, curated catalog for general engineering use.
- Ensure conversions are explicit, deterministic, and testable.
- Enable Conch and system services to list, search, and introspect units.

## Non-Goals (v1)

- Automatic symbolic simplification beyond explicit conversion metadata.
- Domain-specific catalogs (chemistry, astronomy, finance) beyond core SI/imperial.

## Proposed Model (Broad Strokes)

- Canonical **Dimension** definitions for all base and derived dimensions.
- Canonical **Unit** definitions with:
  - name
  - symbol
  - dimension reference
  - system tags (SI, imperial, etc.)
  - conversion metadata (scale/offset/base unit)
- Catalog is versioned and shipped with IrisOS.

## Sources

- Use authoritative references (e.g., NIST, BIPM) for unit definitions and conversion factors.

## Open Questions

- How should catalog updates be versioned and migrated?
- Should catalog allow user-defined extensions or overrides?
- Should constraints and unit compatibility checks be centralized or embedded per unit?

## Next Steps

- Draft ER to define full catalog contents and data format.
- Decide on storage and update mechanism (bootstrap vs external catalog file).
- Add comprehensive tests for catalog coverage and conversion correctness.
