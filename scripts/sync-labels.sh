#!/usr/bin/env bash
# Create or update the labels in .github/labels.yml. Needs gh and python3 with PyYAML.
set -euo pipefail

dry_run=0
if [ "${1:-}" = "--dry-run" ]; then
  dry_run=1
elif [ "$#" -gt 0 ]; then
  echo "usage: $0 [--dry-run]" >&2
  exit 2
fi

root=$(CDPATH='' cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

python3 - <<'PY' | while IFS=$'\t' read -r n c d; do
import yaml

with open(".github/labels.yml", encoding="utf-8") as fh:
    labels = yaml.safe_load(fh)

for item in labels:
    print(f"{item['name']}\t{item['color']}\t{item['description']}")
PY
  if [ "$dry_run" -eq 1 ]; then
    printf 'would sync label: %s (#%s) %s\n' "$n" "$c" "$d"
  else
    gh label create "$n" --color "$c" --description "$d" --force
  fi
done
