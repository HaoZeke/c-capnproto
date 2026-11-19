#!/bin/sh
# codespell rejects one spelling of this reflect constant. The constant
# is a local test name, not an upstream schema field, so the schema uses
# cFromOther and still aliases the other file's constant.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"
bad=$(printf '%s%s' cali ased)
if grep -n -i "$bad" tests/reflect.capnp; then
	echo "reflect constant uses the spelling codespell rewrites" >&2
	exit 1
fi
grep -q 'const cFromOther :Int64 = Other.otherConst;' tests/reflect.capnp
if command -v codespell >/dev/null 2>&1; then
	codespell tests/reflect.capnp
fi
