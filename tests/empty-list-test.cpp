/* empty-list-test.cpp
 *
 * Codecgen regression: decoding List(Text) of length 0 must not return from
 * the whole struct decoder. A later UInt32 field (tag) must still be filled.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "capnp_c.h"
#include "empty-list.capnp.h"
#include "empty-list.h"
}

TEST(EmptyList, EmptyNamesAndTagSurviveDecode) {
  empty_list_then_field_t src;
  memset(&src, 0, sizeof(src));
  src.names = NULL;
  src.n_names = 0;
  src.tag = 0xA11u;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  EmptyListThenField_ptr ptr;
  encode_EmptyListThenField_ptr(cs, &ptr, &src);
  capn_setp(capn_root(&c), 0, ptr.p);

  int64_t sz = capn_size(&c);
  ASSERT_GT(sz, 0);
  uint8_t *buf = (uint8_t *)malloc((size_t)sz);
  ASSERT_NE(buf, nullptr);
  int64_t written = capn_write_mem(&c, buf, (size_t)sz, 0);
  ASSERT_EQ(written, sz);
  capn_free(&c);

  struct capn c2;
  int rc = capn_init_mem(&c2, buf, (size_t)sz, 0);
  ASSERT_EQ(rc, 0);

  EmptyListThenField_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);

  empty_list_then_field_t *dst = NULL;
  decode_EmptyListThenField_ptr(&dst, ptr2);
  ASSERT_NE(dst, nullptr);
  EXPECT_EQ(dst->n_names, 0);
  EXPECT_EQ(dst->names, nullptr);
  EXPECT_EQ(dst->tag, 0xA11u);

  free_EmptyListThenField_ptr(&dst);
  EXPECT_EQ(dst, nullptr);

  capn_free(&c2);
  free(buf);
}
