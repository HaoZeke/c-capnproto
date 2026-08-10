/* extraheader-test.cpp
 *
 * Checks that $C.extraheader and $C.extendedattribute are applied to
 * generated C sources, and that fieldgetset accessors still work.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "capnp_c.h"
#include "extraheader.capnp.h"

#ifndef EXTRAHEADER_PROBE_INCLUDED
#define EXTRAHEADER_PROBE_INCLUDED 0
#endif

#ifndef EXTRAHEADER_GENERATED_H
#define EXTRAHEADER_GENERATED_H "extraheader.capnp.h"
#endif

static capn_text chars_to_text(const char *chars) {
  return (capn_text) {
    .len = (int) strlen(chars),
    .str = chars,
    .seg = NULL,
  };
}

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

TEST(ExtraHeader, GeneratedHeaderContainsInclude) {
  EXPECT_EQ(1, EXTRAHEADER_PROBE_INCLUDED)
      << "$C.extraheader did not pull extraheader-probe.h into extraheader.capnp.h";

  const char *candidates[] = {
    EXTRAHEADER_GENERATED_H,
    "extraheader.capnp.h",
    "tests/extraheader.capnp.h",
    "../tests/extraheader.capnp.h",
  };
  std::string body;
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
    body = read_file(candidates[i]);
    if (!body.empty()) {
      break;
    }
  }
  ASSERT_FALSE(body.empty()) << "could not read generated extraheader.capnp.h";
  EXPECT_NE(std::string::npos, body.find("#include \"extraheader-probe.h\""))
      << "generated header is missing the extraheader include line";
  EXPECT_NE(std::string::npos, body.find("EXTRAHEADER_EXTATTR"))
      << "generated header is missing the extendedattribute prefix";
}

TEST(ExtraHeader, GetterSetterWorks) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  Widget_ptr w = new_Widget(cs);
  Widget_set_id(w, 42);
  EXPECT_EQ((uint32_t) 42, Widget_get_id(w));

  capn_text name = chars_to_text("box");
  Widget_set_name(w, name);
  capn_text got = Widget_get_name(w);
  EXPECT_EQ((uint32_t) 3, (uint32_t) got.len);
  ASSERT_TRUE(got.str != NULL);
  EXPECT_EQ(0, memcmp(got.str, "box", 3));

  capn_free(&c);
}
