#!/bin/sh
# Replay a corpus, and when asked, mutated copies of it, until `seconds`
# have elapsed. A hang (timeout 124) or a signal is a failure. A clean
# non-zero status is a rejected input and is not.
# Usage: fuzz-smoke.sh <seconds> <binary> [--mutate <seed> <rate>] <file>...
# `rate` is the percent of bytes replaced. The seed advances so each
# mutant differs, and the clock bounds how many are produced.
set -u
if [ "$#" -lt 3 ]; then
	echo "usage: fuzz-smoke.sh <seconds> <binary> [--mutate seed rate] <file>..." >&2
	exit 2
fi
seconds=$1
bin=$2
shift 2
seed=0
rate=0
if [ "${1:-}" = "--mutate" ]; then
	seed=$2
	rate=$3
	shift 3
fi
if [ "$#" -lt 1 ]; then
	echo "fuzz-smoke: no corpus files" >&2
	exit 2
fi

run_one() {
	timeout 5 "$bin" "$1"
	rc=$?
	if [ "$rc" -eq 124 ] || [ "$rc" -ge 128 ]; then
		echo "fuzz smoke failed: $bin $1 rc=$rc" >&2
		exit "$rc"
	fi
}

mutate_to() {
	# Print the next seed. Write the mutant to $3.
	seed=$(python3 - "$seed" "$rate" "$1" "$2" << 'PY'
import sys
seed = int(sys.argv[1]) & 0xFFFFFFFF
rate = int(sys.argv[2])
src = open(sys.argv[3], "rb").read()
if not src:
    src = b"\x00"
def nxt():
    global seed
    seed ^= (seed << 13) & 0xFFFFFFFF
    seed ^= (seed >> 17) & 0xFFFFFFFF
    seed ^= (seed << 5) & 0xFFFFFFFF
    seed &= 0xFFFFFFFF
    return seed
out = bytearray(src)
for i in range(len(out)):
    if nxt() % 100 < rate:
        out[i] = nxt() & 0xFF
open(sys.argv[4], "wb").write(out)
print(seed)
PY
)
}

end=$(( $(date +%s) + seconds ))
tmp=$(mktemp)
trap 'rm -f "$tmp"' EXIT
while [ "$(date +%s)" -lt "$end" ]; do
	for f in "$@"; do
		run_one "$f"
		if [ "$rate" -gt 0 ]; then
			mutate_to "$f" "$tmp" || exit 1
			run_one "$tmp"
		fi
		if [ "$(date +%s)" -ge "$end" ]; then
			break
		fi
	done
done
exit 0
