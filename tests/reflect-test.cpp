/* reflect-test.cpp
 *
 * capn-dyn against the official tool: for each value, the text and JSON
 * the C descriptors produce must equal `capnp decode --short` and
 * `capnp convert binary:json --short` byte for byte, and the decoders
 * must rebuild a message the tool reads back as the same value.
 *
 * Built into the CLI interop binary, so it shares its helpers and only
 * exists where meson found `capnp`.
 */

#include "capnp-cli.h"

extern "C" {
#include "capn-dyn.h"
#include "reflect.capnp.h"
#include "reflect-other.capnp.h"
}

namespace {

std::string reflect_schema() {
  return join_path(source_root(), "tests/reflect.capnp");
}

std::vector<std::string> cli(const char *verb, const char *spec, const char *typ,
                             bool compact) {
  std::vector<std::string> argv;
  argv.push_back(capnp_bin());
  argv.push_back(verb);
  if (spec != nullptr) {
    argv.push_back(spec);
  }
  if (compact) {
    argv.push_back("--short");
  }
  argv.push_back("-I");
  argv.push_back(include_dir());
  argv.push_back(reflect_schema());
  argv.push_back(typ);
  return argv;
}

struct Case {
  const char *type;
  const struct capn_dyn_struct *desc;
  const char *text;
};

class Reflect : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!capnp_present()) {
      GTEST_SKIP() << "capnp CLI not found";
    }
  }

  /* Encode `text` with the tool and return the framed message. */
  std::string encode(const Case &c) {
    CmdResult r = run_argv(cli("encode", nullptr, c.type, false), c.text,
                           strlen(c.text));
    EXPECT_EQ(0, r.status) << c.text << "\n" << r.err;
    return r.out;
  }

  std::string want_text(const Case &c, const std::string &bin) {
    CmdResult r = run_argv(cli("decode", nullptr, c.type, true), bin.data(),
                           bin.size());
    EXPECT_EQ(0, r.status) << r.err;
    return strip(r.out);
  }

  std::string want_json(const Case &c, const std::string &bin) {
    CmdResult r = run_argv(cli("convert", "binary:json", c.type, true),
                           bin.data(), bin.size());
    EXPECT_EQ(0, r.status) << r.err;
    return strip(r.out);
  }

  static std::string strip(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) {
      s.pop_back();
    }
    return s;
  }

  void check(const Case &c) {
    std::string bin = encode(c);
    ASSERT_FALSE(bin.empty());
    struct capn m;
    ASSERT_EQ(0, capn_init_mem(&m, (const uint8_t *) bin.data(), bin.size(), 0));
    capn_ptr root = capn_getp(capn_root(&m), 0, 1);

    char *text = capn_dyn_to_text(c.desc, root);
    ASSERT_NE(nullptr, text);
    EXPECT_EQ(want_text(c, bin), text) << c.text;
    free(text);

    char *json = capn_dyn_to_json(c.desc, root, CAPN_DYN_COMPACT);
    ASSERT_NE(nullptr, json);
    EXPECT_EQ(want_json(c, bin), json) << c.text;
    free(json);
    capn_free(&m);
  }
};

}  // namespace

static const Case kCases[] = {
  {"Defaults", &Defaults_dyn, "()"},
  {"Defaults", &Defaults_dyn,
   "(b = false, i8 = 1, i64 = -6400000000, u64 = 18446744073709551615,"
   " f32 = 9.5, f64 = 1e100, t = \"tt\\n\\\"q\\\"\\001\", d = \"dd\\377\","
   " e = gamma, lst = [9, 8], lt = [\"z\"], s = (x = 1, name = \"n\"),"
   " bl = [false, true], ll = [[7], []], ls = [(x = 3)], f32b = 1.0, f64b = nan)"},
  {"Unions", &Unions_dyn, "()"},
  {"Unions", &Unions_dyn, "(before = 1, c = (x = 5, y = 6), mid = (r = gamma))"},
  {"Unions", &Unions_dyn,
   "(a = 3, mid = (q = \"abc\"), grp = (m = 1, n = \"x\", inner = (z = false)))"},
  {"Unions", &Unions_dyn, "(e = [1, 2, 3], mid = (p = true))"},
  {"Lists", &Lists_dyn,
   "(v = [void, void], b = [true, false, true, true, false, false, true, false, true],"
   " i8 = [-1, 2], u16 = [65535], i32 = [1, -2], u64 = [18446744073709551615],"
   " f32 = [1.5, nan, inf, -inf, 1e-7, 123456789, 0.1, 1e-45],"
   " f64 = [1e300, -0.0, 0.1, 5e-324, 1e21], t = [\"a\", \"bb\", \"\"],"
   " d = [\"x\", \"yz\"], e = [alpha, gamma], s = [(i32 = 1), (t = \"q\")],"
   " ll = [[1, 2], [], [3]], lll = [[[\"a\"], [\"b\", \"c\"]], []],"
   " ls = [[(before = 1)], [(b = \"x\")]])"},
  {"Nested", &Nested_dyn, "(i = (d = (x = 3)), e = two)"},
  {"Flagged", &Flagged_dyn, "(x = 1, y = \"yy\")"},
  {"Big", &Big_dyn,
   "(a = 1, b = 2, txt = \"hi\", bin = \"01\","
   " kids = [(a = 5, txt = \"k1\"), (a = 6, kids = [(a = 7)])], name = \"nm\")"},
  {"UsesGenerics", &UsesGenerics_dyn,
   "(pair = (first = \"f\", second = [1]), ib = (val = [1, 2], count = 2),"
   " tb = (val = \"t\"), nested = (val = (val = \"n\")), sb = (val = (i32 = 4)))"},
};

TEST_F(Reflect, TextAndJsonMatchTheOfficialTool) {
  for (const Case &c : kCases) {
    check(c);
  }
}

TEST_F(Reflect, FloatsPrintTheWayTheReferenceDoes) {
  /* Six digits when they round-trip, else eight; doubles 15 then 17. */
  static const char *kF32[] = {"1e-7", "1.5e10", "123456789", "0.1", "16777216",
                               "100000", "1e6", "3.4028235e38", "0.6666667",
                               "12345.678", "1e-5", "-0.0095", "1e15"};
  for (const char *v : kF32) {
    std::string t = std::string("(f32 = [") + v + "])";
    Case c = {"Lists", &Lists_dyn, t.c_str()};
    check(c);
  }
  static const char *kF64[] = {"1e-5", "0.30000000000000004", "123456.789", "1e21",
                               "1.7976931348623157e308", "5e-324", "100000"};
  for (const char *v : kF64) {
    std::string t = std::string("(f64 = [") + v + "])";
    Case c = {"Lists", &Lists_dyn, t.c_str()};
    check(c);
  }
}

TEST_F(Reflect, FarPointerMessagesPrintLikeTheTool) {
  /* One segment per object forces far and double-far pointers. */
  struct capn c;
  capn_init_malloc(&c);
  c.create = [](void *, uint32_t, int sz) -> struct capn_segment * {
    sz += (int) sizeof(struct capn_segment);
    struct capn_segment *s = (struct capn_segment *) calloc(1, (size_t) sz);
    s->data = (char *) (s + 1);
    s->cap = (size_t) sz - sizeof(*s);
    s->user = s;
    return s;
  };
  struct capn_segment *cs = capn_root(&c).seg;
  Big_ptr root = new_Big(cs);
  struct Big b;
  memset(&b, 0, sizeof b);
  b.a = 9;
  b.txt = chars_to_text("far");
  b.kids = new_Big_list(cs, 2);
  struct Big k;
  memset(&k, 0, sizeof k);
  k.a = 1;
  k.name = chars_to_text("k0");
  set_Big(&k, b.kids, 0);
  k.a = 2;
  k.name = chars_to_text("k1");
  set_Big(&k, b.kids, 1);
  write_Big(&b, root);
  ASSERT_EQ(0, capn_set_root(&c, root.p));
  ASSERT_GT(c.segnum, 3u);

  uint8_t buf[16384];
  ssize_t sz = capn_write_mem(&c, buf, sizeof buf, 0);
  ASSERT_GT(sz, 0);
  Case cs_case = {"Big", &Big_dyn, ""};
  std::string bin((const char *) buf, (size_t) sz);

  char *text = capn_dyn_to_text(&Big_dyn, capn_getp(capn_root(&c), 0, 1));
  ASSERT_NE(nullptr, text);
  EXPECT_EQ(want_text(cs_case, bin), text);
  free(text);
  capn_free(&c);
}

TEST_F(Reflect, FieldAccessFollowsTheDescriptor) {
  Case c = {"Unions", &Unions_dyn,
            "(before = 42, c = (x = 5, y = -6), after = 2.5, grp = (m = 9, n = \"hi\"))"};
  std::string bin = encode(c);
  struct capn m;
  ASSERT_EQ(0, capn_init_mem(&m, (const uint8_t *) bin.data(), bin.size(), 0));
  capn_ptr root = capn_getp(capn_root(&m), 0, 1);

  const struct capn_dyn_field *before = capn_dyn_find_field(&Unions_dyn, "before");
  const struct capn_dyn_field *grp = capn_dyn_find_field(&Unions_dyn, "grp");
  const struct capn_dyn_field *cf = capn_dyn_find_field(&Unions_dyn, "c");
  const struct capn_dyn_field *a = capn_dyn_find_field(&Unions_dyn, "a");
  ASSERT_NE(nullptr, before);
  ASSERT_NE(nullptr, grp);
  ASSERT_NE(nullptr, cf);
  ASSERT_NE(nullptr, a);
  EXPECT_EQ(nullptr, capn_dyn_find_field(&Unions_dyn, "nope"));

  EXPECT_EQ(42u, capn_dyn_get_bits(root, before));
  EXPECT_EQ((int) cf->discriminant, capn_dyn_which(&Unions_dyn, root));
  EXPECT_TRUE(capn_dyn_is_active(&Unions_dyn, root, cf));
  EXPECT_FALSE(capn_dyn_is_active(&Unions_dyn, root, a));
  EXPECT_FALSE(capn_dyn_has(&Unions_dyn, root, a));
  EXPECT_TRUE(grp->is_group);

  const struct capn_dyn_field *mf = capn_dyn_find_field(grp->type.st, "m");
  const struct capn_dyn_field *nf = capn_dyn_find_field(grp->type.st, "n");
  ASSERT_NE(nullptr, mf);
  EXPECT_EQ(9u, capn_dyn_get_bits(root, mf));
  capn_text n = capn_dyn_get_text(root, nf);
  EXPECT_EQ(2, n.len);
  EXPECT_EQ(0, memcmp(n.str, "hi", 2));

  const struct capn_dyn_field *y = capn_dyn_find_field(cf->type.st, "y");
  EXPECT_EQ(-6, capn_dyn_bits_to_i64(y, capn_dyn_get_bits(root, y)));
  capn_free(&m);
}

TEST_F(Reflect, SettersWriteWhatTheGettersRead) {
  struct capn c;
  capn_init_malloc(&c);
  Unions_ptr root = new_Unions(capn_root(&c).seg);
  const struct capn_dyn_field *before = capn_dyn_find_field(&Unions_dyn, "before");
  const struct capn_dyn_field *after = capn_dyn_find_field(&Unions_dyn, "after");
  const struct capn_dyn_field *f = capn_dyn_find_field(&Unions_dyn, "f");
  const struct capn_dyn_field *grp = capn_dyn_find_field(&Unions_dyn, "grp");
  const struct capn_dyn_field *m = capn_dyn_find_field(grp->type.st, "m");

  ASSERT_EQ(0, capn_dyn_set_bits(root.p, before, 77));
  ASSERT_EQ(0, capn_dyn_set_bits(root.p, after, capn_from_f64(1.25)));
  ASSERT_EQ(0, capn_dyn_set_which(&Unions_dyn, root.p, f));
  ASSERT_EQ(0, capn_dyn_set_bits(root.p, f, 1));
  ASSERT_EQ(0, capn_dyn_set_bits(root.p, m, 4));

  EXPECT_EQ(77u, capn_dyn_get_bits(root.p, before));
  EXPECT_EQ(1.25, capn_dyn_bits_to_f64(after, capn_dyn_get_bits(root.p, after)));
  EXPECT_EQ((int) f->discriminant, capn_dyn_which(&Unions_dyn, root.p));
  EXPECT_EQ(4u, capn_dyn_get_bits(root.p, m));
  /* The default is applied on read and removed on write. */
  const struct capn_dyn_field *z = capn_dyn_find_field(grp->type.st, "inner");
  EXPECT_NE(nullptr, z);
  EXPECT_EQ(-1, capn_dyn_set_which(&Unions_dyn, root.p, before));
  capn_free(&c);
}
