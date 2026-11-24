/* SPDX-License-Identifier: MIT */
/* Keep generated RPC wire-schema accessors visible under hidden builds. */
#if !defined(CAPNP_C_STATIC) && (defined(_WIN32) || defined(__CYGWIN__))
#define CAPN_SCHEMA_EXPORT __declspec(dllexport)
#if defined(__GNUC__)
/* It also lands on the struct tags, where GCC ignores it with a warning. */
#pragma GCC diagnostic ignored "-Wattributes"
#endif
#endif
#if !defined(CAPNP_C_STATIC) && (defined(__GNUC__) || defined(__clang__)) \
	&& !defined(_WIN32)
#pragma GCC visibility push(default)
#endif
#include "rpc.capnp.c"
#if !defined(CAPNP_C_STATIC) && (defined(__GNUC__) || defined(__clang__)) \
	&& !defined(_WIN32)
#pragma GCC visibility pop
#endif
