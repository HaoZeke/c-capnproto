#!/usr/bin/env bash
# Require an annotated git tag so `git push --follow-tags` pushes it.
set -euo pipefail

if [ "$#" -gt 1 ]; then
  echo "usage: $0 [tag]" >&2
  exit 2
fi

if [ "$#" -eq 1 ]; then
  tag=$1
else
  tag=$(git describe --exact-match --tags HEAD 2>/dev/null || true)
  if [ -z "$tag" ]; then
    echo "no tag on HEAD" >&2
    exit 1
  fi
fi

if ! git show-ref --verify --quiet "refs/tags/${tag}"; then
  echo "tag ${tag} does not exist" >&2
  exit 1
fi

obj_type=$(git cat-file -t "refs/tags/${tag}")
if [ "$obj_type" != "tag" ]; then
  echo "tag ${tag} is lightweight (${obj_type}); create an annotated tag for --follow-tags" >&2
  exit 1
fi

echo "annotated tag ${tag}"
