#!/usr/bin/env bash
# Orchestrates the RPC interop tests.
# Usage: run_rpc_interop.sh <cpp-server> <c-client> [c-server] [cpp-client]
set -u
SERVER=$1
CLIENT=$2
PORT=$((RANDOM % 20000 + 30000))
SPID2=

# Wait until pid is listening on port. A connect would be accepted by a
# server that takes one client, so look at the listen table instead.
# Fifty tries of 50ms is 2.5s, then the run fails instead of hanging.
wait_for_listen() {
	port=$1
	pid=$2
	hex=$(printf '%04X' "$port")
	i=0
	while [ "$i" -lt 50 ]; do
		if ! kill -0 "$pid" 2>/dev/null; then
			echo "server $pid exited before listening on $port" >&2
			return 1
		fi
		if awk -v h="$hex" '$2 ~ ":" h "$" && $4 == "0A" { found = 1 } END { exit !found }' \
			/proc/net/tcp /proc/net/tcp6 2>/dev/null; then
			return 0
		fi
		i=$((i + 1))
		sleep 0.05
	done
	echo "timed out waiting for a listener on $port" >&2
	return 1
}

"$SERVER" "$PORT" &
SPID=$!
cleanup() {
	kill "$SPID" 2>/dev/null
	if [ -n "$SPID2" ]; then
		kill "$SPID2" 2>/dev/null
	fi
}
trap cleanup EXIT
wait_for_listen "$PORT" "$SPID" || exit 1

"$CLIENT" "$PORT"
RC=$?
kill "$SPID" 2>/dev/null
wait "$SPID" 2>/dev/null || true
if [ "$RC" -ne 0 ]; then
	exit "$RC"
fi

if [ "$#" -ge 4 ]; then
	PORT2=$((RANDOM % 20000 + 30000))
	"$3" "$PORT2" &
	SPID2=$!
	wait_for_listen "$PORT2" "$SPID2" || exit 1
	"$4" "$PORT2"
	RC=$?
fi
exit "$RC"
