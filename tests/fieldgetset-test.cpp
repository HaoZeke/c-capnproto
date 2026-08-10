/* fieldgetset-test.cpp
 *
 * Get-after-set for nested $C.fieldgetset pointer fields.
 *
 * TreeNode_set_leaf then Leaf_get_value(TreeNode_get_leaf(n)) must
 * return the value written on the Leaf, not 0.
 *
 * new_Leaf_list is List of a 1-word 0-pointer struct. Spec requires
 * C=7 plus a tag word (B = element count), including the empty list.
 */

#include <gtest/gtest.h>
#include <cstdint>

#include "capnp_c.h"
#include "fieldgetset.capnp.h"

static uint64_t holder_ptr_word(capn_ptr holder) {
  return capn_flip64(*(uint64_t *)(holder.data + holder.datasz));
}

TEST(FieldGetSet, NestedLeafGetAfterSet) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  TreeNode_ptr n = new_TreeNode(cs);
  TreeNode_set_nodeType(n, 1);
  Leaf_ptr l = new_Leaf(cs);
  Leaf_set_value(l, 1);
  TreeNode_set_leaf(n, l);
  EXPECT_EQ(1, Leaf_get_value(TreeNode_get_leaf(n)));

  capn_free(&c);
}

TEST(FieldGetSet, NestedInnerGetAfterSet) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  TreeNode_ptr root = new_TreeNode(cs);
  TreeNode_set_nodeType(root, 2);

  TreeNode_ptr left = new_TreeNode(cs);
  TreeNode_set_nodeType(left, 3);
  Leaf_ptr left_leaf = new_Leaf(cs);
  Leaf_set_value(left_leaf, 7);
  TreeNode_set_leaf(left, left_leaf);

  Inner_ptr inner = new_Inner(cs);
  Inner_set_left(inner, left);
  TreeNode_set_inner(root, inner);

  EXPECT_EQ(2, TreeNode_get_nodeType(root));
  EXPECT_EQ(3, TreeNode_get_nodeType(Inner_get_left(TreeNode_get_inner(root))));
  EXPECT_EQ(7, Leaf_get_value(TreeNode_get_leaf(Inner_get_left(TreeNode_get_inner(root)))));

  capn_free(&c);
}

TEST(FieldGetSet, LeafListEncodesCompositeC7) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  capn_ptr holder = capn_new_struct(cs, 0, 1);
  ASSERT_EQ(CAPN_STRUCT, holder.type);

  Leaf_list leaves = new_Leaf_list(cs, 3);
  ASSERT_EQ(CAPN_LIST, leaves.p.type);
  struct Leaf item;
  item.value = 1;
  set_Leaf(&item, leaves, 0);
  item.value = 2;
  set_Leaf(&item, leaves, 1);
  item.value = 3;
  set_Leaf(&item, leaves, 2);

  ASSERT_EQ(0, capn_setp(holder, 0, leaves.p));
  ASSERT_EQ(0, capn_set_root(&c, holder));

  uint64_t word = holder_ptr_word(holder);
  EXPECT_EQ(UINT64_C(1), word & 3u);
  EXPECT_EQ(UINT64_C(7), (word >> 32) & 7u);

  ASSERT_EQ(1, leaves.p.is_composite_list);
  ASSERT_NE(static_cast<char *>(NULL), leaves.p.data);
  uint64_t tag = capn_flip64(*(uint64_t *)(leaves.p.data - 8));
  EXPECT_EQ(UINT64_C(0), tag & 3u);
  EXPECT_EQ(UINT64_C(3), (tag >> 2) & UINT64_C(0x3fffffff));
  EXPECT_EQ(UINT64_C(1), (tag >> 32) & 0xffffu);
  EXPECT_EQ(UINT64_C(0), tag >> 48);

  struct Leaf got;
  get_Leaf(&got, leaves, 0);
  EXPECT_EQ(1, got.value);
  get_Leaf(&got, leaves, 1);
  EXPECT_EQ(2, got.value);
  get_Leaf(&got, leaves, 2);
  EXPECT_EQ(3, got.value);

  capn_free(&c);
}

TEST(FieldGetSet, EmptyLeafListEncodesCompositeC7) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  capn_ptr holder = capn_new_struct(cs, 0, 1);
  Leaf_list leaves = new_Leaf_list(cs, 0);
  ASSERT_EQ(CAPN_LIST, leaves.p.type);
  ASSERT_EQ(0, capn_setp(holder, 0, leaves.p));
  ASSERT_EQ(0, capn_set_root(&c, holder));

  uint64_t word = holder_ptr_word(holder);
  EXPECT_EQ(UINT64_C(1), word & 3u);
  EXPECT_EQ(UINT64_C(7), (word >> 32) & 7u);
  EXPECT_EQ(UINT64_C(0), word >> 35);

  ASSERT_EQ(1, leaves.p.is_composite_list);
  ASSERT_NE(static_cast<char *>(NULL), leaves.p.data);
  uint64_t tag = capn_flip64(*(uint64_t *)(leaves.p.data - 8));
  EXPECT_EQ(UINT64_C(0), tag & 3u);
  EXPECT_EQ(UINT64_C(0), (tag >> 2) & UINT64_C(0x3fffffff));
  EXPECT_EQ(UINT64_C(1), (tag >> 32) & 0xffffu);
  EXPECT_EQ(UINT64_C(0), tag >> 48);

  capn_free(&c);
}
