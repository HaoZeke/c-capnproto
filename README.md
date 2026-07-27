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

> ## Security warning (inherited)

> The generated code assumes all input to be trusted until bounds-checking is
> restored. Do **not** feed untrusted Cap'n streams to the generated readers
> without an explicit validation layer. Tracking work: bounds checks on
> decode, CI, and tagged releases on this fork.

## Building (Meson, recommended)

```sh
git clone --recurse-submodules https://github.com/HaoZeke/c-capnproto.git
cd c-capnproto
meson setup build
meson compile -C build
# plugin: build/capnpc-c   runtime: build/libcapnp.a (or shared)
meson install -C build   # optional: put capnpc-c on PATH
```

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
capnp compile -o c myschema.capnp
```

Otherwise, you can specify the path to the c plugin:

```sh
capnp compile -o ./capnpc-c myschema.capnp
```

`capnp` generates a C struct that corresponds to each capn proto struct, along with read/write functions that convert to/from capn proto form.

If you want accessor functions for struct members, use attribute  `fieldgetset` in your `.capnp` file as follows:

```capnp
using C = import "${c-capnproto}/compiler/c.capnp";

$C.fieldgetset;

struct MyStruct {}
```

### Example C code

See the unit tests in [`tests/example-test.cpp`](tests/example-test.cpp).
The example schema file is [`tests/addressbook.capnp`](tests/addressbook.capnp).
The tests are written in C++, but only use C features.

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

## Status

This is a merge of 3 forks of [James McKaskill's great
work](https://github.com/jmckaskill/c-capnproto), which has been untouched for
a while:

- [liamstask's fork](https://github.com/liamstask/c-capnproto)
- [baruch's fork](https://github.com/baruch/c-capnproto)
- [kylemanna's fork](https://github.com/kylemanna/c-capnproto)

## Install layout

| Artifact | Path |
|----------|------|
| Plugin | `bin/capnpc-c` |
| Header | `include/capnp_c.h` |
| Library | `lib/libcapnp_c.a` / `.so` |
| pkg-config | `lib/pkgconfig/c-capnproto.pc` |
| Schema helper | `share/c-capnproto/c.capnp` |

Maintaining: [MAINTAINING.md](MAINTAINING.md). Security: [SECURITY.md](SECURITY.md).
