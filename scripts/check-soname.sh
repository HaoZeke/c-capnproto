#!/bin/sh
# ELF soname is libcapnp_c.so.2 (readelf -d). A Mach-O build records
# soversion 2 in the install name (@rpath/libcapnp_c.2.dylib). otool -D
# prints a header line per architecture, then that arch's install name.
# Skip headers and require every install name to match. Exit 77 when the
# file is neither format or the matching tool is missing.
set -eu
lib=${1:?}
magic=$(od -An -N4 -tx1 "$lib" 2>/dev/null | tr -d ' \t\n' | tr 'A-F' 'a-f' || true)

case $magic in
7f454c46)
	if ! command -v readelf >/dev/null 2>&1; then
		exit 77
	fi
	readelf -d "$lib" | grep -q "libcapnp_c.so.2"
	;;
feedfacf|cffaedfe|feedface|cefaedfe|cafebabe|bebafeca)
	if ! command -v otool >/dev/null 2>&1; then
		exit 77
	fi
	otool -D "$lib" | awk '
		NR == 1 || /:$/ { next }
		{ n++; if (index($0, "libcapnp_c.2.dylib") == 0) bad = 1 }
		END { exit !(n && !bad) }
	'
	;;
*)
	exit 77
	;;
esac
