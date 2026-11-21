# Conventions for c-capnproto

## Changelog

`CHANGELOG.md` follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Put user-visible changes under `[Unreleased]` when they land. A release
moves those entries into a dated `[X.Y.Z]` section and updates the compare
links at the bottom of the file.

`.github/workflows/changelog.yml` runs `scripts/check-changelog.sh` on
pull requests. A diff that changes library code must add a non-empty
bullet, or a dated `## [X.Y.Z] - YYYY-MM-DD` heading, to `CHANGELOG.md`.
A diff that only touches docs (`docs/`, `*.md`, `*.rst`,
`*.org`) or CI (`.github/`, `.travis.yml`) is skipped.

## Release

One version is written in `VERSION.txt`, `meson.build`, and `configure.ac`.
CMake reads `VERSION.txt`. The `GIT_TAG` line in `CMakeLists.txt` and the
`revision` line in the README wrap snippet are `v` plus that version.
`scripts/check-versions.sh` fails when those values differ.

On a `vX.Y.Z` tag the script requires the version files to equal `X.Y.Z`.
A tag `vX.Y.Z-<id>` is a prerelease of that version. The files may stay
at `X.Y.Z`, and `.github/workflows/release.yml` marks the GitHub Release
as a prerelease. On a rerun, `gh release edit` sets that flag from the
tag and clears it when the tag has no hyphen.

`scripts/release_notes.py` prints the `## [version]` section. It reads
`CHANGELOG.md` from the repository root, so it works outside that
directory. For a prerelease it prints `## [X.Y.Z-<id>]` when that section
exists, and `## [X.Y.Z]` otherwise. It fails when neither section exists.

Cut the release with an annotated `vX.Y.Z` tag on `main` (run
`cog bump --auto -A "Release {{version}}"` and push with
`git push origin main --follow-tags`). `release.yml` runs for tags
that match `v*`. Before publishing it runs the Ubuntu checks from CI:
meson tests, the capnp CLI parity guard, `scripts/gen-rpc-frames.sh --check`,
the fuzz and install smokes, ASan+UBSan with gcc and clang,
`scripts/packaging-smoke.sh`, and the live C++ peer linked with
`libcapnp-dev`. It also builds and tests with autotools and CMake. CI
runs those two builds on pull requests. The meson job builds a
`meson dist` tarball and runs `autoreconf -fi`, `./configure`, `make`,
and `make check` on the extracted tree. The publish job waits until every
job has passed. Notes are the changelog section. The asset is the tarball.
A prerelease asset and its archive root are `c-capnproto-<version>`,
where `<version>` is the tag with the leading `v` removed.

Tags `v0.4.0` and `v0.4.1` exist and have changelog sections. The GitHub
Releases page stops at `v0.3.0`. `scripts/backfill-releases.sh` opens
those two Releases from the changelog. It does not create tags.
`--dry-run` prints the plan and skips `gh`.

## Semantic version

Versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
The changelog header records the pre-1.0 caveat: a minor release before 1.0
may include a breaking wire or API change. A release of fixes bumps the patch.
A new feature bumps the minor. Record a wire or API break under a Breaking
heading. The version in this tree is `0.4.2`, the patch that follows `v0.4.1`
for the fixes landed since that tag.
