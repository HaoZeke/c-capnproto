/* data-codec-test.cpp
 *
 * Tests for Data and List(Data) codec generation (encode/decode/free).
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "capnp_c.h"
#include "data-codec.capnp.h"
#include "data-codec.h"
}

// Test round-trip encode/decode of a Data field
TEST(DataCodec, RoundTripDataField) {
  uint8_t payload_bytes[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03};
  const int payload_len = sizeof(payload_bytes);

  // Build a user struct
  data_message_t src;
  memset(&src, 0, sizeof(src));
  src.payload = payload_bytes;
  src.payload_len = payload_len;
  src.name = (char *)"test_message";
  src.tag = 42;
  src.n_chunks = 0;
  src.chunks = NULL;

  // Encode into capnp
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  DataMessage_ptr ptr;
  encode_DataMessage_ptr(cs, &ptr, &src);
  capn_setp(capn_root(&c), 0, ptr.p);

  // Serialize to buffer
  int64_t sz = capn_size(&c);
  ASSERT_GT(sz, 0);
  uint8_t *buf = (uint8_t *)malloc(sz);
  ASSERT_NE(buf, nullptr);
  int64_t written = capn_write_mem(&c, buf, sz, 0);
  ASSERT_EQ(written, sz);
  capn_free(&c);

  // Deserialize from buffer
  struct capn c2;
  int rc = capn_init_mem(&c2, buf, sz, 0);
  ASSERT_EQ(rc, 0);

  DataMessage_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);

  // Decode back to user struct
  data_message_t *dst = NULL;
  decode_DataMessage_ptr(&dst, ptr2);
  ASSERT_NE(dst, nullptr);

  // Verify Data field
  EXPECT_EQ(dst->payload_len, payload_len);
  ASSERT_NE(dst->payload, nullptr);
  EXPECT_EQ(memcmp(dst->payload, payload_bytes, payload_len), 0);

  // Verify Text field
  EXPECT_STREQ(dst->name, "test_message");

  // Verify UInt32 field
  EXPECT_EQ(dst->tag, 42u);

  // Verify List(Data) is empty
  EXPECT_EQ(dst->n_chunks, 0);
  EXPECT_EQ(dst->chunks, nullptr);

  // Free
  free_DataMessage_ptr(&dst);
  EXPECT_EQ(dst, nullptr);

  capn_free(&c2);
  free(buf);
}

// Test round-trip encode/decode of a List(Data) field
TEST(DataCodec, RoundTripListDataField) {
  // Build data blobs
  uint8_t blob1[] = {0x01, 0x02, 0x03, 0x04};
  uint8_t blob2[] = {0xAA, 0xBB};
  uint8_t blob3[] = {0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA};

  capnp_data_t chunks[3];
  chunks[0].data = blob1;
  chunks[0].len = sizeof(blob1);
  chunks[1].data = blob2;
  chunks[1].len = sizeof(blob2);
  chunks[2].data = blob3;
  chunks[2].len = sizeof(blob3);

  uint8_t payload_bytes[] = {0x42};

  // Build user struct
  data_message_t src;
  memset(&src, 0, sizeof(src));
  src.payload = payload_bytes;
  src.payload_len = 1;
  src.name = (char *)"chunked";
  src.tag = 99;
  src.n_chunks = 3;
  src.chunks = chunks;

  // Encode
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  DataMessage_ptr ptr;
  encode_DataMessage_ptr(cs, &ptr, &src);
  capn_setp(capn_root(&c), 0, ptr.p);

  // Serialize
  int64_t sz = capn_size(&c);
  ASSERT_GT(sz, 0);
  uint8_t *buf = (uint8_t *)malloc(sz);
  ASSERT_NE(buf, nullptr);
  int64_t written = capn_write_mem(&c, buf, sz, 0);
  ASSERT_EQ(written, sz);
  capn_free(&c);

  // Deserialize
  struct capn c2;
  int rc = capn_init_mem(&c2, buf, sz, 0);
  ASSERT_EQ(rc, 0);

  DataMessage_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);

  data_message_t *dst = NULL;
  decode_DataMessage_ptr(&dst, ptr2);
  ASSERT_NE(dst, nullptr);

  // Verify List(Data)
  EXPECT_EQ(dst->n_chunks, 3);
  ASSERT_NE(dst->chunks, nullptr);

  EXPECT_EQ(dst->chunks[0].len, (int)sizeof(blob1));
  ASSERT_NE(dst->chunks[0].data, nullptr);
  EXPECT_EQ(memcmp(dst->chunks[0].data, blob1, sizeof(blob1)), 0);

  EXPECT_EQ(dst->chunks[1].len, (int)sizeof(blob2));
  ASSERT_NE(dst->chunks[1].data, nullptr);
  EXPECT_EQ(memcmp(dst->chunks[1].data, blob2, sizeof(blob2)), 0);

  EXPECT_EQ(dst->chunks[2].len, (int)sizeof(blob3));
  ASSERT_NE(dst->chunks[2].data, nullptr);
  EXPECT_EQ(memcmp(dst->chunks[2].data, blob3, sizeof(blob3)), 0);

  // Verify other fields survived
  EXPECT_EQ(dst->payload_len, 1);
  ASSERT_NE(dst->payload, nullptr);
  EXPECT_EQ(dst->payload[0], 0x42);
  EXPECT_STREQ(dst->name, "chunked");
  EXPECT_EQ(dst->tag, 99u);

  // Free
  free_DataMessage_ptr(&dst);
  EXPECT_EQ(dst, nullptr);

  capn_free(&c2);
  free(buf);
}

// Test encoding with NULL/empty Data field
TEST(DataCodec, EmptyDataField) {
  data_message_t src;
  memset(&src, 0, sizeof(src));
  src.payload = NULL;
  src.payload_len = 0;
  src.name = (char *)"empty";
  src.tag = 0;
  src.n_chunks = 0;
  src.chunks = NULL;

  // Encode
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  DataMessage_ptr ptr;
  encode_DataMessage_ptr(cs, &ptr, &src);
  capn_setp(capn_root(&c), 0, ptr.p);

  // Serialize
  int64_t sz = capn_size(&c);
  ASSERT_GT(sz, 0);
  uint8_t *buf = (uint8_t *)malloc(sz);
  ASSERT_NE(buf, nullptr);
  int64_t written = capn_write_mem(&c, buf, sz, 0);
  ASSERT_EQ(written, sz);
  capn_free(&c);

  // Deserialize
  struct capn c2;
  int rc = capn_init_mem(&c2, buf, sz, 0);
  ASSERT_EQ(rc, 0);

  DataMessage_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);

  data_message_t *dst = NULL;
  decode_DataMessage_ptr(&dst, ptr2);
  ASSERT_NE(dst, nullptr);

  // Data field should be NULL with length 0
  EXPECT_EQ(dst->payload_len, 0);
  EXPECT_EQ(dst->payload, nullptr);

  // List(Data) should be empty
  EXPECT_EQ(dst->n_chunks, 0);
  EXPECT_EQ(dst->chunks, nullptr);

  EXPECT_STREQ(dst->name, "empty");
  EXPECT_EQ(dst->tag, 0u);

  free_DataMessage_ptr(&dst);
  capn_free(&c2);
  free(buf);
}

// Test that binary data with embedded zeros survives round-trip
TEST(DataCodec, BinaryDataWithZeros) {
  uint8_t payload_bytes[] = {0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x03};
  const int payload_len = sizeof(payload_bytes);

  data_message_t src;
  memset(&src, 0, sizeof(src));
  src.payload = payload_bytes;
  src.payload_len = payload_len;
  src.name = (char *)"zeros";
  src.tag = 7;
  src.n_chunks = 0;
  src.chunks = NULL;

  // Encode
  struct capn c;
  capn_init_malloc(&c);
  struct capn_segment *cs = capn_root(&c).seg;

  DataMessage_ptr ptr;
  encode_DataMessage_ptr(cs, &ptr, &src);
  capn_setp(capn_root(&c), 0, ptr.p);

  // Serialize
  int64_t sz = capn_size(&c);
  uint8_t *buf = (uint8_t *)malloc(sz);
  capn_write_mem(&c, buf, sz, 0);
  capn_free(&c);

  // Deserialize
  struct capn c2;
  capn_init_mem(&c2, buf, sz, 0);

  DataMessage_ptr ptr2;
  ptr2.p = capn_getp(capn_root(&c2), 0, 1);

  data_message_t *dst = NULL;
  decode_DataMessage_ptr(&dst, ptr2);
  ASSERT_NE(dst, nullptr);

  // Verify all bytes including embedded zeros survived
  EXPECT_EQ(dst->payload_len, payload_len);
  ASSERT_NE(dst->payload, nullptr);
  EXPECT_EQ(memcmp(dst->payload, payload_bytes, payload_len), 0);

  free_DataMessage_ptr(&dst);
  capn_free(&c2);
  free(buf);
}
