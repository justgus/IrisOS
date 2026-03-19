# Implementation Plan: v2 Utilities and Tooling

- Status: Draft
- Date: 2026-02-27
- Owners: Mike

## Scope

This plan defines the v2 build, focused on userland utilities and tooling that make the system
usable for real work. It follows v1 kernel completion and expands parsing, ingestion, and
developer tools without growing kernel scope.

## Guiding Principles

- Keep the kernel stable; v2 is userland-first.
- Prefer small, composable tools that operate on IrisOS objects.
- Favor deterministic, introspectable behaviors over feature breadth.

---

## Phase 1 — Data and Language Parsers

### Features

- JSON and XML parsers for data ingestion and interchange.
- Constrained C++ and Python parsers for analysis/metadata extraction.

### Milestones

- M1.1: JSON parser usable by Conch or tooling.
- M1.2: XML parser for legacy inputs.
- M1.3: C++ and Python subset parsers with documented scope.

### ERs

- ER-0024 JSON Parser (Verified)
- ER-0025 XML Parser (Verified)
- ER-0026 C++ Parser (Verified)
- ER-0027 Python Parser (Verified)

---

## Phase 2 — Core Utilities Suite (GNU-style analogs)

### Features

- Object-aware equivalents of core utilities (list, inspect, filter, transform).
- Batch import/export tools for Referee objects and Refract schemas.
- Basic package or bundle mechanism for sharing object graphs and schemas.

### Milestones

- M2.1: Minimal utility suite for object graph operations.
- M2.2: Import/export workflows for schema and data.
- M2.3: Bundle/packaging format for object graph distribution.

### ERs

- ER-0047 Userland Core Utilities Suite (Proposed)
- ER-0048 Referee Import/Export Tools (Proposed)
- ER-0049 Object Graph Bundles and Packages (Proposed)

---

## Phase 3 — Developer Tooling

### Features

- Schema migration tools and validators.
- Profiling/trace utilities for CEO tasks.
- Debugging and inspection helpers for Conch and Referee.

### Milestones

- M3.1: Migration tooling baseline.
- M3.2: Task and object inspection tools.

### ERs

- ER-0050 Schema Migration Tools (Proposed)
- ER-0051 CEO Profiling and Trace Utilities (Proposed)
- ER-0052 Conch Debug and Inspection Tools (Proposed)

---

## Phase 4 — v2 Integration and Usability

### Features

- End-to-end workflows using utilities + parsers + Conch.
- Document v2 limitations and future roadmap.

### Milestones

- M4.1: v2 usability demo complete.
- M4.2: v3 roadmap documented.

### ERs

- ER-0053 v2 Integration and Usability Demo (Proposed)

---

## Open Questions

- Which utilities are required for the first usable userland set?
- Preferred package/bundle format for object graph distribution.
