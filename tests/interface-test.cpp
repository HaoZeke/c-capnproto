/* interface-test.cpp
 *
 * capn_new_interface creates a capability pointer (encoding.html: A=3,
 * B=0, C = capability table index). Create, write into a struct / root,
 * read back, and round-trip through capn_write_mem / capn_init_mem.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>

#include "capnp_c.h"

class InterfaceSession {
public:
  InterfaceSession() { capn_init_malloc(&capn); }
  ~InterfaceSession() { capn_free(&capn); }
  struct capn capn;
};

TEST(Interface, NewInterfaceIsCapabilityPointer) {
  InterfaceSession s;
  capn_ptr root = capn_root(&s.capn);
  capn_ptr iface = capn_new_interface(root.seg, 0, 0);
  ASSERT_EQ(CAPN_INTERFACE, iface.type);
  EXPECT_EQ(0, iface.len);
  EXPECT_EQ(root.seg, iface.seg);
}

TEST(Interface, CreateWriteRead) {
  InterfaceSession s;
  capn_ptr root = capn_root(&s.capn);
  capn_ptr holder = capn_new_struct(root.seg, 0, 1);
  ASSERT_EQ(CAPN_STRUCT, holder.type);
  ASSERT_EQ(0, capn_setp(root, 0, holder));

  capn_ptr iface = capn_new_interface(holder.seg, 0, 0);
  ASSERT_EQ(CAPN_INTERFACE, iface.type);
  iface.len = 7;
  ASSERT_EQ(0, capn_setp(holder, 0, iface));

  capn_ptr got = capn_getp(holder, 0, 1);
  EXPECT_EQ(CAPN_INTERFACE, got.type);
  EXPECT_EQ(7, got.len);

  /* Wire word at the struct's first pointer: A=3, B=0, C=7. */
  uint64_t word = capn_flip64(*(uint64_t *)(holder.data + holder.datasz));
  EXPECT_EQ(UINT64_C(3), word & 3u);
  EXPECT_EQ(UINT64_C(0), (word >> 2) & UINT64_C(0x3fffffff));
  EXPECT_EQ(UINT64_C(7), word >> 32);
}

TEST(Interface, IndexZeroIsNotNull) {
  InterfaceSession s;
  capn_ptr root = capn_root(&s.capn);
  capn_ptr iface = capn_new_interface(root.seg, 0, 0);
  iface.len = 0;
  ASSERT_EQ(0, capn_setp(root, 0, iface));

  capn_ptr got = capn_getp(root, 0, 1);
  EXPECT_EQ(CAPN_INTERFACE, got.type);
  EXPECT_EQ(0, got.len);

  uint64_t word = capn_flip64(*(uint64_t *)root.data);
  EXPECT_EQ(UINT64_C(3), word);
}

TEST(Interface, NullPointerIsNotCapability) {
  InterfaceSession s;
  capn_ptr root = capn_root(&s.capn);
  capn_ptr got = capn_getp(root, 0, 1);
  EXPECT_EQ(CAPN_NULL, got.type);
}

TEST(Interface, RoundTripMem) {
  uint8_t buf[4096];
  int64_t sz = 0;

  {
    struct capn c;
    capn_init_malloc(&c);
    capn_ptr root = capn_root(&c);
    capn_ptr holder = capn_new_struct(root.seg, 0, 1);
    capn_ptr iface = capn_new_interface(holder.seg, 8, 1);
    iface.len = 42;
    ASSERT_EQ(0, capn_setp(holder, 0, iface));
    ASSERT_EQ(0, capn_set_root(&c, holder));
    sz = capn_write_mem(&c, buf, sizeof(buf), 0);
    ASSERT_GT(sz, 0);
    capn_free(&c);
  }

  {
    struct capn c;
    ASSERT_EQ(0, capn_init_mem(&c, buf, (size_t)sz, 0));
    capn_ptr holder = capn_getp(capn_root(&c), 0, 1);
    ASSERT_EQ(CAPN_STRUCT, holder.type);
    capn_ptr got = capn_getp(holder, 0, 1);
    EXPECT_EQ(CAPN_INTERFACE, got.type);
    EXPECT_EQ(42, got.len);
    capn_free(&c);
  }
}

TEST(Interface, ListOfCapabilities) {
  InterfaceSession s;
  capn_ptr root = capn_root(&s.capn);
  capn_ptr list = capn_new_ptr_list(root.seg, 3);
  ASSERT_EQ(CAPN_PTR_LIST, list.type);

  for (int i = 0; i < 3; i++) {
    capn_ptr iface = capn_new_interface(list.seg, 0, 0);
    iface.len = 10 + i;
    ASSERT_EQ(0, capn_setp(list, i, iface));
  }

  ASSERT_EQ(0, capn_setp(root, 0, list));

  capn_ptr got_list = capn_getp(root, 0, 1);
  ASSERT_EQ(CAPN_PTR_LIST, got_list.type);
  ASSERT_EQ(3, got_list.len);
  for (int i = 0; i < 3; i++) {
    capn_ptr got = capn_getp(got_list, i, 1);
    EXPECT_EQ(CAPN_INTERFACE, got.type);
    EXPECT_EQ(10 + i, got.len);
  }
}

TEST(Interface, ReservedOtherPointerIsNull) {
  /* A=3, B=1 (reserved other-pointer kind), C=0. */
  static const uint8_t msg[] = {
      0x00, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00,
      0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  struct capn c;
  ASSERT_EQ(0, capn_init_mem(&c, msg, sizeof(msg), 0));
  capn_ptr got = capn_getp(capn_root(&c), 0, 1);
  EXPECT_EQ(CAPN_NULL, got.type);
  capn_free(&c);
}

TEST(Interface, HandCraftedCapabilityIndex) {
  /* One-word message: capability pointer, index 5. */
  static const uint8_t msg[] = {
      0x00, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00,
      0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
  };
  struct capn c;
  ASSERT_EQ(0, capn_init_mem(&c, msg, sizeof(msg), 0));
  capn_ptr got = capn_getp(capn_root(&c), 0, 1);
  EXPECT_EQ(CAPN_INTERFACE, got.type);
  EXPECT_EQ(5, got.len);
  capn_free(&c);
}
