# Conventions for c-capnproto

## Changelog

`CHANGELOG.md` follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Put user-visible changes under `[Unreleased]` when they land. A release
moves those entries into a dated `[X.Y.Z]` section and updates the compare
links at the bottom of the file.

## Release

One version is written in `VERSION.txt`, `meson.build`, and `configure.ac`.
CMake reads `VERSION.txt`. `scripts/check-versions.sh` fails when those three
values differ. On a `v*` tag (`GITHUB_REF_TYPE=tag`, or an exact tag on
`HEAD`) the same script requires the version to equal the tag with the
leading `v` removed.

Cut the release by tagging `vX.Y.Z` on `main`. `.github/workflows/release.yml`
runs for tags that match `v*`. It builds and tests with meson, autotools, and
CMake. Each of those jobs runs `scripts/check-versions.sh` before the build.
The publish job waits until all three have passed. It opens a GitHub Release
whose notes are that version's `CHANGELOG.md` section, extracted by
`scripts/release_notes.py`, and whose asset is the `meson dist` tarball.
The meson job checks out submodules, so the tarball contains `gtest` and
`autoreconf -fi` succeeds on the extracted tree. A tag that contains a hyphen
is a prerelease.

`scripts/release_notes.py` fails when `CHANGELOG.md` has no section for the
version, and the release job stops with it.

## Semantic version

Versions follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html).
The changelog header records the pre-1.0 caveat: a minor release before 1.0
may include a breaking wire or API change. A release of fixes bumps the patch.
A new feature bumps the minor. Record a wire or API break under a Breaking
heading. The version in this tree is `0.4.2`, the patch that follows `v0.4.1`
for the fixes landed since that tag.
