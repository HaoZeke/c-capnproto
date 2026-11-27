# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
Pre-1.0 minor releases may include breaking wire or API changes.

## [Unreleased]

### Fixed

- RPC promise and release servers bind loopback through
  `posix_loopback_addr()`, so macOS with `_POSIX_C_SOURCE=200809L`
  compiles them.
- `$Json.hex` decodes the strings Cap'n Proto 1.4.0 `kj::decodeHex` decodes. Odd length `0` is empty and `zz` is a zero byte.
- `$Json.base64` decodes the strings Cap'n Proto 1.4.0 `kj::decodeBase64` decodes. `YWI`, `AA==AAAA`, and `AA=A` keep those bytes.
- JSON `null` is rejected for a struct field. `-010` stays rejected, and
  nested pretty JSON matches Cap'n Proto 1.4.0 byte for byte.
- A JSON value for a flattened union is ignored when that union has no
  tag yet. The discriminant stays at its default, and a flattened struct
  pointer is allocated empty.
- Unquoted JSON `9223372036854775807` and `18446744073709551615` are
  rejected before a 64-bit conversion. `9223372036854774784` and
  `18446744073709549568` still decode exactly.
- JSON numbers decode as doubles, matching Cap'n Proto 1.4.0: `9007199254740993`
  becomes `9007199254740992`, and `18446744073709551615` is out of range.
  Quoted 64-bit integers stay exact.
- JSON `null` is rejected for text, data, and list fields. Void `null` is
  still accepted.
- An unannotated Data field rejects a JSON string. Number arrays,
  `$Json.base64`, and `$Json.hex` are unchanged.
- Unknown JSON field names are ignored.
- A JSON number with a leading zero, such as `010`, is rejected. `0`,
  `0.5`, and `-0` stay valid.
- A JSON `\u` surrogate is stored as its own three-byte sequence, so
  `\uD83D\uDE00` is `ed a0 bd ed b8 80`.
- `capn_dyn_to_json` without `CAPN_DYN_COMPACT` pretty-prints with the
  spacing of `capnp convert binary:json`. Compact JSON is unchanged.
- A flattened union `$Json.discriminator` value with no tag leaves the
  union unset. A tag together with the value still selects that member.
- Reading `List(T)` through a brand uses the bound element size, so a
  data-only struct list is read back in text and JSON.
- Reading a struct list as `List(Text)` or `List(Data)` returns the
  first pointer of each element.
- A far pointer to a composite list is resolved before `List(Text)` and
  `List(Data)` read the first pointer of each element.
- `$C.fieldgetset` emits `<Struct>_has_<field>` for a pointer member of
  an unnamed union. The accessor is true when the discriminant selects
  that field and the pointer is non-null.
- A `List(T)` field of a generic instantiation uses the list type of the
  bound type, so `List(T)` bound to a struct is a struct list.
- Each generic instantiation types its own groups. A brand no longer
  retypes the unbound struct's group fields, and a union in a group gets
  enum names from the instantiation.
- A Windows DLL exports the `rpc.capnp` and `rpc-threeparty.capnp`
  accessors. Their generated code carries `CAPN_SCHEMA_EXPORT`, which is
  empty outside the library.
- `$C.rpcstubs` handlers receive the results pointer. `CAPN_RPC_PENDING`
  is `-2` and preserves results written through it; a plain return of
  `1` is an exception.

### Added

- `$C.reflect` and `lib/capn-dyn.h`: a descriptor per struct, group and
  enum (`<Name>_dyn`), with field access by name, union selection and
  setters, `capn_dyn_to_text` (`capnp decode --short`),
  `capn_dyn_to_json` (`capnp convert binary:json`), and
  `capn_dyn_from_text` / `capn_dyn_from_json`, which build the canonical
  message `capnp encode` builds. Generic instantiations are resolved
  through their brands.
- `capn_rpc_free`, `capn_rpc_send_abort`, `capn_rpc_is_aborted` and
  `capn_rpc_set_result_size`.
- RPC capabilities in payloads: `capn_rpc_new_export_cap` and
  `capn_rpc_new_import_cap` build a capability pointer and the capTable,
  `capn_rpc_param_cap` and `capn_rpc_answer_cap` read one, and
  `capn_rpc_send_pipelined_call` calls a capability inside an answer that
  has not arrived.
- Deferred RPC replies: `dispatch` returns `CAPN_RPC_PENDING` and answers
  later with `capn_rpc_return` or `capn_rpc_return_exception`; a `Finish` in
  between cancels the call.
- `$C.rpcstubs`: per interface, the method parameter and result structs, a
  client call helper and result reader per method, and a server vtable with
  a `<Interface>_dispatch` for `capn-rpc.h`.
- `capn_rpc_fd_send`, `capn_rpc_fd_read_frame` and `capn_rpc_fd_pump`: a
  blocking descriptor transport for the vat on Unix-like systems.
- `scripts/regen-generated.sh`, which rewrites every checked-in
  `*.capnp.[ch]` or, with `--check`, fails on a stale one; CI runs it.
- Tests that read back the values baked into generated constants, that
  compare read/write round trips and far-pointer messages with `capnp`,
  and that check RPC frames over 8 KiB.

### Changed

- The shared library soname is 2 (meson `soversion: 2`, libtool
  `-version-info 2:0:0`, CMake `SOVERSION 2`). Public RPC structs grew,
  including `capn_rpc_queued` and `capn_rpc_conn`, so a binary built
  against soname 1 cannot use this library.
- `struct capn_rpc_pending_call` carries `caps` and `ncaps` until
  `capn_rpc_return`. `struct capn_rpc_conn` carries `cap_dest` and
  `cap_dest_n`, which name that table during dispatch.
  `capn_rpc_pending_results` reads the parked results struct. A generated
  stub can return `CAPN_RPC_PENDING` and finish with
  `<Interface>_return_<method>`.
- `capn_rpc_new_export_cap`, `capn_rpc_new_import_cap`,
  `capn_rpc_new_promise_cap` and `capn_rpc_new_answer_cap` append to
  `cap_dest` while a call is being filled or dispatched, otherwise to the
  parked call whose message owns the segment, otherwise to the connection
  list.
- `CAPN_RPC_MAX_ANSWERS`, `CAPN_RPC_MAX_EXPORTS` and
  `CAPN_RPC_MAX_QUESTIONS` are gone. Tables grow up to
  `CAPN_RPC_MAX_TABLE_CAPACITY` (1024). `CAPN_RPC_MAX_ANSWER_BYTES` is
  gone: RPC frames are sized from the message, and `capn_rpc_answer` /
  `capn_rpc_question` hold heap frames that `capn_rpc_free` releases.
  Every `capn_rpc_init` must be paired with `capn_rpc_free`.
- Stream readers refuse a message over 64 MiB, as C++ does, and a buffer
  whose segment table claims more than the buffer holds
  (`CAPN_MAX_MESSAGE_BYTES` overrides the limit at compile time).
- `capn_new_*` return a null pointer for a negative or oversized count.
- The names `which`, `errno`, `stdin`, `stdout`, `stderr`, `unix`, `linux`,
  `i386`, `noreturn`, `complex` and `imaginary` are renamed with a leading
  underscore when a schema field uses them.

### Fixed

- Calls queued on one promise are replayed in arrival order. A resolve
  of another promise can free a lower queue slot, and a later call must
  not be delivered before an earlier one on the same promise.
- A `senderLoopback` Disembargo is echoed only after its target settles
  to a capability the sender hosts. An ordinary local export aborts the
  connection.
- A Text field read from a null pointer wrote back as an empty string,
  turning a schema default or a null into explicit data; it now writes a
  null again.
- `capn_setp` with a same-session far pointer stored the pointer word in
  the struct's first data word instead of the named slot. A far pointer
  that does land in the right slot now deep-copies the object it names;
  copying the word aliases the landing pad. A far pointer that does not
  resolve is stored as null.
- Reusing a question id that is still live aborts the connection. The
  reason is the Call text from C++ 1.4.0: `expected !answer.active;
  questionId is already in use`. That covers an answer still waiting for
  Finish, a call that has not returned, a queued frame, and an unfinished
  Join. A Bootstrap appends `; answerId = N`, which is the extra
  argument on that check.
- `capn_getv`/`capn_setv` for 16, 32 and 64 bit lists treated the element
  offset as bytes; `getv1`/`setv1` mixed bits and bytes; negative offsets
  and counts are rejected, as are negative indices to `capn_get*`,
  `capn_set*`, `capn_getp` and `capn_setp`.
- RPC: frames over 8 KiB were dropped; `Unimplemented` and `Abort` were
  answered with an `Unimplemented` of their own and the echo wrapped the
  message in a second one; a `Release` count of 0xffffffff overflowed.
- `capn_rpc_export` reused an export whenever the server pointer matched,
  so one object served by two dispatchers collapsed into the first.
- `capnpc-c`: a header for a `-> stream` method included the nonexistent
  `capnp/stream.capnp.h`; two schemas in one request read freed memory;
  an all-Void union produced an empty C union; generated constants warned
  under `-Wextra`.
- The checked-in generated sources matched an older plugin (no `has_`
  accessors); they are regenerated.
- An installed `capnpc-c` could not find `libcapnp_c.so`; the meson
  `.pc` file lacked `codegen`; `make dist` failed and the tarball lacked
  files; `.travis.yml` was dead and is removed.

## [0.4.2] - 2026-11-21

### Added

- A `v*` tag publishes a `meson dist` tarball after the test jobs. The
  tarball job runs `make check` on the extracted tree.
- `scripts/backfill-releases.sh` opens GitHub Releases for `v0.4.0` and
  `v0.4.1` from their changelog sections. It does not create tags.
- A pull request that changes the library must add a `CHANGELOG.md`
  bullet. A docs-only or CI-only diff is skipped.
- Sanitizer jobs run ASan and UBSan with gcc and clang.
- A prerelease tag uploads `c-capnproto-<version>.tar.xz`, and the
  archive root uses that same name. `<version>` is the tag without `v`.

### Fixed

- `capnpc-c` accepts an empty string append without passing a null pointer to
  `memcpy`, including under UBSan.
- Generator-only helper parameters are limited to values that affect emitted
  code, keeping production builds clean under `warning_level=3` and `werror`.
- The installed-package smoke build disables the producer's optional test
  dependencies, so downstream CMake consumers do not inherit GTest setup.
- Meson installs `c-capnproto.pc` from `c-capnproto.pc.in`. `codegen` is
  `${bindir}/capnpc-c` and `bindir` is set. The description is
  `Cap'n Proto C bindings`. The `URL` field is removed.
- Encoding a null pointer no longer applies an offset to a null data address.
- Packed framing copies the segment header with `memcpy`. The two-byte
  offset is not a misaligned `uint32_t` store.
- The Meson shared library SONAME is `libcapnp_c.so.0`. Builds before
  0.4.2 used an unversioned `libcapnp_c.so`, so those consumers should relink.
- With `enable_tests` set to `auto`, unit tests are skipped when `gtest`
  is absent. `-Denable_tests=enabled` fails setup if `gtest` cannot be built.
- `-Denable_tests=true` and `-Denable_tests=false` still select the
  feature values. Meson warns that those booleans are deprecated.

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

[Unreleased]: https://github.com/HaoZeke/c-capnproto/compare/v0.4.2...main
[0.4.2]: https://github.com/HaoZeke/c-capnproto/compare/v0.4.1...v0.4.2
[0.4.1]: https://github.com/HaoZeke/c-capnproto/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/HaoZeke/c-capnproto/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/HaoZeke/c-capnproto/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/HaoZeke/c-capnproto/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/HaoZeke/c-capnproto/releases/tag/v0.1.0
