/* brand-test.cpp
 *
 * Typed accessors for generic instantiations. The unbound generic
 * stays a raw pointer. A brand becomes its own struct.
 */

#include "capnp-cli.h"

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

/* Holder(T).Inner is branded with Scope.inherit. Holder(Text) must
 * name that inner struct with T already replaced. */
TEST(Brand, InheritUsesTheEnclosingParameter) {
  struct capn ctx;
  struct capn_segment *seg;
  capn_init_malloc(&ctx);
  seg = capn_root(&ctx).seg;

  Holder_Inner_Text_ptr inner = new_Holder_Inner_Text(seg);
  struct Holder_Inner_Text iv;
  memset(&iv, 0, sizeof iv);
  iv.value.len = 3;
  iv.value.str = "yes";
  write_Holder_Inner_Text(&iv, inner);

  Holder_Text_ptr hp = new_Holder_Text(seg);
  struct Holder_Text holder;
  memset(&holder, 0, sizeof holder);
  holder.inherited = inner;
  holder.rebound = inner;
  write_Holder_Text(&holder, hp);

  struct Holder_Text got;
  memset(&got, 0, sizeof got);
  read_Holder_Text(&got, hp);
  struct Holder_Inner_Text from_inherit;
  struct Holder_Inner_Text from_rebind;
  memset(&from_inherit, 0, sizeof from_inherit);
  memset(&from_rebind, 0, sizeof from_rebind);
  read_Holder_Inner_Text(&from_inherit, got.inherited);
  read_Holder_Inner_Text(&from_rebind, got.rebound);
  ASSERT_NE(nullptr, from_inherit.value.str);
  EXPECT_STREQ("yes", from_inherit.value.str);
  EXPECT_STREQ("yes", from_rebind.value.str);

  capn_free(&ctx);
}

/* Trio(Text, Text, BrandLeaf).items is List(BrandLeaf), a struct list on
 * the wire, as C++ List<T> is. */
TEST(Brand, ListOfABoundStructIsAStructList) {
  if (!capnp_present()) {
    GTEST_SKIP() << "capnp CLI not found";
  }
  const std::string schema = join_path(source_root(), "tests/reflect.capnp");
  const char *want = "(leaves = (items = [(x = 7), (x = 9)]))";

  struct capn ctx;
  capn_init_malloc(&ctx);
  capn_ptr root = capn_root(&ctx);
  struct capn_segment *seg = root.seg;
  struct Trio_Text_Text_BrandLeaf trio;
  memset(&trio, 0, sizeof trio);
  trio.items = new_BrandLeaf_list(seg, 2);
  struct BrandLeaf leaf;
  leaf.x = 7;
  set_BrandLeaf(&leaf, trio.items, 0);
  leaf.x = 9;
  set_BrandLeaf(&leaf, trio.items, 1);
  Trio_Text_Text_BrandLeaf_ptr tp = new_Trio_Text_Text_BrandLeaf(seg);
  write_Trio_Text_Text_BrandLeaf(&trio, tp);
  struct UsesBrandLists uses;
  memset(&uses, 0, sizeof uses);
  uses.leaves = tp;
  UsesBrandLists_ptr up = new_UsesBrandLists(seg);
  write_UsesBrandLists(&uses, up);
  ASSERT_EQ(0, capn_setp(root, 0, up.p));
  uint8_t buf[4096];
  ssize_t sz = capn_write_mem(&ctx, buf, sizeof buf, 0);
  ASSERT_GT(sz, 0);
  capn_free(&ctx);

  std::vector<std::string> argv = capnp_cmd("decode", schema, "UsesBrandLists");
  argv.insert(argv.begin() + 2, "--short");
  CmdResult dec = run_argv(argv, buf, (size_t) sz);
  ASSERT_EQ(0, dec.status) << dec.err;
  while (!dec.out.empty() && dec.out.back() == '\n')
    dec.out.pop_back();
  EXPECT_EQ(want, dec.out);

  CmdResult enc = capnp_encode_text(schema, "UsesBrandLists", want);
  ASSERT_EQ(0, enc.status) << enc.err;
  struct capn msg;
  ASSERT_EQ(0, capn_init_mem(&msg, (const uint8_t *) enc.out.data(),
                             enc.out.size(), 0));
  UsesBrandLists_ptr rp;
  rp.p = capn_getp(capn_root(&msg), 0, 1);
  struct UsesBrandLists ru;
  read_UsesBrandLists(&ru, rp);
  struct Trio_Text_Text_BrandLeaf rt;
  read_Trio_Text_Text_BrandLeaf(&rt, ru.leaves);
  ASSERT_EQ(2, capn_len(rt.items));
  struct BrandLeaf got;
  get_BrandLeaf(&got, rt.items, 0);
  EXPECT_EQ(7u, got.x);
  get_BrandLeaf(&got, rt.items, 1);
  EXPECT_EQ(9u, got.x);
  capn_free(&msg);
}

/* Grouped(Text) is emitted from inside the unbound Grouped.g. Its
 * binding must not retype the unbound group, whose v stays a pointer. */
TEST(Brand, GroupsAreTypedPerInstantiation) {
  struct capn ctx;
  capn_init_malloc(&ctx);
  struct capn_segment *seg = capn_root(&ctx).seg;

  BrandLeaf_ptr lp = new_BrandLeaf(seg);
  struct BrandLeaf leaf;
  leaf.x = 5;
  write_BrandLeaf(&leaf, lp);

  struct Grouped any;
  memset(&any, 0, sizeof any);
  any.g.v = lp.p;
  any.g.u_which = Grouped_g_u_some;
  any.g.u.some = lp.p;
  Grouped_ptr ap = new_Grouped(seg);
  write_Grouped(&any, ap);
  struct Grouped any_got;
  memset(&any_got, 0, sizeof any_got);
  read_Grouped(&any_got, ap);
  struct BrandLeaf from_any;
  BrandLeaf_ptr fp;
  fp.p = any_got.g.v;
  read_BrandLeaf(&from_any, fp);
  EXPECT_EQ(5u, from_any.x);
  ASSERT_EQ(Grouped_g_u_some, any_got.g.u_which);

  struct Grouped_Text text;
  memset(&text, 0, sizeof text);
  text.g.v.str = "v";
  text.g.v.len = 1;
  text.g.u_which = Grouped_Text_g_u_some;
  text.g.u.some.str = "s";
  text.g.u.some.len = 1;
  Grouped_Text_ptr tp = new_Grouped_Text(seg);
  write_Grouped_Text(&text, tp);
  struct Grouped_Text text_got;
  memset(&text_got, 0, sizeof text_got);
  read_Grouped_Text(&text_got, tp);
  EXPECT_STREQ("v", text_got.g.v.str);
  ASSERT_EQ(Grouped_Text_g_u_some, text_got.g.u_which);
  EXPECT_STREQ("s", text_got.g.u.some.str);

  struct Grouped_Data data;
  memset(&data, 0, sizeof data);
  data.g.next = tp;
  Grouped_Data_ptr dp = new_Grouped_Data(seg);
  write_Grouped_Data(&data, dp);
  struct Grouped_Data data_got;
  memset(&data_got, 0, sizeof data_got);
  read_Grouped_Data(&data_got, dp);
  read_Grouped_Text(&text_got, data_got.g.next);
  EXPECT_STREQ("v", text_got.g.v.str);

  capn_free(&ctx);
}
