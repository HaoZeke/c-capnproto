#!/usr/bin/env bash
# Open GitHub Releases for v0.4.0 and v0.4.1 from their changelog
# sections. Those tags exist. The Releases page stops at v0.3.0.
# This script does not create tags. Pass --dry-run to skip gh.
set -euo pipefail

root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

dry=0
if [[ "${1:-}" == "--dry-run" ]]; then
  dry=1
elif [[ -n "${1:-}" ]]; then
  echo "usage: scripts/backfill-releases.sh [--dry-run]" >&2
  exit 1
fi

repo=${GITHUB_REPOSITORY:-HaoZeke/c-capnproto}

for tag in v0.4.0 v0.4.1; do
  ver=${tag#v}
  notes=$(mktemp)
  python3 "$root/scripts/release_notes.py" "$ver" -o "$notes"
  if [[ "$dry" -eq 1 ]]; then
    echo "would publish ${tag} --repo ${repo} ($(wc -c < "$notes") bytes of notes)"
    rm -f "$notes"
    continue
  fi
  if gh release view "$tag" --repo "$repo" >/dev/null 2>&1; then
    gh release edit "$tag" --repo "$repo" --title "$tag" --notes-file "$notes"
  else
    gh release create "$tag" --repo "$repo" --title "$tag" --notes-file "$notes"
  fi
  rm -f "$notes"
done
