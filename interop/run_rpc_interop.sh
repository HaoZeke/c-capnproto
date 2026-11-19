#!/usr/bin/env bash
# Orchestrates the RPC interop tests.
# Usage: run_rpc_interop.sh <cpp-server> <c-client> [c-server] [cpp-client]
set -u
SERVER=$1
CLIENT=$2
PORT=$((RANDOM % 20000 + 30000))
SPID2=

"$SERVER" "$PORT" &
SPID=$!
cleanup() {
	kill "$SPID" 2>/dev/null
	if [ -n "$SPID2" ]; then
		kill "$SPID2" 2>/dev/null
	fi
}
trap cleanup EXIT

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
	"$4" "$PORT2"
	RC=$?
fi
exit "$RC"
