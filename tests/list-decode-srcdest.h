#ifndef LIST_DECODE_SRCDEST_H
#define LIST_DECODE_SRCDEST_H

#include <stdint.h>

typedef struct {
  char *caption;
  int32_t n;
} line_t;

typedef struct {
  int n_chapters;
  line_t **chapters_;
  uint32_t tag;
} book_t;

#endif
