---
GitHub-Issue: N/A
AR-Dependencies: AR-0019
ER-Dependencies: ER-0075, ER-0076
---

# ER-0099 — Caliper Dimension Constraint Contract

## ER Metadata

- ER ID: ER-0099
- Title: Caliper Dimension Constraint Contract
- Status: In Progress
- Date: 2026-10-10
- Owners: Mike
- Type: Enhancement

## Context and Scope

AR-0019 requires constraints such as absolute zero across Kelvin, Celsius, and Fahrenheit, plus higher-level context-dependent constraints. Existing planned catalog and conversion ERs do not define where or how constraints are represented. This ER isolates the v1 constraint scope and validation behavior.

## Requirements

- V1 supports intrinsic dimension constraints only. It does not support context-dependent constraints.
- Temperature values in v1 represent absolute temperature points. Temperature intervals and their distinct arithmetic semantics are not modeled.
- Absolute-zero validation is consistent across Kelvin, Celsius, and Fahrenheit after conversion to a shared physical basis.
- The intrinsic constraint is represented as a minimum absolute temperature of zero kelvin and is checked against the exact rational value in canonical kelvin before target conversion or rounding the displayed result.
- Constraint definitions are deterministic and versioned, and do not alter dimension compatibility or the catalog's stable identity.
- Context-dependent checks are out of scope for v1.

## Dependencies

- ER-0075 Full Caliper Catalog Expansion.
- ER-0076 Runtime Conversion and Compatibility Engine.

## Acceptance Criteria

- The System Engineer's ruling that v1 is limited to intrinsic constraints is recorded in the AR and batch decision ledger.
- Tests cover below, exact-boundary, and above-boundary values expressed in each of K, °C, and °F. The values are `-0.01`, `0`, `0.01` K; `-273.16`, `-273.15`, `-273.14` °C; and `-459.68`, `-459.67`, `-459.66` °F. Below-boundary cases fail with one stable constraint error; boundary and above-boundary cases pass.
- Each below-zero absolute temperature is rejected for every compatible target unit; exact-boundary and above-boundary values pass. Tests exercise the K/°C/°F source and target combinations.
- Exact temperature conversion uses the exact rational factors attached to the immutable catalog v2 Temperature dimension; non-temperature conversions keep their existing factor and rounding behavior.
- No context-dependent constraint model or validation is added in v1.
- `./bootstrap.sh`, `./configure`, `make -j`, `make -C tests check TESTS='test_refract_bootstrap'`, `make check`, and `git diff --check` pass.

## Proposed Approach

- Apply intrinsic constraints to the exact rational value after the source unit's transform to canonical kelvin and before target conversion or binary64 rounding.
- Store the minimum and any exact temperature transforms in catalog v2's Temperature dimension entry, with direct Unit object references; the immutable Unit payloads and their binary64 fields remain unchanged.
- Keep the constraint independent from unit compatibility: units remain compatible by dimension, and physical-domain validation runs after compatibility succeeds.

## Implementation Note

- Ruling (2026-10-11): the System Engineer approved immutable catalog v2. `CALIPER-BASE-002` has not been published; this branch finalizes its payload before its first publication. After release, the v2 identity and payload remain immutable. The legacy `CALIPER-BASE-001` identity and payload are unchanged.
- The current Fahrenheit scale and offset are rounded binary64 values. Interpreted as exact shortest decimal rationals, they do not map `-459.67 °F` to exactly zero kelvin and can overflow the selected signed-64-bit rational domain for ordinary Fahrenheit inputs. Catalog v2 therefore records the exact factors `5/9` and `45967/180` for Fahrenheit. Conversion uses those factors and the physical-domain check uses the same exact root; the implementation does not hide the discrepancy with a tolerance.

### Live Pass (Human)

- Required: No
- Reason: Automated conversion and constraint boundary tests cover the selected scope.
