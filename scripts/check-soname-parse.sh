#!/bin/sh
# Canned otool -D text for thin and fat Mach-O install names.
# A fat file with one wrong arch must fail. Header lines are not names.
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
script=$root/scripts/check-soname.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

cat > "$tmp/otool" << 'EOF'
#!/bin/sh
set -eu
if [ "${1:-}" != "-D" ] || [ -z "${2:-}" ]; then
	echo "usage: otool -D <file>" >&2
	exit 2
fi
cat -- "$2.otool"
EOF
chmod +x "$tmp/otool"

printf '\376\355\372\317' > "$tmp/thin.dylib"
printf '\312\376\272\276' > "$tmp/fat.dylib"

fail=0
run_case() {
	name=$1
	lib=$2
	expect=$3
	cat > "$lib.otool"
	set +e
	PATH="$tmp:$PATH" sh "$script" "$lib"
	rc=$?
	set -e
	if [ "$rc" -ne "$expect" ]; then
		printf '%s: exit %s, expected %s\n' "$name" "$rc" "$expect" >&2
		fail=1
	else
		printf '%s: exit %s\n' "$name" "$rc"
	fi
}

run_case thin-good "$tmp/thin.dylib" 0 << 'EOF'
libcapnp_c.2.dylib:
@rpath/libcapnp_c.2.dylib
EOF

run_case thin-wrong "$tmp/thin.dylib" 1 << 'EOF'
libcapnp_c.2.dylib:
@rpath/libcapnp_c.1.dylib
EOF

run_case fat-good "$tmp/fat.dylib" 0 << 'EOF'
lib.dylib (architecture x86_64):
@rpath/libcapnp_c.2.dylib
lib.dylib (architecture arm64):
@rpath/libcapnp_c.2.dylib
EOF

run_case fat-wrong "$tmp/fat.dylib" 1 << 'EOF'
libcapnp_c.2.dylib (architecture x86_64):
@rpath/libcapnp_c.2.dylib
libcapnp_c.2.dylib (architecture arm64):
@rpath/libcapnp_c.1.dylib
EOF

run_case fat-header "$tmp/fat.dylib" 1 << 'EOF'
libcapnp_c.2.dylib (architecture x86_64):
@rpath/libcapnp_c.1.dylib
libcapnp_c.2.dylib (architecture arm64):
@rpath/libcapnp_c.1.dylib
EOF

exit "$fail"
