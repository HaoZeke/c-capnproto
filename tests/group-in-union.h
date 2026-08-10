#ifndef GROUP_IN_UNION_H
#define GROUP_IN_UNION_H

#include <stdint.h>

typedef struct {
  int kind;
  union {
    struct {
      int32_t x;
      int64_t y;
    } foo;
    struct {
      char *name;
      uint32_t value;
    } bar;
    char *baz;
  } data;
} group_in_union_t;

#endif
