#!/usr/bin/env bash
# Orchestrates the RPC interop tests.
# Usage: run_rpc_interop.sh <cpp-server> <c-client> [c-server] [cpp-client]
set -u
SERVER=$1
CLIENT=$2
SPID=
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

# `EXE_WRAPPER`, or the `MESON_EXE_WRAPPER` meson sets on a cross
# build, runs a peer whose ELF machine is s390x.
EXE_WRAPPER=${EXE_WRAPPER:-${MESON_EXE_WRAPPER:-}}
# The wrapper can be several words, e.g. `env VAR=... qemu-s390x`.
run_wrapped() {
	set -f
	# shellcheck disable=SC2086
	set -- $EXE_WRAPPER "$@"
	set +f
	"$@"
}
wrapped() {
	if [ -n "${EXE_WRAPPER:-}" ] && file -b -- "$1" 2>/dev/null | grep -q 'S/390'; then
		run_wrapped "$@"
	else
		"$@"
	fi
}

start_peer() {
	if [ -n "${EXE_WRAPPER:-}" ] && file -b -- "$1" 2>/dev/null | grep -q 'S/390'; then
		(
			set -f
			# shellcheck disable=SC2086
			set -- $EXE_WRAPPER "$@"
			set +f
			exec "$@"
		) &
	else
		"$@" &
	fi
	START_PID=$!
}

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

# Start server $1 on a free port and wait for its listener. Sets
# LISTEN_PORT and LISTEN_PID. Another process can take the port between
# the check and the bind; a server that exits before listening is
# started again on a new port, up to five times.
start_listening() {
	tries=0
	while [ "$tries" -lt 5 ]; do
		tries=$((tries + 1))
		LISTEN_PORT=$(pick_port)
		start_peer "$1" "$LISTEN_PORT"
		LISTEN_PID=$START_PID
		if wait_for_listen "$LISTEN_PORT" "$LISTEN_PID"; then
			return 0
		fi
		if kill -0 "$LISTEN_PID" 2>/dev/null; then
			return 1
		fi
		wait "$LISTEN_PID" 2>/dev/null
	done
	return 1
}

cleanup() {
	if [ -n "$SPID" ]; then
		kill "$SPID" 2>/dev/null
	fi
	if [ -n "$SPID2" ]; then
		kill "$SPID2" 2>/dev/null
	fi
}
trap cleanup EXIT
start_listening "$SERVER"
rc=$?
SPID=$LISTEN_PID
PORT=$LISTEN_PORT
[ "$rc" -eq 0 ] || exit 1

wrapped "$CLIENT" "$PORT"
RC=$?
kill "$SPID" 2>/dev/null
wait "$SPID" 2>/dev/null || true
if [ "$RC" -ne 0 ]; then
	exit "$RC"
fi

if [ "$#" -ge 4 ]; then
	start_listening "$3"
	rc=$?
	SPID2=$LISTEN_PID
	PORT2=$LISTEN_PORT
	[ "$rc" -eq 0 ] || exit 1
	wrapped "$4" "$PORT2"
	RC=$?
fi
exit "$RC"
