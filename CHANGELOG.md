# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Pre-1.0 minor releases may include breaking wire or API changes.

## [Unreleased]

## [0.4.1] - 2026-08-15

### Changed

- The C++ compiler is requested where the gtest suites and the live
  capnp-C++ interop peer are built, not in `project()`. Building the C
  runtime and `capnpc-c` no longer needs a C++ toolchain at all, which
  is what a consumer of a C library expects. Found while packaging for
  conda-forge, where the build environment has only what the recipe asks
  for and meson stopped with `Unknown compiler(s): c++`.

## [0.4.0] - 2026-08-15

RPC levels 1 through 4, with level 3 over a network layer this family
defines. The vat builds under every build system, not meson alone.

### Added

- RPC level 3, both halves. `Provide` holds a capability under the
  recipient's nonce and `Accept` claims it; an `Accept` with `embargo`
  waits for `Disembargo` with `context.provide`. A `thirdPartyHosted`
  CapDescriptor records an introduction, handed over by
  `capn_rpc_pending_introductions` and finished by
  `capn_rpc_introduction_done`, which releases the vine.
  `capn_rpc_send_provide`, `capn_rpc_send_accept` and
  `capn_rpc_send_disembargo_provide` are the introducer's side.
- `compiler/rpc-threeparty.capnp`, the network layer that names a third
  vat, shared verbatim with capnp-fortran, capnp-janet and capnp-ts.
  `rpc.capnp` leaves those ids to the network, and `rpc-twoparty.capnp`
  declares them empty because a two-party connection has no third to
  name. A vat speaks one layer or the other, not both.
- `capn_rpc_set_vat`: level 3 arrangements belong to a vat rather than a
  connection, since a handoff is made on one and claimed on another.
- `capn_rpc_answer_cap_id`, without which a capability returned in an
  answer could not be called.
- Level 3 goldens the reference `capnp` CLI encodes
  (`tests/fixtures/rpc-{provide,accept,introduce}.bin`), regenerated and
  verified by `scripts/gen-rpc-frames.sh`.
- Sphinx docs from `docs/orgmode/` via ox-rst (`emacs --batch -l docs/export.el`).
  Generated RST is not tracked.

### Fixed

- `libcapnp_c` builds the RPC vat under CMake and autotools too, not
  meson alone, and installs `capn-rpc.h`. The vat tests no longer sit
  behind the `capnp` CLI, which had made the whole RPC suite vanish on a
  runner without the compiler.

## [0.3.0] - 2026-08-10

Wire encode is byte-equivalent with official `capnp` 1.0.2 when objects
are allocated in schema order. Canonical form and packed of the same
unpacked bytes match `capnp convert`.

### Added

- `capn_canonicalize`: single-segment preorder rewrite
  (`encoding.html`). Empty struct is B=-1. Trailing zero data and null
  pointer words are truncated.
- `capn_init_fd` with a read callback (unpacked and packed).
- `capn_set_data` so empty `List(UInt8)` and a null Data pointer stay
  distinct.
- Generated `Foo_has_bar` for pointer fields.
- `capn_new_struct_list`: `List(Struct)` is always C=7 plus a tag,
  including empty lists and 0-pointer / 1-word structs.
- `capn_ok` / `decode_err` so a failed decode is not a wire null.
- Live `capnp` CLI gtests: schema-order encode, canonicalize, and packed
  `memcmp` AddressBook, TestAllTypes, empty struct, one primitive, Text,
  Data, a 32-bit int list, a void list, a bool list, an empty struct list.

### Fixed

- Zero-size struct encodes as offset -1 (`0xFFFFFFFC`), not null.
- C=6 pointer lists upgrade to 0-data / 1-pointer structs on `getp`
  when the schema says `List(Struct)`.
- Packed `0xFF` extra-word count is 0..255 (one byte). Truncated packed
  input is an error.
- Packed `0xFF` extra words follow the C++ rule: keep a following word
  when it has fewer than two zero bytes. One trailing null stays in the
  uncompressed span.
- Packed `capn_write_fp` / `capn_write_fd` stream past a 4 KiB stack
  buffer.
- `capn_validate` keys composite lists by type so `List(T)` and `T[0]`
  are distinct.
- `TestUnion.union1` same-type cases keep per-field offsets.
- `struct_ptr` on a pointer-list of structs uses `read_ptr` (words, not
  bytes; `STRUCT_PTR == 0` is a real struct).
- codecgen encodes NULL Text as a wire null.

### Changed

- Schema-order C encode of AddressBook and TestAllTypes matches
  `capnp encode` byte for byte. A different allocation order is still
  valid wire.

## [0.2.0] - 2026-08-10

First feature release on the HaoZeke fork after the v0.1.0 baseline.

### Added

- `capn_validate` pointer-graph walk (nesting limit 64). Not C++
  `MessageReader` throw parity.
- Decode landing-pad checks and a 64 MiB traversal budget.
- `capn_write_fp` for packed and unpacked stdio. Packed `capn_init_fp`
  reads the first chunk before inflate.
- `capn_new_interface` capability pointers (A=3, B=0, C=index).
- `List(Bool)` `getp` / `setp` / `get1` / `set1`.
- `$C.codecgen`, `$C.extraheader`, `$C.extendedattribute`.
- Ubuntu, macOS, Windows CI plus ASan+UBSan. Ubuntu smokes
  `fuzz-mem` / `fuzz-fp`.
- capnp-fortran AddressBook golden decode.

### Fixed

- Write overflow returns -1. Unset pointers encode as wire null.
- `List(Text)` / `List(Data)` codecgen through `capn_ptr_list.p`.
- Nested fieldgetset get-after-set. Empty Data defaults.

## [0.1.0] - 2026-07-01

HaoZeke fork baseline of the unmaintained
opensourcerouting/c-capnproto tree.

[Unreleased]: https://github.com/HaoZeke/c-capnproto/compare/v0.4.0...main
[0.4.1]: https://github.com/HaoZeke/c-capnproto/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/HaoZeke/c-capnproto/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/HaoZeke/c-capnproto/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/HaoZeke/c-capnproto/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/HaoZeke/c-capnproto/releases/tag/v0.1.0
