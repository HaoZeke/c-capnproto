/* Compare branded list reads and struct-list-as-text with capnp 1.4.0. */

#include "capnp-cli.h"

extern "C" {
#include "capn-dyn.h"
#include "reflect.capnp.h"
}

namespace {

std::string reflect_schema() {
  return join_path(source_root(), "tests/reflect.capnp");
}

std::string strip(std::string s) {
  while (!s.empty() && (s.back() == '\n' || s.back() == ' '))
    s.pop_back();
  return s;
}

std::vector<std::string> cli(const char *verb, const char *spec, const char *typ,
                             bool compact) {
  std::vector<std::string> argv;
  argv.push_back(capnp_bin());
  argv.push_back(verb);
  if (spec != nullptr)
    argv.push_back(spec);
  if (compact)
    argv.push_back("--short");
  argv.push_back("-I");
  argv.push_back(include_dir());
  argv.push_back(reflect_schema());
  argv.push_back(typ);
  return argv;
}

void expect_as(const char *src_type, const char *text,
               const struct capn_dyn_struct *desc, const char *dst_type) {
  CmdResult enc = run_argv(cli("encode", nullptr, src_type, false), text, strlen(text));
  ASSERT_EQ(0, enc.status) << text << "\n" << enc.err;
  struct capn m;
  ASSERT_EQ(0, capn_init_mem(&m, (const uint8_t *) enc.out.data(), enc.out.size(), 0));
  capn_ptr root = capn_getp(capn_root(&m), 0, 1);
  capn_clear_err(&m);

  char *got = capn_dyn_to_text(desc, root);
  ASSERT_NE(nullptr, got);
  CmdResult want = run_argv(cli("decode", nullptr, dst_type, true),
                            enc.out.data(), enc.out.size());
  ASSERT_EQ(0, want.status) << want.err;
  EXPECT_EQ(strip(want.out), got) << src_type << " as " << dst_type;
  EXPECT_EQ(1, capn_ok(&m)) << src_type << " as " << dst_type;
  free(got);

  capn_clear_err(&m);
  char *json = capn_dyn_to_json(desc, root, CAPN_DYN_COMPACT);
  ASSERT_NE(nullptr, json);
  CmdResult want_json = run_argv(cli("convert", "binary:json", dst_type, true),
                                 enc.out.data(), enc.out.size());
  ASSERT_EQ(0, want_json.status) << want_json.err;
  EXPECT_EQ(strip(want_json.out), json) << src_type << " as " << dst_type;
  EXPECT_EQ(1, capn_ok(&m));
  free(json);
  capn_free(&m);
}

}  // namespace

class ListReadParityCli : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!capnp_present())
      GTEST_SKIP() << "capnp CLI not found";
  }
};

TEST_F(ListReadParityCli, BoundDataStructListMatchesCapnp) {
  expect_as("UsesBrandLists",
            "(leaves = (items = [(x = 1)]), texts = (items = [\"a\"]))",
            &UsesBrandLists_dyn, "UsesBrandLists");
}

TEST_F(ListReadParityCli, StructListAsTextMatchesCapnp) {
  expect_as("LST", "(xs = [(t = \"a\"), (t = \"bb\")])", &LT_dyn, "LT");
}

TEST_F(ListReadParityCli, TextListAsStructListMatchesCapnp) {
  expect_as("LT", "(xs = [\"a\", \"bb\"])", &LST_dyn, "LST");
}
