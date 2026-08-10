# Security

## Trust model

Decode rejects out-of-segment struct/list/far landing pads, applies a
64MiB traversal budget, and walks the pointer graph with a nesting
limit (default 64; `capn.nesting_limit`, 0 means that default).
`capn_validate()` returns -1 on over-deep nesting, cycles, traversal
overflow, or a non-null pointer that fails to resolve. Each
`capn_getp` / `capn_resolve` hop also decrements remaining nesting so
a cycle cannot loop forever.

Those failed hops still return `CAPN_NULL`, the same value as a missing
or wire-null field. They are distinguishable after the fact:
`capn_ok(c)` is 0 once `read_ptr` hits OOB, budget, nesting, or a
broken (non-null) pointer; a legitimate null pointer word leaves
`capn_ok` at 1. `capn_clear_err` resets the sticky session flag.
`capn_init_mem` does not walk the graph.

This is **not** C++ `MessageReader` throw-parity. Generated accessors
still return `CAPN_NULL` for both missing and broken and do not check
`capn_ok`. There is no exception, no fail-closed generated getter, and
`capn_validate` remains opt-in. `capn_validate` is a graph walk over
the tests in `tests/bounds-test.cpp`, not a fuzz-proven match to
libkj's exception model. Do not treat the API as untrusted-safe.

Do not decode untrusted network bytes without an additional validation layer.

## Reporting

Report security issues privately to the HaoZeke maintainer (GitHub security
advisories on [HaoZeke/c-capnproto](https://github.com/HaoZeke/c-capnproto)
preferred). Do not open public issues for exploitable decode bugs.

## Lineage

Fork of `opensourcerouting/c-capnproto` (MIT). Bounds-checking and release
hygiene are explicit goals of this fork.
