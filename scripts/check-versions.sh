#!/usr/bin/env bash
# VERSION.txt, meson.build, and configure.ac carry one version.
# On a vX.Y.Z tag (GITHUB_REF_TYPE=tag, or an exact tag on HEAD) that
# version must be X.Y.Z. A tag vX.Y.Z-<id> is a prerelease of X.Y.Z
# when the files still say X.Y.Z.
set -euo pipefail

root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

vt=$(tr -d '[:space:]' < VERSION.txt)
meson_v=$(sed -n "s/^[[:space:]]*version:[[:space:]]*'\([^']*\)'.*/\1/p" meson.build | head -1)
ac_v=$(sed -n "s/^AC_INIT([^,]*,[[:space:]]*\[\{0,1\}\([^]),]*\).*/\1/p" configure.ac | head -1)

echo "VERSION.txt=${vt}"
echo "meson.build=${meson_v}"
echo "configure.ac=${ac_v}"

if [[ -z "$vt" || -z "$meson_v" || -z "$ac_v" ]]; then
  echo "could not read a version from one of the three files" >&2
  exit 1
fi
if [[ "$vt" != "$meson_v" ]] || [[ "$vt" != "$ac_v" ]]; then
  echo "version files disagree" >&2
  exit 1
fi

tag=""
if [[ "${GITHUB_REF_TYPE:-}" == "tag" && -n "${GITHUB_REF_NAME:-}" ]]; then
  tag=$GITHUB_REF_NAME
elif tag=$(git describe --exact-match --tags HEAD 2>/dev/null); then
  :
else
  tag=""
fi

if [[ -n "$tag" ]]; then
  case "$tag" in
    v*) ver=${tag#v} ;;
    *)
      echo "tag ${tag} does not use the v prefix" >&2
      exit 1
      ;;
  esac
  echo "tag=${tag}"
  if [[ "$ver" == "$vt" ]]; then
    echo "version matches tag ${tag}"
  elif [[ "$vt" != *-* ]]; then
    case "$ver" in
      "$vt"-*)
        id=${ver#"${vt}-"}
        if [[ -z "$id" ]]; then
          echo "version ${vt} does not match tag ${tag}" >&2
          exit 1
        fi
        echo "version matches prerelease tag ${tag}"
        ;;
      *)
        echo "version ${vt} does not match tag ${tag}" >&2
        exit 1
        ;;
    esac
  else
    echo "version ${vt} does not match tag ${tag}" >&2
    exit 1
  fi
else
  echo "no exact v tag on HEAD; checked file agreement only"
fi
