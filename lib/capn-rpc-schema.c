/* SPDX-License-Identifier: MIT */
/* Keep generated RPC wire-schema accessors visible under hidden builds. */
#if !defined(CAPNP_C_STATIC) && (defined(__GNUC__) || defined(__clang__)) \
	&& !defined(_WIN32)
#pragma GCC visibility push(default)
#endif
#include "rpc.capnp.c"
#if !defined(CAPNP_C_STATIC) && (defined(__GNUC__) || defined(__clang__)) \
	&& !defined(_WIN32)
#pragma GCC visibility pop
#endif
