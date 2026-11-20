#!/bin/sh
# The streaming return type imports /capnp/stream.capnp. That file has to
# be present in the capnproto 1.4.0 layout under our schema include root,
# and the CI jobs that compile those schemas have to install the package
# that ships the same file.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"

schema=$root/compiler/capnp/stream.capnp
if [ ! -f "$schema" ]; then
	echo "missing $schema" >&2
	exit 1
fi
if ! grep -q '@0x86c366a91393f3f8;' "$schema"; then
	echo "stream schema is not the capnproto 1.4.0 definition" >&2
	exit 1
fi

awk '
/^  [A-Za-z0-9-]+:$/ {
	job = $1
	sub(/:$/, "", job)
}
$1 == "-" && $2 == "name:" && $3 == "Install" && $4 == "capnproto" && NF == 4 {
	pending = job
	next
}
pending != "" && $1 == "run:" {
	if (index($0, "libcapnp-dev") == 0) {
		print "job " pending " does not install libcapnp-dev" > "/dev/stderr"
		exit 1
	}
	seen[pending] = 1
	pending = ""
}
END {
	if (!seen["meson"] || !seen["sanitizers"]) {
		print "meson and sanitizer jobs must install libcapnp-dev" > "/dev/stderr"
		exit 1
	}
}
' .github/workflows/ci.yml

command -v capnp >/dev/null || {
	echo "capnp CLI is required" >&2
	exit 1
}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/out"
capnp compile --src-prefix="$root/tests" -I"$root/compiler" -oc++:"$tmp/out" \
	"$root/tests/interface-field.capnp"
test -s "$tmp/out/interface-field.capnp.h"
