/* bounds-test.cpp
 *
 * Decode-path bounds: truncated frames and malicious pointers must
 * fail cleanly (CAPN_NULL / -1) without over-read or sanitizer fire.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>

#include "capnp_c.h"
#include "addressbook.capnp.h"

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

static void wr32(uint8_t *p, uint32_t v) {
	p[0] = (uint8_t) v;
	p[1] = (uint8_t) (v >> 8);
	p[2] = (uint8_t) (v >> 16);
	p[3] = (uint8_t) (v >> 24);
}

static void wr64(uint8_t *p, uint64_t v) {
	wr32(p, (uint32_t) v);
	wr32(p + 4, (uint32_t) (v >> 32));
}

/* Frame a single segment: 8-byte header + payload. */
static int init_one_seg(struct capn *c, const uint8_t *seg, size_t segsz,
			uint8_t *framed, size_t framed_cap) {
	if (segsz % 8 != 0 || framed_cap < 8 + segsz)
		return -1;
	wr32(framed + 0, 0);
	wr32(framed + 4, (uint32_t) (segsz / 8));
	memcpy(framed + 8, seg, segsz);
	return capn_init_mem(c, framed, 8 + segsz, 0);
}

static capn_ptr root_getp(struct capn *c) {
	return capn_getp(capn_root(c), 0, 1);
}

TEST(DecodeBounds, TruncatedBufferHeaderClaimsMoreThanSz) {
	/* 1 segment, 10 words claimed; only header + 1 word supplied. */
	uint8_t buf[16];
	memset(buf, 0, sizeof(buf));
	wr32(buf + 0, 0);
	wr32(buf + 4, 10);

	struct capn c;
	EXPECT_NE(0, capn_init_mem(&c, buf, sizeof(buf), 0));
}

TEST(DecodeBounds, TruncatedSegmentBody) {
	/* Header + 2-word claim, buffer stops after the header. */
	uint8_t buf[8];
	wr32(buf + 0, 0);
	wr32(buf + 4, 2);

	struct capn c;
	EXPECT_NE(0, capn_init_mem(&c, buf, sizeof(buf), 0));
}

TEST(DecodeBounds, NullRootIsNull) {
	uint8_t seg[8];
	uint8_t framed[16];
	struct capn c;
	memset(seg, 0, sizeof(seg));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	capn_free(&c);
}

TEST(DecodeBounds, HugeStructOffsetIsNull) {
	/* Struct pointer, offset = 0x00FFFFFF words, 1 data word. */
	uint8_t seg[16];
	uint8_t framed[24];
	struct capn c;
	memset(seg, 0, sizeof(seg));
	wr64(seg, (UINT64_C(0x00FFFFFF) << 2) | (UINT64_C(1) << 32));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	capn_free(&c);
}

TEST(DecodeBounds, StructTargetPastSegmentIsNull) {
	/* offset=0, datasz=10 words, segment is only 2 words. */
	uint8_t seg[16];
	uint8_t framed[24];
	struct capn c;
	memset(seg, 0, sizeof(seg));
	wr64(seg, UINT64_C(10) << 32);
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	capn_free(&c);
}

TEST(DecodeBounds, HugeListCountIsNull) {
	/* List(UInt64), element count 2^28, 2-word segment. */
	uint8_t seg[16];
	uint8_t framed[24];
	struct capn c;
	uint64_t val;
	memset(seg, 0, sizeof(seg));
	val = UINT64_C(1)
		| (UINT64_C(5) << 32)
		| (UINT64_C(0x10000000) << 35);
	wr64(seg, val);
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	capn_free(&c);
}

TEST(DecodeBounds, FarPointerMissingSegmentIsNull) {
	/* Far pointer to segment id 99; message has only segment 0. */
	uint8_t seg[8];
	uint8_t framed[16];
	struct capn c;
	memset(seg, 0, sizeof(seg));
	wr64(seg, UINT64_C(2) | (UINT64_C(99) << 32));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	capn_free(&c);
}

TEST(DecodeBounds, FarPointerLandingPadOOBIsNull) {
	/* Far pointer, landing pad word-offset 100 in a 2-word segment. */
	uint8_t seg[16];
	uint8_t framed[24];
	struct capn c;
	memset(seg, 0, sizeof(seg));
	wr64(seg, UINT64_C(2) | (UINT64_C(100) << 3));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	capn_free(&c);
}

TEST(DecodeBounds, DoubleFarMissingSegmentIsNull) {
	/* Double-far (bits 0-2 = 6) to segment id 5. */
	uint8_t seg[8];
	uint8_t framed[16];
	struct capn c;
	memset(seg, 0, sizeof(seg));
	wr64(seg, UINT64_C(6) | (UINT64_C(5) << 32));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	capn_free(&c);
}

TEST(DecodeBounds, VoidListAmplificationIsNull) {
	/* List(Void) with 1e8 elements: payload is empty, traversal budget
	 * must still reject the amplification. */
	uint8_t seg[8];
	uint8_t framed[16];
	struct capn c;
	uint64_t val;
	memset(seg, 0, sizeof(seg));
	val = UINT64_C(1) | (UINT64_C(100000000) << 35);
	wr64(seg, val);
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	capn_free(&c);
}

TEST(DecodeBounds, ValidStructStillReads) {
	/* offset=0, datasz=1, payload 0xefcdab8967452301. */
	uint8_t seg[16];
	uint8_t framed[24];
	struct capn c;
	capn_ptr p;
	memset(seg, 0, sizeof(seg));
	wr64(seg, UINT64_C(1) << 32);
	wr64(seg + 8, UINT64_C(0xefcdab8967452301));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	p = root_getp(&c);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	EXPECT_EQ(8, p.datasz);
	EXPECT_EQ(0, p.ptrs);
	EXPECT_EQ(UINT64_C(0xefcdab8967452301), capn_read64(p, 0));
	capn_free(&c);
}

TEST(DecodeBounds, AddressBookRoundTrip) {
	uint8_t buf[4096];
	ssize_t sz = 0;
	const char *name = "Alice";
	const char *email = "alice@example.com";

	{
		struct capn c;
		capn_init_malloc(&c);
		capn_ptr cr = capn_root(&c);
		struct capn_segment *cs = cr.seg;

		struct Person person = {
			.id = 42,
			.name = chars_to_text(name),
			.email = chars_to_text(email),
		};
		person.employment_which = Person_employment_unemployed;
		person.phones = new_Person_PhoneNumber_list(cs, 1);
		{
			struct Person_PhoneNumber pn = {
				.number = chars_to_text("555"),
				.type = Person_PhoneNumber_Type_mobile,
			};
			set_Person_PhoneNumber(&pn, person.phones, 0);
		}

		struct AddressBook book;
		memset(&book, 0, sizeof(book));
		book.people = new_Person_list(cs, 1);
		set_Person(&person, book.people, 0);

		AddressBook_ptr bp = new_AddressBook(cs);
		write_AddressBook(&book, bp);
		ASSERT_EQ(0, capn_setp(cr, 0, bp.p));
		sz = capn_write_mem(&c, buf, sizeof(buf), 0);
		ASSERT_GT(sz, 0);
		capn_free(&c);
	}

	{
		struct capn rc;
		struct AddressBook rb;
		AddressBook_ptr rroot;
		struct Person rp;
		struct Person_PhoneNumber rpn;
		ASSERT_EQ(0, capn_init_mem(&rc, buf, (size_t) sz, 0));
		rroot.p = capn_getp(capn_root(&rc), 0, 1);
		ASSERT_EQ(CAPN_STRUCT, rroot.p.type);
		read_AddressBook(&rb, rroot);
		EXPECT_EQ(1, capn_len(rb.people));
		get_Person(&rp, rb.people, 0);
		EXPECT_EQ((uint32_t) 42, rp.id);
		EXPECT_CAPN_TEXT_EQ(name, rp.name);
		EXPECT_CAPN_TEXT_EQ(email, rp.email);
		EXPECT_EQ(1, capn_len(rp.phones));
		get_Person_PhoneNumber(&rpn, rp.phones, 0);
		EXPECT_CAPN_TEXT_EQ("555", rpn.number);
		EXPECT_EQ(Person_PhoneNumber_Type_mobile, rpn.type);
		capn_free(&rc);
	}
}
