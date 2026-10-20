---
GitHub-Issue: #68
ER-Dependencies: ER-0018, ER-0019, ER-0020, ER-0034, ER-0035, ER-0036, ER-0048, ER-0049, ER-0050
---

# AR-0004 — Index and Graph Storage Strategy (Recommendation)

- Status: Pending Review
- Date: 2026-02-16
- Owners: Mike

## Implementation Review — 2026-10-20

The System Engineer returned this AR to `Pending Review` after the
[implementation audit](../../Plans/AR-Implementation-Audit-2026-10-20.md) identified
implementation gaps (F4). This status supersedes earlier completion claims while the
remaining work and ER coverage are reviewed. The architectural decision remains accepted;
existing implementation evidence and ER verification records are preserved.

## Context

AR-0003 defines the physical layout (segments + indexes). The next
architectural decision should address how indexes and relationship graph
storage are implemented and queried.

Key questions:
- What index backend is used (custom vs embedded KV vs lightweight DB)?
- How are relationship edges stored and queried (forward, reverse, both)?
- What are the recovery and rebuild guarantees for indexes?
- What are the read/write latency targets and expected working set size?

## Recommendation

AR-0004 should decide on an index/graph storage backend and its invariants,
with a bias toward deterministic, portable, dependency-light choices that
fit the project's ABI and CI constraints.

### V0 Alignment (2026-02-23)

For v0, Referee uses SQLite tables for object records and edge records.
SQLite indexes provide ObjectID lookup and edge traversal; rebuilding
from segment files is deferred until the post-v0 storage layout is implemented.

### Suggested Decision Criteria

- Deterministic on-disk format and recovery behavior
- Minimal external dependencies
- Good fit for ObjectID lookups and graph traversals
- Clear rebuild path from `data/` segments
- Acceptable performance for expected data sizes

### Candidate Approaches

1. Custom, file-backed B-tree indexes for ObjectID and edges
2. Embedded KV store with deterministic file formats
3. SQLite-style relational indexing (ObjectID table + edge tables)

## Notes

This document is a recommendation to scope AR-0004. It does not choose
an implementation yet.
