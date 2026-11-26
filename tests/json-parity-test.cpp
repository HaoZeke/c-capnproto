/* Checks JSON encode and decode against Cap'n Proto 1.4.0 compat/json.c++. */

#include <gtest/gtest.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

extern "C" {
#include "capn-dyn.h"
#include "capnp_c.h"
#include "jsonann.capnp.h"
#include "reflect.capnp.h"
}

namespace {

struct capn_dyn_error err;

void clear_err() { memset(&err, 0, sizeof err); }

int decode(const struct capn_dyn_struct *s, const char *json, struct capn *c, capn_ptr *st) {
  clear_err();
  capn_init_malloc(c);
  return capn_dyn_from_json(s, json, strlen(json), capn_root(c).seg, st, &err);
}

std::string canonical(struct capn *c) {
  struct capn d;
  std::string out;
  capn_init_malloc(&d);
  if (capn_canonicalize(c, &d) == 0 && d.seglist != NULL)
    out.assign(d.seglist->data, d.seglist->len);
  capn_free(&d);
  return out;
}

std::string hex_of(const std::string &b) {
  static const char *digits = "0123456789abcdef";
  std::string h;
  h.resize(b.size() * 2);
  for (size_t i = 0; i < b.size(); i++) {
    unsigned char c = (unsigned char) b[i];
    h[i * 2] = digits[c >> 4];
    h[i * 2 + 1] = digits[c & 15];
  }
  return h;
}

const struct capn_dyn_field *need(const struct capn_dyn_struct *s, const char *name) {
  const struct capn_dyn_field *f = capn_dyn_find_field(s, name);
  EXPECT_NE(nullptr, f) << name;
  return f;
}

}  // namespace

TEST(JsonParity, UnquotedInt64AndUint64MaxStayOutOfRange) {
  struct capn c;
  capn_ptr st;
  const struct capn_dyn_field *i64 = need(&Defaults_dyn, "i64");
  const struct capn_dyn_field *u64 = need(&Defaults_dyn, "u64");

  ASSERT_NE(0, decode(&Defaults_dyn, "{\"i64\":9223372036854775807}", &c, &st));
  capn_free(&c);
  ASSERT_NE(0, decode(&Defaults_dyn, "{\"u64\":18446744073709551615}", &c, &st));
  capn_free(&c);

  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"i64\":9223372036854774784}", &c, &st)) << err.message;
  EXPECT_EQ(9223372036854774784ll, capn_dyn_bits_to_i64(i64, capn_dyn_get_bits(st, i64)));
  capn_free(&c);

  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"u64\":18446744073709549568}", &c, &st)) << err.message;
  EXPECT_EQ(18446744073709549568ull, capn_dyn_get_bits(st, u64));
  capn_free(&c);
}

TEST(JsonParity, NumbersRoundThroughDouble) {
  struct capn c;
  capn_ptr st;
  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"i64\": 9007199254740993}", &c, &st)) << err.message;
  const struct capn_dyn_field *i64 = need(&Defaults_dyn, "i64");
  EXPECT_EQ(9007199254740992ll, capn_dyn_bits_to_i64(i64, capn_dyn_get_bits(st, i64)));
  capn_free(&c);

  ASSERT_NE(0, decode(&Defaults_dyn, "{\"u64\": 18446744073709551615}", &c, &st));
  EXPECT_NE('\0', err.message[0]);
  capn_free(&c);
}

TEST(JsonParity, QuotedIntegersStayExact) {
  struct capn c;
  capn_ptr st;
  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"i64\":\"-9223372036854775808\"}", &c, &st)) << err.message;
  const struct capn_dyn_field *i64 = need(&Defaults_dyn, "i64");
  EXPECT_EQ(INT64_MIN, capn_dyn_bits_to_i64(i64, capn_dyn_get_bits(st, i64)));
  capn_free(&c);

  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"u64\":\"18446744073709551615\"}", &c, &st)) << err.message;
  const struct capn_dyn_field *u64 = need(&Defaults_dyn, "u64");
  EXPECT_EQ(~0ull, capn_dyn_get_bits(st, u64));
  capn_free(&c);
}

TEST(JsonParity, NullRejectedForTextDataAndList) {
  struct capn c;
  capn_ptr st;
  ASSERT_NE(0, decode(&Defaults_dyn, "{\"t\": null}", &c, &st));
  capn_free(&c);
  ASSERT_NE(0, decode(&Defaults_dyn, "{\"d\": null}", &c, &st));
  capn_free(&c);
  ASSERT_NE(0, decode(&Defaults_dyn, "{\"lst\": null}", &c, &st));
  capn_free(&c);
}

TEST(JsonParity, NullOnVoidAndIntegerStay) {
  struct capn c;
  capn_ptr st;
  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"v\": null}", &c, &st)) << err.message;
  capn_free(&c);
  ASSERT_NE(0, decode(&Defaults_dyn, "{\"i8\": null}", &c, &st));
  capn_free(&c);
}

TEST(JsonParity, UnannotatedDataRejectsAString) {
  struct capn c;
  capn_ptr st;
  ASSERT_NE(0, decode(&Defaults_dyn, "{\"d\": \"ab\"}", &c, &st));
  capn_free(&c);
}

TEST(JsonParity, DataArrayBase64AndHexStay) {
  struct capn c;
  capn_ptr st;
  capn_list8 bytes;
  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"d\": [1, 2]}", &c, &st)) << err.message;
  bytes.p = capn_dyn_get_ptr(st, need(&Defaults_dyn, "d"));
  ASSERT_EQ(2, capn_len(bytes));
  EXPECT_EQ(1, capn_get8(bytes, 0));
  EXPECT_EQ(2, capn_get8(bytes, 1));
  capn_free(&c);

  ASSERT_EQ(0, decode(&Doc_dyn, "{\"b64\":\"YWI=\",\"hx\":\"61\",\"hue\":\"verde\"}", &c, &st))
      << err.message;
  bytes.p = capn_dyn_get_ptr(st, need(&Doc_dyn, "b64"));
  ASSERT_EQ(2, capn_len(bytes));
  EXPECT_EQ('a', capn_get8(bytes, 0));
  EXPECT_EQ('b', capn_get8(bytes, 1));
  bytes.p = capn_dyn_get_ptr(st, need(&Doc_dyn, "hx"));
  ASSERT_EQ(1, capn_len(bytes));
  EXPECT_EQ('a', capn_get8(bytes, 0));
  EXPECT_EQ(1u, capn_dyn_get_bits(st, need(&Doc_dyn, "hue")));
  capn_free(&c);
}

TEST(JsonParity, UnknownFieldsAreIgnored) {
  struct capn c;
  capn_ptr st;
  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"nope\": 1}", &c, &st)) << err.message;
  capn_free(&c);
  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"i8\": 3, \"nope\": 1}", &c, &st)) << err.message;
  EXPECT_EQ(3, capn_dyn_bits_to_i64(need(&Defaults_dyn, "i8"),
                                   capn_dyn_get_bits(st, need(&Defaults_dyn, "i8"))));
  capn_free(&c);
}

TEST(JsonParity, TextUnknownFieldStaysRejected) {
  struct capn c;
  capn_ptr st;
  clear_err();
  capn_init_malloc(&c);
  EXPECT_NE(0, capn_dyn_from_text(&Defaults_dyn, "(nope = 1)", strlen("(nope = 1)"),
                                 capn_root(&c).seg, &st, &err));
  capn_free(&c);
}

TEST(JsonParity, NegativeLeadingZeroMatchesCpp) {
  struct capn c;
  capn_ptr st;
  ASSERT_NE(0, decode(&Defaults_dyn, "{\"i8\":-010}", &c, &st));
  capn_free(&c);
}

TEST(JsonParity, StructNullIsRejected) {
  struct capn c;
  capn_ptr st;
  ASSERT_NE(0, decode(&Nested_dyn, "{\"i\":null}", &c, &st));
  capn_free(&c);
}

TEST(JsonParity, NestedPrettyJsonMatchesCpp) {
  struct capn c;
  capn_ptr st;
  const char *text =
      "(label = \"root\", n = 1, kids = ["
      "(label = \"abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUV\", n = 2),"
      "(label = \"beta\", n = 3, kids = ["
      "(label = \"gamma\", n = 4),"
      "(label = \"delta\", n = 5)])])";
  const char *pretty =
      "{ \"label\": \"root\",\n"
      "  \"kids\": [\n"
      "    { \"label\": \"abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUV\",\n"
      "      \"n\": 2 },\n"
      "    { \"label\": \"beta\",\n"
      "      \"kids\": [{\"label\": \"gamma\", \"n\": 4}, {\"label\": \"delta\", \"n\": 5}],\n"
      "      \"n\": 3 } ],\n"
      "  \"n\": 1 }";
  clear_err();
  capn_init_malloc(&c);
  ASSERT_EQ(0, capn_dyn_from_text(&Node_dyn, text, strlen(text), capn_root(&c).seg, &st, &err))
      << err.message;
  char *wide = capn_dyn_to_json(&Node_dyn, st, 0);
  ASSERT_NE(nullptr, wide);
  EXPECT_EQ(pretty, std::string(wide));
  free(wide);
  capn_free(&c);
}

TEST(JsonParity, LeadingZeroInAJsonNumberIsRejected) {
  struct capn c;
  capn_ptr st;
  ASSERT_NE(0, decode(&Defaults_dyn, "{\"i8\": 010}", &c, &st));
  capn_free(&c);
}

TEST(JsonParity, ZeroPointFiveAndNegativeZeroStay) {
  struct capn c;
  capn_ptr st;
  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"i8\": 0}", &c, &st)) << err.message;
  EXPECT_EQ(0, capn_dyn_bits_to_i64(need(&Defaults_dyn, "i8"),
                                   capn_dyn_get_bits(st, need(&Defaults_dyn, "i8"))));
  capn_free(&c);

  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"f64\": 0.5}", &c, &st)) << err.message;
  EXPECT_DOUBLE_EQ(0.5, capn_dyn_bits_to_f64(need(&Defaults_dyn, "f64"),
                                            capn_dyn_get_bits(st, need(&Defaults_dyn, "f64"))));
  capn_free(&c);

  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"f64\": -0}", &c, &st)) << err.message;
  {
    double z = capn_dyn_bits_to_f64(need(&Defaults_dyn, "f64"),
                                   capn_dyn_get_bits(st, need(&Defaults_dyn, "f64")));
    EXPECT_TRUE(z == 0.0);
    EXPECT_TRUE(std::signbit(z));
  }
  capn_free(&c);

  ASSERT_EQ(0, decode(&Defaults_dyn, "{\"i8\": -0}", &c, &st)) << err.message;
  EXPECT_EQ(0, capn_dyn_bits_to_i64(need(&Defaults_dyn, "i8"),
                                   capn_dyn_get_bits(st, need(&Defaults_dyn, "i8"))));
  capn_free(&c);
}

TEST(JsonParity, TextLeadingZeroIsOctal) {
  struct capn c;
  capn_ptr st;
  clear_err();
  capn_init_malloc(&c);
  EXPECT_EQ(0, capn_dyn_from_text(&Defaults_dyn, "(i8 = 010)", strlen("(i8 = 010)"),
                                 capn_root(&c).seg, &st, &err))
      << err.message;
  EXPECT_EQ(8, capn_dyn_bits_to_i64(need(&Defaults_dyn, "i8"),
                                   capn_dyn_get_bits(st, need(&Defaults_dyn, "i8"))));
  capn_free(&c);
}

TEST(JsonParity, SurrogatePairMatchesCppBytes) {
  const char *json =
      "{\"t\": \"\\u00e9\\ud83d\\ude00\\n\", \"i64\": 9007199254740993,"
      " \"u64\": \"18446744073709551615\", \"f64\": \"-Infinity\", \"v\": null}";
  const char *want =
      "0000000006000100000000000000000000c08782feffdfff0000000000000000"
      "fffff73a5e2733060000000000000000000000000000f23f0100000052000000"
      "c3a9eda0bdedb8800a00000000000000";
  struct capn c;
  capn_ptr st;
  ASSERT_EQ(0, decode(&Defaults_dyn, json, &c, &st)) << err.message;
  ASSERT_EQ(0, capn_set_root(&c, st));
  capn_text t = capn_dyn_get_text(st, need(&Defaults_dyn, "t"));
  ASSERT_EQ(9, t.len);
  EXPECT_EQ(0, memcmp(t.str, "\xc3\xa9\xed\xa0\xbd\xed\xb8\x80\n", 9));
  EXPECT_EQ(9007199254740992ll,
            capn_dyn_bits_to_i64(need(&Defaults_dyn, "i64"),
                                capn_dyn_get_bits(st, need(&Defaults_dyn, "i64"))));
  std::string got = canonical(&c);
  EXPECT_EQ(want, hex_of(got));
  capn_free(&c);
}

TEST(JsonParity, PrettyJsonMatchesConvertBinaryJson) {
  struct capn c;
  capn_ptr st;
  const char *text =
      "(f32 = [0.1, -0.0, nan, inf], f64 = [1e-5, -0.0], b = [true, false], v = [void])";
  const char *pretty =
      "{ \"v\": [null],\n"
      "  \"b\": [true, false],\n"
      "  \"f32\": [0.10000000149011612, -0, \"NaN\", \"Infinity\"],\n"
      "  \"f64\": [1e-05, -0] }";
  clear_err();
  capn_init_malloc(&c);
  ASSERT_EQ(0, capn_dyn_from_text(&Lists_dyn, text, strlen(text), capn_root(&c).seg, &st, &err))
      << err.message;
  char *wide = capn_dyn_to_json(&Lists_dyn, st, 0);
  ASSERT_NE(nullptr, wide);
  EXPECT_EQ(pretty, std::string(wide));
  free(wide);
  capn_free(&c);
}

TEST(JsonParity, CompactJsonStaysByteIdentical) {
  struct capn c;
  capn_ptr st;
  const char *text =
      "(f32 = [0.1, -0.0, nan, inf], f64 = [1e-5, -0.0], b = [true, false], v = [void])";
  const char *compact =
      "{\"v\":[null],\"b\":[true,false],\"f32\":[0.10000000149011612,-0,\"NaN\",\"Infinity\"],"
      "\"f64\":[1e-05,-0]}";
  clear_err();
  capn_init_malloc(&c);
  ASSERT_EQ(0, capn_dyn_from_text(&Lists_dyn, text, strlen(text), capn_root(&c).seg, &st, &err))
      << err.message;
  char *tight = capn_dyn_to_json(&Lists_dyn, st, CAPN_DYN_COMPACT);
  ASSERT_NE(nullptr, tight);
  EXPECT_EQ(compact, std::string(tight));
  free(tight);
  capn_free(&c);
}

TEST(JsonParity, ShortPrettyObjectsStayOneLine) {
  struct capn c;
  capn_ptr st;
  const char *unions = "(before = 1, c = (x = 5, y = 6))";
  const char *one_line =
      "{\"before\": 1, \"after\": 0, \"mid\": {\"p\": false}, "
      "\"grp\": {\"m\": 3, \"inner\": {\"z\": true}}, \"c\": {\"x\": \"5\", \"y\": \"6\"}}";
  clear_err();
  capn_init_malloc(&c);
  ASSERT_EQ(0, capn_dyn_from_text(&Unions_dyn, unions, strlen(unions), capn_root(&c).seg, &st, &err))
      << err.message;
  char *wide = capn_dyn_to_json(&Unions_dyn, st, 0);
  ASSERT_NE(nullptr, wide);
  EXPECT_EQ(one_line, std::string(wide));
  free(wide);
  capn_free(&c);
}

TEST(JsonParity, FlattenedUnionValueWaitsForItsTag) {
  struct capn c;
  capn_ptr st;
  const char *doc =
      "{\"full_name\": \"A\", \"hue\": \"verde\", \"b64\": \"YWI=\", \"hx\": \"61\","
      " \"kind\": \"t\", \"payload\": \"zz\"}";
  const char *want =
      "000000000300090000000100000000000000000000000000000001000000000021000000"
      "120000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000900000012000000090000000a00000041000000"
      "0000000061620000000000006100000000000000";

  ASSERT_EQ(0, decode(&Doc_dyn, "{\"payload\":\"zz\"}", &c, &st)) << err.message;
  {
    const struct capn_dyn_struct *tagged = need(&Doc_dyn, "tagged")->type.st;
    const struct capn_dyn_field *left = need(tagged, "left");
    EXPECT_EQ(0, capn_dyn_has(tagged, st, left));
  }
  capn_free(&c);

  ASSERT_EQ(0, decode(&Doc_dyn, "{\"kind\":\"t\",\"payload\":\"zz\"}", &c, &st)) << err.message;
  {
    const struct capn_dyn_field *arm = need(&Doc_dyn, "t");
    const struct capn_dyn_struct *tagged = need(&Doc_dyn, "tagged")->type.st;
    EXPECT_EQ((int) arm->discriminant, capn_dyn_which(&Doc_dyn, st));
    EXPECT_EQ(0, capn_dyn_has(tagged, st, need(tagged, "left")));
  }
  capn_free(&c);

  ASSERT_EQ(0, decode(&Doc_dyn, doc, &c, &st)) << err.message;
  ASSERT_EQ(0, capn_set_root(&c, st));
  EXPECT_EQ(std::string(want), hex_of(canonical(&c)));
  capn_free(&c);
}

TEST(JsonParity, UntaggedFlattenedUnionMatchesCppCanonical) {
  struct capn c;
  capn_ptr st;
  /* capnp convert json:canonical, Cap'n Proto C++ 1.4.0. */
  const char *arm_empty = "fcffffff00000000";
  const char *box_empty = "0000000000000100fcffffff00000000";

  ASSERT_EQ(0, decode(&ArmBox_dyn, "{\"value\":\"zz\"}", &c, &st)) << err.message;
  EXPECT_EQ(0, capn_dyn_which(&ArmBox_dyn, st));
  {
    const struct capn_dyn_field *bar = need(&ArmBox_dyn, "bar");
    capn_ptr p = capn_dyn_get_ptr(st, bar);
    EXPECT_EQ(CAPN_NULL, p.type);
    EXPECT_EQ(0, capn_dyn_has(&ArmBox_dyn, st, bar));
  }
  ASSERT_EQ(0, capn_set_root(&c, st));
  EXPECT_EQ(arm_empty, hex_of(canonical(&c)));
  capn_free(&c);

  ASSERT_EQ(0, decode(&ArmBox_dyn, "{\"type\":\"bar\",\"value\":\"zz\"}", &c, &st)) << err.message;
  {
    const struct capn_dyn_field *bar = need(&ArmBox_dyn, "bar");
    const struct capn_dyn_struct *pay = bar->type.st;
    capn_ptr p = capn_dyn_get_ptr(st, bar);
    capn_text t;
    EXPECT_EQ((int) bar->discriminant, capn_dyn_which(&ArmBox_dyn, st));
    ASSERT_NE(CAPN_NULL, p.type);
    t = capn_dyn_get_text(p, need(pay, "value"));
    EXPECT_EQ(std::string("zz"), std::string(t.str ? t.str : "", (size_t) t.len));
  }
  capn_free(&c);

  ASSERT_EQ(0, decode(&BoxU_dyn, "{\"payload\":\"zz\"}", &c, &st)) << err.message;
  {
    const struct capn_dyn_field *box = need(&BoxU_dyn, "box");
    const struct capn_dyn_struct *inner = box->type.st;
    capn_ptr p = capn_dyn_get_ptr(st, box);
    capn_text t;
    ASSERT_NE(CAPN_NULL, p.type);
    EXPECT_EQ(0, capn_dyn_which(inner, p));
    t = capn_dyn_get_text(p, need(inner, "left"));
    EXPECT_EQ(0, t.len);
  }
  ASSERT_EQ(0, capn_set_root(&c, st));
  EXPECT_EQ(box_empty, hex_of(canonical(&c)));
  capn_free(&c);

  ASSERT_EQ(0, decode(&BoxU_dyn, "{\"kind\":\"left\",\"payload\":\"zz\"}", &c, &st)) << err.message;
  {
    const struct capn_dyn_field *box = need(&BoxU_dyn, "box");
    capn_ptr p = capn_dyn_get_ptr(st, box);
    const struct capn_dyn_struct *inner = box->type.st;
    capn_text t;
    ASSERT_NE(CAPN_NULL, p.type);
    t = capn_dyn_get_text(p, need(inner, "left"));
    EXPECT_EQ(std::string("zz"), std::string(t.str ? t.str : "", (size_t) t.len));
  }
  capn_free(&c);
}

TEST(JsonParity, FlattenedUnionTagAndValueStay) {
  struct capn c;
  capn_ptr st;
  ASSERT_EQ(0, decode(&Doc_dyn, "{\"tagged\":\"left\",\"payload\":\"zz\"}", &c, &st)) << err.message;
  {
    const struct capn_dyn_struct *tagged = need(&Doc_dyn, "tagged")->type.st;
    const struct capn_dyn_field *left = need(tagged, "left");
    EXPECT_EQ(1, capn_dyn_has(tagged, st, left));
    capn_text t = capn_dyn_get_text(st, left);
    EXPECT_EQ(std::string("zz"), std::string(t.str, t.len));
  }
  capn_free(&c);

  ASSERT_EQ(0, decode(&Doc_dyn, "{\"payload\":\"zz\",\"tagged\":\"left\"}", &c, &st)) << err.message;
  {
    const struct capn_dyn_struct *tagged = need(&Doc_dyn, "tagged")->type.st;
    capn_text t = capn_dyn_get_text(st, need(tagged, "left"));
    EXPECT_EQ(std::string("zz"), std::string(t.str, t.len));
  }
  capn_free(&c);
}
