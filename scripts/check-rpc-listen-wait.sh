#!/bin/sh
# The C++ client connects once. If the server is not listening yet the
# connection is refused. This drives the interop runner with a server
# that binds late, and with one that never binds, and requires the
# runner to wait a bounded time.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
py=python3
dir=$(mktemp -d)
trap 'rm -rf "$dir"' EXIT

cat > "$dir/server.py" <<'PY'
import socket, sys, time
time.sleep(float(sys.argv[2]))
s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", int(sys.argv[1])))
s.listen(1)
c, _ = s.accept()
c.sendall(b"ok")
c.close()
s.close()
PY

cat > "$dir/client.py" <<'PY'
import socket, sys
s = socket.create_connection(("127.0.0.1", int(sys.argv[1])), 2)
data = s.recv(8)
s.close()
sys.exit(0 if data == b"ok" else 1)
PY

cat > "$dir/fast-server" <<EOF
#!/bin/sh
exec $py "$dir/server.py" "\$1" 0
EOF
cat > "$dir/slow-server" <<EOF
#!/bin/sh
exec $py "$dir/server.py" "\$1" 0.4
EOF
# Exits on its first start, as a server whose bind lost a port race.
cat > "$dir/busy-once-server" <<EOF
#!/bin/sh
if [ ! -e "$dir/busy-once" ]; then
	: > "$dir/busy-once"
	exit 1
fi
exec $py "$dir/server.py" "\$1" 0
EOF
cat > "$dir/never-server" <<'EOF'
#!/bin/sh
exec sleep 30
EOF
cat > "$dir/client" <<EOF
#!/bin/sh
exec $py "$dir/client.py" "\$1"
EOF
chmod +x "$dir/fast-server" "$dir/slow-server" "$dir/busy-once-server" "$dir/never-server" "$dir/client"

bash "$root/interop/run_rpc_interop.sh" \
	"$dir/fast-server" "$dir/client" \
	"$dir/slow-server" "$dir/client"

bash "$root/interop/run_rpc_interop.sh" \
	"$dir/busy-once-server" "$dir/client"

start=$(date +%s)
if bash "$root/interop/run_rpc_interop.sh" \
	"$dir/fast-server" "$dir/client" \
	"$dir/never-server" "$dir/client"; then
	echo "a server that never listens must fail" >&2
	exit 1
fi
elapsed=$(( $(date +%s) - start ))
if [ "$elapsed" -gt 10 ]; then
	echo "listen wait ran for ${elapsed}s, which is not bounded" >&2
	exit 1
fi
