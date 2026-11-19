#!/usr/bin/env bash
# C vat serves Adder; the capnp-C++ client pipelines through its promise.
# Usage: run_rpc_promise.sh <c-server> <cpp-peer>
set -u
SERVER=$1
CLIENT=$2
SPID=
trap '[ -n "$SPID" ] && kill "$SPID" 2>/dev/null' EXIT

# A random port that no socket in the tables uses yet.
pick_port() {
	while :; do
		p=$((RANDOM % 20000 + 30000))
		h=$(printf '%04X' "$p")
		if ! awk -v h="$h" '$2 ~ ":" h "$" { found = 1 } END { exit !found }' \
			/proc/net/tcp /proc/net/tcp6 2>/dev/null; then
			echo "$p"
			return
		fi
	done
}

# The C++ client connects once. Wait until the C vat is listening.
# /proc/net/tcp state 0A is LISTEN; the local port is the hex field.
# Another process can take the port between the check and the bind; a
# server that exits before listening is started again on a new port.
tries=0
while [ "$tries" -lt 5 ]; do
	tries=$((tries + 1))
	PORT=$(pick_port)
	"$SERVER" "$PORT" &
	SPID=$!
	HEXPORT=$(printf '%04X' "$PORT")
	for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
		if awk -v p=":$HEXPORT" 'NR>1 && index($2, p) && $4=="0A" { found=1 } END { exit !found }' /proc/net/tcp; then
			break 2
		fi
		kill -0 "$SPID" 2>/dev/null || break
		sleep 0.05
	done
	if kill -0 "$SPID" 2>/dev/null; then
		break
	fi
	wait "$SPID" 2>/dev/null
done

"$CLIENT" client "$PORT"
RC=$?
exit $RC
