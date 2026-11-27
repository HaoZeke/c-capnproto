#!/usr/bin/env bash
# C vat serves Adder; the capnp-C++ client pipelines through its promise.
# Usage: run_rpc_promise.sh <c-server> <cpp-peer>
set -u
SERVER=$1
CLIENT=$2
SPID=
trap '[ -n "$SPID" ] && kill "$SPID" 2>/dev/null' EXIT
# shellcheck source=listen_table.sh
. "$(dirname "$0")/listen_table.sh"

# `EXE_WRAPPER`, or the `MESON_EXE_WRAPPER` meson sets on a cross
# build, runs a peer whose ELF machine is s390x (EM_S390, 22, at byte 18).
EXE_WRAPPER=${EXE_WRAPPER:-${MESON_EXE_WRAPPER:-}}
is_s390() {
	[ "$(od -An -j18 -N2 -tx1 -- "$1" 2>/dev/null | tr -d ' \n')" = 0016 ]
}
run_peer() {
	if [ -n "$EXE_WRAPPER" ] && is_s390 "$1"; then
		# The wrapper can be several words, e.g. `env VAR=... qemu-s390x`.
		set -f
		# shellcheck disable=SC2086
		set -- $EXE_WRAPPER "$@"
		set +f
		"$@"
	else
		"$@"
	fi
}

# A random port that no socket in the tables uses yet.
pick_port() {
	while :; do
		p=$((RANDOM % 20000 + 30000))
		if ! port_in_use "$p"; then
			echo "$p"
			return
		fi
	done
}

# The C++ client connects once. Wait until the C vat is listening.
# On Linux, state 0A in /proc/net/tcp is LISTEN. macOS and BSD use
# netstat instead. Another process can take the port between the check
# and the bind; a server that exits before listening is started again
# on a new port. A guest server under qemu-user takes longer to start,
# so wait 2.5s.
tries=0
while [ "$tries" -lt 5 ]; do
	tries=$((tries + 1))
	PORT=$(pick_port)
	run_peer "$SERVER" "$PORT" &
	SPID=$!
	i=0
	while [ "$i" -lt 50 ]; do
		i=$((i + 1))
		if port_listening "$PORT"; then
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

run_peer "$CLIENT" client "$PORT"
RC=$?
exit $RC
