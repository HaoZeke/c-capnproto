#!/bin/sh
# Run one fuzzer over the given corpus until `seconds` have elapsed.
# A hang (timeout 124) or a signal is a failure. A clean non-zero
# status is a rejected input and is not.
# Usage: fuzz-smoke.sh <seconds> <binary> <file> [file...]
set -u
if [ "$#" -lt 3 ]; then
	echo "usage: fuzz-smoke.sh <seconds> <binary> <file>..." >&2
	exit 2
fi
seconds=$1
bin=$2
shift 2
end=$(( $(date +%s) + seconds ))
while [ "$(date +%s)" -lt "$end" ]; do
	for f in "$@"; do
		timeout 5 "$bin" "$f"
		rc=$?
		if [ "$rc" -eq 124 ] || [ "$rc" -ge 128 ]; then
			echo "fuzz smoke failed: $bin $f rc=$rc" >&2
			exit "$rc"
		fi
	done
done
exit 0
