/* json-ann-test.cpp
 *
 * $Json.name, flatten, discriminator, base64 and hex on the dynamic
 * codec. The compact JSON is what JsonCodec emits for the same schema.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <string>

extern "C" {
#include "capn-dyn.h"
#include "capnp_c.h"
#include "jsonann.capnp.h"
}

namespace {

std::string to_json(const char *in, struct capn_dyn_error *err) {
  struct capn c;
  capn_ptr st;
  std::string out;
  capn_init_malloc(&c);
  if (capn_dyn_from_json(&Doc_dyn, in, strlen(in), capn_root(&c).seg, &st, err) == 0) {
    char *j = capn_dyn_to_json(&Doc_dyn, st, CAPN_DYN_COMPACT);
    if (j != nullptr) {
      out = j;
      free(j);
    }
  }
  capn_free(&c);
  return out;
}

const struct capn_dyn_field *field_of(const struct capn_dyn_struct *s, const char *name) {
  return capn_dyn_find_field(s, name);
}

}  // namespace

TEST(JsonAnn, RenamesFlattensAndEncodesData) {
  const char *in =
      "{\"full_name\":\"Ada\",\"age\":3,\"foo\":1,\"bar\":\"b\",\"qux\":\"q\","
      "\"p.a\":\"A\",\"p.bee\":2,\"tagged\":\"left\",\"payload\":\"L\","
      "\"raw\":[1,2],\"b64\":\"YWI=\",\"hx\":\"00ff\",\"hue\":\"verde\","
      "\"nest\":{\"town\":\"Oslo\"},\"kind\":\"n\",\"n\":9}";
  struct capn_dyn_error err;
  memset(&err, 0, sizeof err);
  std::string got = to_json(in, &err);
  ASSERT_FALSE(got.empty()) << err.message << " at " << err.offset;
  EXPECT_EQ(in, got);

  struct capn c;
  capn_ptr st;
  capn_init_malloc(&c);
  ASSERT_EQ(0, capn_dyn_from_json(&Doc_dyn, in, strlen(in), capn_root(&c).seg, &st, &err))
      << err.message;
  const struct capn_dyn_field *name = field_of(&Doc_dyn, "fullName");
  const struct capn_dyn_field *age = field_of(&Doc_dyn, "age");
  const struct capn_dyn_field *hue = field_of(&Doc_dyn, "hue");
  const struct capn_dyn_field *b64 = field_of(&Doc_dyn, "b64");
  ASSERT_NE(nullptr, name);
  capn_text t = capn_dyn_get_text(st, name);
  EXPECT_EQ(std::string("Ada"), std::string(t.str, t.len));
  EXPECT_EQ(3u, capn_dyn_get_bits(st, age));
  EXPECT_EQ(1u, capn_dyn_get_bits(st, hue));
  capn_ptr data = capn_dyn_get_ptr(st, b64);
  capn_list8 bytes;
  bytes.p = data;
  ASSERT_EQ(2, capn_len(bytes));
  EXPECT_EQ('a', capn_get8(bytes, 0));
  EXPECT_EQ('b', capn_get8(bytes, 1));
  const struct capn_dyn_field *flat = field_of(&Doc_dyn, "flat");
  ASSERT_NE(nullptr, flat);
  EXPECT_EQ(1u, capn_dyn_get_bits(st, field_of(flat->type.st, "foo")));
  capn_free(&c);
}

TEST(JsonAnn, DiscriminatorSelectsVoidAndRenamedArm) {
  const char *void_arm =
      "{\"full_name\":\"\",\"age\":0,\"foo\":0,\"bar\":\"\",\"qux\":\"\","
      "\"p.a\":\"\",\"p.bee\":0,\"tagged\":\"right_n\",\"payload\":4,"
      "\"raw\":[],\"b64\":\"\",\"hx\":\"\",\"hue\":\"red\","
      "\"nest\":{\"town\":\"\"},\"kind\":\"v\"}";
  struct capn_dyn_error err;
  memset(&err, 0, sizeof err);
  std::string got = to_json(void_arm, &err);
  ASSERT_FALSE(got.empty()) << err.message << " at " << err.offset;
  EXPECT_EQ(void_arm, got);
}

TEST(JsonAnn, FlattenedDefaultStructStaysImmutable) {
  const char *in = "{\"value\":\"changed\"}";
  struct capn_dyn_error err;
  struct capn c;
  capn_ptr st;
  memset(&err, 0, sizeof err);
  capn_init_malloc(&c);
  ASSERT_EQ(0, capn_dyn_from_json(&Doc_dyn, in, strlen(in), capn_root(&c).seg, &st, &err))
      << err.message;

  const struct capn_dyn_field *nest = field_of(&Doc_dyn, "defaultNest");
  ASSERT_NE(nullptr, nest);
  const struct capn_dyn_field *value = field_of(nest->type.st, "value");
  ASSERT_NE(nullptr, value);
  capn_text decoded = capn_dyn_get_text(capn_dyn_get_ptr(st, nest), value);
  EXPECT_EQ("changed", std::string(decoded.str, decoded.len));

  capn_ptr empty = capn_new_struct(capn_root(&c).seg, 0, 1);
  capn_text def = capn_dyn_get_text(capn_dyn_get_ptr(empty, nest), value);
  EXPECT_EQ("default", std::string(def.str, def.len));
  capn_free(&c);
}
