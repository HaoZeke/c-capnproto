#!/bin/sh
# The RPC schema accessors built into libcapnp_c are public API. Every
# function the generated sources define must carry CAPN_SCHEMA_EXPORT,
# and a Windows DLL must export it. The DLL is built with a MinGW cross
# compiler when one is on PATH.
set -eu
export LC_ALL=C

if [ "$#" -ne 1 ]; then
  printf '%s\n' 'usage: check-schema-export.sh <source-root>' >&2
  exit 1
fi
root=$1
gen="$root/compiler/rpc.capnp.c $root/compiler/rpc-threeparty.capnp.c"

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT HUP INT TERM

status=0
# shellcheck disable=SC2086
awk '
  /^[A-Za-z_]/ && !/^static[ \t]/ && /\(/ && !/;[ \t]*$/ {
    pre = substr($0, 1, index($0, "(") - 1)
    if (!match(pre, /[A-Za-z_][A-Za-z0-9_]*$/))
      next
    print substr(pre, RSTART, RLENGTH)
    if ($0 !~ /^CAPN_SCHEMA_EXPORT[ \t]/) {
      print FILENAME ": definition lacks CAPN_SCHEMA_EXPORT: " $0 > "/dev/stderr"
      failed = 1
    }
  }
  END { exit failed }
' $gen > "$tmpdir/raw" || status=1
sort -u "$tmpdir/raw" > "$tmpdir/expected"
if [ ! -s "$tmpdir/expected" ]; then
  printf '%s\n' 'no schema accessor definitions found' >&2
  exit 1
fi

cc=${MINGW_CC:-x86_64-w64-mingw32-gcc}
objdump=${MINGW_OBJDUMP:-x86_64-w64-mingw32-objdump}
if ! command -v "$cc" >/dev/null 2>&1 || ! command -v "$objdump" >/dev/null 2>&1; then
  if [ "$status" -ne 0 ]; then
    exit "$status"
  fi
  printf '%s\n' "$cc not found; skipping the DLL export check"
  exit 77
fi

if ! "$cc" -shared -fvisibility=hidden -DCAPNP_C_BUILDING \
    -I"$root/lib" -I"$root/compiler" \
    "$root/lib/capn.c" "$root/lib/capn-malloc.c" "$root/lib/capn-stream.c" \
    "$root/lib/capn-rpc.c" "$root/lib/capn-dyn.c" \
    "$root/lib/capn-rpc-schema.c" "$root/lib/capn-rpc-threeparty-schema.c" \
    -lws2_32 -o "$tmpdir/capnp_c.dll" 2> "$tmpdir/cc.log"; then
  cat "$tmpdir/cc.log" >&2
  printf '%s\n' 'MinGW build of the DLL failed' >&2
  exit 1
fi
"$objdump" -p "$tmpdir/capnp_c.dll" |
  awk '/Ordinal\/Name Pointer/ { on = 1; next } on && NF == 0 { on = 0 }
       on && /^\t\[/ { print $NF }' | sort -u > "$tmpdir/actual"
comm -23 "$tmpdir/expected" "$tmpdir/actual" > "$tmpdir/missing"
if [ -s "$tmpdir/missing" ]; then
  printf '%s\n' 'schema accessors missing from the DLL exports:' >&2
  cat "$tmpdir/missing" >&2
  status=1
fi
exit "$status"
