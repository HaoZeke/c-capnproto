#!/usr/bin/env bash
# run_rpc_interop.sh must hand an s390x peer to EXE_WRAPPER even when
# `file` is missing. The peers here are bare s390x ELF headers that
# only the wrapper can run. Usage: interop-wrapper-check.sh ROOT
set -eu
root=$1
dir=$(mktemp -d)
trap 'rm -rf "$dir"' EXIT

for peer in server client; do
	printf '\177ELF\002\002\001\000\000\000\000\000\000\000\000\000\000\002\000\026' \
		>"$dir/$peer"
	chmod +x "$dir/$peer"
done

cat >"$dir/wrap" <<'EOF'
#!/bin/sh
case "$1" in
*server) exec python3 -c 'import socket, sys
s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("127.0.0.1", int(sys.argv[1])))
s.listen(1)
s.accept()[0].close()' "$2" ;;
*) exec python3 -c 'import socket, sys
socket.create_connection(("127.0.0.1", int(sys.argv[1]))).close()' "$2" ;;
esac
EOF
chmod +x "$dir/wrap"

mkdir "$dir/bin"
printf '#!/bin/sh\nexit 127\n' >"$dir/bin/file"
chmod +x "$dir/bin/file"

PATH="$dir/bin:$PATH" EXE_WRAPPER="$dir/wrap" \
	bash "$root/interop/run_rpc_interop.sh" "$dir/server" "$dir/client"

# Meson passes a cross file's `exe_wrapper` list as one string of words.
PATH="$dir/bin:$PATH" EXE_WRAPPER="env WRAP_ARGS=x $dir/wrap" \
	bash "$root/interop/run_rpc_interop.sh" "$dir/server" "$dir/client"
