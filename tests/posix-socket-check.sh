#!/bin/sh
# The interop sources compile without INADDR_LOOPBACK and MSG_DONTWAIT.
set -eu
CC=$1
ROOT=$2
for src in interop/frame_wrap_test.c interop/rpc_server.c; do
  "$CC" -std=c99 -D_POSIX_C_SOURCE=200809L -fsyntax-only \
    -include "$ROOT/tests/no-bsd-socket.h" \
    -I"$ROOT/interop" -I"$ROOT/lib" -I"$ROOT/compiler" "$ROOT/$src"
done
