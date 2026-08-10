/* list-c6-upgrade-test.cpp
 *
 * Schema List(Struct { t @0 :Text }) must decode a wire C=6 pointer
 * list (0 data words + 1 pointer per element) the same way C++ does.
 * A real List(Text) stays a pointer list: capn_get_text chases.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>

#include "capnp_c.h"
#include "test.capnp.h"

static capn_text chars_to_text(const char *chars) {
  return (capn_text) {
    .len = (int) strlen(chars),
    .str = chars,
    .seg = NULL,
  };
}

#define EXPECT_CAPN_TEXT_EQ(expected, t) \
  do { \
    EXPECT_EQ(strlen((expected)), (uint32_t) (t).len); \
    EXPECT_STREQ((expected), (t).str); \
  } while (0)

/* One-segment frame: segs-1=0, then word count, then payload. */
static int init_one_seg(struct capn *c, const uint8_t *seg, size_t segsz,
                        uint8_t *framed, size_t framed_cap) {
  if (segsz % 8 != 0 || framed_cap < 8 + segsz)
    return -1;
  memset(framed, 0, framed_cap);
  framed[0] = 0;
  framed[4] = (uint8_t) (segsz / 8);
  memcpy(framed + 8, seg, segsz);
  return capn_init_mem(c, framed, 8 + segsz, 0);
}

/* Hand-crafted C=6 list of one Text pointer "hi".
 *
 * word0: list A=1 C=6 D=1 offset=0  -> 0x0000000e00000001
 * word1: list A=1 C=2 D=3 offset=0  -> 0x0000001a00000001  (Text)
 * word2: 'h' 'i' NUL pad
 */
static const uint8_t kC6OneText[] = {
  0x01, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x1a, 0x00, 0x00, 0x00,
  'h', 'i', 0, 0, 0, 0, 0, 0,
};

TEST(ListC6Upgrade, GetStructPSeesText) {
  uint8_t framed[8 + sizeof(kC6OneText)];
  struct capn c;
  ASSERT_EQ(0, init_one_seg(&c, kC6OneText, sizeof(kC6OneText),
                            framed, sizeof(framed)));

  capn_ptr list = capn_getp(capn_root(&c), 0, 1);
  ASSERT_EQ(CAPN_PTR_LIST, list.type);
  ASSERT_EQ(1, list.len);

  TestLists_StructP_list l = {list};
  struct TestLists_StructP s;
  memset(&s, 0, sizeof(s));
  get_TestLists_StructP(&s, l, 0);
  EXPECT_CAPN_TEXT_EQ("hi", s.f);

  capn_free(&c);
}

TEST(ListC6Upgrade, GetpResolve0IsInnerZeroDataOnePtr) {
  uint8_t framed[8 + sizeof(kC6OneText)];
  struct capn c;
  ASSERT_EQ(0, init_one_seg(&c, kC6OneText, sizeof(kC6OneText),
                            framed, sizeof(framed)));

  capn_ptr list = capn_getp(capn_root(&c), 0, 1);
  capn_ptr el = capn_getp(list, 0, 0);
  EXPECT_EQ(CAPN_STRUCT, el.type);
  EXPECT_EQ(1, el.is_list_member);
  EXPECT_EQ(0, el.datasz);
  EXPECT_EQ(1, el.ptrs);
  EXPECT_CAPN_TEXT_EQ("hi", capn_get_text(el, 0, chars_to_text("")));

  capn_free(&c);
}

TEST(ListC6Upgrade, GetTextOnPtrListStillChases) {
  uint8_t framed[8 + sizeof(kC6OneText)];
  struct capn c;
  ASSERT_EQ(0, init_one_seg(&c, kC6OneText, sizeof(kC6OneText),
                            framed, sizeof(framed)));

  capn_ptr list = capn_getp(capn_root(&c), 0, 1);
  ASSERT_EQ(CAPN_PTR_LIST, list.type);
  capn_ptr chased = capn_getp(list, 0, 1);
  EXPECT_EQ(CAPN_LIST, chased.type);
  EXPECT_EQ(1, chased.datasz);
  EXPECT_EQ(3, chased.len);
  EXPECT_CAPN_TEXT_EQ("hi", capn_get_text(list, 0, chars_to_text("")));

  capn_free(&c);
}

TEST(ListC6Upgrade, NewPtrListRoundTripText) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  capn_ptr_list names;
  names.p = capn_new_ptr_list(cs, 1);
  ASSERT_EQ(CAPN_PTR_LIST, names.p.type);
  ASSERT_EQ(0, capn_set_text(names.p, 0, chars_to_text("hello")));
  EXPECT_CAPN_TEXT_EQ("hello", capn_get_text(names.p, 0, chars_to_text("")));

  capn_free(&c);
}

TEST(ListC6Upgrade, UpgradedStructExtraDataIsDefault) {
  uint8_t framed[8 + sizeof(kC6OneText)];
  struct capn c;
  ASSERT_EQ(0, init_one_seg(&c, kC6OneText, sizeof(kC6OneText),
                            framed, sizeof(framed)));

  capn_ptr list = capn_getp(capn_root(&c), 0, 1);
  TestLists_StructPc_list l = {list};
  struct TestLists_StructPc s;
  memset(&s, 0, sizeof(s));
  get_TestLists_StructPc(&s, l, 0);
  EXPECT_CAPN_TEXT_EQ("hi", s.f);
  EXPECT_EQ(UINT64_C(0), s.pad);

  capn_free(&c);
}
