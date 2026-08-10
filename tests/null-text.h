#ifndef NULL_TEXT_H
#define NULL_TEXT_H

#include <stdint.h>

typedef struct {
  uint32_t n;
} kid_t;

typedef struct {
  char *note;
  kid_t *child;
} wrap_t;

#endif
