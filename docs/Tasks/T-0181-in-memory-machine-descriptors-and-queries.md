# T-0181 — In-Memory Machine Descriptors and Queries

## Task Metadata

- Task ID: T-0181
- Status: Implemented - Not Verified
- Epic: EP-002
- Parent Task: T-0167
- Sprint Assigned: SP-003
- Estimate: 5 points
- Priority: High

## Goal

Represent and query deterministic runtime facts for processors, memory, buses, and devices without persistence or authority.

## Approved Scope

- Bus and device descriptors carry stable resource IDs, type/name metadata, and parent relationships.
- Machine inventory owns immutable descriptor values.
- Construction rejects duplicate resource IDs and invalid references.
- Lookup by resource ID and enumeration by category return deterministic results in resource-ID order.
- Descriptors expose no drivers, operations, resource handles, or access authority.

## Acceptance Criteria

1. Descriptors represent concrete processor cores, register files, memory regions, buses, and devices.
2. Deterministic identifiers and ordering support in-memory lookup and enumeration.
3. Descriptors expose no resource-access operations.
4. Tests exercise representative inventory construction and queries.

## Validation

- `./bootstrap.sh`
- `./configure CXXFLAGS="-g -O2 -Wno-unused-const-variable -Wno-unused-private-field"`
- `make -j4`
- `make check`

Result: 27 tests passed, including duplicate-ID, dangling-reference, parent-cycle, lookup, and deterministic-order checks.
