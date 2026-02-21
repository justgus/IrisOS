---
GitHub-Issue: N/A
---

# AR-0001 — Expected-Style Error Model

- Status: Accepted
- Date: 2026-02-14
- Owners: Mike

## Context

Referee is an object storage system that performs:

- Object serialization/deserialization
- Index-based lookup
- Relationship graph reconstruction
- Cross-file or cross-system references

The system must define consistent error propagation semantics across all layers.

The project targets Debian with GCC and Automake. ABI stability,
testability, and deterministic behavior are priorities.

We must decide:

- Whether exceptions are permitted
- How NotFound is represented
- How storage and serialization failures propagate
- Whether error types are strongly typed

## Decision

Referee adopts an expected-style return model:

1. Public APIs return:

   `std::expected<T, ErrorCode>`

2. Exceptions are NOT used for normal control flow.

3. Exceptions do not cross module boundaries.

4. `NotFound` is represented as an ErrorCode,
   not as `std::optional<T>`.

5. All error codes are strongly typed and namespaced.

6. A project alias will be defined:

   `template<typename T>`
   `using Result = std::expected<T, ErrorCode>;`

## Alternatives Considered

### 1. Exceptions Everywhere

Rejected due to:
- ABI instability across shared library boundaries
- Harder deterministic testing
- Hidden control flow

### 2. Optional for Lookups + Exceptions for Failures

Rejected due to:
- Mixed semantics
- Ambiguity between "not found" and real error

### 3. Return Error Enums with Out Parameters

Rejected due to:
- Cluttered call sites
- Poor composability

## Consequences

Positive:

- Explicit error handling
- Deterministic control flow
- Strong test assertions
- Stable ABI boundaries

Negative:

- More verbose call sites
- Slight increase in template surface

## Implementation Notes

- Introduce `enum class ErrorCode`
- Define canonical error categories
- Provide helper functions for ergonomic chaining
- Update storage layer first
- Tests assert on specific error codes

