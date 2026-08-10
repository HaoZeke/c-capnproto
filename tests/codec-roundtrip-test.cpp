/* codec-roundtrip-test.cpp
 *
 * Encode/decode/free round-trip for $C.codecgen.
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
#include "codec-roundtrip.h"
#include "codec-roundtrip.capnp.h"
}

#ifndef STRING_DUP
#error "generated header must define STRING_DUP"
#endif

TEST(CodecRoundTrip, EncodeWriteSerializeReadDecodeFree) {
  item_t src;
  memset(&src, 0, sizeof(src));
  src.name = (char *)"alpha";
  src.n = 42;

  uint8_t buf[4096];
  ssize_t sz = 0;

  {
    struct capn c;
    capn_init_malloc(&c);
    struct capn_segment *cs = capn_root(&c).seg;

    struct Item wire;
    memset(&wire, 0, sizeof(wire));
    encode_Item(cs, &wire, &src);

    Item_ptr ptr = new_Item(cs);
    write_Item(&wire, ptr);
    ASSERT_EQ(0, capn_setp(capn_root(&c), 0, ptr.p));
    sz = capn_write_mem(&c, buf, sizeof(buf), 0);
    ASSERT_GT(sz, 0);
    capn_free(&c);
  }

  {
    struct capn rc;
    ASSERT_EQ(0, capn_init_mem(&rc, buf, (size_t)sz, 0));
    Item_ptr rptr;
    rptr.p = capn_getp(capn_root(&rc), 0, 1);

    struct Item wire;
    memset(&wire, 0, sizeof(wire));
    read_Item(&wire, rptr);

    item_t dst;
    memset(&dst, 0, sizeof(dst));
    decode_Item(&dst, &wire);

    EXPECT_STREQ("alpha", dst.name);
    EXPECT_EQ(42, dst.n);

    free_Item(&dst);
    capn_free(&rc);
  }
}

TEST(CodecRoundTrip, PtrHelpersAndNullText) {
  item_t src;
  memset(&src, 0, sizeof(src));
  src.name = NULL;
  src.n = -7;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  Item_ptr ptr;
  encode_Item_ptr(cs, &ptr, &src);
  ASSERT_EQ(0, capn_setp(capn_root(&c), 0, ptr.p));

  int64_t sz = capn_size(&c);
  ASSERT_GT(sz, 0);
  uint8_t *buf = (uint8_t *)malloc((size_t)sz);
  ASSERT_NE(buf, nullptr);
  ASSERT_EQ(sz, capn_write_mem(&c, buf, sz, 0));
  capn_free(&c);

  struct capn c2;
  ASSERT_EQ(0, capn_init_mem(&c2, buf, (size_t)sz, 0));
  Item_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);

  item_t *dst = NULL;
  decode_Item_ptr(&dst, ptr2);
  ASSERT_NE(dst, nullptr);
  ASSERT_NE(dst->name, nullptr);
  EXPECT_STREQ("", dst->name);
  EXPECT_EQ(-7, dst->n);

  free_Item_ptr(&dst);
  EXPECT_EQ(dst, nullptr);

  /* NULL user struct encodes as capn_null; decode yields NULL. */
  encode_Item_ptr(capn_root(&c2).seg, &ptr2, NULL);
  item_t *nil = (item_t *)0x1;
  decode_Item_ptr(&nil, ptr2);
  EXPECT_EQ(nil, nullptr);

  capn_free(&c2);
  free(buf);
}
