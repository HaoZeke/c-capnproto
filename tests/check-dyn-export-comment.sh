#!/bin/sh
set -eu

if [ "$#" -lt 2 ]; then
  printf '%s\n' 'usage: check-dyn-export-comment.sh <library> <fixture>' >&2
  exit 1
fi

dir=$(dirname "$0")
set +e
out=$(sh "$dir/check-dyn-export.sh" "$1" "$2" 2>&1)
rc=$?
set -e

if [ "$rc" -ne 1 ]; then
  printf '%s\n' "$out" >&2
  exit 1
fi

case "$out" in
  *"public declaration lacks CAPN_EXPORT:"*missing_export_after_comment*)
    exit 0
    ;;
esac

printf '%s\n' "$out" >&2
exit 1
