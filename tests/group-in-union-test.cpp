/* group-in-union-test.cpp
 *
 * Encode/decode both group variants of a :group inside a union.
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
#include "group-in-union.h"
#include "group-in-union.capnp.h"
}

static void roundtrip(const group_in_union_t *src, group_in_union_t **dst) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  GroupInUnion_ptr ptr;
  encode_GroupInUnion_ptr(cs, &ptr, (group_in_union_t *)src);
  ASSERT_EQ(0, capn_setp(capn_root(&c), 0, ptr.p));

  int64_t sz = capn_size(&c);
  ASSERT_GT(sz, 0);
  uint8_t *buf = (uint8_t *)malloc((size_t)sz);
  ASSERT_NE(buf, nullptr);
  ASSERT_EQ(sz, capn_write_mem(&c, buf, sz, 0));
  capn_free(&c);

  struct capn c2;
  ASSERT_EQ(0, capn_init_mem(&c2, buf, (size_t)sz, 0));
  GroupInUnion_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);
  decode_GroupInUnion_ptr(dst, ptr2);
  capn_free(&c2);
  free(buf);
}

TEST(GroupInUnion, EncodeDecodeFoo) {
  group_in_union_t src;
  memset(&src, 0, sizeof(src));
  src.kind = GroupInUnion_data_foo;
  src.data.foo.x = 11;
  src.data.foo.y = 22;

  group_in_union_t *dst = NULL;
  roundtrip(&src, &dst);
  ASSERT_NE(dst, nullptr);
  EXPECT_EQ(GroupInUnion_data_foo, dst->kind);
  EXPECT_EQ(11, dst->data.foo.x);
  EXPECT_EQ(22, dst->data.foo.y);
  free_GroupInUnion_ptr(&dst);
  EXPECT_EQ(dst, nullptr);
}

TEST(GroupInUnion, EncodeDecodeBar) {
  group_in_union_t src;
  memset(&src, 0, sizeof(src));
  src.kind = GroupInUnion_data_bar;
  src.data.bar.name = (char *)"beta";
  src.data.bar.value = 99u;

  group_in_union_t *dst = NULL;
  roundtrip(&src, &dst);
  ASSERT_NE(dst, nullptr);
  EXPECT_EQ(GroupInUnion_data_bar, dst->kind);
  ASSERT_NE(dst->data.bar.name, nullptr);
  EXPECT_STREQ("beta", dst->data.bar.name);
  EXPECT_EQ(99u, dst->data.bar.value);
  free_GroupInUnion_ptr(&dst);
}

TEST(GroupInUnion, EncodeDecodeBaz) {
  group_in_union_t src;
  memset(&src, 0, sizeof(src));
  src.kind = GroupInUnion_data_baz;
  src.data.baz = (char *)"gamma";

  group_in_union_t *dst = NULL;
  roundtrip(&src, &dst);
  ASSERT_NE(dst, nullptr);
  EXPECT_EQ(GroupInUnion_data_baz, dst->kind);
  ASSERT_NE(dst->data.baz, nullptr);
  EXPECT_STREQ("gamma", dst->data.baz);
  free_GroupInUnion_ptr(&dst);
}
