#!/usr/bin/env bash
# Regenerate the checked-in capnpc-c output from its schemas.
# Run with --check to assert the checked-in files still match what the
# current plugin emits. Usage: regen-generated.sh [--check] [path/to/capnpc-c]
set -euo pipefail

here=$(cd "$(dirname "$0")/.." && pwd)
check=0
if [[ "${1:-}" == "--check" ]]; then
  check=1
  shift
fi
plugin=${1:-$here/build/capnpc-c}

command -v capnp >/dev/null || {
  echo "regen-generated: no capnp CLI on PATH" >&2
  exit 1
}
[[ -x $plugin ]] || {
  echo "regen-generated: no plugin at $plugin (build it first)" >&2
  exit 1
}

out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

# schema directory, schema name (without .capnp)
schemas=(
  "compiler c++" "compiler c" "compiler rpc" "compiler rpc-threeparty"
  "compiler rpc-twoparty" "compiler schema" "compiler adder" "compiler test"
  "tests addressbook" "tests extraheader" "tests fieldgetset"
  "tests codegen-defaults"
)

cd "$here"
status=0
for entry in "${schemas[@]}"; do
  read -r dir name <<<"$entry"
  work=$out/$dir-$name
  mkdir -p "$work"
  capnp compile --src-prefix="$dir" -Icompiler -o "$plugin:$work" "$dir/$name.capnp"
  for ext in c h; do
    gen=$work/$name.capnp.$ext
    dest=$dir/$name.capnp.$ext
    if [[ $check -eq 1 ]]; then
      if cmp -s "$gen" "$dest"; then
        echo "ok   $dest"
      else
        echo "STALE $dest" >&2
        status=1
      fi
    else
      cp "$gen" "$dest"
    fi
  done
done
exit $status
