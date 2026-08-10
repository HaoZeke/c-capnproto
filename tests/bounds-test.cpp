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
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	EXPECT_EQ(1, capn_ok(&c));
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
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	EXPECT_EQ(0, capn_ok(&c));
	capn_clear_err(&c);
	EXPECT_EQ(1, capn_ok(&c));
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
	EXPECT_EQ(0, capn_ok(&c));
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
	EXPECT_EQ(0, capn_ok(&c));
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
	EXPECT_EQ(0, capn_ok(&c));
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
	EXPECT_EQ(0, capn_ok(&c));
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
	EXPECT_EQ(0, capn_ok(&c));
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
	EXPECT_EQ(0, capn_ok(&c));
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
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}

/* N structs (0 data / 1 ptr) chained from the root pointer. Word i
 * (i < n) is a struct pointer to word i+1; word n is null. */
static int init_struct_chain(struct capn *c, int n, uint8_t *seg,
			     size_t seg_cap, uint8_t *framed, size_t framed_cap) {
	size_t segsz;
	int i;

	if (n < 1)
		return -1;
	segsz = (size_t) (n + 1) * 8;
	if (seg_cap < segsz)
		return -1;
	memset(seg, 0, segsz);
	for (i = 0; i < n; i++)
		wr64(seg + (size_t) i * 8, UINT64_C(1) << 48);
	return init_one_seg(c, seg, segsz, framed, framed_cap);
}

static capn_ptr follow_ptrs(capn_ptr p, int hops) {
	int i;
	for (i = 0; i < hops; i++)
		p = capn_getp(p, 0, 1);
	return p;
}

static int init_two_seg(struct capn *c,
			const uint8_t *s0, size_t z0,
			const uint8_t *s1, size_t z1,
			uint8_t *framed, size_t framed_cap) {
	size_t need = 16 + z0 + z1;
	if (z0 % 8 != 0 || z1 % 8 != 0 || framed_cap < need)
		return -1;
	wr32(framed + 0, 1);
	wr32(framed + 4, (uint32_t) (z0 / 8));
	wr32(framed + 8, (uint32_t) (z1 / 8));
	wr32(framed + 12, 0);
	memcpy(framed + 16, s0, z0);
	memcpy(framed + 16 + z0, s1, z1);
	return capn_init_mem(c, framed, need, 0);
}

TEST(DecodeGraph, OverDeepNestingGetpIsNull) {
	uint8_t seg[64];
	uint8_t framed[72];
	struct capn c;
	capn_ptr p;

	ASSERT_EQ(0, init_struct_chain(&c, 5, seg, sizeof(seg), framed, sizeof(framed)));
	c.nesting_limit = 3;
	p = root_getp(&c);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	p = follow_ptrs(p, 2);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	p = capn_getp(p, 0, 1);
	EXPECT_EQ(CAPN_NULL, p.type);
	EXPECT_EQ(0, capn_ok(&c));
	capn_free(&c);
}

TEST(DecodeGraph, OverDeepNestingValidateFails) {
	uint8_t seg[64];
	uint8_t framed[72];
	struct capn c;

	ASSERT_EQ(0, init_struct_chain(&c, 5, seg, sizeof(seg), framed, sizeof(framed)));
	c.nesting_limit = 3;
	EXPECT_NE(0, capn_validate(&c));
	capn_free(&c);
}

TEST(DecodeGraph, NestingAtLimitValidateOk) {
	uint8_t seg[64];
	uint8_t framed[72];
	struct capn c;
	capn_ptr p;

	ASSERT_EQ(0, init_struct_chain(&c, 3, seg, sizeof(seg), framed, sizeof(framed)));
	c.nesting_limit = 3;
	EXPECT_EQ(0, capn_validate(&c));
	p = follow_ptrs(root_getp(&c), 2);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	p = capn_getp(p, 0, 1);
	EXPECT_EQ(CAPN_NULL, p.type);
	capn_free(&c);
}

TEST(DecodeGraph, DefaultNestingAllows64Rejects65) {
	uint8_t seg[600];
	uint8_t framed[616];
	struct capn c;
	capn_ptr p;

	ASSERT_EQ(0, init_struct_chain(&c, 65, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(0, c.nesting_limit);
	EXPECT_EQ(1, capn_ok(&c));
	p = follow_ptrs(root_getp(&c), 63);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	EXPECT_EQ(1, capn_ok(&c));
	p = capn_getp(p, 0, 1);
	EXPECT_EQ(CAPN_NULL, p.type);
	EXPECT_EQ(0, capn_ok(&c));
	EXPECT_NE(0, capn_validate(&c));
	capn_free(&c);

	ASSERT_EQ(0, init_struct_chain(&c, 64, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(1, capn_ok(&c));
	p = follow_ptrs(root_getp(&c), 63);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_EQ(0, capn_validate(&c));
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}

TEST(DecodeGraph, MissingOptionalPtrFieldKeepsOk) {
	/* Struct: 0 data words, 1 pointer, that pointer is a wire-null word. */
	uint8_t seg[16];
	uint8_t framed[24];
	struct capn c;
	capn_ptr p, child;

	memset(seg, 0, sizeof(seg));
	wr64(seg, UINT64_C(1) << 48);
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(1, capn_ok(&c));
	p = root_getp(&c);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	EXPECT_EQ(1, (int) p.ptrs);
	EXPECT_EQ(1, capn_ok(&c));
	child = capn_getp(p, 0, 1);
	EXPECT_EQ(CAPN_NULL, child.type);
	EXPECT_EQ(1, capn_ok(&c));
	/* Schema evolution: a slot past ptrs is missing, not broken. */
	child = capn_getp(p, 1, 1);
	EXPECT_EQ(CAPN_NULL, child.type);
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}

TEST(DecodeGraph, CyclicStructsValidateFails) {
	/* A (word 1) -> B (word 2) -> A. Offset from word 2 to word 1 is -2. */
	uint8_t seg[24];
	uint8_t framed[32];
	struct capn c;
	uint32_t neg;
	capn_ptr p;
	int i;

	memset(seg, 0, sizeof(seg));
	wr64(seg + 0, UINT64_C(1) << 48);
	wr64(seg + 8, UINT64_C(1) << 48);
	neg = 1u + ~(2u << 2);
	wr64(seg + 16, (uint64_t) neg | (UINT64_C(1) << 48));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_NE(0, capn_validate(&c));

	p = root_getp(&c);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	for (i = 0; i < 100; i++) {
		p = capn_getp(p, 0, 1);
		if (p.type == CAPN_NULL)
			break;
	}
	EXPECT_EQ(CAPN_NULL, p.type);
	EXPECT_LT(i, 100);
	capn_free(&c);
}

TEST(DecodeGraph, CyclicFarPointersValidateFails) {
	/* Seg0 A.ptr is far to seg1 word 0 (B). Seg1 B.ptr is far to
	 * seg0 word 0 (A). */
	uint8_t s0[16];
	uint8_t s1[16];
	uint8_t framed[48];
	struct capn c;
	capn_ptr p;
	int i;

	memset(s0, 0, sizeof(s0));
	memset(s1, 0, sizeof(s1));
	wr64(s0 + 0, UINT64_C(1) << 48);
	wr64(s0 + 8, UINT64_C(2) | (UINT64_C(1) << 32));
	wr64(s1 + 0, UINT64_C(1) << 48);
	wr64(s1 + 8, UINT64_C(2));
	ASSERT_EQ(0, init_two_seg(&c, s0, sizeof(s0), s1, sizeof(s1),
				 framed, sizeof(framed)));
	EXPECT_NE(0, capn_validate(&c));

	p = root_getp(&c);
	EXPECT_EQ(CAPN_STRUCT, p.type);
	for (i = 0; i < 100; i++) {
		p = capn_getp(p, 0, 1);
		if (p.type == CAPN_NULL)
			break;
	}
	EXPECT_EQ(CAPN_NULL, p.type);
	EXPECT_LT(i, 100);
	capn_free(&c);
}

TEST(DecodeGraph, VoidListAmplificationValidateFails) {
	uint8_t seg[8];
	uint8_t framed[16];
	struct capn c;
	uint64_t val;

	memset(seg, 0, sizeof(seg));
	val = UINT64_C(1) | (UINT64_C(100000000) << 35);
	wr64(seg, val);
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(CAPN_NULL, root_getp(&c).type);
	c.traversal_used = 0;
	EXPECT_NE(0, capn_validate(&c));
	capn_free(&c);
}

TEST(DecodeGraph, NullRootValidateOk) {
	uint8_t seg[8];
	uint8_t framed[16];
	struct capn c;

	memset(seg, 0, sizeof(seg));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(0, capn_validate(&c));
	capn_free(&c);
}

TEST(DecodeGraph, ValidStructValidateOk) {
	uint8_t seg[16];
	uint8_t framed[24];
	struct capn c;

	memset(seg, 0, sizeof(seg));
	wr64(seg, UINT64_C(1) << 32);
	wr64(seg + 8, UINT64_C(0xefcdab8967452301));
	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	EXPECT_EQ(0, capn_validate(&c));
	EXPECT_EQ(CAPN_STRUCT, root_getp(&c).type);
	capn_free(&c);
}

/* Composite List(T): tag + two structs, each 0 data / 1 pointer.
 * Decode sets list.data to T[0], the same address as the list body.
 * A vset keyed only by (seg, data) therefore aliases List(T) and T[0]. */
static void wr_composite_list_of_two(uint8_t *seg, size_t t0_ptr, size_t t1_ptr) {
	/* word 0: list pointer, offset 0, C=7 composite, D=2 words */
	wr64(seg + 0, UINT64_C(1) | (UINT64_C(7) << 32) | (UINT64_C(2) << 35));
	/* word 1: tag, count=2, data=0, ptrs=1 */
	wr64(seg + 8, (UINT64_C(2) << 2) | (UINT64_C(1) << 48));
	wr64(seg + 16, t0_ptr);
	wr64(seg + 24, t1_ptr);
}

TEST(DecodeGraph, CompositeListAfterElem0StillWalksTail) {
	/* Root struct (2 ptrs): ptr[0] = T[0], ptr[1] = List(T).
	 * T[1] is a self-cycle. After visiting T[0], validate must still
	 * walk T[1] and fail. A (seg, data) vset marks List(T) done. */
	uint8_t seg[48];
	uint8_t framed[56];
	struct capn c;
	uint32_t neg;
	capn_ptr root, t0, list, t1;

	memset(seg, 0, sizeof(seg));
	/* word 0: root struct, offset 0, 0 data, 2 ptrs */
	wr64(seg + 0, UINT64_C(2) << 48);
	/* word 1: struct ptr to T[0] at word 4; offset from word 2 is 2 */
	wr64(seg + 8, (UINT64_C(2) << 2) | (UINT64_C(1) << 48));
	/* word 2: list ptr to tag at word 3; offset 0, C=7, D=2 */
	wr64(seg + 16, UINT64_C(1) | (UINT64_C(7) << 32) | (UINT64_C(2) << 35));
	/* word 3: composite tag, count=2, data=0, ptrs=1 */
	wr64(seg + 24, (UINT64_C(2) << 2) | (UINT64_C(1) << 48));
	/* word 4: T[0].ptr null */
	/* word 5: T[1] self-cycle, offset -1 */
	neg = 1u + ~(1u << 2);
	wr64(seg + 40, (uint64_t) neg | (UINT64_C(1) << 48));

	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	root = root_getp(&c);
	ASSERT_EQ(CAPN_STRUCT, root.type);
	EXPECT_EQ(0, root.datasz);
	EXPECT_EQ(2, root.ptrs);

	t0 = capn_getp(root, 0, 1);
	ASSERT_EQ(CAPN_STRUCT, t0.type);
	EXPECT_EQ(0, t0.datasz);
	EXPECT_EQ(1, t0.ptrs);

	list = capn_getp(root, 1, 1);
	ASSERT_EQ(CAPN_LIST, list.type);
	EXPECT_EQ(1, list.is_composite_list);
	EXPECT_EQ(2, list.len);
	EXPECT_EQ(t0.data, list.data);

	t1 = capn_getp(list, 1, 1);
	ASSERT_EQ(CAPN_STRUCT, t1.type);
	EXPECT_EQ(CAPN_STRUCT, capn_getp(t1, 0, 1).type);

	EXPECT_NE(0, capn_validate(&c));
	capn_free(&c);
}

TEST(DecodeGraph, CompositeListElem0FromTailIsNotFalseCycle) {
	/* List(T) of 2; T[1].ptr -> T[0]. Shared subobject, not a cycle.
	 * While List(T) is on the path, a (seg, data) vset treats T[0] as
	 * re-entering the list. */
	uint8_t seg[32];
	uint8_t framed[40];
	struct capn c;
	uint32_t neg;
	capn_ptr list, t0, t1;

	memset(seg, 0, sizeof(seg));
	neg = 1u + ~(2u << 2);
	wr_composite_list_of_two(seg, 0, (uint64_t) neg | (UINT64_C(1) << 48));

	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	list = root_getp(&c);
	ASSERT_EQ(CAPN_LIST, list.type);
	EXPECT_EQ(1, list.is_composite_list);
	EXPECT_EQ(2, list.len);
	t0 = capn_getp(list, 0, 1);
	t1 = capn_getp(list, 1, 1);
	ASSERT_EQ(CAPN_STRUCT, t0.type);
	ASSERT_EQ(CAPN_STRUCT, t1.type);
	EXPECT_EQ(t0.data, capn_getp(t1, 0, 1).data);
	EXPECT_EQ(t0.data, list.data);

	EXPECT_EQ(0, capn_validate(&c));
	capn_free(&c);
}

TEST(DecodeGraph, CompositeListAndElem0BothCleanValidateOk) {
	/* Same layout as CompositeListAfterElem0StillWalksTail, T[1] null. */
	uint8_t seg[48];
	uint8_t framed[56];
	struct capn c;
	capn_ptr root, t0, list;

	memset(seg, 0, sizeof(seg));
	wr64(seg + 0, UINT64_C(2) << 48);
	wr64(seg + 8, (UINT64_C(2) << 2) | (UINT64_C(1) << 48));
	wr64(seg + 16, UINT64_C(1) | (UINT64_C(7) << 32) | (UINT64_C(2) << 35));
	wr64(seg + 24, (UINT64_C(2) << 2) | (UINT64_C(1) << 48));

	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof(seg), framed, sizeof(framed)));
	root = root_getp(&c);
	t0 = capn_getp(root, 0, 1);
	list = capn_getp(root, 1, 1);
	ASSERT_EQ(CAPN_STRUCT, t0.type);
	ASSERT_EQ(CAPN_LIST, list.type);
	EXPECT_EQ(t0.data, list.data);
	EXPECT_EQ(0, capn_validate(&c));
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
		EXPECT_EQ(0, capn_validate(&rc));
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
