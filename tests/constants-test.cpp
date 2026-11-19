/* constants-test.cpp
 *
 * The values capnpc-c bakes into compiler/test.capnp.c for the schema's
 * constants, checked against the literals in compiler/test.capnp (the
 * same values the C++ reference suite asserts). Scalars, text, data,
 * enums, every list type and a struct constant with nested lists.
 */

#include <gtest/gtest.h>
#include <cmath>
#include <cstring>

#include "capnp_c.h"
#include "test.capnp.h"

namespace {

void expect_text(const char *want, capn_text t) {
  ASSERT_NE(nullptr, t.str);
  EXPECT_EQ(strlen(want), static_cast<size_t>(t.len));
  EXPECT_EQ(0, memcmp(want, t.str, strlen(want)));
}

void expect_data(const char *want, capn_data d) {
  ASSERT_EQ(static_cast<int>(strlen(want)), capn_len(d));
  capn_list8 l;
  l.p = d.p;
  for (size_t i = 0; i < strlen(want); i++)
    EXPECT_EQ(static_cast<uint8_t>(want[i]), capn_get8(l, static_cast<int>(i)));
}

}  // namespace

TEST(Constants, Scalars) {
  EXPECT_EQ(1u, TestConstants_boolConst);
  EXPECT_EQ(-123, TestConstants_int8Const);
  EXPECT_EQ(-12345, TestConstants_int16Const);
  EXPECT_EQ(-12345678, TestConstants_int32Const);
  EXPECT_EQ(-123456789012345LL, TestConstants_int64Const);
  EXPECT_EQ(234u, TestConstants_uint8Const);
  EXPECT_EQ(45678u, TestConstants_uint16Const);
  EXPECT_EQ(3456789012u, TestConstants_uint32Const);
  EXPECT_EQ(12345678901234567890ULL, TestConstants_uint64Const);
  EXPECT_EQ(1234.5f, capn_to_f32(TestConstants_float32Const.u));
  EXPECT_EQ(-123e45, capn_to_f64(TestConstants_float64Const.u));
  EXPECT_EQ(TestEnum_corge, TestConstants_enumConst);
  EXPECT_EQ(12345u, globalInt);
}

TEST(Constants, TextAndData) {
  expect_text("foo", TestConstants_textConst);
  expect_data("bar", TestConstants_dataConst);
  expect_text("foobar", globalText);
}

TEST(Constants, Lists) {
  EXPECT_EQ(6, capn_ptr_len(TestConstants_voidListConst));

  ASSERT_EQ(4, capn_len(TestConstants_boolListConst));
  EXPECT_EQ(1, capn_get1(TestConstants_boolListConst, 0));
  EXPECT_EQ(0, capn_get1(TestConstants_boolListConst, 1));
  EXPECT_EQ(0, capn_get1(TestConstants_boolListConst, 2));
  EXPECT_EQ(1, capn_get1(TestConstants_boolListConst, 3));

  ASSERT_EQ(2, capn_len(TestConstants_int8ListConst));
  EXPECT_EQ(111, static_cast<int8_t>(capn_get8(TestConstants_int8ListConst, 0)));
  EXPECT_EQ(-111, static_cast<int8_t>(capn_get8(TestConstants_int8ListConst, 1)));
  EXPECT_EQ(11111, static_cast<int16_t>(capn_get16(TestConstants_int16ListConst, 0)));
  EXPECT_EQ(-11111, static_cast<int16_t>(capn_get16(TestConstants_int16ListConst, 1)));
  EXPECT_EQ(111111111, static_cast<int32_t>(capn_get32(TestConstants_int32ListConst, 0)));
  EXPECT_EQ(-111111111, static_cast<int32_t>(capn_get32(TestConstants_int32ListConst, 1)));
  EXPECT_EQ(1111111111111111111LL, static_cast<int64_t>(capn_get64(TestConstants_int64ListConst, 0)));
  EXPECT_EQ(-1111111111111111111LL, static_cast<int64_t>(capn_get64(TestConstants_int64ListConst, 1)));
  EXPECT_EQ(111u, capn_get8(TestConstants_uint8ListConst, 0));
  EXPECT_EQ(222u, capn_get8(TestConstants_uint8ListConst, 1));
  EXPECT_EQ(33333u, capn_get16(TestConstants_uint16ListConst, 0));
  EXPECT_EQ(44444u, capn_get16(TestConstants_uint16ListConst, 1));
  ASSERT_EQ(1, capn_len(TestConstants_uint32ListConst));
  EXPECT_EQ(3333333333u, capn_get32(TestConstants_uint32ListConst, 0));
  ASSERT_EQ(1, capn_len(TestConstants_uint64ListConst));
  EXPECT_EQ(11111111111111111111ULL, capn_get64(TestConstants_uint64ListConst, 0));

  ASSERT_EQ(4, capn_len(TestConstants_float32ListConst));
  EXPECT_EQ(5555.5f, capn_to_f32(capn_get32(TestConstants_float32ListConst, 0)));
  EXPECT_TRUE(std::isinf(capn_to_f32(capn_get32(TestConstants_float32ListConst, 1))));
  EXPECT_LT(capn_to_f32(capn_get32(TestConstants_float32ListConst, 2)), 0.0f);
  EXPECT_TRUE(std::isnan(capn_to_f32(capn_get32(TestConstants_float32ListConst, 3))));
  ASSERT_EQ(4, capn_len(TestConstants_float64ListConst));
  EXPECT_EQ(7777.75, capn_to_f64(capn_get64(TestConstants_float64ListConst, 0)));
  EXPECT_TRUE(std::isinf(capn_to_f64(capn_get64(TestConstants_float64ListConst, 1))));
  EXPECT_TRUE(std::isnan(capn_to_f64(capn_get64(TestConstants_float64ListConst, 3))));

  capn_text def = {0, "", NULL};
  ASSERT_EQ(3, capn_len(TestConstants_textListConst));
  expect_text("plugh", capn_get_text(TestConstants_textListConst.p, 0, def));
  expect_text("xyzzy", capn_get_text(TestConstants_textListConst.p, 1, def));
  expect_text("thud", capn_get_text(TestConstants_textListConst.p, 2, def));
  ASSERT_EQ(3, capn_len(TestConstants_dataListConst));
  expect_data("oops", capn_get_data(TestConstants_dataListConst.p, 0));
  expect_data("exhausted", capn_get_data(TestConstants_dataListConst.p, 1));
  expect_data("rfc3092", capn_get_data(TestConstants_dataListConst.p, 2));

  ASSERT_EQ(2, capn_len(TestConstants_enumListConst));
  EXPECT_EQ(TestEnum_foo, capn_get16(TestConstants_enumListConst, 0));
  EXPECT_EQ(TestEnum_garply, capn_get16(TestConstants_enumListConst, 1));

  ASSERT_EQ(3, capn_len(TestConstants_structListConst));
  struct TestAllTypes s;
  memset(&s, 0, sizeof s);
  get_TestAllTypes(&s, TestConstants_structListConst, 2);
  expect_text("structlist 3", s.textField);
}

TEST(Constants, StructWithNestedValues) {
  struct TestAllTypes s;
  memset(&s, 0, sizeof s);
  read_TestAllTypes(&s, TestConstants_structConst);

  EXPECT_EQ(1u, s.boolField);
  EXPECT_EQ(-12, s.int8Field);
  EXPECT_EQ(3456, s.int16Field);
  EXPECT_EQ(-78901234, s.int32Field);
  EXPECT_EQ(56789012345678LL, s.int64Field);
  EXPECT_EQ(90u, s.uInt8Field);
  EXPECT_EQ(1234u, s.uInt16Field);
  EXPECT_EQ(56789012u, s.uInt32Field);
  EXPECT_EQ(345678901234567890ULL, s.uInt64Field);
  EXPECT_EQ(-1.25e-10f, s.float32Field);
  EXPECT_EQ(345.0, s.float64Field);
  expect_text("baz", s.textField);
  expect_data("qux", s.dataField);
  EXPECT_EQ(TestEnum_baz, s.enumField);

  struct TestAllTypes inner;
  memset(&inner, 0, sizeof inner);
  read_TestAllTypes(&inner, s.structField);
  expect_text("nested", inner.textField);
  struct TestAllTypes deeper;
  memset(&deeper, 0, sizeof deeper);
  read_TestAllTypes(&deeper, inner.structField);
  expect_text("really nested", deeper.textField);

  ASSERT_EQ(3, capn_ptr_len(s.voidList));
  ASSERT_EQ(5, capn_len(s.boolList));
  EXPECT_EQ(0, capn_get1(s.boolList, 0));
  EXPECT_EQ(1, capn_get1(s.boolList, 4));
  ASSERT_EQ(4, capn_len(s.int8List));
  EXPECT_EQ(-0x80, static_cast<int8_t>(capn_get8(s.int8List, 2)));
  EXPECT_EQ(0x7f, static_cast<int8_t>(capn_get8(s.int8List, 3)));
  ASSERT_EQ(4, capn_len(s.int64List));
  EXPECT_EQ(INT64_MIN, static_cast<int64_t>(capn_get64(s.int64List, 2)));
  EXPECT_EQ(INT64_MAX, static_cast<int64_t>(capn_get64(s.int64List, 3)));
  ASSERT_EQ(4, capn_len(s.uInt64List));
  EXPECT_EQ(0xffffffffffffffffULL, capn_get64(s.uInt64List, 3));
  ASSERT_EQ(6, capn_len(s.float32List));
  EXPECT_EQ(1234567.0f, capn_to_f32(capn_get32(s.float32List, 1)));
  EXPECT_EQ(1e37f, capn_to_f32(capn_get32(s.float32List, 2)));
  ASSERT_EQ(6, capn_len(s.float64List));
  EXPECT_EQ(-1e-306, capn_to_f64(capn_get64(s.float64List, 5)));

  capn_text def = {0, "", NULL};
  ASSERT_EQ(3, capn_len(s.textList));
  expect_text("grault", capn_get_text(s.textList.p, 2, def));
  ASSERT_EQ(3, capn_len(s.dataList));
  expect_data("fred", capn_get_data(s.dataList.p, 2));
  ASSERT_EQ(3, capn_len(s.enumList));
  EXPECT_EQ(TestEnum_qux, capn_get16(s.enumList, 0));
  EXPECT_EQ(TestEnum_grault, capn_get16(s.enumList, 2));
  ASSERT_EQ(3, capn_len(s.structList));
  struct TestAllTypes e;
  memset(&e, 0, sizeof e);
  get_TestAllTypes(&e, s.structList, 1);
  expect_text("x structlist 2", e.textField);
}

TEST(Constants, StructConstantsAtFileScope) {
  struct TestAllTypes g;
  memset(&g, 0, sizeof g);
  read_TestAllTypes(&g, globalStruct);
  EXPECT_EQ(54321, g.int32Field);
}
