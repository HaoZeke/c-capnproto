#ifndef EMPTY_LIST_H
#define EMPTY_LIST_H

#include <stdint.h>

typedef struct {
  char **names;
  int n_names;
  uint32_t tag;
} empty_list_then_field_t;

#endif
