#ifndef DATA_CODEC_H
#define DATA_CODEC_H

#include <stdint.h>
#include "capnp_c.h"

typedef struct {
  uint8_t *payload;
  int payload_len;
  char *name;
  int n_chunks;
  capnp_data_t *chunks;
  uint32_t tag;
} data_message_t;

#endif
