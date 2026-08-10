/* fieldgetset-test.cpp
 *
 * Get-after-set for nested $C.fieldgetset pointer fields.
 *
 * TreeNode_set_leaf then Leaf_get_value(TreeNode_get_leaf(n)) must
 * return the value written on the Leaf, not 0.
 */

#include <gtest/gtest.h>
#include <cstdint>

#include "capnp_c.h"
#include "fieldgetset.capnp.h"

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
