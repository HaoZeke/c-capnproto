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
