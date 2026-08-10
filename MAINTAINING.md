# Maintaining HaoZeke/c-capnproto

## Location

| Item | Path |
|------|------|
| Clone | `~/Git/Github/Fortran/c-capnproto` |
| Sibling | `~/Git/Github/Fortran/capnp-fortran` |
| Remote | `git@github.com:HaoZeke/c-capnproto.git` |
| Branch | `main` |

## Release checklist

1. `meson setup build && meson compile -C build && meson test -C build`
2. ASan job green in CI
3. Bump `project(... version: ...)` in `meson.build`
4. Tag `vX.Y.Z` and push tag
5. Bump vendored snapshot in GrokOS `grok-policyd/third_party/c-capnproto` if needed

## Consumers

- **GrokOS grok-policyd**: Cap'n peer codec (`policy.capnp` → C) + nng
- **capnp-fortran interop**: golden wire bytes vs this runtime

## Upstream

Do not treat `opensourcerouting/c-capnproto` as living upstream for merges
without review; it is unmaintained. Cherry-pick useful PRs deliberately.

OSR #63 pointed at `gitlab.com/dkml/ext/c-capnproto` (Jonah Beckford).
That tree is a CMake/MSVC port, not a second living C home. Pull C-level
fixes from it; do not switch the canonical remote.

## Absorbed patches (keep author)

| Source | What |
|--------|------|
| Jonah Beckford (`jonahbeckford` / DKML) | MSVC field alignment, `SSIZE_T`, binary stdin, `capnp_use` parens, no left-shift of negatives; optional top-level `CMakeLists.txt` (no `dk`, no CMakePresets, gtest submodule kept) |
| Rongsong Shen (`shen390s`) | `header_render` memcpy (packed unaligned store), null copy-tree parent |
| Angelo Haller (`szanni`) | AFL `fuzz-mem` / `fuzz-fp` harness |
| yeger00 | `__KERNEL__` stdlib replacements (`kmalloc`/`kfree`, no `stdio`); sample under `examples/kernel/` |

Absorbed as ports, not dump-merges: shen390s `$C.codecgen` emitter (`compiler/codecgen.c`) without the `ctx.c` rewrite; Jonah `$C.extraheader` / `$C.extendedattribute`.

Not absorbed: shen390s `ctx.c` compiler rewrite, DKML `dk` wrapper / CMakePresets / GitLab CI / gtest-submodule removal, yeger full in-tree kbuild copy, Degui XOR/MISRA (wire-incompatible), cbrune `const2` (opensourcerouting #54 broke the build and was reverted), commaai prefix hack, `aligned(64)` whole-struct ARM hacks (superseded by field `ALIGNED_(8)`).

## 32-bit / alignment

Windows CI is `windows-latest` x64. There is **no Win32 GHA matrix**. A
32-bit Windows runner is not validated here; adding one without a
working ILP32 toolchain would be a broken job.

`sizeof(struct capn_segment) % 8 == 0` is the property that fails on
ILP32 when field `ALIGNED_(8)` is missing (typical pre-fix size 44,
`44 & 7 == 4`). It is enforced without a 32-bit linker:

| Check | Where | What it needs |
|-------|-------|----------------|
| Compile-time bitfield | `lib/capn-malloc.c` `check_segment_alignment` | any compile of that file |
| gtest | `TEST(Alignment, SegmentSizeMultipleOf8)` | 64-bit CI is enough to keep the assert |

Optional host smoke when `gcc -m32 -c` can compile a file that
includes `<stdint.h>` (needs `gnu/stubs-32.h` from a 32-bit libc
dev package: Debian/Ubuntu `gcc-multilib`, Arch `lib32-glibc`):

```sh
scripts/smoke-m32.sh
```

That recompiles `lib/capn*.c` plus `_Static_assert` companions under
`-m32`, so the bitfield check sees 32-bit pointers. Exit 2 is honest
**N/A** (no ILP32 headers or no `-m32`).

On the maintainer builder this is **N/A**: `gcc -print-multi-lib`
lists `32;@m32`, but `lib32-glibc` / `lib32-gcc-libs` are not
installed, so `#include <stdint.h>` dies on `gnu/stubs-32.h` and
linking cannot find 32-bit `libc` / `libgcc_s`. No i686 cross
compiler is installed. No Ubuntu `gcc-multilib` GHA job is added
(that apt package is not present on the Arch builder). Do not add a
Win32 GHA matrix for the same reason.
