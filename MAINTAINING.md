# Maintaining HaoZeke/c-capnproto

## Location

| Item | Path |
|------|------|
| Clone | `~/Git/Github/Fortran/c-capnproto` |
| Sibling | `~/Git/Github/Fortran/capnp-fortran` |
| Remote | `git@github.com:HaoZeke/c-capnproto.git` |
| Branch | `master` |

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

Not absorbed (tracked as work, not merged): `shen390s` compiler rewrite/codecgen, DKML `dk` wrapper / CMakePresets / GitLab CI / gtest-submodule removal, yeger full in-tree kbuild copy, Degui XOR/MISRA (wire-incompatible), cbrune `const2` (OSR #54 broke the build and was reverted), commaai prefix hack, `aligned(64)` whole-struct ARM hacks (superseded by field `ALIGNED_(8)`).
