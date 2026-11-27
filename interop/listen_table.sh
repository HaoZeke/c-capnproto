# Sourced by the RPC runners. Not executed on its own.
#
# A connect would be accepted by a server that takes one client, so the
# runners read the listen table instead. Linux keeps the /proc/net/tcp
# check. macOS and BSD have no such files; their netstat(1) (the socket
# form, not Linux -ltn) prints "host.port" and a LISTEN state.

listen_table_proc() {
	[ -r /proc/net/tcp ] || [ -r /proc/net/tcp6 ]
}

proc_port_used() {
	h=$(printf '%04X' "$1")
	awk -v h="$h" '$2 ~ ":" h "$" { found = 1 } END { exit !found }' \
		/proc/net/tcp /proc/net/tcp6 2>/dev/null
}

proc_port_listening() {
	h=$(printf '%04X' "$1")
	awk -v h="$h" '$2 ~ ":" h "$" && $4 == "0A" { found = 1 } END { exit !found }' \
		/proc/net/tcp /proc/net/tcp6 2>/dev/null
}

# First synopsis of macOS/BSD netstat(1): active sockets.
# -a shows LISTEN (hidden by default). -n keeps the port numeric.
# -p tcp selects the protocol. It is not Linux netstat -p, which names
# a program. The local address is field 4, "host.port"; the port is the
# component after the last dot (127.0.0.1.8080, *.8080, ::1.8080).
bsd_netstat() {
	if command -v netstat >/dev/null 2>&1; then
		LC_ALL=C netstat -an -p tcp 2>/dev/null
		return
	fi
	if [ -x /usr/sbin/netstat ]; then
		LC_ALL=C /usr/sbin/netstat -an -p tcp 2>/dev/null
		return
	fi
	return 1
}

# Rows on stdin. want=0: any TCP state. want=1: state word LISTEN.
# Extra columns after LISTEN (-v) are ignored. $NF is not the state then.
bsd_rows_match() {
	awk -v p="$1" -v want="$2" '
$1 ~ /^tcp/ {
	n = split($4, a, ".")
	if (n < 1 || a[n] != p)
		next
	if (want == 0) {
		found = 1
		next
	}
	for (i = 5; i <= NF; i++)
		if ($i == "LISTEN")
			found = 1
}
END { exit !found }
'
}

bsd_port_used() {
	bsd_netstat | bsd_rows_match "$1" 0
}

bsd_port_listening() {
	bsd_netstat | bsd_rows_match "$1" 1
}

port_in_use() {
	if listen_table_proc; then
		proc_port_used "$1"
	else
		bsd_port_used "$1"
	fi
}

port_listening() {
	if listen_table_proc; then
		proc_port_listening "$1"
	else
		bsd_port_listening "$1"
	fi
}
