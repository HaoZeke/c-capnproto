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

/* Dense List(UInt8): every byte nonzero so packing expands. A 4096-byte
 * segment becomes 4098 packed and does not fit a one-shot 4 KiB buffer. */
static const int kDenseBytes = 4096;

static void fill_dense_list8(struct capn *c, uint8_t *bytes, int nbyte) {
  struct capn_ptr root = capn_root(c);
  capn_list8 data = capn_new_list8(root.seg, nbyte);
  EXPECT_EQ(CAPN_LIST, data.p.type);
  memset(bytes, 0xFF, (size_t)nbyte);
  EXPECT_EQ(nbyte, capn_setv8(data, 0, bytes, nbyte));
  EXPECT_EQ(0, capn_setp(root, 0, data.p));
}

static size_t max_segment_len(struct capn *c) {
  size_t max_len = 0;
  struct capn_segment *s;
  for (s = c->seglist; s; s = s->next) {
    if (s->len > max_len)
      max_len = s->len;
  }
  return max_len;
}

static void expect_dense_list8(struct capn *c, const uint8_t *bytes, int nbyte) {
  uint8_t got[4096];
  capn_list8 out;
  EXPECT_LE(nbyte, (int)sizeof(got));
  out.p = capn_getp(capn_root(c), 0, 1);
  EXPECT_EQ(CAPN_LIST, out.p.type);
  EXPECT_EQ(nbyte, out.p.len);
  EXPECT_EQ(nbyte, capn_getv8(out, 0, got, nbyte));
  EXPECT_EQ(0, memcmp(got, bytes, (size_t)nbyte));
}

static uint8_t g_fd_out[16384];
static size_t g_fd_out_len;

static ssize_t collect_fd_write(int fd, const void *p, size_t count) {
  (void)fd;
  if (g_fd_out_len + count > sizeof(g_fd_out))
    return -1;
  memcpy(g_fd_out + g_fd_out_len, p, count);
  g_fd_out_len += count;
  return (ssize_t)count;
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

/* encoding.html packing: tag 0xFF is followed by 8 bytes then a count N
 * of extra uncompressed words. N is one byte, so 0..255 extra words
 * (256 words = 2 KiB per span). Walk a packed buffer by those rules. */
static int packed_popcount(uint8_t tag) {
  int n = 0;
  for (int i = 0; i < 8; i++) {
    if (tag & (1u << i))
      n++;
  }
  return n;
}

static bool walk_packed(const uint8_t *p, size_t n, size_t *words, int *ff_spans) {
  size_t i = 0;
  *words = 0;
  *ff_spans = 0;
  while (i < n) {
    uint8_t tag = p[i++];
    if (tag == 0x00) {
      if (i >= n)
        return false;
      *words += 1u + p[i++];
    } else if (tag == 0xFF) {
      if (i + 9 > n)
        return false;
      i += 8;
      uint8_t extra = p[i++];
      if (i + (size_t)extra * 8 > n)
        return false;
      i += (size_t)extra * 8;
      *words += 1u + extra;
      (*ff_spans)++;
    } else {
      int cnt = packed_popcount(tag);
      if (i + (size_t)cnt > n)
        return false;
      i += (size_t)cnt;
      (*words)++;
    }
  }
  return true;
}

static int deflate_all(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap,
                       size_t *outlen) {
  struct capn_stream s;
  memset(&s, 0, sizeof(s));
  s.next_in = in;
  s.avail_in = inlen;
  s.next_out = out;
  s.avail_out = outcap;
  int r = capn_deflate(&s);
  if (r != 0)
    return r;
  if (s.avail_in != 0)
    return -1;
  *outlen = outcap - s.avail_out;
  return 0;
}

static int inflate_all(const uint8_t *in, size_t inlen, uint8_t *out, size_t outlen) {
  struct capn_stream s;
  memset(&s, 0, sizeof(s));
  s.next_in = in;
  s.avail_in = inlen;
  s.next_out = out;
  s.avail_out = outlen;
  int r = capn_inflate(&s);
  if (r != 0)
    return r;
  if (s.avail_out != 0)
    return -1;
  return 0;
}

TEST(Stream, PackingSpecVectorStructPointers) {
  /* encoding.html: struct pointer + text pointer. */
  AlignedData<2> unpacked = {{
      0x08, 0x00, 0x00, 0x00, 0x03, 0x00, 0x02, 0x00,
      0x19, 0x00, 0x00, 0x00, 0xaa, 0x01, 0x00, 0x00,
  }};
  const uint8_t packed_spec[] = {
      0x51, 0x08, 0x03, 0x02, 0x31, 0x19, 0xaa, 0x01,
  };
  uint8_t packed[32];
  size_t n = 0;
  ASSERT_EQ(0, deflate_all(unpacked.bytes, sizeof(unpacked.bytes), packed, sizeof(packed), &n));
  ASSERT_EQ(sizeof(packed_spec), n);
  EXPECT_EQ(0, memcmp(packed, packed_spec, n));

  uint8_t round[16];
  ASSERT_EQ(0, inflate_all(packed_spec, sizeof(packed_spec), round, sizeof(round)));
  EXPECT_EQ(0, memcmp(round, unpacked.bytes, sizeof(round)));
}

TEST(Stream, PackingSpecVectorZeroRun) {
  /* encoding.html: 32 zero bytes -> 00 03 (N+1 = 4 zero words). */
  AlignedData<4> unpacked;
  memset(unpacked.bytes, 0, sizeof(unpacked.bytes));
  const uint8_t packed_spec[] = {0x00, 0x03};
  uint8_t packed[16];
  size_t n = 0;
  ASSERT_EQ(0, deflate_all(unpacked.bytes, sizeof(unpacked.bytes), packed, sizeof(packed), &n));
  ASSERT_EQ(sizeof(packed_spec), n);
  EXPECT_EQ(0, memcmp(packed, packed_spec, n));

  uint8_t round[32];
  ASSERT_EQ(0, inflate_all(packed_spec, sizeof(packed_spec), round, sizeof(round)));
  EXPECT_EQ(0, memcmp(round, unpacked.bytes, sizeof(round)));
}

TEST(Stream, PackingSpecVectorUncompressedSpan) {
  /* encoding.html: 32 bytes of 0x8a -> ff 8a{8} 03 8a{24}. */
  AlignedData<4> unpacked;
  memset(unpacked.bytes, 0x8a, sizeof(unpacked.bytes));
  uint8_t packed_spec[2 + 8 + 24];
  packed_spec[0] = 0xff;
  memset(packed_spec + 1, 0x8a, 8);
  packed_spec[9] = 0x03;
  memset(packed_spec + 10, 0x8a, 24);
  uint8_t packed[64];
  size_t n = 0;
  ASSERT_EQ(0, deflate_all(unpacked.bytes, sizeof(unpacked.bytes), packed, sizeof(packed), &n));
  ASSERT_EQ(sizeof(packed_spec), n);
  EXPECT_EQ(0, memcmp(packed, packed_spec, n));

  uint8_t round[32];
  ASSERT_EQ(0, inflate_all(packed_spec, sizeof(packed_spec), round, sizeof(round)));
  EXPECT_EQ(0, memcmp(round, unpacked.bytes, sizeof(round)));
}

TEST(Stream, DeflateFfExtraWordsCappedAt255) {
  /* 256 all-nonzero words: one 0xFF span with N=255 extra words. */
  const size_t words256 = 256;
  AlignedData<256> in256;
  memset(in256.bytes, 0x8a, sizeof(in256.bytes));
  uint8_t packed[8192];
  size_t n = 0;
  ASSERT_EQ(0, deflate_all(in256.bytes, sizeof(in256.bytes), packed, sizeof(packed), &n));

  size_t unpacked_words = 0;
  int ff_spans = 0;
  ASSERT_TRUE(walk_packed(packed, n, &unpacked_words, &ff_spans));
  EXPECT_EQ(words256, unpacked_words);
  EXPECT_EQ(1, ff_spans);
  ASSERT_GE(n, 10u);
  EXPECT_EQ(0xFF, packed[0]);
  EXPECT_EQ(255, packed[9]);

  uint8_t round[256 * 8];
  ASSERT_EQ(0, inflate_all(packed, n, round, sizeof(round)));
  EXPECT_EQ(0, memcmp(round, in256.bytes, sizeof(round)));
}

TEST(Stream, DeflateFfRunSplitsAfter255ExtraWords) {
  /* 257 all-nonzero words: first span takes 1+255, leftover word is a
   * second 0xFF span. A 256-extra cap stores N as 0 and is not spec. */
  const size_t words257 = 257;
  AlignedData<257> in257;
  memset(in257.bytes, 0x8a, sizeof(in257.bytes));
  uint8_t packed[8192];
  size_t n = 0;
  ASSERT_EQ(0, deflate_all(in257.bytes, sizeof(in257.bytes), packed, sizeof(packed), &n));

  size_t unpacked_words = 0;
  int ff_spans = 0;
  ASSERT_TRUE(walk_packed(packed, n, &unpacked_words, &ff_spans)) << "n=" << n;
  EXPECT_EQ(words257, unpacked_words);
  EXPECT_GE(ff_spans, 2);
  ASSERT_GE(n, 10u);
  EXPECT_EQ(0xFF, packed[0]);
  EXPECT_EQ(255, packed[9]);

  uint8_t round[257 * 8];
  ASSERT_EQ(0, inflate_all(packed, n, round, sizeof(round)));
  EXPECT_EQ(0, memcmp(round, in257.bytes, sizeof(round)));
}

TEST(Stream, DeflateFfRun512WordsTwoFullSpans) {
  const size_t words512 = 512;
  AlignedData<512> in512;
  memset(in512.bytes, 0x8a, sizeof(in512.bytes));
  uint8_t packed[8192];
  size_t n = 0;
  ASSERT_EQ(0, deflate_all(in512.bytes, sizeof(in512.bytes), packed, sizeof(packed), &n));

  size_t unpacked_words = 0;
  int ff_spans = 0;
  ASSERT_TRUE(walk_packed(packed, n, &unpacked_words, &ff_spans)) << "n=" << n;
  EXPECT_EQ(words512, unpacked_words);
  EXPECT_EQ(2, ff_spans);

  uint8_t round[512 * 8];
  ASSERT_EQ(0, inflate_all(packed, n, round, sizeof(round)));
  EXPECT_EQ(0, memcmp(round, in512.bytes, sizeof(round)));
}

TEST(Stream, InflateNeedMoreWhenInputExhausted) {
  uint8_t out[32];
  memset(out, 0xAB, sizeof(out));
  struct capn_stream s;
  memset(&s, 0, sizeof(s));
  s.next_out = out;
  s.avail_out = sizeof(out);
  s.next_in = NULL;
  s.avail_in = 0;
  EXPECT_EQ(CAPN_NEED_MORE, capn_inflate(&s));
  EXPECT_EQ(sizeof(out), s.avail_out);

  /* A complete 0x00 0x00 tag unpacks one zero word; asking for more
   * output after the input ends is still NEED_MORE. */
  const uint8_t one_zero[] = {0x00, 0x00};
  memset(&s, 0, sizeof(s));
  s.next_in = one_zero;
  s.avail_in = sizeof(one_zero);
  s.next_out = out;
  s.avail_out = 16;
  EXPECT_EQ(CAPN_NEED_MORE, capn_inflate(&s));
  EXPECT_EQ(8u, s.avail_out);
}

TEST(Stream, InitMemPackedRejectsTruncated) {
  uint8_t buf[2048];
  memset(buf, 0, sizeof(buf));

  struct capn ctx1;
  capn_init_malloc(&ctx1);
  fill_one_segment(&ctx1, UINT64_C(0x1011121314151617));
  int64_t n = capn_write_mem(&ctx1, buf, sizeof(buf), 1);
  ASSERT_EQ(14, n);
  capn_free(&ctx1);

  struct capn ctx_ok;
  ASSERT_EQ(0, capn_init_mem(&ctx_ok, buf, (size_t)n, 1));
  capn_free(&ctx_ok);

  for (int64_t cut = 0; cut < n; cut++) {
    struct capn ctx;
    EXPECT_NE(0, capn_init_mem(&ctx, buf, (size_t)cut, 1)) << "cut=" << cut;
  }
}

TEST(Stream, InitMemPackedRejectsIncompleteTag) {
  struct capn ctx;

  /* Cut mid-tag: 0x00 with no count byte. */
  const uint8_t cut_zero[] = {0x00};
  EXPECT_NE(0, capn_init_mem(&ctx, cut_zero, sizeof(cut_zero), 1));

  /* 0xFF cut inside the 8-byte word. */
  const uint8_t cut_ff_word[] = {0xFF, 1, 2, 3, 4};
  EXPECT_NE(0, capn_init_mem(&ctx, cut_ff_word, sizeof(cut_ff_word), 1));

  /* 0xFF + 8 bytes, missing N. */
  const uint8_t cut_ff_n[] = {0xFF, 1, 2, 3, 4, 5, 6, 7, 8};
  EXPECT_NE(0, capn_init_mem(&ctx, cut_ff_n, sizeof(cut_ff_n), 1));

  /* 0xFF + word + N=2, only one extra word of payload. */
  const uint8_t cut_ff_payload[] = {
      0xFF, 1, 2, 3, 4, 5, 6, 7, 8, 2, 1, 1, 1, 1, 1, 1, 1, 1,
  };
  EXPECT_NE(0, capn_init_mem(&ctx, cut_ff_payload, sizeof(cut_ff_payload), 1));

  /* Tag 0x51 needs three data bytes; only one follows. */
  const uint8_t cut_sparse[] = {0x51, 0x08};
  EXPECT_NE(0, capn_init_mem(&ctx, cut_sparse, sizeof(cut_sparse), 1));
}

TEST(Stream, WriteFpPackedDenseSegmentRoundTrip) {
  uint8_t bytes[4096];
  uint8_t mem[16384];
  uint8_t from_fp[16384];
  FILE *f = tmpfile();
  ASSERT_TRUE(f != NULL);

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_dense_list8(&ctx1, bytes, kDenseBytes);
  ASSERT_GE(max_segment_len(&ctx1), (size_t)kDenseBytes);

  int64_t nmem = capn_write_mem(&ctx1, mem, sizeof(mem), 1);
  ASSERT_GT(nmem, 0);

  int n = capn_write_fp(&ctx1, f, 1);
  EXPECT_GT(n, 0);
  EXPECT_EQ(nmem, n);

  rewind(f);
  size_t nr = fread(from_fp, 1, sizeof(from_fp), f);
  EXPECT_EQ((size_t)nmem, nr);
  EXPECT_EQ(0, memcmp(mem, from_fp, (size_t)nmem));

  rewind(f);
  ASSERT_EQ(0, capn_init_fp(&ctx2, f, 1));
  expect_dense_list8(&ctx2, bytes, kDenseBytes);

  capn_free(&ctx1);
  capn_free(&ctx2);
  fclose(f);
}

TEST(Stream, WriteFdPackedDenseSegmentRoundTrip) {
  uint8_t bytes[4096];
  uint8_t mem[16384];

  struct capn ctx1, ctx2;
  capn_init_malloc(&ctx1);
  fill_dense_list8(&ctx1, bytes, kDenseBytes);
  ASSERT_GE(max_segment_len(&ctx1), (size_t)kDenseBytes);

  int64_t nmem = capn_write_mem(&ctx1, mem, sizeof(mem), 1);
  ASSERT_GT(nmem, 0);

  g_fd_out_len = 0;
  int n = capn_write_fd(&ctx1, collect_fd_write, 0, 1);
  EXPECT_GT(n, 0);
  EXPECT_EQ(nmem, n);
  EXPECT_EQ((size_t)nmem, g_fd_out_len);
  EXPECT_EQ(0, memcmp(mem, g_fd_out, (size_t)nmem));

  ASSERT_EQ(0, capn_init_mem(&ctx2, g_fd_out, g_fd_out_len, 1));
  expect_dense_list8(&ctx2, bytes, kDenseBytes);

  capn_free(&ctx1);
  capn_free(&ctx2);
}
