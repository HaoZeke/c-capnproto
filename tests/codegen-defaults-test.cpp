/* codegen-defaults-test.cpp
 *
 * Empty Data/List defaults must compile and round-trip without an
 * undeclared capn_buf. List(Text) fields are capn_ptr_list so capn_len
 * compiles. Integer/enum/float constants are const with a SCREAMING_SNAKE
 * #define.
 */

#include <gtest/gtest.h>
#include <errno.h>
#include <stdio.h>
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

#ifndef TEST_CAPNP_GENERATED_C
#define TEST_CAPNP_GENERATED_C "test.capnp.c"
#endif

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

TEST(CodegenDefaults, StructListUsesNewStructList) {
  const char *candidates[] = {
    CODEGEN_DEFAULTS_GENERATED_C,
    "codegen-defaults.capnp.c",
    "tests/codegen-defaults.capnp.c",
    "../tests/codegen-defaults.capnp.c",
  };
  std::string body = read_generated(candidates, sizeof(candidates) / sizeof(candidates[0]));
  ASSERT_FALSE(body.empty()) << "could not read generated codegen-defaults.capnp.c";
  EXPECT_NE(std::string::npos, body.find("capn_new_struct_list(s, len,"))
      << "generated new_*_list must call capn_new_struct_list";
  EXPECT_EQ(std::string::npos, body.find("capn_new_list(s, len,"))
      << "generated new_*_list must not call size-based capn_new_list";
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

TEST(CodegenDefaults, TextDefaultSurvivesReadWriteRoundTrip) {
  struct capn src;
  capn_init_malloc(&src);
  Greeting_ptr g = new_Greeting(capn_root(&src).seg);
  capn_set_root(&src, g.p);
  ASSERT_EQ(capn_getp(g.p, 0, 1).type, CAPN_NULL);

  struct Greeting read;
  memset(&read, 0, sizeof(read));
  read_Greeting(&read, g);
  ASSERT_STREQ(read.hello.str, "hi");
  ASSERT_STREQ(read.plain.str, "");

  struct capn dst;
  capn_init_malloc(&dst);
  Greeting_ptr copy = new_Greeting(capn_root(&dst).seg);
  write_Greeting(&read, copy);
  capn_set_root(&dst, copy.p);

  EXPECT_EQ(capn_getp(copy.p, 0, 1).type, CAPN_NULL);
  EXPECT_EQ(capn_getp(copy.p, 1, 1).type, CAPN_NULL);

  struct Greeting again;
  memset(&again, 0, sizeof(again));
  read_Greeting(&again, copy);
  EXPECT_STREQ(again.hello.str, "hi");

  capn_free(&dst);
  capn_free(&src);
}

TEST(CodegenDefaults, ExplicitEmptyTextOverridesDefault) {
  struct capn c;
  capn_init_malloc(&c);
  Greeting_ptr g = new_Greeting(capn_root(&c).seg);
  struct Greeting s;
  memset(&s, 0, sizeof(s));
  s.hello = chars_to_text("");
  write_Greeting(&s, g);

  EXPECT_EQ(capn_getp(g.p, 0, 1).type, CAPN_LIST);
  struct Greeting back;
  memset(&back, 0, sizeof(back));
  read_Greeting(&back, g);
  EXPECT_EQ(back.hello.len, 0);
  capn_free(&c);
}

TEST(CodegenDefaults, ReservedNamesAndVoidOnlyUnionCompile) {
  struct capn c;
  capn_init_malloc(&c);
  Names_ptr p = new_Names(capn_root(&c).seg);

  struct Names in;
  memset(&in, 0, sizeof(in));
  in._which = 7;
  in._errno = 42;
  in._linux = 1;
  in._stdin = chars_to_text("in");
  in.voids_which = Names_voids_b;
  write_Names(&in, p);

  struct Names out;
  memset(&out, 0, sizeof(out));
  read_Names(&out, p);
  EXPECT_EQ(7, out._which);
  EXPECT_EQ(42u, out._errno);
  EXPECT_EQ(1u, out._linux);
  EXPECT_STREQ("in", out._stdin.str);
  EXPECT_EQ(Names_voids_b, out.voids_which);
  capn_free(&c);
}

TEST(CodegenDefaults, ConstantInitializersNameEveryField) {
  const char *candidates[] = {
    TEST_CAPNP_GENERATED_C,
    "test.capnp.c",
    "compiler/test.capnp.c",
    "../compiler/test.capnp.c",
  };
  std::string body = read_generated(candidates, sizeof(candidates) / sizeof(candidates[0]));
  ASSERT_FALSE(body.empty()) << "could not read generated test.capnp.c";
  size_t checked = 0;
  size_t pos = 0;
  while (pos < body.size()) {
    size_t eol = body.find('\n', pos);
    if (eol == std::string::npos) {
      eol = body.size();
    }
    std::string line = body.substr(pos, eol - pos);
    pos = eol + 1;
    bool is_text = line.find("capn_text ") != std::string::npos;
    if (!is_text && line.find("&capn_seg") != std::string::npos) {
      checked++;
      EXPECT_NE(std::string::npos, line.find("&capn_seg,0,0,0}"))
          << "capn_ptr constants must initialise nesting, nesting_valid "
             "and cap_idx, or -Wextra warns in every including translation "
             "unit: " << line;
    }
  }
  EXPECT_GT(checked, 0u);
}

TEST(CodegenDefaults, EmptyPlaceholderIsNotAMergeableStringLiteral) {
  const char *candidates[] = {
    CODEGEN_DEFAULTS_GENERATED_C,
    "codegen-defaults.capnp.c",
    "tests/codegen-defaults.capnp.c",
    "../tests/codegen-defaults.capnp.c",
  };
  std::string body = read_generated(candidates, sizeof(candidates) / sizeof(candidates[0]));
  ASSERT_FALSE(body.empty()) << "could not read generated codegen-defaults.capnp.c";
  EXPECT_NE(std::string::npos, body.find("capn_val0_str[1]"));
  EXPECT_EQ(std::string::npos, body.find("capn_val0 = {0,\"\""))
      << "write_ compares text pointers with the placeholder, so it must "
         "not share an address with a caller's literal \"\" at -O2";
}
