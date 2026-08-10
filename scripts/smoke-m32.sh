#!/bin/sh
# 32-bit compile smoke for struct capn_segment alignment.
#
# Cap'n Proto words are 8 bytes. On ILP32, sizeof(struct capn_segment)
# is not a multiple of 8 unless the ALIGNED_(8) field attributes hold.
# lib/capn-malloc.c encodes that as a compile-time bitfield
# (check_segment_alignment); this script compiles the runtime with
# gcc -m32 so that check actually sees 32-bit pointers.
#
# Needs ILP32 libc headers (gnu/stubs-32.h). An empty gcc -m32 -c
# without those headers is not a working 32-bit toolchain.
#
# Usage (from anywhere):
#   scripts/smoke-m32.sh
# Env:
#   CC  C compiler that accepts -m32 (default: gcc)
#
# Exit:
#   0  i386 objects built; sizeof(capn_segment) % 8 == 0
#   2  no working gcc -m32 -c (honest N/A)
#   1  compile or alignment failure

set -eu

CC="${CC:-gcc}"

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

if [ ! -f lib/capn-malloc.c ] || [ ! -f lib/capnp_c.h ]; then
	echo "smoke-m32: not a c-capnproto tree: $root" >&2
	exit 1
fi

tmp=$(mktemp -d "${TMPDIR:-/tmp}/c-capnproto-m32.XXXXXX")
trap 'rm -rf "$tmp"' EXIT INT TERM

# Must include a libc header. gcc -m32 can emit an empty i386 object
# without lib32-glibc; compiling capn.c then dies on gnu/stubs-32.h.
probe="$tmp/probe.c"
printf '#include <stdint.h>\nint probe_sizeof_void_star = (int)sizeof(void *);\n' >"$probe"

if ! "$CC" -m32 -c -o "$tmp/probe.o" "$probe" >"$tmp/probe.log" 2>&1; then
	echo "smoke-m32: N/A (no ILP32 libc headers; gcc -m32 -c cannot compile stdint.h)"
	echo "  CC=$CC"
	"$CC" --version 2>/dev/null | head -n 1 || true
	echo "  --- compiler output ---"
	sed 's/^/  /' "$tmp/probe.log"
	exit 2
fi

file_probe=$(file "$tmp/probe.o" 2>/dev/null || true)
case "$file_probe" in
*32-bit*|*i386*|*Intel\ 80386*)
	;;
*)
	echo "smoke-m32: N/A (gcc -m32 did not produce an i386 object)"
	echo "  file: $file_probe"
	exit 2
	;;
esac

echo "smoke-m32: compiler $($CC --version | head -n 1)"
echo "smoke-m32: probe $file_probe"

inc="-I${root}/lib -std=c99"
src_ok=0
for src in lib/capn.c lib/capn-malloc.c lib/capn-stream.c; do
	base=$(basename "$src" .c)
	if "$CC" -m32 $inc -c -o "$tmp/${base}.o" "$src"; then
		echo "smoke-m32: compiled $src -> $(file "$tmp/${base}.o")"
		src_ok=$((src_ok + 1))
	else
		echo "smoke-m32: FAIL compiling $src with -m32" >&2
		exit 1
	fi
done

# Companion asserts + integer dumps (visible in -S output).
cat >"$tmp/align.c" <<'EOF'
#include "capnp_c.h"
#include <stddef.h>

_Static_assert(sizeof(void *) == 4, "smoke-m32 expects ILP32");
_Static_assert(sizeof(struct capn_segment) % 8 == 0,
	"sizeof(struct capn_segment) must be a multiple of 8");
_Static_assert(offsetof(struct capn_segment, data) % 8 == 0,
	"capn_segment.data must be 8-byte aligned");
_Static_assert(offsetof(struct capn_segment, len) % 8 == 0,
	"capn_segment.len must be 8-byte aligned");
_Static_assert(offsetof(struct capn_segment, cap) % 8 == 0,
	"capn_segment.cap must be 8-byte aligned");
_Static_assert(offsetof(struct capn_segment, user) % 8 == 0,
	"capn_segment.user must be 8-byte aligned");

const int smoke_sizeof_void_star = (int)sizeof(void *);
const int smoke_sizeof_capn_segment = (int)sizeof(struct capn_segment);
const int smoke_capn_segment_mod8 = (int)(sizeof(struct capn_segment) % 8);
EOF

if ! "$CC" -m32 $inc -c -o "$tmp/align.o" "$tmp/align.c"; then
	echo "smoke-m32: FAIL alignment _Static_assert under -m32" >&2
	exit 1
fi
"$CC" -m32 $inc -S -o "$tmp/align.s" "$tmp/align.c"
echo "smoke-m32: align object $(file "$tmp/align.o")"
echo "smoke-m32: sizeof dumps (from -S):"
grep -E 'smoke_sizeof_void_star|smoke_sizeof_capn_segment|smoke_capn_segment_mod8' -A1 "$tmp/align.s" | sed 's/^/  /'

echo "smoke-m32: PASS ($src_ok lib sources + align asserts, ILP32, no link)"
exit 0
