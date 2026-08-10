/* codegen-defaults-test.cpp
 *
 * Empty Data/List defaults must compile and round-trip without an
 * undeclared capn_buf.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "capnp_c.h"
#include "codegen-defaults.capnp.h"

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
  EXPECT_EQ(0, r.tags.type == 0 ? 0 : r.tags.len);

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
