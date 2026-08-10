/* fortran-golden-test.cpp
 *
 * Decode the capnp-fortran AddressBook goldens (flat, packed, canonical)
 * with this runtime and encode the same logical book. A missing fixture
 * fails the test; it is not skipped.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "capnp_c.h"
#include "addressbook.capnp.h"

#ifndef FORTRAN_GOLDEN_DIR
#define FORTRAN_GOLDEN_DIR ""
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

/* Pinned copy of tests/fixtures/addressbook.bin (288 bytes). */
static const uint8_t kAddressBookFlat[] = {
  0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00,
  0x7b, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00,
  0x21, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00,
  0x39, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0xc8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x35, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00,
  0x39, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x41, 0x6c, 0x69, 0x63, 0x65, 0x00, 0x00, 0x00, 0x61, 0x6c, 0x69, 0x63, 0x65, 0x40, 0x65, 0x78,
  0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x35, 0x35, 0x35, 0x2d, 0x31, 0x32, 0x31, 0x32,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x49, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x42, 0x6f, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x6f, 0x62, 0x40, 0x65, 0x78, 0x61, 0x6d,
  0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00,
  0x35, 0x35, 0x35, 0x2d, 0x34, 0x35, 0x36, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x35, 0x35, 0x35, 0x2d, 0x37, 0x36, 0x35, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

/* Pinned copy of tests/fixtures/addressbook.packed.bin (151 bytes). */
static const uint8_t kAddressBookPacked[] = {
  0x10, 0x23, 0x40, 0x01, 0x11, 0x01, 0x57, 0x51, 0x08, 0x01, 0x04, 0x11, 0x7b, 0x02, 0x11, 0x21,
  0x32, 0x11, 0x21, 0x92, 0x11, 0x29, 0x17, 0x11, 0x39, 0x22, 0x03, 0xc8, 0x01, 0x11, 0x35, 0x22,
  0x11, 0x35, 0x82, 0x11, 0x39, 0x27, 0x00, 0x00, 0x1f, 0x41, 0x6c, 0x69, 0x63, 0x65, 0xff, 0x61,
  0x6c, 0x69, 0x63, 0x65, 0x40, 0x65, 0x78, 0x01, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f,
  0x01, 0x6d, 0x51, 0x04, 0x01, 0x01, 0x00, 0x00, 0x11, 0x01, 0x4a, 0xff, 0x35, 0x35, 0x35, 0x2d,
  0x31, 0x32, 0x31, 0x32, 0x00, 0x00, 0x00, 0x07, 0x4d, 0x49, 0x54, 0x07, 0x42, 0x6f, 0x62, 0xff,
  0x62, 0x6f, 0x62, 0x40, 0x65, 0x78, 0x61, 0x6d, 0x01, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d,
  0x00, 0x51, 0x08, 0x01, 0x01, 0x01, 0x01, 0x11, 0x09, 0x4a, 0x01, 0x02, 0x11, 0x09, 0x4a, 0xff,
  0x35, 0x35, 0x35, 0x2d, 0x34, 0x35, 0x36, 0x37, 0x00, 0x00, 0x00, 0xff, 0x35, 0x35, 0x35, 0x2d,
  0x37, 0x36, 0x35, 0x34, 0x00, 0x00, 0x00,
};

/* Pinned copy of tests/fixtures/addressbook.canonical.bin (272 bytes). */
static const uint8_t kAddressBookCanonical[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x57, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x7b, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x21, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00, 0x92, 0x00, 0x00, 0x00,
  0x29, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
  0xc8, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x31, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
  0x31, 0x00, 0x00, 0x00, 0x82, 0x00, 0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x6c, 0x69, 0x63, 0x65, 0x00, 0x00, 0x00,
  0x61, 0x6c, 0x69, 0x63, 0x65, 0x40, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f,
  0x6d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00, 0x35, 0x35, 0x35, 0x2d, 0x31, 0x32, 0x31, 0x32,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x49, 0x54, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x42, 0x6f, 0x62, 0x00, 0x00, 0x00, 0x00, 0x00, 0x62, 0x6f, 0x62, 0x40, 0x65, 0x78, 0x61, 0x6d,
  0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a, 0x00, 0x00, 0x00,
  0x35, 0x35, 0x35, 0x2d, 0x34, 0x35, 0x36, 0x37, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x35, 0x35, 0x35, 0x2d, 0x37, 0x36, 0x35, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static std::vector<uint8_t> read_all(const char *path) {
  std::vector<uint8_t> out;
  FILE *f = fopen(path, "rb");
  if (!f) {
    return out;
  }
  uint8_t buf[512];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
    out.insert(out.end(), buf, buf + n);
  }
  fclose(f);
  return out;
}

static std::string join_dir(const char *dir, const char *name) {
  std::string path(dir);
  if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\') {
    path += '/';
  }
  path += name;
  return path;
}

static std::vector<uint8_t> load_fixture(const char *name) {
  std::vector<std::string> candidates;
  if (FORTRAN_GOLDEN_DIR[0] != '\0') {
    candidates.push_back(join_dir(FORTRAN_GOLDEN_DIR, name));
  }
  candidates.push_back(join_dir("tests/fixtures", name));
  candidates.push_back(join_dir("../tests/fixtures", name));
  candidates.push_back(join_dir("fixtures", name));
  for (size_t i = 0; i < candidates.size(); i++) {
    std::vector<uint8_t> body = read_all(candidates[i].c_str());
    if (!body.empty()) {
      return body;
    }
  }
  return std::vector<uint8_t>();
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

static void decode_and_check(const uint8_t *buf, size_t sz, int packed,
                             const char *label) {
  struct capn rc;
  ASSERT_EQ(0, capn_init_mem(&rc, buf, sz, packed)) << label << ": init";
  AddressBook_ptr rroot;
  rroot.p = capn_getp(capn_root(&rc), 0, 1);
  ASSERT_NE(CAPN_NULL, rroot.p.type) << label << ": root";
  struct AddressBook book;
  read_AddressBook(&book, rroot);
  expect_alice_bob(&book, label);
  capn_free(&rc);
}

static int build_alice_bob(struct capn *c) {
  capn_init_malloc(c);
  struct capn_segment *cs = capn_root(c).seg;

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
  return capn_set_root(c, ab.p);
}

static ssize_t encode_alice_bob(uint8_t *buf, size_t cap, int packed) {
  struct capn c;
  if (build_alice_bob(&c) != 0) {
    capn_free(&c);
    return -1;
  }
  ssize_t sz = capn_write_mem(&c, buf, cap, packed);
  capn_free(&c);
  return sz;
}

static void expect_file_matches(const char *name, const uint8_t *want, size_t want_sz) {
  std::vector<uint8_t> got = load_fixture(name);
  ASSERT_FALSE(got.empty())
      << "missing fixture " << name
      << " (set FORTRAN_GOLDEN_DIR or run from the source tree)";
  ASSERT_EQ(want_sz, got.size()) << name << " size";
  EXPECT_EQ(0, memcmp(want, got.data(), want_sz)) << name << " bytes";
}

TEST(FortranGolden, CommittedBinsMatchPinnedHex) {
  expect_file_matches("addressbook.bin", kAddressBookFlat, sizeof(kAddressBookFlat));
  expect_file_matches("addressbook.packed.bin", kAddressBookPacked,
                      sizeof(kAddressBookPacked));
  expect_file_matches("addressbook.canonical.bin", kAddressBookCanonical,
                      sizeof(kAddressBookCanonical));
}

TEST(FortranGolden, DecodeFlatGolden) {
  decode_and_check(kAddressBookFlat, sizeof(kAddressBookFlat), 0, "flat");
}

TEST(FortranGolden, DecodePackedGolden) {
  decode_and_check(kAddressBookPacked, sizeof(kAddressBookPacked), 1, "packed");
}

TEST(FortranGolden, DecodeCanonicalGolden) {
  /* capnp convert binary:canonical writes a single unframed segment.
   * Wrap it in a one-segment stream header so capn_init_mem can read it. */
  uint8_t framed[8 + sizeof(kAddressBookCanonical)];
  memset(framed, 0, sizeof(framed));
  uint32_t words = (uint32_t) (sizeof(kAddressBookCanonical) / 8);
  framed[4] = (uint8_t) words;
  framed[5] = (uint8_t) (words >> 8);
  framed[6] = (uint8_t) (words >> 16);
  framed[7] = (uint8_t) (words >> 24);
  memcpy(framed + 8, kAddressBookCanonical, sizeof(kAddressBookCanonical));
  decode_and_check(framed, sizeof(framed), 0, "canonical");
}

TEST(FortranGolden, EncodeRoundTrip) {
  uint8_t buf[4096];
  ssize_t sz = encode_alice_bob(buf, sizeof(buf), 0);
  ASSERT_GT(sz, 0);
  decode_and_check(buf, (size_t) sz, 0, "c-flat");
}

TEST(FortranGolden, EncodePackedRoundTrip) {
  uint8_t buf[4096];
  ssize_t sz = encode_alice_bob(buf, sizeof(buf), 1);
  ASSERT_GT(sz, 0);
  decode_and_check(buf, (size_t) sz, 1, "c-packed");
}

static void expect_unframed_eq(struct capn *c, const uint8_t *want, size_t want_n,
                               const char *label) {
  ASSERT_EQ(1u, c->segnum) << label << ": one segment";
  ASSERT_NE((struct capn_segment *) NULL, c->seglist) << label;
  ASSERT_EQ(want_n, c->seglist->len) << label << " unframed size";
  for (size_t i = 0; i < want_n; i += 8) {
    if (memcmp(c->seglist->data + i, want + i, 8) != 0) {
      ADD_FAILURE() << label << " word " << (i / 8);
      EXPECT_EQ(0, memcmp(c->seglist->data, want, want_n)) << label << " bytes";
      return;
    }
  }
}

TEST(FortranGolden, CanonicalizeCEncodeMatchesGolden) {
  struct capn src, dst;
  ASSERT_EQ(0, build_alice_bob(&src));
  capn_init_malloc(&dst);
  ASSERT_EQ(0, capn_canonicalize(&src, &dst));
  expect_unframed_eq(&dst, kAddressBookCanonical, sizeof(kAddressBookCanonical),
                     "c-encode");

  uint8_t framed[8 + sizeof(kAddressBookCanonical)];
  int64_t wsz = capn_write_mem(&dst, framed, sizeof(framed), 0);
  ASSERT_EQ((int64_t) sizeof(framed), wsz);
  decode_and_check(framed, (size_t) wsz, 0, "c-canonical-framed");

  capn_ptr people = capn_getp(capn_getp(capn_root(&dst), 0, 1), 0, 1);
  ASSERT_EQ(CAPN_LIST, people.type);
  capn_ptr alice = capn_getp(people, 0, 1);
  capn_ptr phones = capn_getp(alice, 2, 1);
  ASSERT_EQ(1, phones.is_composite_list);
  EXPECT_EQ(0u, phones.datasz) << "mobile type word truncated";
  EXPECT_EQ(1u, phones.ptrs);

  capn_free(&src);
  capn_free(&dst);
}

TEST(FortranGolden, CanonicalizeGoldenIsIdempotent) {
  uint8_t framed[8 + sizeof(kAddressBookCanonical)];
  memset(framed, 0, sizeof(framed));
  uint32_t words = (uint32_t) (sizeof(kAddressBookCanonical) / 8);
  framed[4] = (uint8_t) words;
  framed[5] = (uint8_t) (words >> 8);
  framed[6] = (uint8_t) (words >> 16);
  framed[7] = (uint8_t) (words >> 24);
  memcpy(framed + 8, kAddressBookCanonical, sizeof(kAddressBookCanonical));

  struct capn src, dst;
  ASSERT_EQ(0, capn_init_mem(&src, framed, sizeof(framed), 0));
  capn_init_malloc(&dst);
  ASSERT_EQ(0, capn_canonicalize(&src, &dst));
  expect_unframed_eq(&dst, kAddressBookCanonical, sizeof(kAddressBookCanonical),
                     "golden-again");
  capn_free(&src);
  capn_free(&dst);
}

TEST(Canonicalize, NullArgsAndUsedDstFail) {
  struct capn src, dst;
  capn_init_malloc(&src);
  capn_init_malloc(&dst);
  EXPECT_NE(0, capn_canonicalize(NULL, &dst));
  EXPECT_NE(0, capn_canonicalize(&src, NULL));
  EXPECT_NE(0, capn_canonicalize(&src, &src));
  (void) capn_root(&dst);
  EXPECT_NE(0, capn_canonicalize(&src, &dst));
  capn_free(&src);
  capn_free(&dst);
}

TEST(Canonicalize, NullRootIsEightZeroBytes) {
  struct capn src, dst;
  capn_init_malloc(&src);
  (void) capn_root(&src);
  capn_init_malloc(&dst);
  ASSERT_EQ(0, capn_canonicalize(&src, &dst));
  ASSERT_EQ(1u, dst.segnum);
  ASSERT_EQ((size_t) 8, dst.seglist->len);
  EXPECT_EQ(0, memcmp(dst.seglist->data, "\0\0\0\0\0\0\0\0", 8));
  capn_free(&src);
  capn_free(&dst);
}

TEST(Canonicalize, EmptyStructOffsetIsMinusOne) {
  /* Root empty struct with a non-canonical offset (B=1, C=0, D=0). */
  uint8_t framed[24];
  struct capn src, dst;
  memset(framed, 0, sizeof(framed));
  framed[4] = 2;
  framed[8] = 0x04; /* B = 1 word */
  ASSERT_EQ(0, capn_init_mem(&src, framed, sizeof(framed), 0));
  capn_init_malloc(&dst);
  ASSERT_EQ(0, capn_canonicalize(&src, &dst));
  ASSERT_EQ((size_t) 8, dst.seglist->len);
  EXPECT_EQ((unsigned char) 0xfc, (unsigned char) dst.seglist->data[0]);
  EXPECT_EQ((unsigned char) 0xff, (unsigned char) dst.seglist->data[1]);
  EXPECT_EQ((unsigned char) 0xff, (unsigned char) dst.seglist->data[2]);
  EXPECT_EQ((unsigned char) 0xff, (unsigned char) dst.seglist->data[3]);
  EXPECT_EQ(0, memcmp(dst.seglist->data + 4, "\0\0\0\0", 4));
  capn_ptr p = capn_getp(capn_root(&dst), 0, 1);
  EXPECT_EQ(CAPN_STRUCT, p.type);
  EXPECT_EQ(0u, p.datasz);
  EXPECT_EQ(0u, p.ptrs);
  capn_free(&src);
  capn_free(&dst);
}

TEST(Canonicalize, CyclicStructsFail) {
  uint8_t seg[24];
  uint8_t framed[32];
  struct capn src, dst;
  uint32_t neg;
  memset(seg, 0, sizeof(seg));
  /* root -> A (word 1, 0 data 1 ptr); A -> B (word 2); B -> A. */
  seg[6] = 0x01; /* D=1 */
  seg[14] = 0x01;
  neg = 1u + ~(2u << 2);
  seg[16] = (uint8_t) neg;
  seg[17] = (uint8_t) (neg >> 8);
  seg[18] = (uint8_t) (neg >> 16);
  seg[19] = (uint8_t) (neg >> 24);
  seg[22] = 0x01;
  memset(framed, 0, sizeof(framed));
  framed[4] = 3;
  memcpy(framed + 8, seg, sizeof(seg));
  ASSERT_EQ(0, capn_init_mem(&src, framed, sizeof(framed), 0));
  capn_init_malloc(&dst);
  EXPECT_NE(0, capn_canonicalize(&src, &dst));
  capn_free(&src);
  capn_free(&dst);
}
