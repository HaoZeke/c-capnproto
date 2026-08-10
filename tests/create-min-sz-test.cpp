/* create-min-sz-test.cpp
 *
 * Meson compiles lib/capn-malloc.c for this binary with
 * -DCAPN_CREATE_MIN_SZ=512 so create() uses a 512-byte floor instead of
 * the default 4096. Separate from capn-test, which uses the default.
 */

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>

extern "C" {
#include "capnp_c.h"
}

TEST(CreateMinSz, OverrideIs512) {
  struct capn c;
  capn_init_malloc(&c);
  capn_ptr root = capn_root(&c);
  ASSERT_TRUE(root.seg != NULL);
  size_t alloc = root.seg->cap + sizeof(struct capn_segment);
  EXPECT_EQ(size_t{512}, alloc);
  EXPECT_EQ(size_t{0}, alloc % 512);
  EXPECT_NE(size_t{4096}, alloc);
  capn_free(&c);
}
