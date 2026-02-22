---
GitHub-Issue: #68
ER-Dependencies: ER-0018, ER-0019, ER-0020
---

# AR-0004 — Index and Graph Storage Strategy (Recommendation)

- Status: Accepted
- Date: 2026-02-16
- Owners: Mike

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
