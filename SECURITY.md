# Security

## Trust model

Decode now rejects out-of-segment struct/list/far landing pads (see
`tests/bounds-test.cpp`). This is **partial**: pointer landing checks and a
64MiB traversal budget, not a full graph validator. Generated accessors still
trust a `capn_ptr` that has already been decoded. Do not treat the API as
untrusted-safe.

Do not decode untrusted network bytes without an additional validation layer.

## Reporting

Report security issues privately to the HaoZeke maintainer (GitHub security
advisories on [HaoZeke/c-capnproto](https://github.com/HaoZeke/c-capnproto)
preferred). Do not open public issues for exploitable decode bugs.

## Lineage

Fork of `opensourcerouting/c-capnproto` (MIT). Bounds-checking and release
hygiene are explicit goals of this fork.
