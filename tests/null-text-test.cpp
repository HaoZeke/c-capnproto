/* null-text-test.cpp
 *
 * Codecgen regression: a NULL Text pointer must encode without SEGV (no
 * strlen(NULL)) and decode as "". A NULL nested struct pointer must encode
 * as CAPN_NULL (not a zero-filled object) and decode as NULL.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "capnp_c.h"
#include "null-text.capnp.h"
#include "null-text.h"
}

TEST(NullText, NullNoteEncodesWithoutSegvAndDecodesEmpty) {
  wrap_t src;
  memset(&src, 0, sizeof(src));
  src.note = NULL;
  src.child = NULL;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  Wrap_ptr ptr;
  encode_Wrap_ptr(cs, &ptr, &src);
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

  Wrap_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);

  wrap_t *dst = NULL;
  decode_Wrap_ptr(&dst, ptr2);
  ASSERT_NE(dst, nullptr);
  ASSERT_NE(dst->note, nullptr);
  EXPECT_STREQ(dst->note, "");

  free_Wrap_ptr(&dst);
  EXPECT_EQ(dst, nullptr);

  capn_free(&c2);
  free(buf);
}

TEST(NullText, NullChildEncodesAsCapnNullAndDecodesNull) {
  wrap_t src;
  memset(&src, 0, sizeof(src));
  src.note = (char *)"ok";
  src.child = NULL;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  Wrap_ptr ptr;
  encode_Wrap_ptr(cs, &ptr, &src);
  capn_setp(capn_root(&c), 0, ptr.p);

  struct Wrap encoded;
  memset(&encoded, 0, sizeof(encoded));
  read_Wrap(&encoded, ptr);
  capn_resolve(&encoded.child.p);
  EXPECT_EQ(encoded.child.p.type, CAPN_NULL);

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

  Wrap_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);

  wrap_t *dst = NULL;
  decode_Wrap_ptr(&dst, ptr2);
  ASSERT_NE(dst, nullptr);
  EXPECT_STREQ(dst->note, "ok");
  EXPECT_EQ(dst->child, nullptr);

  free_Wrap_ptr(&dst);
  EXPECT_EQ(dst, nullptr);

  capn_free(&c2);
  free(buf);
}
