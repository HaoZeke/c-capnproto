/* union-pointer-has-test.cpp
 *
 * UDef_has_b matches C++ Reader::hasB(): true when which() selects b
 * and the text pointer is non-null.
 */

#include <gtest/gtest.h>
#include <cstring>

#include "capnp_c.h"
#include "union-pointer-has.capnp.h"

TEST(UnionPointerHas, HasBFollowsDiscriminant) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;
  UDef_ptr p = new_UDef(cs);
  struct UDef u;
  int off;
  int i;

  memset(&u, 0, sizeof u);
  u.which = UDef_b;
  write_UDef(&u, p);
  EXPECT_EQ(0, UDef_has_b(p));

  u.b.str = "x";
  u.b.len = 1;
  write_UDef(&u, p);
  EXPECT_EQ(1, UDef_has_b(p));

  off = -1;
  for (i = 0; i < 4; i++) {
    if (capn_getp(p.p, i, 1).type != CAPN_NULL) {
      off = i;
      break;
    }
  }
  ASSERT_GE(off, 0);

  u.which = UDef_a;
  u.a = 1;
  write_UDef(&u, p);
  EXPECT_NE((int)CAPN_NULL, (int)capn_getp(p.p, off, 1).type);
  EXPECT_EQ(0, UDef_has_b(p));

  capn_free(&c);
}
