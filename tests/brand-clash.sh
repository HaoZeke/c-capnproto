#!/bin/sh
# A struct whose C name is Box_Text must not share it with Box(Text).
set -eu
CAPNP=$1
CAPNPC=$2
ROOT=$3
LIB=$4
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

compile_schema() {
  "$CAPNP" compile --src-prefix="$ROOT/tests" -I"$ROOT/tests" -I"$ROOT/compiler" \
    -o"$CAPNPC:$TMP" "$1"
}

compile_schema "$ROOT/tests/brand-hold.capnp"
compile_schema "$ROOT/tests/brand-clash.capnp"
compile_schema "$ROOT/tests/brand-separate.capnp"

BRAND=$(sed -n 's/^struct \(Box_Text_b[0-9a-f]*\) {$/\1/p' \
  "$TMP/brand-clash.capnp.h")
if test -z "$BRAND"; then
  echo "brand was not given a distinct name" >&2
  exit 1
fi
if ! grep -q '^typedef struct {capn_ptr p;} Box_Text_ptr;$' "$TMP/brand-hold.capnp.h"; then
  echo "user struct lost the name Box_Text" >&2
  exit 1
fi
if ! grep -q '^struct Box_Text {$' "$TMP/brand-separate.capnp.h"; then
  echo "a brand with no clash lost its readable name" >&2
  exit 1
fi

cat >"$TMP/consumer.c" <<EOF
#include "brand-hold.capnp.h"
#include "brand-clash.capnp.h"
#include <string.h>
int main(void) {
	struct capn c;
	struct Use u;
	struct ${BRAND} b;
	struct Box_Text plain;
	capn_init_malloc(&c);
	memset(&u, 0, sizeof u);
	memset(&b, 0, sizeof b);
	u.t = new_${BRAND}(capn_root(&c).seg);
	b.val.str = "hi";
	b.val.len = 2;
	write_${BRAND}(&b, u.t);
	memset(&b, 0, sizeof b);
	read_${BRAND}(&b, u.t);
	memset(&plain, 0, sizeof plain);
	capn_free(&c);
	return b.val.len == 2 && plain.n == 0 ? 0 : 1;
}
EOF

for cc in gcc clang; do
  for f in brand-hold.capnp brand-clash.capnp brand-separate.capnp consumer; do
    "$cc" -std=c99 -pedantic -Wall -Werror -I"$TMP" -I"$ROOT/lib" \
      -c -o "$TMP/check-$f.o" "$TMP/$f.c"
  done
done

# `SAN_FLAGS` is the sanitizer list the library was built with. A
# clang build links the runtime into each program, so the library
# only names the symbols, and `ldd` shows no runtime.
SAN=${SAN_FLAGS:-}
if [ -z "$SAN" ] && { ldd "$LIB" 2>/dev/null | grep -q libasan ||
  nm -D --undefined-only "$LIB" 2>/dev/null | grep -q ' __[a-z]*san_'; }; then
  SAN="-fsanitize=address,undefined"
fi
gcc -std=c99 -pedantic $SAN -I"$TMP" -I"$ROOT/lib" \
  -L"$(dirname "$LIB")" -Wl,-rpath,"$(dirname "$LIB")" \
  -o "$TMP/consumer" \
  "$TMP/consumer.c" "$TMP/brand-hold.capnp.c" "$TMP/brand-clash.capnp.c" \
  -lcapnp_c
"$TMP/consumer"
