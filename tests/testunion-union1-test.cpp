/* testunion-union1-test.cpp
 *
 * TestUnion.union1 packs u1f0s1 / u1f1s1 / u1f2s1 as same-type
 * members. write_TestUnion must use the member that was set, not the
 * last same-C-type field. Setting only u1f1s1 must set the schema bit.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "capnp_c.h"
#include "test.capnp.h"

#ifndef TEST_CAPNP_GENERATED_C
#define TEST_CAPNP_GENERATED_C "compiler/test.capnp.c"
#endif

static std::string read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    return std::string();
  }
  std::string body;
  char buf[4096];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    body.append(buf, n);
  }
  fclose(f);
  return body;
}

static std::string read_generated_c() {
  const char *candidates[] = {
    TEST_CAPNP_GENERATED_C,
    "compiler/test.capnp.c",
    "../compiler/test.capnp.c",
  };
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    std::string body = read_file(candidates[i]);
    if (!body.empty()) {
      return body;
    }
  }
  return std::string();
}

struct EncodedUnion {
  uint16_t union0;
  uint16_t union1;
  uint16_t union2;
  uint16_t union3;
  int first_data_bit;
  unsigned bit129;
  uint8_t data[64];
};

/* First set bit after the four 16-bit discriminants, matching
 * capnproto encoding-test.c++ initUnion. -1 if none. */
static int first_set_bit_after_discriminants(const uint8_t *data, size_t n) {
  if (n < 8) {
    return -1;
  }
  int offset = 0;
  for (size_t i = 8; i < n; i++) {
    if (data[i] != 0) {
      uint8_t bits = data[i];
      while ((bits & 1u) == 0) {
        ++offset;
        bits >>= 1;
      }
      return offset;
    }
    offset += 8;
  }
  return -1;
}

static void encode_union1(void (*fill)(struct TestUnion *), EncodedUnion *out) {
  memset(out, 0, sizeof(*out));

  struct capn c;
  capn_init_malloc(&c);
  capn_ptr root = capn_root(&c);
  TestUnion_ptr p = new_TestUnion(root.seg);
  EXPECT_EQ(0, capn_setp(root, 0, p.p));

  struct TestUnion s;
  memset(&s, 0, sizeof(s));
  fill(&s);
  write_TestUnion(&s, p);

  EXPECT_EQ(CAPN_STRUCT, p.p.type);
  EXPECT_GE(p.p.datasz, 8);
  if (p.p.type != CAPN_STRUCT || p.p.datasz < 8 || p.p.data == NULL) {
    capn_free(&c);
    return;
  }
  size_t n = (size_t)p.p.datasz;
  if (n > sizeof(out->data)) {
    n = sizeof(out->data);
  }
  memcpy(out->data, p.p.data, n);
  out->union0 = capn_read16(p.p, 0);
  out->union1 = capn_read16(p.p, 2);
  out->union2 = capn_read16(p.p, 4);
  out->union3 = capn_read16(p.p, 6);
  out->bit129 = (capn_read8(p.p, 16) & 2) != 0;
  out->first_data_bit = first_set_bit_after_discriminants(
      (const uint8_t *)p.p.data, (size_t)p.p.datasz);

  capn_free(&c);
}

static void fill_u1f0s1(struct TestUnion *s) {
  s->union1_which = TestUnion_union1_u1f0s1;
  s->union1.u1f0s1 = 1;
}

static void fill_u1f1s1(struct TestUnion *s) {
  s->union1_which = TestUnion_union1_u1f1s1;
  s->union1.u1f1s1 = 1;
}

static void fill_u1f2s1(struct TestUnion *s) {
  s->union1_which = TestUnion_union1_u1f2s1;
  s->union1.u1f2s1 = 1;
}

static void fill_u1f0s8(struct TestUnion *s) {
  s->union1_which = TestUnion_union1_u1f0s8;
  s->union1.u1f0s8 = 123;
}

static void fill_u1f1s8(struct TestUnion *s) {
  s->union1_which = TestUnion_union1_u1f1s8;
  s->union1.u1f1s8 = 123;
}

static void fill_u1f2s8(struct TestUnion *s) {
  s->union1_which = TestUnion_union1_u1f2s8;
  s->union1.u1f2s8 = 123;
}

TEST(TestUnion, U1f1s1TrueWithoutU1f0s1SetsDistinctDiscriminantAndBit) {
  EncodedUnion e;
  encode_union1(fill_u1f1s1, &e);

  EXPECT_EQ(0, e.union0);
  EXPECT_EQ((uint16_t)TestUnion_union1_u1f1s1, e.union1);
  EXPECT_EQ(0, e.union2);
  EXPECT_EQ(0, e.union3);

  /* C++ encoding-test UnionLayout: data bit 65 after discriminants
   * (struct bit 129). Must be set without writing u1f0s1 / u1f2s1. */
  EXPECT_EQ(1u, e.bit129);
  EXPECT_EQ(65, e.first_data_bit);
  EXPECT_TRUE((e.data[16] & 2u) != 0) << "struct bit 129 unset";
  EXPECT_EQ(0, e.data[16] & ~2u) << "unexpected extra bits in byte 16";
}

TEST(TestUnion, Union1BoolsU1f0U1f1U1f2) {
  EncodedUnion a, b, c;
  encode_union1(fill_u1f0s1, &a);
  encode_union1(fill_u1f1s1, &b);
  encode_union1(fill_u1f2s1, &c);

  EXPECT_EQ((uint16_t)TestUnion_union1_u1f0s1, a.union1);
  EXPECT_EQ((uint16_t)TestUnion_union1_u1f1s1, b.union1);
  EXPECT_EQ((uint16_t)TestUnion_union1_u1f2s1, c.union1);

  EXPECT_NE(a.union1, b.union1);
  EXPECT_NE(b.union1, c.union1);
  EXPECT_NE(a.union1, c.union1);

  EXPECT_EQ(1u, a.bit129);
  EXPECT_EQ(1u, b.bit129);
  EXPECT_EQ(1u, c.bit129);
  EXPECT_EQ(65, a.first_data_bit);
  EXPECT_EQ(65, b.first_data_bit);
  EXPECT_EQ(65, c.first_data_bit);
}

TEST(TestUnion, Union1Int8U1f0U1f1U1f2) {
  EncodedUnion a, b, c;
  encode_union1(fill_u1f0s8, &a);
  encode_union1(fill_u1f1s8, &b);
  encode_union1(fill_u1f2s8, &c);

  EXPECT_EQ((uint16_t)TestUnion_union1_u1f0s8, a.union1);
  EXPECT_EQ((uint16_t)TestUnion_union1_u1f1s8, b.union1);
  EXPECT_EQ((uint16_t)TestUnion_union1_u1f2s8, c.union1);
  EXPECT_EQ(123, a.data[17]);
  EXPECT_EQ(123, b.data[17]);
  EXPECT_EQ(123, c.data[17]);
  /* C++ UnionLayout offset 72 after discriminants. */
  EXPECT_EQ(72, a.first_data_bit);
  EXPECT_EQ(72, b.first_data_bit);
  EXPECT_EQ(72, c.first_data_bit);
}

TEST(TestUnion, U1f1s1RoundTripNamedMember) {
  struct capn c;
  capn_init_malloc(&c);
  TestUnion_ptr p = new_TestUnion(capn_root(&c).seg);

  struct TestUnion w;
  memset(&w, 0, sizeof(w));
  w.union1_which = TestUnion_union1_u1f1s1;
  w.union1.u1f1s1 = 1;
  write_TestUnion(&w, p);

  struct TestUnion r;
  memset(&r, 0, sizeof(r));
  read_TestUnion(&r, p);
  EXPECT_EQ(TestUnion_union1_u1f1s1, r.union1_which);
  EXPECT_EQ(1u, r.union1.u1f1s1);

  capn_free(&c);
}

TEST(TestUnion, GeneratedReadWriteUsePerFieldUnion1Members) {
  std::string body = read_generated_c();
  ASSERT_FALSE(body.empty()) << "could not read generated test.capnp.c";

  /* Collapsed emitter writes every bool case through u1f2s1. */
  EXPECT_NE(std::string::npos, body.find("s->union1.u1f0s1"))
      << "read/write_TestUnion must name u1f0s1";
  EXPECT_NE(std::string::npos, body.find("s->union1.u1f1s1"))
      << "read/write_TestUnion must name u1f1s1";
  EXPECT_NE(std::string::npos, body.find("s->union1.u1f2s1"))
      << "read/write_TestUnion must name u1f2s1";
  EXPECT_NE(std::string::npos, body.find("s->union1.u1f0s8"))
      << "read/write_TestUnion must name u1f0s8";
  EXPECT_NE(std::string::npos, body.find("s->union1.u1f1s8"))
      << "read/write_TestUnion must name u1f1s8";

  /* Same-type cases must not share one body (last field's offset). */
  EXPECT_EQ(std::string::npos,
            body.find("case TestUnion_union1_u1f0s1:\n"
                      "\tcase TestUnion_union1_u1f1s1:\n"
                      "\tcase TestUnion_union1_u1f2s1:"))
      << "union1 bool cases still collapsed onto one offset";
  EXPECT_EQ(std::string::npos,
            body.find("case TestUnion_union1_u1f0s8:\n"
                      "\tcase TestUnion_union1_u1f1s8:\n"
                      "\tcase TestUnion_union1_u1f2s8:"))
      << "union1 int8 cases still collapsed onto one offset";
}
