# Security

## Trust model

Generated readers currently assume **trusted Cap'n input**. Do not decode
untrusted network bytes with the generated API until decode bounds checks are
complete on this fork.

## Reporting

Report security issues privately to the HaoZeke maintainer (GitHub security
advisories on [HaoZeke/c-capnproto](https://github.com/HaoZeke/c-capnproto)
preferred). Do not open public issues for exploitable decode bugs.

## Lineage

Fork of `opensourcerouting/c-capnproto` (MIT). Bounds-checking and release
hygiene are explicit goals of this fork.
