/* list-decode-srcdest-test.cpp
 *
 * Codecgen List(struct) round-trip where the dest C field name
 * (chapters_) differs from the schema field (chapters). A src/dest
 * swap in gen_call_list_decoder fails to compile this target.
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "capnp_c.h"
#include "list-decode-srcdest.h"
#include "list-decode-srcdest.capnp.h"
}

static void roundtrip(const book_t *src, book_t **dst) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  Book_ptr ptr;
  encode_Book_ptr(cs, &ptr, (book_t *)src);
  ASSERT_EQ(0, capn_setp(capn_root(&c), 0, ptr.p));

  int64_t sz = capn_size(&c);
  ASSERT_GT(sz, 0);
  uint8_t *buf = (uint8_t *)malloc((size_t)sz);
  ASSERT_NE(buf, nullptr);
  ASSERT_EQ(sz, capn_write_mem(&c, buf, sz, 0));
  capn_free(&c);

  struct capn c2;
  ASSERT_EQ(0, capn_init_mem(&c2, buf, (size_t)sz, 0));
  Book_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);
  decode_Book_ptr(dst, ptr2);
  capn_free(&c2);
  free(buf);
}

TEST(ListDecodeSrcDest, EncodeDecodeTwoDistinctLines) {
  line_t a;
  memset(&a, 0, sizeof(a));
  a.caption = (char *)"alpha";
  a.n = 11;

  line_t b;
  memset(&b, 0, sizeof(b));
  b.caption = (char *)"beta";
  b.n = 22;

  line_t *items[2] = {&a, &b};

  book_t src;
  memset(&src, 0, sizeof(src));
  src.n_chapters = 2;
  src.chapters_ = items;
  src.tag = 0xA11u;

  book_t *dst = NULL;
  roundtrip(&src, &dst);
  ASSERT_NE(dst, nullptr);
  EXPECT_EQ(2, dst->n_chapters);
  ASSERT_NE(dst->chapters_, nullptr);
  ASSERT_NE(dst->chapters_[0], nullptr);
  ASSERT_NE(dst->chapters_[1], nullptr);
  ASSERT_NE(dst->chapters_[0]->caption, nullptr);
  ASSERT_NE(dst->chapters_[1]->caption, nullptr);
  EXPECT_STREQ("alpha", dst->chapters_[0]->caption);
  EXPECT_EQ(11, dst->chapters_[0]->n);
  EXPECT_STREQ("beta", dst->chapters_[1]->caption);
  EXPECT_EQ(22, dst->chapters_[1]->n);
  EXPECT_EQ(0xA11u, dst->tag);

  free_Book_ptr(&dst);
  EXPECT_EQ(dst, nullptr);
}

TEST(ListDecodeSrcDest, EmptyListPreservesTag) {
  book_t src;
  memset(&src, 0, sizeof(src));
  src.n_chapters = 0;
  src.chapters_ = NULL;
  src.tag = 0xB0B0u;

  book_t *dst = NULL;
  roundtrip(&src, &dst);
  ASSERT_NE(dst, nullptr);
  EXPECT_EQ(0, dst->n_chapters);
  EXPECT_EQ(dst->chapters_, nullptr);
  EXPECT_EQ(0xB0B0u, dst->tag);

  free_Book_ptr(&dst);
  EXPECT_EQ(dst, nullptr);
}
