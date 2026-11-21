#!/usr/bin/env bash
# Fail a pull request that changes product code without a new
# CHANGELOG.md bullet. A docs-only or CI-only diff is skipped.
set -euo pipefail

root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

if [[ -n "${CHECK_BASE:-}" ]]; then
  base=$CHECK_BASE
elif [[ -n "${GITHUB_BASE_REF:-}" ]]; then
  git fetch --no-tags origin "$GITHUB_BASE_REF"
  base="origin/${GITHUB_BASE_REF}"
else
  echo "no base ref; set CHECK_BASE or GITHUB_BASE_REF" >&2
  exit 1
fi

if ! git rev-parse --verify "${base}^{commit}" >/dev/null 2>&1; then
  echo "base ${base} is not a commit" >&2
  exit 1
fi

mapfile -t files < <(git diff --name-only "${base}...HEAD")
if [[ ${#files[@]} -eq 0 ]]; then
  echo "no files changed; changelog check skipped"
  exit 0
fi

docs_or_ci=1
for f in "${files[@]}"; do
  case "$f" in
    docs/*|*.md|*.rst|*.org|.github/*|.travis.yml|LICENSE|COPYING|NOTICE)
      ;;
    *)
      docs_or_ci=0
      ;;
  esac
done

if [[ "$docs_or_ci" -eq 1 ]]; then
  echo "docs-only or CI-only change; changelog check skipped"
  printf '  %s\n' "${files[@]}"
  exit 0
fi

has_log=0
for f in "${files[@]}"; do
  if [[ "$f" == "CHANGELOG.md" ]]; then
    has_log=1
  fi
done
if [[ "$has_log" -ne 1 ]]; then
  echo "CHANGELOG.md was not updated. Add a bullet under [Unreleased]." >&2
  echo "A docs-only or CI-only change is skipped." >&2
  exit 1
fi

if ! git diff -U0 "${base}...HEAD" -- CHANGELOG.md | grep -E '^\+([[:blank:]]*[-*][[:blank:]]+[^[:space:]]|## \[[0-9]+\.[0-9]+\.[0-9]+\] - [0-9]{4}-[0-9]{2}-[0-9]{2})' >/dev/null; then
  echo "CHANGELOG.md has no non-empty bullet or dated release heading" >&2
  exit 1
fi
echo "changelog entry present"
