# ER-0103 Independent Review — 2026-10-15

- Reviewer: separate read-only Codex context `review_scope`; no files edited.
- Scope: existing operation capability metadata format, dedicated tests, integration, and ER criteria.
- Result: Pass; no open findings. Broader AR-0023 policy scope is not resolved by this review.

| Criterion | Verdict | Implementation and observed validation |
|---|---|---|
| Malformed containers/elements return CorruptData | Pass | schema_registry.cc checks array/string shape; raw CBOR null, empty/string-valued object, string/number/bool, mixed and nested arrays fail direct retrieval and operation listing |
| Missing, empty, valid arrays retain compatibility | Pass | Dedicated raw-record tests exercise omitted capabilities, explicit empty array, and two strings |
| Reopen preserves requirements and inheritance | Pass | Fresh file-backed store/registry instances retain both strings and order; own and inherited listing distinguish operation source |
| Existing bootstrap, dispatch, Conch behavior | Pass | Fresh focused registry/bootstrap/Conch suite passes |
| Test distributed through Automake | Pass | Declared check_PROGRAM source is included through generated DIST_SOURCES |
| Broader AR-0023 policy profile | Unverified | System Engineer scope ruling remains pending; outside this ER |

Independent command:

```sh
env TMPDIR=/private/tmp make -C tests check TESTS='test_refract_capabilities test_refract_registry test_refract_bootstrap test_conch_authoring'
```

Passed 4/4 programs, 55 checks (3 new, 21 registry, 16 bootstrap, 15 Conch). `git diff --check`
passed. The reviewer inspected malformed/raw-data counterexamples, valid legacy encoding, and
file-backed fresh-instance inheritance paths. No new policy or grant semantics were introduced.

Parent validation independently recorded build success and full suite 32/32. The reviewer did not
repeat the full suite and does not claim that as its own run. New test integration required local
`./bootstrap.sh` and `./configure`; generated tracked outputs are excluded from publication.

This ER requires no human-observed pass: all criteria concern a deterministic persisted-data decoder
and local APIs. It does not change the existing human gates on other ERs. CI is recorded separately
in the batch ledger; this report does not infer CI success from local tests.
