/* capn-stream-test.cpp
 *
 * Copyright (C) 2013 James McKaskill
 * Copyright (C) 2014 Steve Dee
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include "capn-stream.c"
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

template <int wordCount>
union AlignedData {
  uint8_t bytes[wordCount * 8];
  uint64_t words[wordCount];
};

TEST(Stream, ReadRejectsTooManySegments) {
  /* Wire word 0 is (segnum - 1). 1024 => 1025 segments, above CAPN_MAX_SEGS. */
  AlignedData<1> data = {{
    0x00, 0x04, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
  }};
  struct capn ctx;
  EXPECT_NE(0, capn_init_mem(&ctx, data.bytes, sizeof(data.bytes), 0));
}

TEST(Stream, ReadEmptyStream_Even) {
  AlignedData<2> data = {{
    1, 0, 0, 0, // num of segs - 1
    0, 0, 0, 0,
    0, 0, 0, 0,
    2, 3, 4, 0, // garbage that should be ignored
  }};

  struct capn ctx;
  ASSERT_NE(0, capn_init_mem(&ctx, data.bytes, 12, 0));
  ASSERT_EQ(0, capn_init_mem(&ctx, data.bytes, 16, 0));
  EXPECT_EQ(2, ctx.segnum);
  EXPECT_EQ(0, ctx.seglist->len);
  EXPECT_EQ(0, ctx.seglist->next->len);
  capn_free(&ctx);
}

TEST(Stream, ReadEmptyStream_Odd) {
  AlignedData<3> data = {{
    2, 0, 0, 0, // num of segs - 1
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    2, 3, 4, 0, // garbage that should be ignored
  }};

  struct capn ctx;
  ASSERT_NE(0, capn_init_mem(&ctx, data.bytes, 12, 0));

  ASSERT_EQ(0, capn_init_mem(&ctx, data.bytes, 16, 0));
  EXPECT_EQ(3, ctx.segnum);
  EXPECT_EQ(0, ctx.seglist->len);
  EXPECT_EQ(0, ctx.seglist->next->len);
  capn_free(&ctx);

  ASSERT_EQ(0, capn_init_mem(&ctx, data.bytes, 20, 0));
  EXPECT_EQ(3, ctx.segnum);
  EXPECT_EQ(0, ctx.seglist->len);
  EXPECT_EQ(0, ctx.seglist->next->len);
  capn_free(&ctx);
}

TEST(Stream, ReadStream_Even) {
  AlignedData<5> data = {{
     1, 0, 0, 0, // num of segs - 1
     1, 0, 0, 0,
     2, 0, 0, 0,
     2, 3, 4, 0, // garbage that should be ignored
     1, 2, 3, 4, 5, 6, 7, 8,
     9,10,11,12,13,14,15,16,
    17,18,19,20,21,22,23,24,
  }};

  struct capn ctx;
  ASSERT_NE(0, capn_init_mem(&ctx, data.bytes, 36, 0));
  ASSERT_EQ(0, capn_init_mem(&ctx, data.bytes, 40, 0));
  EXPECT_EQ(2, ctx.segnum);
  EXPECT_EQ(8, ctx.seglist->len);
  EXPECT_EQ(1, ctx.seglist->data[0]);
  EXPECT_EQ(16, ctx.seglist->next->len);
  EXPECT_EQ(9, ctx.seglist->next->data[0]);
  capn_free(&ctx);
}

static struct capn_segment *CreateSmallSegment(void *u, uint32_t id, int sz) {
  sz += sizeof(struct capn_segment);
  struct capn_segment *s = (struct capn_segment*) calloc(1, sz);
  s->data = (char*) (s+1);
  s->cap = sz - sizeof(*s);
  s->user = s;
  return s;
}

TEST(Stream, SizeEmptyStream) {
  struct capn ctx;
  capn_init_malloc(&ctx);
  struct capn_ptr root = capn_root(&ctx);
  ASSERT_EQ(CAPN_PTR_LIST, root.type);
  EXPECT_EQ(2*8, capn_size(&ctx));

  capn_free(&ctx);
}

TEST(Stream, SizeOneSegment) {
  struct capn ctx;
  capn_init_malloc(&ctx);
  struct capn_ptr root = capn_root(&ctx);
  struct capn_ptr ptr = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr));
  EXPECT_EQ(0, capn_write64(ptr, 0, UINT64_C(0x1011121314151617)));
  EXPECT_EQ(3*8, capn_size(&ctx));

  capn_free(&ctx);
}

TEST(Stream, SizeTwoSegments) {
  struct capn ctx;
  capn_init_malloc(&ctx);
  ctx.create = &CreateSmallSegment;
  struct capn_ptr root = capn_root(&ctx);
  struct capn_ptr ptr1 = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr1));
  EXPECT_EQ(0, capn_write64(ptr1, 0, UINT64_C(0xfffefdfcfbfaf9f8)));
  EXPECT_EQ(2, ctx.segnum);

  /* 2 words: header
   * 1 word: segment 1
   * 2 words: segment 2
   */
  EXPECT_EQ(5*8, capn_size(&ctx));

  capn_free(&ctx);
}

TEST(Stream, SizeThreeSegments) {
  struct capn ctx;
  capn_init_malloc(&ctx);
  ctx.create = &CreateSmallSegment;
  struct capn_ptr root = capn_root(&ctx);
  struct capn_ptr ptr1 = capn_new_struct(root.seg, 0, 1);
  EXPECT_EQ(0, capn_setp(root, 0, ptr1));
  struct capn_ptr ptr2 = capn_new_struct(ptr1.seg, 4, 0);
  EXPECT_EQ(0, capn_setp(ptr1, 0, ptr2));
  EXPECT_EQ(0, capn_write32(ptr2, 0, 0x12345678));
  EXPECT_EQ(3, ctx.segnum);

  EXPECT_EQ(7*8, capn_size(&ctx));

  capn_free(&ctx);
}

TEST(Stream, WriteEmptyStream) {
  uint8_t buf[2048];

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  struct capn_ptr root = capn_root(&ctx1);
  ASSERT_EQ(CAPN_PTR_LIST, root.type);
  EXPECT_EQ(-1, capn_write_mem(&ctx1, buf, 2*8-1, 0));
  EXPECT_EQ(2*8, capn_write_mem(&ctx1, buf, 2048, 0));
  ASSERT_EQ(0, capn_init_mem(&ctx2, buf, 2048, 0));
  EXPECT_EQ(1, ctx2.segnum);
  EXPECT_EQ(8, ctx2.seglist->len);
  EXPECT_EQ(0, ctx2.seglist->next);

  capn_free(&ctx1);
  capn_free(&ctx2);
}

TEST(Stream, WriteEmptyStreamPacked) {
  uint8_t buf[2048];

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  struct capn_ptr root = capn_root(&ctx1);
  ASSERT_EQ(CAPN_PTR_LIST, root.type);
  EXPECT_EQ(-1, capn_write_mem(&ctx1, buf, 3, 1));
  EXPECT_EQ(4, capn_write_mem(&ctx1, buf, 2048, 1));
  ASSERT_EQ(0, capn_init_mem(&ctx2, buf, 2048, 1));
  EXPECT_EQ(1, ctx2.segnum);
  EXPECT_EQ(8, ctx2.seglist->len);
  EXPECT_EQ(0, ctx2.seglist->next);

  capn_free(&ctx1);
  capn_free(&ctx2);
}

TEST(Stream, WritePackedHeaderOverflowTinyBuffer) {
  /* capn_write_mem_packed writes the uncompressed header at
   * p + headersz + 2 (headerlen uint32s). A caller-claimed size that
   * cannot hold that scratch must return -1 and must not store past sz.
   */
  uint8_t buf[64];
  memset(buf, 0xAB, sizeof(buf));

  struct capn ctx;
  capn_init_malloc(&ctx);
  struct capn_ptr root = capn_root(&ctx);
  ASSERT_EQ(CAPN_PTR_LIST, root.type);

  /* One segment: headerlen = 2, headersz = 8. Scratch ends at 8+2+8 = 18. */
  EXPECT_EQ(-1, capn_write_mem(&ctx, buf, 8, 1));
  EXPECT_EQ(-1, capn_write_mem(&ctx, buf, 17, 1));
  for (size_t i = 17; i < sizeof(buf); i++) {
    EXPECT_EQ(0xAB, buf[i]) << i;
  }

  capn_free(&ctx);
}

TEST(Stream, WriteOneSegment) {
  uint8_t buf[2048];

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);

  struct capn_ptr root = capn_root(&ctx1);
  struct capn_ptr ptr = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr));
  EXPECT_EQ(0, capn_write64(ptr, 0, UINT64_C(0x1011121314151617)));

  EXPECT_EQ(-1, capn_write_mem(&ctx1, buf, 3*8-1, 0));
  EXPECT_EQ(3*8, capn_write_mem(&ctx1, buf, 2048, 0));
  ASSERT_EQ(0, capn_init_mem(&ctx2, buf, 2048, 0));
  EXPECT_EQ(1, ctx2.segnum);

  root = capn_root(&ctx2);
  ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(UINT64_C(0x1011121314151617), capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
}

TEST(Stream, WriteOneSegmentPacked) {
  uint8_t buf[2048];

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);

  struct capn_ptr root = capn_root(&ctx1);
  struct capn_ptr ptr = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr));
  EXPECT_EQ(0, capn_write64(ptr, 0, UINT64_C(0x1011121314151617)));

  EXPECT_EQ(-1, capn_write_mem(&ctx1, buf, 13, 1));
  EXPECT_EQ(14, capn_write_mem(&ctx1, buf, 2048, 1));
  ASSERT_EQ(0, capn_init_mem(&ctx2, buf, 2048, 1));
  EXPECT_EQ(1, ctx2.segnum);

  root = capn_root(&ctx2);
  ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(UINT64_C(0x1011121314151617), capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
}

TEST(Stream, WriteTwoSegments) {
  struct capn ctx1, ctx2;
  uint8_t buf[5*8];

  capn_init_malloc(&ctx1);
  ctx1.create = &CreateSmallSegment;
  struct capn_ptr root = capn_root(&ctx1);
  struct capn_ptr ptr1 = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr1));
  EXPECT_EQ(0, capn_write64(ptr1, 0, UINT64_C(0xfffefdfcfbfaf9f8)));
  EXPECT_EQ(2, ctx1.segnum);

  /* 2 words: header
   * 1 word: segment 1
   * 2 words: segment 2
   */
  EXPECT_EQ(5*8, capn_write_mem(&ctx1, buf, 5*8, 0));

  ASSERT_EQ(0, capn_init_mem(&ctx2, buf, 2048, 0));
  root = capn_root(&ctx2);
  ptr1 = capn_getp(root, 0, 1);
  EXPECT_EQ(UINT64_C(0xfffefdfcfbfaf9f8), capn_read64(ptr1, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
}

TEST(Stream, WriteTwoSegmentsPacked) {
  struct capn ctx1, ctx2;
  uint8_t buf[5*8];

  capn_init_malloc(&ctx1);
  ctx1.create = &CreateSmallSegment;
  struct capn_ptr root = capn_root(&ctx1);
  struct capn_ptr ptr1 = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr1));
  EXPECT_EQ(0, capn_write64(ptr1, 0, UINT64_C(0xfffefdfcfbfaf9f8)));
  EXPECT_EQ(2, ctx1.segnum);

  /* 2 words: header
   * 1 word: segment 1
   * 2 words: segment 2
   */
  EXPECT_EQ(20, capn_write_mem(&ctx1, buf, 5*8, 1));

  ASSERT_EQ(0, capn_init_mem(&ctx2, buf, 2048, 1));
  root = capn_root(&ctx2);
  ptr1 = capn_getp(root, 0, 1);
  EXPECT_EQ(UINT64_C(0xfffefdfcfbfaf9f8), capn_read64(ptr1, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
}

TEST(Stream, WriteThreeSegments) {
  struct capn ctx1, ctx2;
  uint8_t buf[2048];

  capn_init_malloc(&ctx1);
  ctx1.create = &CreateSmallSegment;
  struct capn_ptr root = capn_root(&ctx1);
  struct capn_ptr ptr1 = capn_new_struct(root.seg, 0, 1);
  EXPECT_EQ(0, capn_setp(root, 0, ptr1));
  struct capn_ptr ptr2 = capn_new_struct(ptr1.seg, 4, 0);
  EXPECT_EQ(0, capn_setp(ptr1, 0, ptr2));
  EXPECT_EQ(0, capn_write32(ptr2, 0, 0x12345678));
  EXPECT_EQ(3, ctx1.segnum);

  EXPECT_EQ(-1, capn_write_mem(&ctx1, buf, 7*8-1, 0));
  EXPECT_EQ(7*8, capn_write_mem(&ctx1, buf, 2048, 0));

  EXPECT_EQ(0, capn_init_mem(&ctx2, buf, 2048, 0));
  root = capn_root(&ctx2);
  ptr1 = capn_getp(root, 0, 1);
  ptr2 = capn_getp(ptr1, 0, 1);
  EXPECT_EQ(0x12345678, capn_read32(ptr2, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
}

TEST(Stream, WriteThreeSegmentsPacked) {
  struct capn ctx1, ctx2;
  uint8_t buf[2048];

  capn_init_malloc(&ctx1);
  ctx1.create = &CreateSmallSegment;
  struct capn_ptr root = capn_root(&ctx1);
  struct capn_ptr ptr1 = capn_new_struct(root.seg, 0, 1);
  EXPECT_EQ(0, capn_setp(root, 0, ptr1));
  struct capn_ptr ptr2 = capn_new_struct(ptr1.seg, 4, 0);
  EXPECT_EQ(0, capn_setp(ptr1, 0, ptr2));
  EXPECT_EQ(0, capn_write32(ptr2, 0, 0x12345678));
  EXPECT_EQ(3, ctx1.segnum);

  EXPECT_EQ(-1, capn_write_mem(&ctx1, buf, 20, 1));
  EXPECT_EQ(21, capn_write_mem(&ctx1, buf, 2048, 1));

  EXPECT_EQ(0, capn_init_mem(&ctx2, buf, 2048, 1));
  root = capn_root(&ctx2);
  ptr1 = capn_getp(root, 0, 1);
  ptr2 = capn_getp(ptr1, 0, 1);
  EXPECT_EQ(0x12345678, capn_read32(ptr2, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
}

static void fill_one_segment(struct capn *c, uint64_t val) {
  struct capn_ptr root = capn_root(c);
  struct capn_ptr ptr = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr));
  EXPECT_EQ(0, capn_write64(ptr, 0, val));
}

TEST(Stream, WriteFpRejectsEmptyAndNull) {
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);

  struct capn ctx;
  capn_init_malloc(&ctx);
  EXPECT_EQ(-1, capn_write_fp(&ctx, f, 0));
  EXPECT_EQ(-1, capn_write_fp(&ctx, f, 1));
  capn_root(&ctx);
  EXPECT_EQ(-1, capn_write_fp(&ctx, NULL, 0));
  EXPECT_EQ(-1, capn_write_fp(&ctx, NULL, 1));

  capn_free(&ctx);
  fclose(f);
}

TEST(Stream, WriteFpUnpackedRoundTrip) {
  const uint64_t val = UINT64_C(0x1011121314151617);
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_one_segment(&ctx1, val);

  int n = capn_write_fp(&ctx1, f, 0);
  EXPECT_EQ(3 * 8, n);

  rewind(f);
  ASSERT_EQ(0, capn_init_fp(&ctx2, f, 0));
  EXPECT_EQ(1, ctx2.segnum);
  struct capn_ptr root = capn_root(&ctx2);
  struct capn_ptr ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(val, capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  fclose(f);
}

TEST(Stream, InitFpPackedFromWriteMem) {
  const uint64_t val = UINT64_C(0x1011121314151617);
  uint8_t buf[2048];

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_one_segment(&ctx1, val);
  int64_t n = capn_write_mem(&ctx1, buf, sizeof(buf), 1);
  ASSERT_EQ(14, n);

  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);
  ASSERT_EQ((size_t)n, fwrite(buf, 1, (size_t)n, f));
  rewind(f);

  ASSERT_EQ(0, capn_init_fp(&ctx2, f, 1));
  EXPECT_EQ(1, ctx2.segnum);
  struct capn_ptr root = capn_root(&ctx2);
  struct capn_ptr ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(val, capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  fclose(f);
}

TEST(Stream, WriteFpPackedRoundTrip) {
  const uint64_t val = UINT64_C(0x1011121314151617);
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_one_segment(&ctx1, val);

  int n = capn_write_fp(&ctx1, f, 1);
  EXPECT_EQ(14, n);

  rewind(f);
  ASSERT_EQ(0, capn_init_fp(&ctx2, f, 1));
  EXPECT_EQ(1, ctx2.segnum);
  struct capn_ptr root = capn_root(&ctx2);
  struct capn_ptr ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(val, capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  fclose(f);
}

TEST(Stream, WriteFpMatchesWriteMem) {
  const uint64_t val = UINT64_C(0xfffefdfcfbfaf9f8);
  uint8_t mem[2048];
  uint8_t from_fp[2048];

  struct capn ctx;
  capn_init_malloc(&ctx);
  fill_one_segment(&ctx, val);

  for (int packed = 0; packed <= 1; packed++) {
    int64_t nmem = capn_write_mem(&ctx, mem, sizeof(mem), packed);
    ASSERT_GT(nmem, 0);

    FILE *f = tmpfile();
    ASSERT_TRUE(f != NULL);
    int nfp = capn_write_fp(&ctx, f, packed);
    EXPECT_EQ(nmem, nfp) << "packed=" << packed;

    rewind(f);
    size_t nr = fread(from_fp, 1, sizeof(from_fp), f);
    EXPECT_EQ((size_t)nmem, nr);
    EXPECT_EQ(0, memcmp(mem, from_fp, (size_t)nmem)) << "packed=" << packed;
    fclose(f);
  }

  capn_free(&ctx);
}

TEST(Stream, WriteFpTwoSegmentsPackedRoundTrip) {
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  ctx1.create = &CreateSmallSegment;
  struct capn_ptr root = capn_root(&ctx1);
  struct capn_ptr ptr1 = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr1));
  EXPECT_EQ(0, capn_write64(ptr1, 0, UINT64_C(0xfffefdfcfbfaf9f8)));
  EXPECT_EQ(2, ctx1.segnum);

  int n = capn_write_fp(&ctx1, f, 1);
  EXPECT_EQ(20, n);

  rewind(f);
  ASSERT_EQ(0, capn_init_fp(&ctx2, f, 1));
  root = capn_root(&ctx2);
  ptr1 = capn_getp(root, 0, 1);
  EXPECT_EQ(UINT64_C(0xfffefdfcfbfaf9f8), capn_read64(ptr1, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  fclose(f);
}

#ifndef _MSC_VER
static ssize_t test_write_fd(int fd, const void *p, size_t count) {
  return write(fd, p, count);
}

static ssize_t test_read_fd(int fd, void *p, size_t count) {
  return read(fd, p, count);
}

TEST(Stream, InitFdRejectsNullRead) {
  struct capn ctx;
  EXPECT_EQ(-1, capn_init_fd(&ctx, NULL, 0, 0));
  EXPECT_EQ(-1, capn_init_fd(&ctx, NULL, 0, 1));
}

TEST(Stream, WriteFdInitFdUnpackedRoundTrip) {
  const uint64_t val = UINT64_C(0x1011121314151617);
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_one_segment(&ctx1, val);

  int fd = fileno(f);
  int n = capn_write_fd(&ctx1, test_write_fd, fd, 0);
  EXPECT_EQ(3 * 8, n);

  ASSERT_EQ((off_t)0, lseek(fd, 0, SEEK_SET));
  ASSERT_EQ(0, capn_init_fd(&ctx2, test_read_fd, fd, 0));
  EXPECT_EQ(1, ctx2.segnum);
  struct capn_ptr root = capn_root(&ctx2);
  struct capn_ptr ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(val, capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  fclose(f);
}

TEST(Stream, WriteFdInitFdPackedRoundTrip) {
  const uint64_t val = UINT64_C(0x1011121314151617);
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_one_segment(&ctx1, val);

  int fd = fileno(f);
  int n = capn_write_fd(&ctx1, test_write_fd, fd, 1);
  EXPECT_EQ(14, n);

  ASSERT_EQ((off_t)0, lseek(fd, 0, SEEK_SET));
  ASSERT_EQ(0, capn_init_fd(&ctx2, test_read_fd, fd, 1));
  EXPECT_EQ(1, ctx2.segnum);
  struct capn_ptr root = capn_root(&ctx2);
  struct capn_ptr ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(val, capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  fclose(f);
}

TEST(Stream, WriteFdInitFdPipeUnpackedRoundTrip) {
  const uint64_t val = UINT64_C(0xfffefdfcfbfaf9f8);
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_one_segment(&ctx1, val);

  int n = capn_write_fd(&ctx1, test_write_fd, fds[1], 0);
  EXPECT_EQ(3 * 8, n);
  ASSERT_EQ(0, close(fds[1]));

  ASSERT_EQ(0, capn_init_fd(&ctx2, test_read_fd, fds[0], 0));
  EXPECT_EQ(1, ctx2.segnum);
  struct capn_ptr root = capn_root(&ctx2);
  struct capn_ptr ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(val, capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  ASSERT_EQ(0, close(fds[0]));
}

TEST(Stream, WriteFdInitFdPipePackedRoundTrip) {
  const uint64_t val = UINT64_C(0x1011121314151617);
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_one_segment(&ctx1, val);

  int n = capn_write_fd(&ctx1, test_write_fd, fds[1], 1);
  EXPECT_EQ(14, n);
  ASSERT_EQ(0, close(fds[1]));

  ASSERT_EQ(0, capn_init_fd(&ctx2, test_read_fd, fds[0], 1));
  EXPECT_EQ(1, ctx2.segnum);
  struct capn_ptr root = capn_root(&ctx2);
  struct capn_ptr ptr = capn_getp(root, 0, 1);
  EXPECT_EQ(val, capn_read64(ptr, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  ASSERT_EQ(0, close(fds[0]));
}

TEST(Stream, WriteFdInitFdTwoSegmentsPackedRoundTrip) {
  int fds[2];
  ASSERT_EQ(0, pipe(fds));

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  ctx1.create = &CreateSmallSegment;
  struct capn_ptr root = capn_root(&ctx1);
  struct capn_ptr ptr1 = capn_new_struct(root.seg, 8, 0);
  EXPECT_EQ(0, capn_setp(root, 0, ptr1));
  EXPECT_EQ(0, capn_write64(ptr1, 0, UINT64_C(0xfffefdfcfbfaf9f8)));
  EXPECT_EQ(2, ctx1.segnum);

  int n = capn_write_fd(&ctx1, test_write_fd, fds[1], 1);
  EXPECT_EQ(20, n);
  ASSERT_EQ(0, close(fds[1]));

  ASSERT_EQ(0, capn_init_fd(&ctx2, test_read_fd, fds[0], 1));
  root = capn_root(&ctx2);
  ptr1 = capn_getp(root, 0, 1);
  EXPECT_EQ(UINT64_C(0xfffefdfcfbfaf9f8), capn_read64(ptr1, 0));

  capn_free(&ctx1);
  capn_free(&ctx2);
  ASSERT_EQ(0, close(fds[0]));
}
#endif /* !_MSC_VER */
