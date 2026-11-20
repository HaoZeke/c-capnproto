/* reflectgen.h - descriptor tables for $C.reflect (see reflectgen.c) */

#ifndef CAPNPC_REFLECTGEN_H
#define CAPNPC_REFLECTGEN_H

#include "codecgen.h"

#define ANNOTATION_REFLECT 0xd0e1c2a3b4f50617UL

int reflect_file_enabled(struct node *file);
void declare_reflect(capnp_ctx_t *ctx, struct node *file_node);
void define_reflect(capnp_ctx_t *ctx, struct node *file_node);

#endif /* CAPNPC_REFLECTGEN_H */
