capnpc-c (HaoZeke fork)
=======================

[![CI](https://github.com/HaoZeke/c-capnproto/actions/workflows/ci.yml/badge.svg)](https://github.com/HaoZeke/c-capnproto/actions/workflows/ci.yml)

Pure **C** runtime + `capnpc-c` plugin for [Cap'n Proto](https://capnproto.org/).

This repository is **[HaoZeke/c-capnproto](https://github.com/HaoZeke/c-capnproto)**,
a maintained fork of `opensourcerouting/c-capnproto` (itself from
`jmckaskill/c-capnproto`). Upstream declared itself unmaintained (no releases,
unreviewed PRs; last real work ~2023). Public forks surveyed 2026-07: no
active maintainer with a credible commit series. This fork exists as the
canonical pure-C Cap'n home next to
[HaoZeke/capnp-fortran](https://github.com/HaoZeke/capnp-fortran).

**Upstream lineage:** `jmckaskill` → `opensourcerouting` → **this fork**.

> ## Security warning

> Decode rejects out-of-segment pointer landings, applies a 64MiB traversal
> budget, and walks the pointer graph (`capn_validate`, nesting limit 64).
> Failed hops are still `CAPN_NULL`; check `capn_ok` to tell them from a
> missing field. This is still **not** C++ `MessageReader` throw-parity.
> Do **not** treat generated readers as untrusted-safe. See
> [SECURITY.md](SECURITY.md).

## Building (Meson, recommended)

```sh
git clone --recurse-submodules https://github.com/HaoZeke/c-capnproto.git
cd c-capnproto
meson setup build
meson compile -C build
# plugin: build/capnpc-c   runtime: build/libcapnp.a (or shared)
meson install -C build   # optional: put capnpc-c on PATH
```

## Building (CMake, optional)

Meson is the recommended build. CMake is an install-path alternative
(MSVC / IDE / packagers) and is not the default. It does not use the
DKML `dk` wrapper and does not fetch or delete the `gtest/` submodule.

```sh
cmake -S . -B build-cmake
cmake --build build-cmake
cmake --install build-cmake   # optional: libcapnp_c, capnpc-c, header, c.capnp, pc
ctest --test-dir build-cmake  # only if CMake found an installed GTest
```

Tests are skipped when GTest is not installed. Use meson (or autotools
with the existing `gtest/` submodule) for the full suite.

## Building (autotools)

```sh
git clone --recurse-submodules https://github.com/HaoZeke/c-capnproto.git
cd c-capnproto
autoreconf -f -i -s
./configure
make
make check
```

## Usage

### Generating C code from a `.capnp` schema file

The `compiler` directory contains the C language plugin (`capnpc-c`) for use with the `capnp` tool: https://capnproto.org/capnp-tool.html.

`capnp` will by default search `$PATH` for `capnpc-c` - if it's on your PATH, you can generate code for your schema as follows:

```sh
# after meson install; ${prefix} is /usr/local by default
capnp compile -I${prefix}/share/c-capnproto -o c schema.capnp
```

Otherwise, you can specify the path to the c plugin. From a source checkout, `-Icompiler` finds the same schema:

```sh
capnp compile -Icompiler -o ./capnpc-c schema.capnp
```

`capnp` generates a C struct that corresponds to each capn proto struct, along with read/write functions that convert to/from capn proto form. Generated headers `#include "c.capnp.h"` (installed next to `capnp_c.h`).

You **must** call `capn_set_root(c, person.p)` (or `capn_setp(capn_root(c), 0, person.p)`) after `new_*` / `write_*`. Those helpers only fill a struct in a segment; they do not attach it as the message root. Omitting the call writes a valid empty message (`capn_getp(root, 0)` is null; `capnp decode` shows `()`). Empty messages are legal -- they are just not the payload you built.

Zero-init C structs (`struct Person p = {0};` or `memset`) before `write_*` so optional pointer fields (nested structs, lists, text, data) stay unset. A `CAPN_NULL` / NULL `data` pointer is encoded as a wire null; C++ `hasFoo()` is false and this reader returns `CAPN_NULL`. Uninitialized (garbage) pointer fields are not safe. `capn_new_struct(seg, 0, 0)` is a real empty struct (`struct Empty {}`): A=0 B=-1 C=D=0 (`0xFFFFFFFC`), not null.

If you want accessor functions for struct members, import the C annotations (`/c.capnp`) and use `$C.fieldgetset`:

```capnp
using C = import "/c.capnp";

$C.fieldgetset;

struct MyStruct {}
```

Pointer fields also get `Foo_has_bar(Foo_ptr p)`: nonzero when the wire
pointer is not `CAPN_NULL`. That is C++ `hasFoo()`. `get_` / `read_` still
substitute schema defaults, so a null Text reads as `""` while
`Foo_has_note` is 0. Use `capn_getp` (or `has_`) when null vs empty
matters.

Empty Text is a `capn_text` with `str` non-NULL and `len == 0` (`capn_set_text`
writes a 1-byte NUL list). Null Text is `str == NULL`; `capn_set_text`
writes a wire null. Empty Data is `capn_new_list8(seg, 0)` then
`capn_set_data`. Null Data is a zeroed `capn_data`. Codecgen `NULL` Text
is wire null; a non-NULL Data pointer with length 0 is an empty list.

### Example C code

See the unit tests in [`tests/example-test.cpp`](tests/example-test.cpp).
The example schema file is [`tests/addressbook.capnp`](tests/addressbook.capnp).
The tests are written in C++, but only use C features.

Encode/decode against the capnp-fortran AddressBook goldens lives in
[`tests/fortran-golden-test.cpp`](tests/fortran-golden-test.cpp)
([`tests/fortran-golden.md`](tests/fortran-golden.md)).

Official `capnp` CLI interop (C encode then `capnp decode`; `capnp encode`
of checked-in text then `read_`) lives in
[`tests/capnp-cli-interop-test.cpp`](tests/capnp-cli-interop-test.cpp).
Meson registers that gtest only when `find_program('capnp')` succeeds.
Ubuntu CI installs `capnproto` so the tests run there. C encode is not
required to match `capnp encode` byte for byte.

Typical write path:

```c
struct capn c;
capn_init_malloc(&c);
Person_ptr pp = new_Person(capn_root(&c).seg);
write_Person(&person, pp);
capn_set_root(&c, pp.p);   /* required; otherwise the message is empty */
sz = capn_write_mem(&c, buf, sizeof(buf), 0);
capn_free(&c);
```

Canonical form (`encoding.html`) is a single-segment rewrite with no far
pointers, no holes, preorder layout, and trailing zero data/pointer words
truncated (empty struct `B = -1`):

```c
struct capn canon;
capn_init_malloc(&canon);
if (capn_canonicalize(&c, &canon) == 0) {
    /* unframed: canon.seglist->data[0 .. len) */
    /* 1-segment stream: capn_write_mem(&canon, buf, sz, 0) */
}
capn_free(&canon);
```

You need to compile these runtime library files and link them into your own project's binaries:

* [`lib/capn.c`](lib/capn.c)
* [`lib/capn-malloc.c`](lib/capn-malloc.c)
* [`lib/capn-stream.c`](lib/capn-stream.c)

Your include path must contain the runtime library directory
[`lib`](lib). Header file [`lib/capnp_c.h`](lib/capnp_c.h) contains
the public interfaces of the library.

Using make-based builds, make may try to compile `${x}.capnp` from
`${x}.capnp.c` using its built-in rule for compiling `${y}` from
`${y}.c`. You can either disable make's built-in compile rules or just
this specific case with the no-op rule: `%.capnp: ;`.

For further reference, please see the other unit tests in [`tests`](tests), and header file [`lib/capnp_c.h`](lib/capnp_c.h).

The project [`quagga-capnproto`](https://github.com/opensourcerouting/quagga-capnproto) uses `c-capnproto` and contains some good examples, as found with [this github repository search](https://github.com/opensourcerouting/quagga-capnproto/search?utf8=%E2%9C%93&q=capn&type=):

* Serialization in function [`bgp_notify_send()`](https://github.com/opensourcerouting/quagga-capnproto/blob/27061648f3418fac0d217b16a46add534343e841/bgpd/bgp_zmq.c#L81-L96) in file `quagga-capnproto/bgpd/bgp_zmq.c`
* Deserialization in function [`qzc_callback()`](https://github.com/opensourcerouting/quagga-capnproto/blob/27061648f3418fac0d217b16a46add534343e841/lib/qzc.c#L249-L257) in file `quagga-capnproto/lib/qzc.c`

## Linux kernel (optional)

The runtime can be compiled into a kernel module (`__KERNEL__`). This is
not the default and is not covered by CI. See [KERNEL.md](KERNEL.md) and
the sample in [`examples/kernel`](examples/kernel).

### Small messages

`capn_init_malloc` allocates a new 4096-byte segment on the first write of
each session (`-DCAPN_CREATE_MIN_SZ=<power of two>` overrides the floor).
Calling it once per small message pays that malloc every time.
Reuse one `struct capn` (and its arena) across messages, or skip the heap
allocator and feed a caller buffer with `capn_init_mem` /
`capn_append_segment`.

## Fuzzing

Harnesses live in [`fuzz/read_mem.c`](fuzz/read_mem.c) (memory reader) and
[`fuzz/read_fp.c`](fuzz/read_fp.c) (`FILE*` reader). They link the addressbook
example schema. Seed corpus: `fuzz/in/` (checked-in `Person.bin`; the autotools
`fuzz-mem` / `fuzz-fp` targets also create it via `capnp encode`). CI Ubuntu
builds the meson harnesses (`-Dfuzz=true`) and runs a one-shot smoke on that
seed; it does not launch unbounded AFL.

### Meson (libFuzzer or AFL++)

```sh
meson setup build-fuzz -Dfuzz=true -Denable_tests=false
meson compile -C build-fuzz
# harnesses: build-fuzz/fuzz-mem  build-fuzz/fuzz-fp
meson test -C build-fuzz   # fuzz-mem-smoke / fuzz-fp-smoke on Person.bin
# same one-shot: ./build-fuzz/fuzz-mem fuzz/in/Person.bin
```

libFuzzer: configure with clang and `-Db_sanitize=fuzzer,address`. AFL++: set
`CC`/`CXX` to `afl-clang-fast` (or `afl-clang`) before `meson setup`.

### Autotools (AFL)

With `afl-clang` and `capnp` on `PATH`:

```sh
make fuzz-mem    # packed-memory reader
make fuzz-fp     # FILE* reader
```

## Status

Lineage: [James McKaskill](https://github.com/jmckaskill/c-capnproto) merged with
[liamstask](https://github.com/liamstask/c-capnproto),
[baruch](https://github.com/baruch/c-capnproto), and
[kylemanna](https://github.com/kylemanna/c-capnproto), then
[opensourcerouting](https://github.com/opensourcerouting/c-capnproto).
This fork also carries C-level fixes from Jonah Beckford (DKML/MSVC),
Rongsong Shen (packed-header alignment, copy-tree parent), and Angelo
Haller (AFL harness). See [MAINTAINING.md](MAINTAINING.md).
Windows CI is x64; 32-bit coverage is the compile-time
`sizeof(capn_segment)%8` check. Optional `scripts/smoke-m32.sh` when
an ILP32 `gcc -m32` exists (N/A on the maintainer builder).

## Install layout

| Artifact | Path |
|----------|------|
| Plugin | `bin/capnpc-c` |
| Header | `include/capnp_c.h`, `include/c.capnp.h` |
| Library | `lib/libcapnp_c.a` / `.so` |
| pkg-config | `lib/pkgconfig/c-capnproto.pc` |
| Schema helper | `share/c-capnproto/c.capnp` (`import "/c.capnp"` with `-I` this dir) |

Maintaining: [MAINTAINING.md](MAINTAINING.md). Security: [SECURITY.md](SECURITY.md).
