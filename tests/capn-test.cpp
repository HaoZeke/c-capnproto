/* capn-test.cpp
 *
 * Copyright (C) 2013 James McKaskill
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <cstddef>

static int g_AddTag = 1;
#define ADD_TAG g_AddTag

#include "capn.c"
#include "capn-malloc.c"

template <int wordCount>
union AlignedData {
  uint8_t bytes[wordCount * 8];
  uint64_t words[wordCount];
};

class Session {
public:
  Session() {capn_init_malloc(&capn);}
  ~Session() {capn_free(&capn);}
  struct capn capn;
};





TEST(WireFormat, SimpleRawDataStruct) {
  AlignedData<2> data = {{
    // Struct ref, offset = 1, dataSize = 1, referenceCount = 0
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    // Content for the data segment.
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
  }};

  struct capn_segment seg;
  memset(&seg, 0, sizeof(seg));
  seg.data = (char*) data.bytes;
  seg.len = seg.cap = sizeof(data.bytes);

  struct capn ctx;
  memset(&ctx, 0, sizeof(ctx));
  capn_append_segment(&ctx, &seg);

  EXPECT_EQ(&seg, ctx.seglist);
  EXPECT_EQ(&seg, ctx.lastseg);
  EXPECT_EQ(&seg.hdr, ctx.segtree);
  EXPECT_EQ(1, ctx.segnum);
  EXPECT_EQ(0, seg.id);

  struct capn_ptr ptr = capn_getp(capn_root(&ctx), 0, 1);
  EXPECT_EQ(CAPN_STRUCT, ptr.type);
  EXPECT_EQ(8, ptr.datasz);
  EXPECT_EQ(0, ptr.ptrs);

  EXPECT_EQ(UINT64_C(0xefcdab8967452301), capn_read64(ptr, 0));
  EXPECT_EQ(UINT64_C(0), capn_read64(ptr, 8));
  EXPECT_EQ(UINT32_C(0x67452301), capn_read32(ptr, 0));
  EXPECT_EQ(UINT32_C(0xefcdab89), capn_read32(ptr, 4));
  EXPECT_EQ(UINT32_C(0), capn_read32(ptr, 8));
  EXPECT_EQ(UINT16_C(0x2301), capn_read16(ptr, 0));
  EXPECT_EQ(UINT16_C(0x6745), capn_read16(ptr, 2));
  EXPECT_EQ(UINT16_C(0xab89), capn_read16(ptr, 4));
  EXPECT_EQ(UINT16_C(0xefcd), capn_read16(ptr, 6));
  EXPECT_EQ(UINT16_C(0), capn_read16(ptr, 8));
}

static void setupStruct(struct capn *ctx) {
  struct capn_ptr root = capn_root(ctx);
  ASSERT_EQ(CAPN_PTR_LIST, root.type);
  ASSERT_EQ(1, root.len);

  struct capn_ptr ptr = capn_new_struct(root.seg, 16, 6);
  ASSERT_EQ(CAPN_STRUCT, ptr.type);
  EXPECT_EQ(16, ptr.datasz);
  EXPECT_EQ(6, ptr.ptrs);
  EXPECT_EQ(0, capn_setp(root, 0, ptr));

  EXPECT_EQ(0, capn_write64(ptr, 0, UINT64_C(0x1011121314151617)));
  EXPECT_EQ(0, capn_write32(ptr, 8, UINT32_C(0x20212223)));
  EXPECT_EQ(0, capn_write16(ptr, 12, UINT16_C(0x3031)));
  EXPECT_EQ(0, capn_write8(ptr, 14, 0x40));
  EXPECT_EQ(0, capn_write8(ptr, 15, (1 << 6) | (1 << 5) | (1 << 4) | (1 << 2)));

  capn_ptr subStruct = capn_new_struct(ptr.seg, 8, 0);
  ASSERT_EQ(CAPN_STRUCT, subStruct.type);
  EXPECT_EQ(8, subStruct.datasz);
  EXPECT_EQ(0, subStruct.ptrs);
  EXPECT_EQ(0, capn_write32(subStruct, 0, 123));
  EXPECT_NE(0, capn_write32(subStruct, 8, 124));
  EXPECT_EQ(0, capn_setp(ptr, 0, subStruct));

  capn_list32 list32 = capn_new_list32(ptr.seg, 3);
  capn_list64 list64 = {list32.p};
  ASSERT_EQ(CAPN_LIST, list32.p.type);
  EXPECT_EQ(3, list32.p.len);
  EXPECT_EQ(4, list32.p.datasz);
  EXPECT_EQ(0, capn_set32(list32, 0, 200));
  EXPECT_EQ(0, capn_set32(list32, 1, 201));
  EXPECT_EQ(0, capn_set32(list32, 2, 202));
  EXPECT_NE(0, capn_set32(list32, 3, 203));
  EXPECT_NE(0, capn_set64(list64, 0, 405));
  EXPECT_EQ(0, capn_setp(ptr, 1, list32.p));

  capn_ptr list = capn_new_list(ptr.seg, 4, 4, 1);
  ASSERT_EQ(CAPN_LIST, list.type);
  ASSERT_EQ(1, list.is_composite_list);
  EXPECT_EQ(4, list.len);
  EXPECT_EQ(8, list.datasz);
  EXPECT_EQ(1, list.ptrs);
  EXPECT_EQ(0, capn_setp(ptr, 2, list));
  for (int i = 0; i < 4; i++) {
    capn_ptr element = capn_getp(list, i, 1);
    ASSERT_EQ(CAPN_STRUCT, element.type);
    EXPECT_EQ(1, element.is_list_member);
    EXPECT_EQ(8, element.datasz);
    EXPECT_EQ(1, element.ptrs);
    EXPECT_EQ(0, capn_write32(element, 0, 300+i));

    capn_ptr subelement = capn_new_struct(element.seg, 8, 0);
    ASSERT_EQ(CAPN_STRUCT, subelement.type);
    EXPECT_EQ(8, subelement.datasz);
    EXPECT_EQ(0, subelement.ptrs);
    EXPECT_EQ(0, capn_write32(subelement, 0, 400+i));
    EXPECT_EQ(0, capn_setp(element, 0, subelement));
  }

  list = capn_new_ptr_list(ptr.seg, 5);
  ASSERT_EQ(CAPN_PTR_LIST, list.type);
  EXPECT_EQ(5, list.len);
  EXPECT_EQ(0, capn_setp(ptr, 3, list));
  for (int i = 0; i < 5; i++) {
    capn_list16 element = capn_new_list16(list.seg, i+1);
    ASSERT_EQ(CAPN_LIST, element.p.type);
    EXPECT_EQ(i+1, element.p.len);
    EXPECT_EQ(2, element.p.datasz);
    EXPECT_EQ(0, element.p.ptrs);
    EXPECT_EQ(0, capn_setp(list, i, element.p));
    for (int j = 0; j <= i; j++) {
      EXPECT_EQ(0, capn_set16(element, j, 500+j));
    }
  }

  capn_ptr recurse = capn_new_struct(ptr.seg, 0, 2);
  EXPECT_EQ(CAPN_STRUCT, recurse.type);
  EXPECT_EQ(0, recurse.datasz);
  EXPECT_EQ(2, recurse.ptrs);
  EXPECT_EQ(0, capn_setp(recurse, 0, recurse));
  EXPECT_EQ(0, capn_setp(ptr, 4, recurse));

}

static void checkStruct(struct capn *ctx) {
  capn_ptr ptr = capn_getp(capn_root(ctx), 0, 1);
  EXPECT_EQ(CAPN_STRUCT, ptr.type);
  EXPECT_EQ(16, ptr.datasz);
  EXPECT_EQ(6, ptr.ptrs);
  EXPECT_EQ(UINT64_C(0x1011121314151617), capn_read64(ptr, 0));
  EXPECT_EQ(UINT32_C(0x20212223), capn_read32(ptr, 8));
  EXPECT_EQ(0x3031, capn_read16(ptr, 12));
  EXPECT_EQ(0x40, capn_read8(ptr, 14));
  EXPECT_EQ((1 << 6) | (1 << 5) | (1 << 4) | (1 << 2), capn_read8(ptr, 15));

  capn_ptr subStruct = capn_getp(ptr, 0, 1);
  EXPECT_EQ(CAPN_STRUCT, subStruct.type);
  EXPECT_EQ(8, subStruct.datasz);
  EXPECT_EQ(0, subStruct.ptrs);
  EXPECT_EQ(123, capn_read32(subStruct, 0));

  capn_list32 list32 = {capn_getp(ptr, 1, 1)};
  capn_list8 list8 = {list32.p};
  capn_list16 list16 = {list32.p};
  capn_list64 list64 = {list32.p};
  EXPECT_EQ(CAPN_LIST, list32.p.type);
  EXPECT_EQ(3, list32.p.len);
  EXPECT_EQ(4, list32.p.datasz);
  EXPECT_EQ(0, list32.p.ptrs);
  EXPECT_EQ(200, capn_get32(list32, 0));
  EXPECT_EQ(201, capn_get32(list32, 1));
  EXPECT_EQ(202, capn_get32(list32, 2));
  EXPECT_EQ(0, capn_get32(list32, 3));
  EXPECT_EQ(0, capn_get64(list64, 0));
  EXPECT_EQ(201, capn_get8(list8, 1));
  EXPECT_EQ(202, capn_get16(list16, 2));

  capn_ptr list = capn_getp(ptr, 2, 1);
  EXPECT_EQ(CAPN_LIST, list.type);
  EXPECT_EQ(1, list.is_composite_list);
  EXPECT_EQ(4, list.len);
  EXPECT_EQ(8, list.datasz);
  EXPECT_EQ(1, list.ptrs);

  for (int i = 0; i < 4; i++) {
    capn_ptr element = capn_getp(list, i, 1);
    EXPECT_EQ(CAPN_STRUCT, element.type);
    EXPECT_EQ(1, element.is_list_member);
    EXPECT_EQ(8, element.datasz);
    EXPECT_EQ(1, element.ptrs);
    EXPECT_EQ(300+i, capn_read32(element,0));

    capn_ptr subelement = capn_getp(element, 0, 1);
    EXPECT_EQ(CAPN_STRUCT, subelement.type);
    EXPECT_EQ(8, subelement.datasz);
    EXPECT_EQ(0, subelement.ptrs);
    EXPECT_EQ(400+i, capn_read32(subelement, 0));
  }

  list = capn_getp(ptr, 3, 1);
  EXPECT_EQ(CAPN_PTR_LIST, list.type);
  EXPECT_EQ(5, list.len);
  for (int i = 0; i < 5; i++) {
    capn_list16 element = {capn_getp(list, i, 1)};
    EXPECT_EQ(CAPN_LIST, element.p.type);
    EXPECT_EQ(i+1, element.p.len);
    EXPECT_EQ(2, element.p.datasz);
    EXPECT_EQ(0, element.p.ptrs);
    for (int j = 0; j <= i; j++) {
      EXPECT_EQ(500+j, capn_get16(element, j));
    }
  }

  capn_ptr recurse = capn_getp(ptr, 4, 1);
  EXPECT_EQ(CAPN_STRUCT, recurse.type);
  EXPECT_EQ(0, recurse.datasz);
  EXPECT_EQ(2, recurse.ptrs);
  capn_ptr recurse_mbr = capn_getp(recurse, 0, 1);
  EXPECT_EQ(CAPN_STRUCT, recurse_mbr.type);
  EXPECT_EQ(0, recurse_mbr.datasz);
  EXPECT_EQ(2, recurse_mbr.ptrs);
  EXPECT_EQ(recurse.seg, recurse_mbr.seg);
  EXPECT_EQ(recurse.data, recurse_mbr.data);
  EXPECT_EQ(CAPN_NULL, capn_getp(recurse, 1, 1).type);
}

TEST(WireFormat, StructRoundTrip_OneSegment) {
  Session ctx;
  setupStruct(&ctx.capn);

  // word count:
  //    1  root reference
  //    8  root struct
  //    1  sub message
  //    2  3-element int32 list
  //   13  struct list
  //         1 tag
  //        12 4x struct
  //           1 data segment
  //           1 reference segment
  //           1 sub-struct
  //   11  list list
  //         5 references to sub-lists
  //         6 sub-lists (4x 1 word, 1x 2 words)
  //    2  recurse
  // -----
  //   38
  ASSERT_EQ(1, ctx.capn.segnum);
  EXPECT_EQ(38*8, ctx.capn.seglist->len);

  checkStruct(&ctx.capn);

  struct capn ctx2;
  memset(&ctx2, 0, sizeof(ctx2));
  capn_append_segment(&ctx2, ctx.capn.seglist);
  checkStruct(&ctx2);
}

static struct capn_segment *CreateSmallSegment(void *u, uint32_t id, int sz) {
  sz += sizeof(struct capn_segment);
  struct capn_segment *s = (struct capn_segment*) calloc(1, sz);
  s->data = (char*) (s+1);
  s->cap = sz - sizeof(*s);
  s->user = s;
  return s;
}

static void getSegments(struct capn *c, struct capn_segment **s, size_t num) {
  ASSERT_EQ(num, c->segnum);

  s[0] = c->seglist;
  for (size_t i = 1; i < num; i++) {
    s[i] = s[i-1]->next;
  }

  for (size_t i = 0; i < num; i++) {
    EXPECT_EQ(s[i]->id, i);
  }
}

TEST(WireFormat, StructRoundTrip_OneSegmentPerAllocation) {
  Session ctx;
  ctx.capn.create = &CreateSmallSegment;

  setupStruct(&ctx.capn);

  struct capn_segment *segments[16];
  getSegments(&ctx.capn, segments, 16);

  // Check that each segment has the expected size.  Recall that the first word of each segment will
  // actually be a reference to the first thing allocated within that segment.
  EXPECT_EQ( 8, segments[ 0]->len);  // root ref
  EXPECT_EQ(72, segments[ 1]->len);  // root struct
  EXPECT_EQ(16, segments[ 2]->len);  // sub-struct
  EXPECT_EQ(24, segments[ 3]->len);  // 3-element int32 list
  EXPECT_EQ(80, segments[ 4]->len);  // struct list
  EXPECT_EQ(16, segments[ 5]->len);  // struct list substruct 1
  EXPECT_EQ(16, segments[ 6]->len);  // struct list substruct 2
  EXPECT_EQ(16, segments[ 7]->len);  // struct list substruct 3
  EXPECT_EQ(16, segments[ 8]->len);  // struct list substruct 4
  EXPECT_EQ(48, segments[ 9]->len);  // list list
  EXPECT_EQ(16, segments[10]->len);  // list list sublist 1
  EXPECT_EQ(16, segments[11]->len);  // list list sublist 2
  EXPECT_EQ(16, segments[12]->len);  // list list sublist 3
  EXPECT_EQ(16, segments[13]->len);  // list list sublist 4
  EXPECT_EQ(24, segments[14]->len);  // list list sublist 5
  EXPECT_EQ(24, segments[15]->len);  // recurse struct

  checkStruct(&ctx.capn);

  struct capn ctx2;
  memset(&ctx2, 0, sizeof(ctx2));
  for (size_t i = 0; i < sizeof(segments)/sizeof(segments[0]); i++) {
    capn_append_segment(&ctx2, segments[i]);
  }

  checkStruct(&ctx2);
}

TEST(WireFormat, StructRoundTrip_OneSegmentPerAllocation_NoTag) {
  Session ctx;
  ctx.capn.create = &CreateSmallSegment;

  g_AddTag = 0;
  setupStruct(&ctx.capn);
  g_AddTag = 1;

  struct capn_segment *segments[31];
  getSegments(&ctx.capn, segments, 31);

  // Check that each segment has the expected size. Note that we have plenty
  // of 16 byte double far ptrs.
  EXPECT_EQ( 8, segments[ 0]->len);  // root ref
  EXPECT_EQ(64, segments[ 1]->len);  // root struct
  EXPECT_EQ(16, segments[ 2]->len);  // root struct ptr
  EXPECT_EQ( 8, segments[ 3]->len);  // sub-struct
  EXPECT_EQ(16, segments[ 4]->len);  // sub-struct ptr
  EXPECT_EQ(16, segments[ 5]->len);  // 3-element int32 list
  EXPECT_EQ(16, segments[ 6]->len);  // 3-element int32 list ptr
  EXPECT_EQ(72, segments[ 7]->len);  // struct list
  EXPECT_EQ(16, segments[ 8]->len);  // struct list ptr
  EXPECT_EQ( 8, segments[ 9]->len);  // struct list substruct 1
  EXPECT_EQ(16, segments[10]->len);  // struct list substruct 1 ptr
  EXPECT_EQ( 8, segments[11]->len);  // struct list substruct 2
  EXPECT_EQ(16, segments[12]->len);  // struct list substruct 2 ptr
  EXPECT_EQ( 8, segments[13]->len);  // struct list substruct 3
  EXPECT_EQ(16, segments[14]->len);  // struct list substruct 3 ptr
  EXPECT_EQ( 8, segments[15]->len);  // struct list substruct 4
  EXPECT_EQ(16, segments[16]->len);  // struct list substruct 4 ptr
  EXPECT_EQ(40, segments[17]->len);  // list list
  EXPECT_EQ(16, segments[18]->len);  // list list ptr
  EXPECT_EQ( 8, segments[19]->len);  // list list sublist 1
  EXPECT_EQ(16, segments[20]->len);  // list list sublist 1 ptr
  EXPECT_EQ( 8, segments[21]->len);  // list list sublist 2
  EXPECT_EQ(16, segments[22]->len);  // list list sublist 2 ptr
  EXPECT_EQ( 8, segments[23]->len);  // list list sublist 3
  EXPECT_EQ(16, segments[24]->len);  // list list sublist 3 ptr
  EXPECT_EQ( 8, segments[25]->len);  // list list sublist 4
  EXPECT_EQ(16, segments[26]->len);  // list list sublist 4 ptr
  EXPECT_EQ(16, segments[27]->len);  // list list sublist 5
  EXPECT_EQ(16, segments[28]->len);  // list list sublist 5 ptr
  EXPECT_EQ(16, segments[29]->len);  // recurse struct
  EXPECT_EQ(16, segments[30]->len);  // recurse struct ptr

  checkStruct(&ctx.capn);

  struct capn ctx2;
  memset(&ctx2, 0, sizeof(ctx2));
  for (size_t i = 0; i < sizeof(segments)/sizeof(segments[0]); i++) {
    capn_append_segment(&ctx2, segments[i]);
  }

  checkStruct(&ctx2);
}


static struct capn_segment *CreateSegment64(void *u, uint32_t id, int sz) {
  if (sz < 64) {
    sz = 64;
  }
  sz += sizeof(struct capn_segment);
  struct capn_segment *s = (struct capn_segment*) calloc(1, sz);
  s->data = (char*) (s+1);
  s->cap = sz - sizeof(*s);
  s->user = s;
  return s;
}

TEST(WireFormat, StructRoundTrip_MultipleSegmentsWithMultipleAllocations) {
  Session ctx;
  ctx.capn.create = &CreateSegment64;

  setupStruct(&ctx.capn);

  // Verify that we made 6 segments.
  ASSERT_EQ(6, ctx.capn.segnum);

  struct capn_segment *segments[6];
  segments[0] = ctx.capn.seglist;
  for (int i = 1; i < 6; i++) {
    segments[i] = segments[i-1]->next;
  }

  for (int i = 0; i < 6; i++) {
    EXPECT_EQ(segments[i]->id, i);
  }

  // Check that each segment has the expected size.  Recall that each object will be prefixed by an
  // extra word if its parent is in a different segment.
  EXPECT_EQ(64, segments[0]->len);  // root ref (8), sub-struct (8+tag), 3-element list (16+tag), list substruct 1 (8+tag)
  EXPECT_EQ(72, segments[1]->len);  // root struct (64+tag)
  EXPECT_EQ(80, segments[2]->len);  // struct list (72+tag)
  EXPECT_EQ(64, segments[3]->len);  // list substruct 2,3,4 3*(8+tag), sublist 3 (8+tag)
  EXPECT_EQ(64, segments[4]->len);  // list list (40+tag), sublist 1,2 2*8
  EXPECT_EQ(64, segments[5]->len);  // sublist 4 (8+tag), 5 (16+tag), recurse struct (16+tag)

  checkStruct(&ctx.capn);

  struct capn ctx2;
  memset(&ctx2, 0, sizeof(ctx2));
  for (size_t i = 0; i < sizeof(segments)/sizeof(segments[0]); i++) {
    capn_append_segment(&ctx2, segments[i]);
  }

  checkStruct(&ctx2);
}

TEST(WireFormat, CopyStruct) {
  Session ctx1, ctx2;
  setupStruct(&ctx1.capn);
  checkStruct(&ctx1.capn);

  capn_ptr root = capn_root(&ctx2.capn);
  EXPECT_EQ(0, capn_setp(root, 0, capn_getp(capn_root(&ctx1.capn), 0, 1)));

  checkStruct(&ctx2.capn);
}

TEST(Alignment, SegmentSizeMultipleOf8) {
  /* ARM/Sparc (and 32-bit x86): sizeof(capn_segment) must be a multiple of 8.
   * lib/capn-malloc.c check_segment_alignment is the compile-time twin. */
  EXPECT_EQ(size_t{0}, sizeof(struct capn_segment) % 8);
}

TEST(Alignment, SegmentFieldOffsets) {
  EXPECT_EQ(size_t{0}, offsetof(struct capn_segment, data) % 8);
  EXPECT_EQ(size_t{0}, offsetof(struct capn_segment, len) % 8);
  EXPECT_EQ(size_t{0}, offsetof(struct capn_segment, cap) % 8);
  EXPECT_EQ(size_t{0}, offsetof(struct capn_segment, user) % 8);
}

TEST(Alignment, MallocSegmentData) {
  struct capn c;
  capn_init_malloc(&c);
  capn_ptr root = capn_root(&c);
  ASSERT_TRUE(root.seg != NULL);
  EXPECT_EQ(uintptr_t{0}, reinterpret_cast<uintptr_t>(root.seg) % 8);
  EXPECT_EQ(uintptr_t{0}, reinterpret_cast<uintptr_t>(root.seg->data) % 8);
  capn_free(&c);
}

TEST(Alignment, MallocSegmentDefaultMinSize) {
  /* create() rounds the first small write up to CAPN_CREATE_MIN_SZ (4096). */
  struct capn c;
  capn_init_malloc(&c);
  capn_ptr root = capn_root(&c);
  ASSERT_TRUE(root.seg != NULL);
  size_t alloc = root.seg->cap + sizeof(struct capn_segment);
  EXPECT_EQ(size_t{4096}, alloc);
  EXPECT_EQ(size_t{0}, alloc % 4096);
  capn_free(&c);
}

TEST(Alignment, MallocSegmentRoundsUpTo4096) {
  struct capn c;
  capn_init_malloc(&c);
  capn_ptr root = capn_root(&c);
  ASSERT_TRUE(root.seg != NULL);
  /* 5000 data bytes plus list tag do not fit in the first 4096-byte
   * segment, so create() allocates a second and rounds it up. */
  capn_list8 big = capn_new_list8(root.seg, 5000);
  ASSERT_EQ(CAPN_LIST, big.p.type);
  ASSERT_TRUE(big.p.seg != NULL);
  size_t alloc = big.p.seg->cap + sizeof(struct capn_segment);
  EXPECT_EQ(size_t{0}, alloc % 4096);
  EXPECT_GE(alloc, size_t{4096});
  capn_free(&c);
}

/* 30-bit signed pointer offset is [-2^29, 2^29-1] words. */
static const int64_t kPtrOffMinWords = -(INT64_C(1) << 29);
static const int64_t kPtrOffMaxWords = (INT64_C(1) << 29) - 1;

TEST(WireFormat, WritePtrTagOverflowDoesNotWrite) {
  uint64_t word = UINT64_C(0xdeadbeefcafebabe);
  capn_ptr p;
  memset(&p, 0, sizeof(p));
  p.type = CAPN_STRUCT;
  p.datasz = 8;
  p.ptrs = 0;

  EXPECT_EQ(-1, write_ptr_tag((char *)&word, p, (kPtrOffMinWords - 1) * 8));
  EXPECT_EQ(UINT64_C(0xdeadbeefcafebabe), word);

  EXPECT_EQ(-1, write_ptr_tag((char *)&word, p, (kPtrOffMaxWords + 1) * 8));
  EXPECT_EQ(UINT64_C(0xdeadbeefcafebabe), word);

  EXPECT_EQ(0, write_ptr_tag((char *)&word, p, kPtrOffMinWords * 8));
  EXPECT_NE(UINT64_C(0xdeadbeefcafebabe), word);
  word = UINT64_C(0xdeadbeefcafebabe);
  EXPECT_EQ(0, write_ptr_tag((char *)&word, p, kPtrOffMaxWords * 8));
  EXPECT_NE(UINT64_C(0xdeadbeefcafebabe), word);
}

TEST(WireFormat, NullStructPointerEncodesAsNull) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_ptr outer = capn_new_struct(root.seg, 0, 1);
  ASSERT_EQ(CAPN_STRUCT, outer.type);
  ASSERT_EQ(0, capn_set_root(&ctx.capn, outer));

  capn_ptr nil;
  memset(&nil, 0, sizeof(nil));
  EXPECT_EQ(0, capn_setp(outer, 0, nil));
  EXPECT_EQ(CAPN_NULL, capn_getp(outer, 0, 1).type);

  /* Present-looking struct metadata with no payload must still encode as
   * null, not a far/copy of a NULL data pointer. */
  capn_ptr hollow;
  memset(&hollow, 0, sizeof(hollow));
  hollow.type = CAPN_STRUCT;
  hollow.datasz = 8;
  EXPECT_EQ(0, capn_setp(outer, 0, hollow));
  EXPECT_EQ(CAPN_NULL, capn_getp(outer, 0, 1).type);
}

TEST(WireFormat, WritePtrTagNegativeOffsetRoundTrip) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_ptr behind = capn_new_struct(root.seg, 8, 0);
  ASSERT_EQ(CAPN_STRUCT, behind.type);
  EXPECT_EQ(0, capn_write64(behind, 0, UINT64_C(0x0102030405060708)));

  capn_ptr holder = capn_new_struct(behind.seg, 0, 1);
  ASSERT_EQ(CAPN_STRUCT, holder.type);
  EXPECT_LT(behind.data, holder.data);
  EXPECT_EQ(0, capn_setp(holder, 0, behind));

  capn_ptr got = capn_getp(holder, 0, 1);
  EXPECT_EQ(CAPN_STRUCT, got.type);
  EXPECT_EQ(8, got.datasz);
  EXPECT_EQ(UINT64_C(0x0102030405060708), capn_read64(got, 0));
}

TEST(WireFormat, BoundsOkRange) {
  char pad[18];
  char *buf = pad + 1;
  struct capn_segment s;
  memset(&s, 0, sizeof(s));
  s.data = buf;
  s.len = 16;

  EXPECT_NE(0, bounds_ok(&s, buf, 16));
  EXPECT_NE(0, bounds_ok(&s, buf, 0));
  EXPECT_NE(0, bounds_ok(&s, buf + 16, 0));
  EXPECT_NE(0, bounds_ok(&s, buf + 8, 8));
  EXPECT_EQ(0, bounds_ok(&s, buf + 16, 1));
  EXPECT_EQ(0, bounds_ok(&s, buf, 17));
  EXPECT_EQ(0, bounds_ok(&s, buf + 8, 9));
  EXPECT_EQ(0, bounds_ok(&s, pad, 1));
  EXPECT_EQ(0, bounds_ok(NULL, buf, 1));
  EXPECT_EQ(0, bounds_ok(&s, NULL, 0));
  EXPECT_EQ(0, bounds_ok(&s, buf, (size_t) -1));
}

TEST(WireFormat, ReadPtrRejectsOOBPointerWord) {
  AlignedData<1> data = {{0}};
  struct capn_segment seg;
  struct capn ctx;
  capn_ptr p;
  memset(&seg, 0, sizeof(seg));
  seg.data = (char*) data.bytes;
  seg.len = seg.cap = sizeof(data.bytes);
  memset(&ctx, 0, sizeof(ctx));
  capn_append_segment(&ctx, &seg);

  p = read_ptr(&seg, seg.data + (ptrdiff_t)seg.len, CAPN_NESTING_DEFAULT);
  EXPECT_EQ(CAPN_NULL, p.type);
  p = read_ptr(&seg, seg.data - 8, CAPN_NESTING_DEFAULT);
  EXPECT_EQ(CAPN_NULL, p.type);
  p = read_ptr(NULL, seg.data, CAPN_NESTING_DEFAULT);
  EXPECT_EQ(CAPN_NULL, p.type);
}

TEST(WireFormat, GetpHugeOffsetIsNull) {
  AlignedData<2> data = {{
    /* struct, offset = 0x00FFFFFF words, dataSize = 1 */
    0xfc, 0xff, 0xff, 0x03, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  }};
  struct capn_segment seg;
  struct capn ctx;
  capn_ptr p;
  memset(&seg, 0, sizeof(seg));
  seg.data = (char*) data.bytes;
  seg.len = seg.cap = sizeof(data.bytes);
  memset(&ctx, 0, sizeof(ctx));
  capn_append_segment(&ctx, &seg);

  p = capn_getp(capn_root(&ctx), 0, 1);
  EXPECT_EQ(CAPN_NULL, p.type);
}

/* encoding.html List(Bool): C=1, bits packed little-endian (bit 0 = LSB).
 * A list pointer with offset 0, C=1, D=10 is 0x0000005100000001
 * (LE bytes 01 00 00 00 51 00 00 00). Bits 0, 3, 9 set produce 0x09, 0x02. */
TEST(WireFormat, BitListRawDecode) {
  AlignedData<2> data = {{
    0x01, 0x00, 0x00, 0x00, 0x51, 0x00, 0x00, 0x00,
    0x09, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  }};
  struct capn_segment seg;
  struct capn ctx;
  memset(&seg, 0, sizeof(seg));
  seg.data = (char*) data.bytes;
  seg.len = seg.cap = sizeof(data.bytes);
  memset(&ctx, 0, sizeof(ctx));
  capn_append_segment(&ctx, &seg);

  capn_list1 bits = {capn_getp(capn_root(&ctx), 0, 1)};
  EXPECT_EQ(CAPN_BIT_LIST, bits.p.type);
  EXPECT_EQ(10, bits.p.len);
  EXPECT_EQ(1, capn_get1(bits, 0));
  EXPECT_EQ(0, capn_get1(bits, 1));
  EXPECT_EQ(0, capn_get1(bits, 2));
  EXPECT_EQ(1, capn_get1(bits, 3));
  EXPECT_EQ(0, capn_get1(bits, 8));
  EXPECT_EQ(1, capn_get1(bits, 9));
  EXPECT_EQ(0, capn_get1(bits, 10));
}

TEST(WireFormat, BitListGetpInnerMember) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_list1 bits = capn_new_list1(root.seg, 10);
  ASSERT_EQ(CAPN_BIT_LIST, bits.p.type);
  EXPECT_EQ(10, bits.p.len);
  EXPECT_EQ(0, capn_set1(bits, 0, 1));
  EXPECT_EQ(0, capn_set1(bits, 3, 1));
  EXPECT_EQ(0, capn_set1(bits, 9, 1));

  capn_ptr inner = capn_getp(bits.p, 3, 0);
  EXPECT_EQ(CAPN_BIT_LIST, inner.type);
  EXPECT_EQ(1, inner.is_list_member);
  EXPECT_EQ(1, inner.len);
  capn_list1 one = {inner};
  EXPECT_EQ(1, capn_get1(one, 0));
  EXPECT_EQ(0, capn_get1(one, 1));

  capn_ptr zero = capn_getp(bits.p, 1, 1);
  EXPECT_EQ(CAPN_BIT_LIST, zero.type);
  EXPECT_EQ(1, zero.is_list_member);
  capn_list1 z = {zero};
  EXPECT_EQ(0, capn_get1(z, 0));

  EXPECT_EQ(CAPN_NULL, capn_getp(bits.p, 10, 0).type);
  EXPECT_EQ(CAPN_NULL, capn_getp(bits.p, -1, 0).type);
  EXPECT_EQ(1, capn_get1(bits, 3));
}

TEST(WireFormat, BitListSetpCopiesBit) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_list1 src = capn_new_list1(root.seg, 8);
  capn_list1 dst = capn_new_list1(root.seg, 8);
  ASSERT_EQ(CAPN_BIT_LIST, src.p.type);
  ASSERT_EQ(CAPN_BIT_LIST, dst.p.type);
  EXPECT_EQ(0, capn_set1(src, 2, 1));
  EXPECT_EQ(0, capn_set1(src, 5, 1));
  EXPECT_EQ(0, capn_set1(dst, 0, 1));
  EXPECT_EQ(0, capn_set1(dst, 7, 1));

  EXPECT_EQ(0, capn_setp(dst.p, 1, capn_getp(src.p, 2, 0)));
  EXPECT_EQ(0, capn_setp(dst.p, 4, capn_getp(src.p, 5, 0)));
  EXPECT_EQ(0, capn_setp(dst.p, 7, capn_getp(src.p, 0, 0)));

  EXPECT_EQ(1, capn_get1(dst, 0));
  EXPECT_EQ(1, capn_get1(dst, 1));
  EXPECT_EQ(0, capn_get1(dst, 2));
  EXPECT_EQ(1, capn_get1(dst, 4));
  EXPECT_EQ(0, capn_get1(dst, 7));

  capn_ptr nil;
  memset(&nil, 0, sizeof(nil));
  EXPECT_EQ(0, capn_setp(dst.p, 0, nil));
  EXPECT_EQ(0, capn_get1(dst, 0));

  EXPECT_EQ(-1, capn_setp(dst.p, 8, capn_getp(src.p, 2, 0)));
  EXPECT_EQ(-1, capn_setp(dst.p, -1, capn_getp(src.p, 2, 0)));
  EXPECT_EQ(1, capn_get1(dst, 1));
}

TEST(WireFormat, BitListSetpRejectsBadTgtLeavesBits) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_list1 bits = capn_new_list1(root.seg, 4);
  capn_ptr junk = capn_new_ptr_list(root.seg, 1);
  EXPECT_EQ(0, capn_set1(bits, 0, 1));
  EXPECT_EQ(0, capn_set1(bits, 2, 1));

  EXPECT_EQ(-1, capn_setp(bits.p, 1, junk));
  EXPECT_EQ(1, capn_get1(bits, 0));
  EXPECT_EQ(0, capn_get1(bits, 1));
  EXPECT_EQ(1, capn_get1(bits, 2));
  EXPECT_EQ(0, capn_get1(bits, 3));
}

TEST(WireFormat, BitListSetpIntoStructPointer) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_ptr holder = capn_new_struct(root.seg, 0, 1);
  capn_list1 bits = capn_new_list1(root.seg, 11);
  ASSERT_EQ(CAPN_STRUCT, holder.type);
  EXPECT_EQ(0, capn_set1(bits, 1, 1));
  EXPECT_EQ(0, capn_set1(bits, 8, 1));
  EXPECT_EQ(0, capn_set1(bits, 10, 1));
  EXPECT_EQ(0, capn_setp(holder, 0, bits.p));
  EXPECT_EQ(0, capn_set_root(&ctx.capn, holder));

  capn_list1 got = {capn_getp(holder, 0, 1)};
  EXPECT_EQ(CAPN_BIT_LIST, got.p.type);
  EXPECT_EQ(11, got.p.len);
  EXPECT_EQ(0, capn_get1(got, 0));
  EXPECT_EQ(1, capn_get1(got, 1));
  EXPECT_EQ(1, capn_get1(got, 8));
  EXPECT_EQ(1, capn_get1(got, 10));
}

TEST(WireFormat, BitListSetpInnerIntoPointerField) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_ptr holder = capn_new_struct(root.seg, 0, 1);
  capn_list1 bits = capn_new_list1(root.seg, 8);
  EXPECT_EQ(0, capn_set1(bits, 3, 1));
  EXPECT_EQ(0, capn_setp(holder, 0, capn_getp(bits.p, 3, 0)));

  capn_list1 got = {capn_getp(holder, 0, 1)};
  EXPECT_EQ(CAPN_BIT_LIST, got.p.type);
  EXPECT_EQ(1, got.p.len);
  EXPECT_EQ(0, got.p.is_list_member);
  EXPECT_EQ(1, capn_get1(got, 0));

  EXPECT_EQ(0, capn_setp(holder, 0, capn_getp(bits.p, 1, 0)));
  got.p = capn_getp(holder, 0, 1);
  EXPECT_EQ(CAPN_BIT_LIST, got.p.type);
  EXPECT_EQ(1, got.p.len);
  EXPECT_EQ(0, capn_get1(got, 0));
}

TEST(WireFormat, BitListCopyAcrossSession) {
  Session a, b;
  capn_ptr src_root = capn_root(&a.capn);
  capn_ptr src = capn_new_struct(src_root.seg, 0, 1);
  capn_list1 bits = capn_new_list1(src.seg, 5);
  EXPECT_EQ(0, capn_set1(bits, 0, 1));
  EXPECT_EQ(0, capn_set1(bits, 4, 1));
  EXPECT_EQ(0, capn_setp(src, 0, bits.p));
  EXPECT_EQ(0, capn_set_root(&a.capn, src));

  capn_ptr dst_root = capn_root(&b.capn);
  EXPECT_EQ(0, capn_setp(dst_root, 0, capn_getp(src_root, 0, 1)));
  capn_list1 got = {capn_getp(capn_getp(dst_root, 0, 1), 0, 1)};
  EXPECT_EQ(CAPN_BIT_LIST, got.p.type);
  EXPECT_EQ(5, got.p.len);
  EXPECT_EQ(1, capn_get1(got, 0));
  EXPECT_EQ(0, capn_get1(got, 1));
  EXPECT_EQ(1, capn_get1(got, 4));
}

TEST(WireFormat, BitListWritePtrTag) {
  uint64_t word = 0;
  capn_ptr p;
  memset(&p, 0, sizeof(p));
  p.type = CAPN_BIT_LIST;
  p.len = 10;
  p.datasz = 2;
  EXPECT_EQ(0, write_ptr_tag((char *)&word, p, 0));
  EXPECT_EQ(UINT64_C(0x5100000001), capn_flip64(word));
}

TEST(WireFormat, Get1Set1OnCompositeList) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_ptr list = capn_new_list(root.seg, 3, 8, 0);
  ASSERT_EQ(CAPN_LIST, list.type);
  for (int i = 0; i < 3; i++) {
    capn_ptr el = capn_getp(list, i, 0);
    ASSERT_EQ(CAPN_STRUCT, el.type);
    EXPECT_EQ(0, capn_write1(el, 0, i != 1));
  }
  capn_list1 as_bits = {list};
  EXPECT_EQ(1, capn_get1(as_bits, 0));
  EXPECT_EQ(0, capn_get1(as_bits, 1));
  EXPECT_EQ(1, capn_get1(as_bits, 2));
  EXPECT_EQ(0, capn_set1(as_bits, 1, 1));
  EXPECT_EQ(1, capn_get1(as_bits, 1));
  EXPECT_EQ(1, capn_read8(capn_getp(list, 1, 0), 0) & 1);
}

TEST(WireFormat, Get1Set1OnPtrListOfStructs) {
  Session ctx;
  capn_ptr root = capn_root(&ctx.capn);
  capn_ptr list = capn_new_ptr_list(root.seg, 2);
  capn_ptr a = capn_new_struct(root.seg, 8, 0);
  capn_ptr b = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_write1(a, 0, 1));
  EXPECT_EQ(0, capn_write1(b, 0, 0));
  EXPECT_EQ(0, capn_setp(list, 0, a));
  EXPECT_EQ(0, capn_setp(list, 1, b));

  capn_list1 as_bits = {list};
  EXPECT_EQ(1, capn_get1(as_bits, 0));
  EXPECT_EQ(0, capn_get1(as_bits, 1));
  EXPECT_EQ(0, capn_set1(as_bits, 1, 1));
  EXPECT_EQ(1, capn_get1(as_bits, 1));
  EXPECT_EQ(1, capn_read8(capn_getp(list, 1, 1), 0) & 1);
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

