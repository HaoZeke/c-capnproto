#!/usr/bin/env bash
# C vat serves Adder; the capnp-C++ client pipelines through its promise.
# Usage: run_rpc_promise.sh <c-server> <cpp-peer>
set -u
SERVER=$1
CLIENT=$2
PORT=$((RANDOM % 20000 + 30000))

"$SERVER" "$PORT" &
SPID=$!
trap 'kill $SPID 2>/dev/null' EXIT

# The C++ client connects once. Wait until the C vat is listening.
# /proc/net/tcp state 0A is LISTEN; the local port is the hex field.
HEXPORT=$(printf '%04X' "$PORT")
for _ in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20; do
	if awk -v p=":$HEXPORT" 'NR>1 && index($2, p) && $4=="0A" { found=1 } END { exit !found }' /proc/net/tcp; then
		break
	fi
	sleep 0.05
done

"$CLIENT" client "$PORT"
RC=$?
exit $RC
