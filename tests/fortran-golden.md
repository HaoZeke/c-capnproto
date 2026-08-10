# Fortran AddressBook golden fixtures

These bytes are the AddressBook sample checked in by
[HaoZeke/capnp-fortran](https://github.com/HaoZeke/capnp-fortran) under
`test/fixtures/`. That tree documents them as output of the reference
`capnp encode` tool (flat, packed, and `capnp convert binary:canonical`)
for `schema/addressbook.capnp`. The C tests consume copies here so this
repo does not need a sibling Fortran checkout at test time.

| File | Bytes | Role |
| --- | ---: | --- |
| `tests/fixtures/addressbook.bin` | 288 | framed flat message |
| `tests/fixtures/addressbook.packed.bin` | 151 | packed framing of the same message |
| `tests/fixtures/addressbook.canonical.bin` | 272 | canonical form (no segment table) |

Payload: two people, Alice (id 123, school MIT, one mobile) and Bob
(id 456, unemployed, home + work). Field checks in
`tests/fortran-golden-test.cpp` match `test/test_interop.f90` in
capnp-fortran.

A missing fixture is a test failure, not a skip. The same bytes are
pinned as hex arrays in the test so a truncated or swapped `.bin` fails
`CommittedBinsMatchPinnedHex`.

C encode of the same logical book is checked by decoding the C-written
bytes and asserting the Alice/Bob fields. Raw C encode layout and
C-packed bytes of a re-serialized golden are not required to match
`capnp encode` byte for byte; the Fortran goldens are the decode oracle.

`capn_canonicalize` of a C-encoded Alice/Bob book must match
`addressbook.canonical.bin` (272 unframed bytes). Alice's mobile phone
drops the trailing zero type word (`PhoneNumber` data size 0). A second
canonicalize of that output is identical.
