/* brand-test.cpp
 *
 * Typed accessors for generic instantiations. The unbound generic
 * stays a raw pointer. A brand becomes its own struct.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "reflect.capnp.h"
}

TEST(Brand, TypedAccessorsRoundTrip) {
  struct capn ctx;
  struct capn_segment *seg;
  capn_init_malloc(&ctx);
  seg = capn_root(&ctx).seg;

  Box_Text_ptr tb = new_Box_Text(seg);
  struct Box_Text box;
  memset(&box, 0, sizeof box);
  box.val.len = 2;
  box.val.str = "hi";
  box.count = 3;
  write_Box_Text(&box, tb);

  struct Box_Text got;
  memset(&got, 0, sizeof got);
  read_Box_Text(&got, tb);
  EXPECT_EQ(3u, got.count);
  ASSERT_NE(nullptr, got.val.str);
  EXPECT_STREQ("hi", got.val.str);

  Box_Text_list lst = new_Box_Text_list(seg, 1);
  set_Box_Text(&box, lst, 0);
  struct Box_Text from_list;
  memset(&from_list, 0, sizeof from_list);
  get_Box_Text(&from_list, lst, 0);
  EXPECT_STREQ("hi", from_list.val.str);
  EXPECT_EQ(3u, from_list.count);

  Box_Box_Text_ptr nested = new_Box_Box_Text(seg);
  struct Box_Box_Text outer;
  memset(&outer, 0, sizeof outer);
  outer.val = tb;
  outer.count = 1;
  write_Box_Box_Text(&outer, nested);
  struct Box_Box_Text og;
  memset(&og, 0, sizeof og);
  read_Box_Box_Text(&og, nested);
  struct Box_Text inner;
  memset(&inner, 0, sizeof inner);
  read_Box_Text(&inner, og.val);
  EXPECT_EQ(1u, og.count);
  ASSERT_NE(nullptr, inner.val.str);
  EXPECT_STREQ("hi", inner.val.str);

  Pair_Text_List_Int32_ptr pp = new_Pair_Text_List_Int32(seg);
  struct Pair_Text_List_Int32 pair;
  memset(&pair, 0, sizeof pair);
  pair.first.len = 1;
  pair.first.str = "f";
  capn_list32 nums = capn_new_list32(seg, 2);
  capn_set32(nums, 0, 1);
  capn_set32(nums, 1, 2);
  pair.second = nums;
  write_Pair_Text_List_Int32(&pair, pp);
  struct Pair_Text_List_Int32 pg;
  memset(&pg, 0, sizeof pg);
  read_Pair_Text_List_Int32(&pg, pp);
  EXPECT_STREQ("f", pg.first.str);
  EXPECT_EQ(2, capn_len(pg.second));
  EXPECT_EQ(1, capn_get32(pg.second, 0));
  EXPECT_EQ(2, capn_get32(pg.second, 1));

  struct Box generic;
  memset(&generic, 0, sizeof generic);
  generic.count = 9;
  Box_ptr gp = new_Box(seg);
  write_Box(&generic, gp);
  struct Box gg;
  memset(&gg, 0, sizeof gg);
  read_Box(&gg, gp);
  EXPECT_EQ(9u, gg.count);

  capn_free(&ctx);
}
