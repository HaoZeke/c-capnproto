/* capnp-cli-interop-test.cpp
 *
 * Interop with the official `capnp` CLI. Field-level decode still
 * runs. Schema-order C encode, canonicalize, and packed must memcmp
 * the official encoder (encoding.html canonical form; same unpacked
 * bytes pack identically).
 *
 * Meson registers this binary only when find_program('capnp') succeeds.
 * The tests skip only when the CLI binary is missing at runtime.
 */

#include <gtest/gtest.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "capnp_c.h"
#include "addressbook.capnp.h"
#include "test.capnp.h"

#ifndef CAPNP_CLI
#define CAPNP_CLI ""
#endif

#ifndef CAPNP_SOURCE_ROOT
#define CAPNP_SOURCE_ROOT ""
#endif

static capn_text chars_to_text(const char *chars) {
  return (capn_text) {
    .len = (int) strlen(chars),
    .str = chars,
    .seg = NULL,
  };
}

#define EXPECT_CAPN_TEXT_EQ(expected, t) \
  do { \
    EXPECT_EQ(strlen((expected)), (uint32_t) (t).len); \
    EXPECT_STREQ((expected), (t).str); \
  } while (0)

static const char *capnp_bin(void) {
  const char *env = getenv("CAPNP_CLI");
  if (env && env[0] != '\0') {
    return env;
  }
  if (CAPNP_CLI[0] != '\0') {
    return CAPNP_CLI;
  }
  return NULL;
}

static std::string source_root(void) {
  const char *env = getenv("CAPNP_SOURCE_ROOT");
  if (env && env[0] != '\0') {
    return env;
  }
  if (CAPNP_SOURCE_ROOT[0] != '\0') {
    return CAPNP_SOURCE_ROOT;
  }
  return ".";
}

static std::string join_path(const std::string &dir, const char *name) {
  if (dir.empty()) {
    return name;
  }
  char last = dir[dir.size() - 1];
  if (last == '/' || last == '\\') {
    return dir + name;
  }
  return dir + "/" + name;
}

static std::string include_dir(void) {
  return join_path(source_root(), "compiler");
}

static std::string addressbook_schema(void) {
  return join_path(source_root(), "tests/addressbook.capnp");
}

static std::string testalltypes_schema(void) {
  return join_path(source_root(), "compiler/test.capnp");
}

static std::string fixture_path(const char *name) {
  return join_path(source_root(), (std::string("tests/fixtures/") + name).c_str());
}

static bool capnp_present(void) {
  const char *bin = capnp_bin();
  return bin != NULL && access(bin, X_OK) == 0;
}

struct CmdResult {
  int status;
  std::string out;
  std::string err;
};

static void close_fd(int *fd) {
  if (*fd >= 0) {
    close(*fd);
    *fd = -1;
  }
}

static bool read_all_fd(int fd, std::string *out) {
  char buf[4096];
  ssize_t n;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    out->append(buf, (size_t) n);
  }
  return n == 0;
}

static CmdResult run_argv(const std::vector<std::string> &argv,
                          const void *stdin_data, size_t stdin_len) {
  CmdResult r;
  r.status = -1;
  if (argv.empty()) {
    r.err = "empty argv";
    return r;
  }

  int in_pipe[2] = {-1, -1};
  int out_pipe[2] = {-1, -1};
  int err_pipe[2] = {-1, -1};
  if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
    r.err = "pipe failed";
    close_fd(&in_pipe[0]);
    close_fd(&in_pipe[1]);
    close_fd(&out_pipe[0]);
    close_fd(&out_pipe[1]);
    close_fd(&err_pipe[0]);
    close_fd(&err_pipe[1]);
    return r;
  }

  pid_t pid = fork();
  if (pid < 0) {
    r.err = "fork failed";
    close_fd(&in_pipe[0]);
    close_fd(&in_pipe[1]);
    close_fd(&out_pipe[0]);
    close_fd(&out_pipe[1]);
    close_fd(&err_pipe[0]);
    close_fd(&err_pipe[1]);
    return r;
  }

  if (pid == 0) {
    if (dup2(in_pipe[0], STDIN_FILENO) < 0 ||
        dup2(out_pipe[1], STDOUT_FILENO) < 0 ||
        dup2(err_pipe[1], STDERR_FILENO) < 0) {
      _exit(127);
    }
    close(in_pipe[0]);
    close(in_pipe[1]);
    close(out_pipe[0]);
    close(out_pipe[1]);
    close(err_pipe[0]);
    close(err_pipe[1]);
    std::vector<char *> cargv;
    cargv.reserve(argv.size() + 1);
    for (size_t i = 0; i < argv.size(); i++) {
      cargv.push_back(const_cast<char *>(argv[i].c_str()));
    }
    cargv.push_back(NULL);
    execv(argv[0].c_str(), cargv.data());
    _exit(127);
  }

  close_fd(&in_pipe[0]);
  close_fd(&out_pipe[1]);
  close_fd(&err_pipe[1]);

  void (*old_pipe)(int) = signal(SIGPIPE, SIG_IGN);
  const uint8_t *p = (const uint8_t *) stdin_data;
  size_t left = stdin_len;
  while (left > 0) {
    ssize_t n = write(in_pipe[1], p, left);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }
    p += (size_t) n;
    left -= (size_t) n;
  }
  close_fd(&in_pipe[1]);
  signal(SIGPIPE, old_pipe);

  read_all_fd(out_pipe[0], &r.out);
  read_all_fd(err_pipe[0], &r.err);
  close_fd(&out_pipe[0]);
  close_fd(&err_pipe[0]);

  int st = 0;
  if (waitpid(pid, &st, 0) < 0) {
    r.status = -1;
    return r;
  }
  if (WIFEXITED(st)) {
    r.status = WEXITSTATUS(st);
  } else if (WIFSIGNALED(st)) {
    r.status = 128 + WTERMSIG(st);
  } else {
    r.status = -1;
  }
  return r;
}

static std::vector<std::string> capnp_cmd(const char *verb,
                                          const std::string &schema,
                                          const char *typ) {
  std::vector<std::string> argv;
  argv.push_back(capnp_bin());
  argv.push_back(verb);
  argv.push_back("-I");
  argv.push_back(include_dir());
  argv.push_back(schema);
  argv.push_back(typ);
  return argv;
}

static std::string read_file_text(const std::string &path) {
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) {
    return "";
  }
  std::string out;
  char buf[512];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    out.append(buf, n);
  }
  fclose(f);
  return out;
}

static void expect_alice_bob(struct AddressBook *book, const char *label) {
  ASSERT_EQ(2, capn_len(book->people)) << label << ": two people";

  struct Person alice;
  get_Person(&alice, book->people, 0);
  EXPECT_EQ((uint32_t) 123, alice.id) << label << ": alice id";
  EXPECT_CAPN_TEXT_EQ("Alice", alice.name);
  EXPECT_CAPN_TEXT_EQ("alice@example.com", alice.email);
  EXPECT_EQ(1, capn_len(alice.phones)) << label << ": alice one phone";
  struct Person_PhoneNumber aph;
  get_Person_PhoneNumber(&aph, alice.phones, 0);
  EXPECT_CAPN_TEXT_EQ("555-1212", aph.number);
  EXPECT_EQ(Person_PhoneNumber_Type_mobile, aph.type);
  EXPECT_EQ(Person_employment_school, alice.employment_which);
  EXPECT_CAPN_TEXT_EQ("MIT", alice.employment.school);

  struct Person bob;
  get_Person(&bob, book->people, 1);
  EXPECT_EQ((uint32_t) 456, bob.id) << label << ": bob id";
  EXPECT_CAPN_TEXT_EQ("Bob", bob.name);
  EXPECT_CAPN_TEXT_EQ("bob@example.com", bob.email);
  EXPECT_EQ(2, capn_len(bob.phones)) << label << ": bob two phones";
  struct Person_PhoneNumber bph0;
  get_Person_PhoneNumber(&bph0, bob.phones, 0);
  EXPECT_CAPN_TEXT_EQ("555-4567", bph0.number);
  EXPECT_EQ(Person_PhoneNumber_Type_home, bph0.type);
  struct Person_PhoneNumber bph1;
  get_Person_PhoneNumber(&bph1, bob.phones, 1);
  EXPECT_CAPN_TEXT_EQ("555-7654", bph1.number);
  EXPECT_EQ(Person_PhoneNumber_Type_work, bph1.type);
  EXPECT_EQ(Person_employment_unemployed, bob.employment_which);
}

static ssize_t encode_alice_bob(uint8_t *buf, size_t cap) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  AddressBook_ptr ab = new_AddressBook(cs);
  Person_list people = new_Person_list(cs, 2);

  struct Person alice;
  memset(&alice, 0, sizeof(alice));
  alice.id = 123;
  alice.name = chars_to_text("Alice");
  alice.email = chars_to_text("alice@example.com");
  alice.phones = new_Person_PhoneNumber_list(cs, 1);
  struct Person_PhoneNumber aph;
  memset(&aph, 0, sizeof(aph));
  aph.number = chars_to_text("555-1212");
  aph.type = Person_PhoneNumber_Type_mobile;
  set_Person_PhoneNumber(&aph, alice.phones, 0);
  alice.employment_which = Person_employment_school;
  alice.employment.school = chars_to_text("MIT");
  set_Person(&alice, people, 0);

  struct Person bob;
  memset(&bob, 0, sizeof(bob));
  bob.id = 456;
  bob.name = chars_to_text("Bob");
  bob.email = chars_to_text("bob@example.com");
  bob.phones = new_Person_PhoneNumber_list(cs, 2);
  struct Person_PhoneNumber bph0;
  memset(&bph0, 0, sizeof(bph0));
  bph0.number = chars_to_text("555-4567");
  bph0.type = Person_PhoneNumber_Type_home;
  set_Person_PhoneNumber(&bph0, bob.phones, 0);
  struct Person_PhoneNumber bph1;
  memset(&bph1, 0, sizeof(bph1));
  bph1.number = chars_to_text("555-7654");
  bph1.type = Person_PhoneNumber_Type_work;
  set_Person_PhoneNumber(&bph1, bob.phones, 1);
  bob.employment_which = Person_employment_unemployed;
  set_Person(&bob, people, 1);

  struct AddressBook book;
  memset(&book, 0, sizeof(book));
  book.people = people;
  write_AddressBook(&book, ab);
  if (capn_set_root(&c, ab.p) != 0) {
    capn_free(&c);
    return -1;
  }
  ssize_t sz = capn_write_mem(&c, buf, cap, 0);
  capn_free(&c);
  return sz;
}

static ssize_t encode_testalltypes(uint8_t *buf, size_t cap) {
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  struct TestAllTypes t;
  memset(&t, 0, sizeof(t));
  t.textField = chars_to_text("hello-cli");

  const uint8_t data_bytes[] = {0x01, 0x02, 0x03, 0xff};
  capn_list8 data = capn_new_list8(cs, 4);
  if (capn_setv8(data, 0, data_bytes, 4) != 4) {
    capn_free(&c);
    return -1;
  }
  t.dataField.p = data.p;

  TestAllTypes_ptr inner = new_TestAllTypes(cs);
  struct TestAllTypes inner_s;
  memset(&inner_s, 0, sizeof(inner_s));
  inner_s.textField = chars_to_text("nested");
  write_TestAllTypes(&inner_s, inner);
  t.structField = inner;

  t.enumField = TestEnum_baz;

  t.boolList = capn_new_list1(cs, 3);
  EXPECT_EQ(0, capn_set1(t.boolList, 0, 1));
  EXPECT_EQ(0, capn_set1(t.boolList, 1, 0));
  EXPECT_EQ(0, capn_set1(t.boolList, 2, 1));

  t.int32List = capn_new_list32(cs, 5);
  EXPECT_EQ(0, capn_set32(t.int32List, 0, 1));
  EXPECT_EQ(0, capn_set32(t.int32List, 1, (uint32_t) -2));
  EXPECT_EQ(0, capn_set32(t.int32List, 2, 3));
  EXPECT_EQ(0, capn_set32(t.int32List, 3, 0));
  EXPECT_EQ(0, capn_set32(t.int32List, 4, 42));

  t.textList.p = capn_new_ptr_list(cs, 2);
  EXPECT_EQ(0, capn_set_text(t.textList.p, 0, chars_to_text("alpha")));
  EXPECT_EQ(0, capn_set_text(t.textList.p, 1, chars_to_text("beta")));

  capn_list8 d0 = capn_new_list8(cs, 1);
  EXPECT_EQ(0, capn_set8(d0, 0, 0xaa));
  capn_list8 d1 = capn_new_list8(cs, 2);
  EXPECT_EQ(0, capn_set8(d1, 0, 0xbb));
  EXPECT_EQ(0, capn_set8(d1, 1, 0xcc));
  t.dataList.p = capn_new_ptr_list(cs, 2);
  EXPECT_EQ(0, capn_setp(t.dataList.p, 0, d0.p));
  EXPECT_EQ(0, capn_setp(t.dataList.p, 1, d1.p));

  t.structList = new_TestAllTypes_list(cs, 2);
  struct TestAllTypes s0;
  memset(&s0, 0, sizeof(s0));
  s0.textField = chars_to_text("s0");
  set_TestAllTypes(&s0, t.structList, 0);
  struct TestAllTypes s1;
  memset(&s1, 0, sizeof(s1));
  s1.textField = chars_to_text("s1");
  set_TestAllTypes(&s1, t.structList, 1);

  t.enumList = capn_new_list16(cs, 2);
  EXPECT_EQ(0, capn_set16(t.enumList, 0, (uint16_t) TestEnum_foo));
  EXPECT_EQ(0, capn_set16(t.enumList, 1, (uint16_t) TestEnum_bar));

  TestAllTypes_ptr tp = new_TestAllTypes(cs);
  write_TestAllTypes(&t, tp);
  if (capn_set_root(&c, tp.p) != 0) {
    capn_free(&c);
    return -1;
  }
  ssize_t sz = capn_write_mem(&c, buf, cap, 0);
  capn_free(&c);
  return sz;
}

static capn_list8 data_as_list8(capn_data d) {
  capn_list8 l;
  l.p = d.p;
  return l;
}

static void expect_testalltypes(struct TestAllTypes *t, const char *label) {
  EXPECT_CAPN_TEXT_EQ("hello-cli", t->textField);

  ASSERT_EQ(4, capn_len(t->dataField)) << label << ": dataField len";
  capn_list8 data_bytes = data_as_list8(t->dataField);
  EXPECT_EQ(0x01u, capn_get8(data_bytes, 0));
  EXPECT_EQ(0x02u, capn_get8(data_bytes, 1));
  EXPECT_EQ(0x03u, capn_get8(data_bytes, 2));
  EXPECT_EQ(0xffu, capn_get8(data_bytes, 3));

  struct TestAllTypes inner;
  memset(&inner, 0, sizeof(inner));
  read_TestAllTypes(&inner, t->structField);
  EXPECT_CAPN_TEXT_EQ("nested", inner.textField);

  EXPECT_EQ(TestEnum_baz, t->enumField);

  ASSERT_EQ(3, capn_len(t->boolList)) << label << ": boolList len";
  EXPECT_EQ(1, capn_get1(t->boolList, 0));
  EXPECT_EQ(0, capn_get1(t->boolList, 1));
  EXPECT_EQ(1, capn_get1(t->boolList, 2));

  ASSERT_EQ(5, capn_len(t->int32List)) << label << ": int32List len";
  EXPECT_EQ(1, (int32_t) capn_get32(t->int32List, 0));
  EXPECT_EQ(-2, (int32_t) capn_get32(t->int32List, 1));
  EXPECT_EQ(3, (int32_t) capn_get32(t->int32List, 2));
  EXPECT_EQ(0, (int32_t) capn_get32(t->int32List, 3));
  EXPECT_EQ(42, (int32_t) capn_get32(t->int32List, 4));

  ASSERT_EQ(2, capn_len(t->textList)) << label << ": textList len";
  static const capn_text empty = {0, "", 0};
  EXPECT_CAPN_TEXT_EQ("alpha", capn_get_text(t->textList.p, 0, empty));
  EXPECT_CAPN_TEXT_EQ("beta", capn_get_text(t->textList.p, 1, empty));

  ASSERT_EQ(2, capn_len(t->dataList)) << label << ": dataList len";
  capn_data d0 = capn_get_data(t->dataList.p, 0);
  capn_list8 d0b = data_as_list8(d0);
  ASSERT_EQ(1, capn_len(d0));
  EXPECT_EQ(0xaau, capn_get8(d0b, 0));
  capn_data d1 = capn_get_data(t->dataList.p, 1);
  capn_list8 d1b = data_as_list8(d1);
  ASSERT_EQ(2, capn_len(d1));
  EXPECT_EQ(0xbbu, capn_get8(d1b, 0));
  EXPECT_EQ(0xccu, capn_get8(d1b, 1));

  ASSERT_EQ(2, capn_len(t->structList)) << label << ": structList len";
  struct TestAllTypes s0;
  memset(&s0, 0, sizeof(s0));
  get_TestAllTypes(&s0, t->structList, 0);
  EXPECT_CAPN_TEXT_EQ("s0", s0.textField);
  struct TestAllTypes s1;
  memset(&s1, 0, sizeof(s1));
  get_TestAllTypes(&s1, t->structList, 1);
  EXPECT_CAPN_TEXT_EQ("s1", s1.textField);

  ASSERT_EQ(2, capn_len(t->enumList)) << label << ": enumList len";
  EXPECT_EQ((uint16_t) TestEnum_foo, capn_get16(t->enumList, 0));
  EXPECT_EQ((uint16_t) TestEnum_bar, capn_get16(t->enumList, 1));
}

static void expect_contains(const std::string &hay, const char *needle) {
  EXPECT_NE(std::string::npos, hay.find(needle))
      << "missing `" << needle << "` in:\n" << hay;
}

class CapnpCli : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!capnp_present()) {
      GTEST_SKIP() << "capnp CLI not found (meson find_program missed it)";
    }
  }
};

TEST_F(CapnpCli, CEncodeAddressBook_DecodeShowsAliceBob) {
  uint8_t buf[4096];
  ssize_t sz = encode_alice_bob(buf, sizeof(buf));
  ASSERT_GT(sz, 0);

  CmdResult r = run_argv(
      capnp_cmd("decode", addressbook_schema(), "AddressBook"),
      buf, (size_t) sz);
  ASSERT_EQ(0, r.status) << "capnp decode stderr:\n" << r.err
                         << "\nstdout:\n" << r.out;
  expect_contains(r.out, "Alice");
  expect_contains(r.out, "Bob");
  expect_contains(r.out, "alice@example.com");
  expect_contains(r.out, "bob@example.com");
  expect_contains(r.out, "555-1212");
  expect_contains(r.out, "555-4567");
  expect_contains(r.out, "555-7654");
  expect_contains(r.out, "MIT");
}

TEST_F(CapnpCli, CapnpEncodeAddressBookText_CReadMatches) {
  std::string text = read_file_text(fixture_path("addressbook.txt"));
  ASSERT_FALSE(text.empty()) << "missing " << fixture_path("addressbook.txt");

  CmdResult r = run_argv(
      capnp_cmd("encode", addressbook_schema(), "AddressBook"),
      text.data(), text.size());
  ASSERT_EQ(0, r.status) << "capnp encode stderr:\n" << r.err;
  ASSERT_FALSE(r.out.empty()) << "capnp encode wrote no bytes";

  struct capn rc;
  ASSERT_EQ(0, capn_init_mem(&rc, (const uint8_t *) r.out.data(),
                             r.out.size(), 0));
  AddressBook_ptr root;
  root.p = capn_getp(capn_root(&rc), 0, 1);
  ASSERT_NE(CAPN_NULL, root.p.type);
  struct AddressBook book;
  read_AddressBook(&book, root);
  expect_alice_bob(&book, "capnp-encode");
  capn_free(&rc);
}

TEST_F(CapnpCli, CEncodeTestAllTypes_DecodeShowsPointers) {
  uint8_t buf[8192];
  ssize_t sz = encode_testalltypes(buf, sizeof(buf));
  ASSERT_GT(sz, 0);

  CmdResult r = run_argv(
      capnp_cmd("decode", testalltypes_schema(), "TestAllTypes"),
      buf, (size_t) sz);
  ASSERT_EQ(0, r.status) << "capnp decode stderr:\n" << r.err
                         << "\nstdout:\n" << r.out;
  expect_contains(r.out, "hello-cli");
  expect_contains(r.out, "nested");
  expect_contains(r.out, "alpha");
  expect_contains(r.out, "beta");
  expect_contains(r.out, "s0");
  expect_contains(r.out, "s1");
  expect_contains(r.out, "int32List");
  expect_contains(r.out, "-2");
  expect_contains(r.out, "42");
  expect_contains(r.out, "baz");
}

TEST_F(CapnpCli, CapnpEncodeTestAllTypesText_CReadMatches) {
  std::string text = read_file_text(fixture_path("testalltypes.txt"));
  ASSERT_FALSE(text.empty()) << "missing " << fixture_path("testalltypes.txt");

  CmdResult r = run_argv(
      capnp_cmd("encode", testalltypes_schema(), "TestAllTypes"),
      text.data(), text.size());
  ASSERT_EQ(0, r.status) << "capnp encode stderr:\n" << r.err;
  ASSERT_FALSE(r.out.empty()) << "capnp encode wrote no bytes";

  struct capn rc;
  ASSERT_EQ(0, capn_init_mem(&rc, (const uint8_t *) r.out.data(),
                             r.out.size(), 0));
  TestAllTypes_ptr root;
  root.p = capn_getp(capn_root(&rc), 0, 1);
  ASSERT_NE(CAPN_NULL, root.p.type);
  struct TestAllTypes t;
  memset(&t, 0, sizeof(t));
  read_TestAllTypes(&t, root);
  expect_testalltypes(&t, "capnp-encode");
  capn_free(&rc);
}

static std::string hex_span(const void *p, size_t n, size_t maxn) {
  const uint8_t *b = (const uint8_t *) p;
  if (n > maxn) {
    n = maxn;
  }
  std::string s;
  char tmp[8];
  for (size_t i = 0; i < n; i++) {
    snprintf(tmp, sizeof(tmp), "%02x%s", b[i], ((i + 1) % 8) == 0 ? " " : "");
    s += tmp;
  }
  return s;
}

static void expect_bytes_eq(const void *got, size_t got_n, const void *want,
                            size_t want_n, const char *label) {
  const uint8_t *g = (const uint8_t *) got;
  const uint8_t *w = (const uint8_t *) want;
  if (got_n == want_n && memcmp(g, w, got_n) == 0) {
    return;
  }
  size_t n = got_n < want_n ? got_n : want_n;
  size_t first = 0;
  while (first < n && g[first] == w[first]) {
    first++;
  }
  ADD_FAILURE() << label << ": size got " << got_n << " want " << want_n
                << " first differ at " << first
                << "\ngot  " << hex_span(g, got_n, 160)
                << "\nwant " << hex_span(w, want_n, 160);
}

static CmdResult capnp_encode_text(const std::string &schema, const char *typ,
                                   const char *text) {
  return run_argv(capnp_cmd("encode", schema, typ), text, strlen(text));
}

static CmdResult capnp_convert(const char *spec, const void *data, size_t len) {
  std::vector<std::string> argv;
  argv.push_back(capnp_bin());
  argv.push_back("convert");
  argv.push_back(spec);
  return run_argv(argv, data, len);
}

static Person_ptr person_at(Person_list l, int i) {
  Person_ptr p;
  p.p = capn_getp(l.p, i, 0);
  return p;
}

static Person_PhoneNumber_ptr phone_at(Person_PhoneNumber_list l, int i) {
  Person_PhoneNumber_ptr p;
  p.p = capn_getp(l.p, i, 0);
  return p;
}

static void set_employment_school(Person_ptr p, const char *school) {
  capn_resolve(&p.p);
  capn_write16(p.p, 4, (uint16_t) Person_employment_school);
  capn_set_text(p.p, 3, chars_to_text(school));
}

static void set_employment_unemployed(Person_ptr p) {
  capn_resolve(&p.p);
  capn_write16(p.p, 4, (uint16_t) Person_employment_unemployed);
}

/* Root first, then each pointer field as the C++ text encoder sets it. */
static int build_alice_bob_schema_order(struct capn *c) {
  struct capn_segment *cs = capn_root(c).seg;
  AddressBook_ptr ab = new_AddressBook(cs);
  Person_list people = new_Person_list(cs, 2);

  Person_ptr alice = person_at(people, 0);
  Person_set_id(alice, 123);
  Person_set_name(alice, chars_to_text("Alice"));
  Person_set_email(alice, chars_to_text("alice@example.com"));
  {
    Person_PhoneNumber_list phones = new_Person_PhoneNumber_list(cs, 1);
    Person_PhoneNumber_ptr ph = phone_at(phones, 0);
    Person_PhoneNumber_set_number(ph, chars_to_text("555-1212"));
    Person_PhoneNumber_set_type(ph, Person_PhoneNumber_Type_mobile);
    Person_set_phones(alice, phones);
  }
  set_employment_school(alice, "MIT");

  Person_ptr bob = person_at(people, 1);
  Person_set_id(bob, 456);
  Person_set_name(bob, chars_to_text("Bob"));
  Person_set_email(bob, chars_to_text("bob@example.com"));
  {
    Person_PhoneNumber_list phones = new_Person_PhoneNumber_list(cs, 2);
    Person_PhoneNumber_ptr ph0 = phone_at(phones, 0);
    Person_PhoneNumber_set_number(ph0, chars_to_text("555-4567"));
    Person_PhoneNumber_set_type(ph0, Person_PhoneNumber_Type_home);
    Person_PhoneNumber_ptr ph1 = phone_at(phones, 1);
    Person_PhoneNumber_set_number(ph1, chars_to_text("555-7654"));
    Person_PhoneNumber_set_type(ph1, Person_PhoneNumber_Type_work);
    Person_set_phones(bob, phones);
  }
  set_employment_unemployed(bob);

  AddressBook_set_people(ab, people);
  return capn_set_root(c, ab.p);
}

static ssize_t encode_alice_bob_schema_order(uint8_t *buf, size_t cap,
                                             int packed) {
  struct capn c;
  capn_init_malloc(&c);
  if (build_alice_bob_schema_order(&c) != 0) {
    capn_free(&c);
    return -1;
  }
  ssize_t sz = capn_write_mem(&c, buf, cap, packed);
  capn_free(&c);
  return sz;
}

static capn_data list8_as_data(capn_list8 l) {
  capn_data d;
  d.p = l.p;
  return d;
}

static capn_ptr_list ptr_list_of(capn_ptr p) {
  capn_ptr_list l;
  l.p = p;
  return l;
}

static TestAllTypes_ptr testall_at(TestAllTypes_list l, int i) {
  TestAllTypes_ptr p;
  p.p = capn_getp(l.p, i, 0);
  return p;
}

static int build_testalltypes_schema_order(struct capn *c) {
  struct capn_segment *cs = capn_root(c).seg;
  TestAllTypes_ptr tp = new_TestAllTypes(cs);

  TestAllTypes_set_textField(tp, chars_to_text("hello-cli"));

  {
    capn_list8 data = capn_new_list8(cs, 4);
    const uint8_t bytes[] = {0x01, 0x02, 0x03, 0xff};
    if (capn_setv8(data, 0, bytes, 4) != 4) {
      return -1;
    }
    TestAllTypes_set_dataField(tp, list8_as_data(data));
  }

  {
    TestAllTypes_ptr inner = new_TestAllTypes(cs);
    TestAllTypes_set_textField(inner, chars_to_text("nested"));
    TestAllTypes_set_structField(tp, inner);
  }

  TestAllTypes_set_enumField(tp, TestEnum_baz);

  {
    capn_list1 bl = capn_new_list1(cs, 3);
    if (capn_set1(bl, 0, 1) || capn_set1(bl, 1, 0) || capn_set1(bl, 2, 1)) {
      return -1;
    }
    TestAllTypes_set_boolList(tp, bl);
  }

  {
    capn_list32 il = capn_new_list32(cs, 5);
    if (capn_set32(il, 0, 1) || capn_set32(il, 1, (uint32_t) -2) ||
        capn_set32(il, 2, 3) || capn_set32(il, 3, 0) ||
        capn_set32(il, 4, 42)) {
      return -1;
    }
    TestAllTypes_set_int32List(tp, il);
  }

  {
    capn_ptr_list tl = ptr_list_of(capn_new_ptr_list(cs, 2));
    if (capn_set_text(tl.p, 0, chars_to_text("alpha")) ||
        capn_set_text(tl.p, 1, chars_to_text("beta"))) {
      return -1;
    }
    TestAllTypes_set_textList(tp, tl);
  }

  {
    capn_ptr_list dl = ptr_list_of(capn_new_ptr_list(cs, 2));
    capn_list8 d0 = capn_new_list8(cs, 1);
    capn_list8 d1 = capn_new_list8(cs, 2);
    if (capn_set8(d0, 0, 0xaa) || capn_set8(d1, 0, 0xbb) ||
        capn_set8(d1, 1, 0xcc) || capn_setp(dl.p, 0, d0.p) ||
        capn_setp(dl.p, 1, d1.p)) {
      return -1;
    }
    TestAllTypes_set_dataList(tp, dl);
  }

  {
    TestAllTypes_list sl = new_TestAllTypes_list(cs, 2);
    TestAllTypes_set_textField(testall_at(sl, 0), chars_to_text("s0"));
    TestAllTypes_set_textField(testall_at(sl, 1), chars_to_text("s1"));
    TestAllTypes_set_structList(tp, sl);
  }

  {
    capn_list16 el = capn_new_list16(cs, 2);
    if (capn_set16(el, 0, (uint16_t) TestEnum_foo) ||
        capn_set16(el, 1, (uint16_t) TestEnum_bar)) {
      return -1;
    }
    TestAllTypes_set_enumList(tp, el);
  }

  return capn_set_root(c, tp.p);
}

static ssize_t encode_testalltypes_schema_order(uint8_t *buf, size_t cap) {
  struct capn c;
  capn_init_malloc(&c);
  if (build_testalltypes_schema_order(&c) != 0) {
    capn_free(&c);
    return -1;
  }
  ssize_t sz = capn_write_mem(&c, buf, cap, 0);
  capn_free(&c);
  return sz;
}

static std::string canonicalize_unframed(struct capn *src) {
  struct capn dst;
  capn_init_malloc(&dst);
  if (capn_canonicalize(src, &dst) != 0 || dst.segnum != 1 ||
      dst.seglist == NULL) {
    capn_free(&dst);
    return "";
  }
  std::string out(dst.seglist->data, dst.seglist->len);
  capn_free(&dst);
  return out;
}

static std::string encode_then_canonicalize(int (*build)(struct capn *)) {
  struct capn src;
  capn_init_malloc(&src);
  std::string out;
  if (build(&src) == 0) {
    out = canonicalize_unframed(&src);
  }
  capn_free(&src);
  return out;
}

TEST_F(CapnpCli, OfficialAddressBookEncodeMatchesCommittedFlat) {
  std::string text = read_file_text(fixture_path("addressbook.txt"));
  ASSERT_FALSE(text.empty());
  CmdResult r = capnp_encode_text(addressbook_schema(), "AddressBook",
                                  text.c_str());
  ASSERT_EQ(0, r.status) << r.err;
  std::string committed = read_file_text(fixture_path("addressbook.bin"));
  ASSERT_FALSE(committed.empty());
  expect_bytes_eq(r.out.data(), r.out.size(), committed.data(),
                  committed.size(), "official-vs-committed-flat");
}

TEST_F(CapnpCli, SchemaOrderAddressBookMatchesCapnpEncode) {
  std::string text = read_file_text(fixture_path("addressbook.txt"));
  ASSERT_FALSE(text.empty());
  CmdResult want = capnp_encode_text(addressbook_schema(), "AddressBook",
                                     text.c_str());
  ASSERT_EQ(0, want.status) << want.err;

  uint8_t buf[4096];
  ssize_t sz = encode_alice_bob_schema_order(buf, sizeof(buf), 0);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "addressbook-schema-order");
}

TEST_F(CapnpCli, SchemaOrderAddressBookPackedMatchesConvert) {
  uint8_t buf[4096];
  ssize_t sz = encode_alice_bob_schema_order(buf, sizeof(buf), 0);
  ASSERT_GT(sz, 0);

  CmdResult want = capnp_convert("binary:packed", buf, (size_t) sz);
  ASSERT_EQ(0, want.status) << want.err;

  uint8_t packed[4096];
  ssize_t psz = encode_alice_bob_schema_order(packed, sizeof(packed), 1);
  ASSERT_GT(psz, 0);
  expect_bytes_eq(packed, (size_t) psz, want.out.data(), want.out.size(),
                  "addressbook-packed");
}

TEST_F(CapnpCli, InitMemThenPackedMatchesConvert) {
  std::string text = read_file_text(fixture_path("addressbook.txt"));
  ASSERT_FALSE(text.empty());
  CmdResult enc = capnp_encode_text(addressbook_schema(), "AddressBook",
                                    text.c_str());
  ASSERT_EQ(0, enc.status) << enc.err;

  CmdResult want = capnp_convert("binary:packed", enc.out.data(),
                                 enc.out.size());
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  ASSERT_EQ(0, capn_init_mem(&c, (const uint8_t *) enc.out.data(),
                             enc.out.size(), 0));
  uint8_t packed[4096];
  ssize_t psz = capn_write_mem(&c, packed, sizeof(packed), 1);
  capn_free(&c);
  ASSERT_GT(psz, 0);
  expect_bytes_eq(packed, (size_t) psz, want.out.data(), want.out.size(),
                  "init-mem-packed");
}

TEST_F(CapnpCli, CanonicalizeAddressBookMatchesConvert) {
  std::string text = read_file_text(fixture_path("addressbook.txt"));
  ASSERT_FALSE(text.empty());
  CmdResult enc = capnp_encode_text(addressbook_schema(), "AddressBook",
                                    text.c_str());
  ASSERT_EQ(0, enc.status) << enc.err;

  CmdResult want = capnp_convert("binary:canonical", enc.out.data(),
                                 enc.out.size());
  ASSERT_EQ(0, want.status) << want.err;

  std::string from_c = encode_then_canonicalize(build_alice_bob_schema_order);
  ASSERT_FALSE(from_c.empty());
  expect_bytes_eq(from_c.data(), from_c.size(), want.out.data(),
                  want.out.size(), "addressbook-canonical-from-c");

  struct capn official;
  ASSERT_EQ(0, capn_init_mem(&official, (const uint8_t *) enc.out.data(),
                             enc.out.size(), 0));
  std::string from_off = canonicalize_unframed(&official);
  capn_free(&official);
  ASSERT_FALSE(from_off.empty());
  expect_bytes_eq(from_off.data(), from_off.size(), want.out.data(),
                  want.out.size(), "addressbook-canonical-from-official");
}

TEST_F(CapnpCli, SchemaOrderTestAllTypesMatchesCapnpEncode) {
  std::string text = read_file_text(fixture_path("testalltypes.txt"));
  ASSERT_FALSE(text.empty());
  CmdResult want = capnp_encode_text(testalltypes_schema(), "TestAllTypes",
                                     text.c_str());
  ASSERT_EQ(0, want.status) << want.err;

  uint8_t buf[8192];
  ssize_t sz = encode_testalltypes_schema_order(buf, sizeof(buf));
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "testalltypes-schema-order");
}

TEST_F(CapnpCli, CanonicalizeTestAllTypesMatchesConvert) {
  std::string text = read_file_text(fixture_path("testalltypes.txt"));
  ASSERT_FALSE(text.empty());
  CmdResult enc = capnp_encode_text(testalltypes_schema(), "TestAllTypes",
                                    text.c_str());
  ASSERT_EQ(0, enc.status) << enc.err;

  CmdResult want = capnp_convert("binary:canonical", enc.out.data(),
                                 enc.out.size());
  ASSERT_EQ(0, want.status) << want.err;

  std::string from_c = encode_then_canonicalize(build_testalltypes_schema_order);
  ASSERT_FALSE(from_c.empty());
  expect_bytes_eq(from_c.data(), from_c.size(), want.out.data(),
                  want.out.size(), "testalltypes-canonical-from-c");

  struct capn official;
  ASSERT_EQ(0, capn_init_mem(&official, (const uint8_t *) enc.out.data(),
                             enc.out.size(), 0));
  std::string from_off = canonicalize_unframed(&official);
  capn_free(&official);
  ASSERT_FALSE(from_off.empty());
  expect_bytes_eq(from_off.data(), from_off.size(), want.out.data(),
                  want.out.size(), "testalltypes-canonical-from-official");
}

static ssize_t write_root(struct capn *c, capn_ptr root, uint8_t *buf,
                          size_t cap) {
  if (capn_set_root(c, root) != 0) {
    return -1;
  }
  return capn_write_mem(c, buf, cap, 0);
}

TEST_F(CapnpCli, EmptyStructMatchesEncode) {
  CmdResult want = capnp_encode_text(testalltypes_schema(), "TestEmptyStruct",
                                     "()");
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  capn_init_malloc(&c);
  TestEmptyStruct_ptr p = new_TestEmptyStruct(capn_root(&c).seg);
  uint8_t buf[64];
  ssize_t sz = write_root(&c, p.p, buf, sizeof(buf));
  capn_free(&c);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "empty-struct");
}

TEST_F(CapnpCli, Int32OnlyMatchesEncode) {
  CmdResult want = capnp_encode_text(testalltypes_schema(), "TestAllTypes",
                                     "(int32Field = 42)");
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  capn_init_malloc(&c);
  TestAllTypes_ptr tp = new_TestAllTypes(capn_root(&c).seg);
  TestAllTypes_set_int32Field(tp, 42);
  uint8_t buf[512];
  ssize_t sz = write_root(&c, tp.p, buf, sizeof(buf));
  capn_free(&c);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "int32-only");
}

TEST_F(CapnpCli, TextOnlyMatchesEncode) {
  CmdResult want = capnp_encode_text(testalltypes_schema(), "TestAllTypes",
                                     "(textField = \"hi\")");
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  capn_init_malloc(&c);
  TestAllTypes_ptr tp = new_TestAllTypes(capn_root(&c).seg);
  TestAllTypes_set_textField(tp, chars_to_text("hi"));
  uint8_t buf[512];
  ssize_t sz = write_root(&c, tp.p, buf, sizeof(buf));
  capn_free(&c);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "text-only");
}

TEST_F(CapnpCli, DataOnlyMatchesEncode) {
  CmdResult want = capnp_encode_text(testalltypes_schema(), "TestAllTypes",
                                     "(dataField = 0x\"01 02\")");
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;
  TestAllTypes_ptr tp = new_TestAllTypes(cs);
  capn_list8 d = capn_new_list8(cs, 2);
  ASSERT_EQ(0, capn_set8(d, 0, 0x01));
  ASSERT_EQ(0, capn_set8(d, 1, 0x02));
  TestAllTypes_set_dataField(tp, list8_as_data(d));
  uint8_t buf[512];
  ssize_t sz = write_root(&c, tp.p, buf, sizeof(buf));
  capn_free(&c);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "data-only");
}

TEST_F(CapnpCli, Int32ListOnlyMatchesEncode) {
  CmdResult want = capnp_encode_text(testalltypes_schema(), "TestAllTypes",
                                     "(int32List = [1, 2, 3])");
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;
  TestAllTypes_ptr tp = new_TestAllTypes(cs);
  capn_list32 il = capn_new_list32(cs, 3);
  ASSERT_EQ(0, capn_set32(il, 0, 1));
  ASSERT_EQ(0, capn_set32(il, 1, 2));
  ASSERT_EQ(0, capn_set32(il, 2, 3));
  TestAllTypes_set_int32List(tp, il);
  uint8_t buf[512];
  ssize_t sz = write_root(&c, tp.p, buf, sizeof(buf));
  capn_free(&c);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "int32list-only");
}

TEST_F(CapnpCli, VoidListOnlyMatchesEncode) {
  CmdResult want = capnp_encode_text(testalltypes_schema(), "TestAllTypes",
                                     "(voidList = [void, void])");
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;
  TestAllTypes_ptr tp = new_TestAllTypes(cs);
  capn_ptr vl = capn_new_list(cs, 2, 0, 0);
  TestAllTypes_set_voidList(tp, vl);
  uint8_t buf[512];
  ssize_t sz = write_root(&c, tp.p, buf, sizeof(buf));
  capn_free(&c);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "voidlist-only");
}

TEST_F(CapnpCli, BoolListOnlyMatchesEncode) {
  CmdResult want = capnp_encode_text(testalltypes_schema(), "TestAllTypes",
                                     "(boolList = [true, false, true])");
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;
  TestAllTypes_ptr tp = new_TestAllTypes(cs);
  capn_list1 bl = capn_new_list1(cs, 3);
  ASSERT_EQ(0, capn_set1(bl, 0, 1));
  ASSERT_EQ(0, capn_set1(bl, 1, 0));
  ASSERT_EQ(0, capn_set1(bl, 2, 1));
  TestAllTypes_set_boolList(tp, bl);
  uint8_t buf[512];
  ssize_t sz = write_root(&c, tp.p, buf, sizeof(buf));
  capn_free(&c);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "boollist-only");
}

TEST_F(CapnpCli, EmptyPeopleListMatchesEncode) {
  CmdResult want = capnp_encode_text(addressbook_schema(), "AddressBook",
                                     "(people = [])");
  ASSERT_EQ(0, want.status) << want.err;

  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;
  AddressBook_ptr ab = new_AddressBook(cs);
  Person_list people = new_Person_list(cs, 0);
  AddressBook_set_people(ab, people);
  uint8_t buf[128];
  ssize_t sz = write_root(&c, ab.p, buf, sizeof(buf));
  capn_free(&c);
  ASSERT_GT(sz, 0);
  expect_bytes_eq(buf, (size_t) sz, want.out.data(), want.out.size(),
                  "empty-people");
}
