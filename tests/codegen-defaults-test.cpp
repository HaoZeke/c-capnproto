/* codegen-defaults-test.cpp
 *
 * Empty Data/List defaults must compile and round-trip without an
 * undeclared capn_buf. List(Text) fields are capn_ptr_list so capn_len
 * compiles. Integer/enum/float constants are const with a SCREAMING_SNAKE
 * #define.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

#include "capnp_c.h"
#include "codegen-defaults.capnp.h"

static capn_text chars_to_text(const char *chars) {
  return (capn_text) {
    .len = (int) strlen(chars),
    .str = chars,
    .seg = NULL,
  };
}

#ifndef CODEGEN_DEFAULTS_GENERATED_C
#define CODEGEN_DEFAULTS_GENERATED_C "codegen-defaults.capnp.c"
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

static std::string read_generated(const char *const *candidates, size_t n) {
  for (size_t i = 0; i < n; i++) {
    std::string body = read_file(candidates[i]);
    if (!body.empty()) {
      return body;
    }
  }
  return std::string();
}

TEST(CodegenDefaults, EmptyDataGeneratedCOmitsUndeclaredCapnBuf) {
  const char *candidates[] = {
    CODEGEN_DEFAULTS_GENERATED_C,
    "codegen-defaults.capnp.c",
    "tests/codegen-defaults.capnp.c",
    "../tests/codegen-defaults.capnp.c",
  };
  std::string body = read_generated(candidates, sizeof(candidates) / sizeof(candidates[0]));
  ASSERT_FALSE(body.empty()) << "could not read generated codegen-defaults.capnp.c";

  bool uses_buf = body.find("capn_buf") != std::string::npos;
  bool declares_buf = body.find("static const uint8_t capn_buf") != std::string::npos;
  if (uses_buf) {
    EXPECT_TRUE(declares_buf)
        << "generated .c references capn_buf without declaring it";
  }
}

TEST(CodegenDefaults, EmptyDataDefaultRoundTrip) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  Rec_ptr p = new_Rec(cs);
  struct Rec unset;
  read_Rec(&unset, p);
  EXPECT_EQ(0, unset.info.p.len);
  EXPECT_EQ(0, capn_len(unset.info));

  struct Rec w;
  memset(&w, 0, sizeof(w));
  write_Rec(&w, p);

  struct Rec r;
  read_Rec(&r, p);
  EXPECT_EQ(0, r.info.p.len);
  EXPECT_EQ(0, capn_len(r.info));
  EXPECT_EQ(0, capn_len(r.tags));

  uint8_t buf[4096];
  ssize_t sz = capn_write_mem(&c, buf, sizeof(buf), 0);
  ASSERT_GT(sz, 0);

  struct capn rc;
  ASSERT_EQ(0, capn_init_mem(&rc, buf, (size_t) sz, 0));
  Rec_ptr rp;
  rp.p = capn_getp(capn_root(&rc), 0, 1);
  struct Rec rr;
  read_Rec(&rr, rp);
  EXPECT_EQ(0, rr.info.p.len);
  EXPECT_EQ(0, capn_len(rr.info));

  capn_free(&rc);
  capn_free(&c);
}

TEST(CodegenDefaults, ListTextCapnLen) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  struct Event e;
  memset(&e, 0, sizeof(e));
  e.args.p = capn_new_ptr_list(cs, 3);
  ASSERT_EQ(0, capn_set_text(e.args.p, 0, chars_to_text("one")));
  ASSERT_EQ(0, capn_set_text(e.args.p, 1, chars_to_text("two")));
  ASSERT_EQ(0, capn_set_text(e.args.p, 2, chars_to_text("three")));
  EXPECT_EQ(3, capn_len(e.args));
  EXPECT_EQ(3, capn_ptr_len(e.args.p));

  Event_ptr ep = new_Event(cs);
  write_Event(&e, ep);
  int setp_ret = capn_setp(capn_root(&c), 0, ep.p);
  ASSERT_EQ(0, setp_ret);

  struct Event r;
  read_Event(&r, ep);
  EXPECT_EQ(3, capn_len(r.args));
  EXPECT_EQ(3, capn_ptr_len(r.args.p));

  capn_text t0 = capn_get_text(r.args.p, 0, (capn_text){0, NULL, NULL});
  EXPECT_EQ(3, t0.len);
  ASSERT_TRUE(t0.str != NULL);
  EXPECT_EQ(0, memcmp(t0.str, "one", 3));

  capn_free(&c);
}

TEST(CodegenDefaults, GeneratedConstantsAreConst) {
  EXPECT_TRUE((std::is_const<decltype(answer)>::value));
  EXPECT_TRUE((std::is_const<decltype(flag)>::value));
  EXPECT_TRUE((std::is_const<decltype(count)>::value));
  EXPECT_TRUE((std::is_const<decltype(big)>::value));
  EXPECT_TRUE((std::is_const<decltype(hue)>::value));
  EXPECT_TRUE((std::is_const<decltype(ratio)>::value));
  EXPECT_TRUE((std::is_const<decltype(answerCamel)>::value));

  EXPECT_EQ(42, answer);
  EXPECT_EQ(42, ANSWER);
  EXPECT_EQ(1u, flag);
  EXPECT_EQ(1, FLAG);
  EXPECT_EQ(7u, count);
  EXPECT_EQ(7u, COUNT);
  EXPECT_EQ(UINT64_C(0x100000000), big);
  EXPECT_EQ(UINT64_C(0x100000000), BIG);
  EXPECT_EQ(Color_green, hue);
  EXPECT_EQ(1u, HUE);
  EXPECT_FLOAT_EQ(1.5f, ratio.f);
  EXPECT_EQ(99, answerCamel);
  EXPECT_EQ(99, ANSWER_CAMEL);
}
