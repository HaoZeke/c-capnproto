/* interface-field-test.cpp
 *
 * Plugin must emit compiling accessors for an interface field. Encoding
 * represents the field as a capability pointer (A=3); there is no skip.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>

#include "capnp_c.h"
#include "interface-field.capnp.h"

TEST(InterfaceField, GeneratedTypesCompile) {
  Echo_ptr e;
  Echo_list el;
  Holder_ptr h;
  (void)e;
  (void)el;
  (void)h;
  EXPECT_EQ((size_t)2, Holder_pointer_count);
}

TEST(InterfaceField, GetSetCapability) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  Holder_ptr h = new_Holder(cs);
  Echo_ptr e = new_Echo(cs);
  EXPECT_EQ(CAPN_INTERFACE, e.p.type);
  e.p.cap_idx = 3;
  Holder_set_cap(h, e);

  Echo_ptr got = Holder_get_cap(h);
  EXPECT_EQ(CAPN_INTERFACE, got.p.type);
  EXPECT_EQ(0, got.p.len);
  EXPECT_EQ(3u, got.p.cap_idx);

  capn_free(&c);
}

TEST(InterfaceField, StructPointerIsASchemaMismatch) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;
  Holder_ptr h = new_Holder(cs);
  capn_ptr body = capn_new_struct(cs, 8, 0);
  ASSERT_EQ(CAPN_STRUCT, body.type);
  capn_write32(body, 0, 1);
  ASSERT_EQ(0, capn_setp(h.p, 0, body));

  Echo_ptr missing = Holder_get_cap(h);
  EXPECT_EQ(CAPN_NULL, missing.p.type);
  EXPECT_EQ(0, capn_ok(&c));

  capn_clear_err(&c);
  Echo_ptr e = new_Echo(cs);
  e.p.cap_idx = 4;
  Holder_set_cap(h, e);
  Echo_ptr got = Holder_get_cap(h);
  EXPECT_EQ(CAPN_INTERFACE, got.p.type);
  EXPECT_EQ(4u, got.p.cap_idx);
  EXPECT_EQ(1, capn_ok(&c));
  capn_free(&c);
}

TEST(InterfaceField, WriteReadRoundTrip) {
  uint8_t buf[4096];
  int64_t sz = 0;

  {
    struct capn c;
    capn_init_malloc(&c);
    struct capn_segment *cs = capn_root(&c).seg;
    Holder_ptr h = new_Holder(cs);
    Echo_ptr e = new_Echo(cs);
    e.p.cap_idx = 11;
    Holder_set_cap(h, e);
    ASSERT_EQ(0, capn_set_root(&c, h.p));
    sz = capn_write_mem(&c, buf, sizeof(buf), 0);
    ASSERT_GT(sz, 0);
    capn_free(&c);
  }

  {
    struct capn c;
    ASSERT_EQ(0, capn_init_mem(&c, buf, (size_t)sz, 0));
    Holder_ptr h;
    h.p = capn_getp(capn_root(&c), 0, 1);
    ASSERT_EQ(CAPN_STRUCT, h.p.type);
    Echo_ptr got = Holder_get_cap(h);
    EXPECT_EQ(CAPN_INTERFACE, got.p.type);
    EXPECT_EQ(0, got.p.len);
    EXPECT_EQ(11u, got.p.cap_idx);
    capn_free(&c);
  }
}

TEST(InterfaceField, ListOfInterfaces) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  Holder_ptr h = new_Holder(cs);
  Echo_list ls = new_Echo_list(cs, 2);
  ASSERT_EQ(CAPN_PTR_LIST, ls.p.type);
  ASSERT_EQ(2, ls.p.len);

  Echo_ptr e0 = new_Echo(cs);
  e0.p.cap_idx = 1;
  ASSERT_EQ(0, capn_setp(ls.p, 0, e0.p));
  Echo_ptr e1 = new_Echo(cs);
  e1.p.cap_idx = 2;
  ASSERT_EQ(0, capn_setp(ls.p, 1, e1.p));
  Holder_set_caps(h, ls);

  Echo_list got = Holder_get_caps(h);
  ASSERT_EQ(CAPN_PTR_LIST, got.p.type);
  ASSERT_EQ(2, capn_len(got));
  capn_ptr g0 = capn_getp(got.p, 0, 1);
  capn_ptr g1 = capn_getp(got.p, 1, 1);
  EXPECT_EQ(CAPN_INTERFACE, g0.type);
  EXPECT_EQ(0, g0.len);
  EXPECT_EQ(1u, g0.cap_idx);
  EXPECT_EQ(CAPN_INTERFACE, g1.type);
  EXPECT_EQ(0, g1.len);
  EXPECT_EQ(2u, g1.cap_idx);

  capn_free(&c);
}

TEST(InterfaceField, StreamMethodHeaderIsSelfContained) {
  EXPECT_EQ(0, Sink_write_method.ordinal);
  EXPECT_EQ(1, Sink_done_method.ordinal);
}
