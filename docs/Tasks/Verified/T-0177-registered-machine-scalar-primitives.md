# T-0177 — Registered Machine Scalar Primitives

## Task Metadata

- Task ID: T-0177
- Status: Verified
- Epic: EP-002
- Parent Task: T-0166
- Sprint Assigned: SP-002
- Estimate: 5 points
- Priority: High
- Owner: Implementation Engineer

## Goal

Define portable Machine scalar values and register their stable identities with Refract.

## Scope

- Bit, nibble, byte, fixed-width word, byte-order, alignment, and address value types.
- Deterministic construction, equality, and validation.
- Stable Refract definitions and registration tests.

## Out of Scope

- Host pointers, hardware access, processor descriptors, buffers, and packets.

## Acceptance Criteria

1. Scalar values do not depend on host byte order or pointer layout.
2. Invalid widths, alignment, or address construction fail deterministically.
3. Each public scalar type has a stable Refract identity and inspectable definition.
4. Tests cover construction, equality, validation, and registration.
5. Existing tests continue to pass.

## Validation

- `./bootstrap.sh`
- `CPPFLAGS="$(pkg-config --cflags nlohmann_json)" CXXFLAGS="-g -O2 -Wno-unused-const-variable -Wno-unused-private-field" ./configure`
- `make -j`
- `make check`

Result: 26 tests passed, including 8 Machine primitive checks. The local configure flags supply Homebrew's nlohmann include path and suppress two pre-existing Apple-Clang warnings; they are not source changes.

## Verification

Accepted by the System Engineer on 2026-08-21 after review of the implementation and validation results.
