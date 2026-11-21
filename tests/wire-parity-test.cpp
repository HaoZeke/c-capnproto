/* wire-parity-test.cpp
 *
 * Pointers C++ 1.4.0 rejects must fail here, and pointers it reads
 * must come back with the same bytes. Where the capnp tool is on
 * PATH, builder messages are canonicalised and packed by both and
 * the bytes are compared.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "capnp_c.h"
#include "capnp-cli.h"

static void wr32(uint8_t *p, uint32_t v) {
	p[0] = (uint8_t) v;
	p[1] = (uint8_t) (v >> 8);
	p[2] = (uint8_t) (v >> 16);
	p[3] = (uint8_t) (v >> 24);
}

static int init_one_seg(struct capn *c, const uint8_t *seg, size_t segsz,
			uint8_t *framed, size_t framed_cap) {
	if (segsz % 8 != 0 || framed_cap < 8 + segsz)
		return -1;
	wr32(framed + 0, 0);
	wr32(framed + 4, (uint32_t) (segsz / 8));
	memcpy(framed + 8, seg, segsz);
	return capn_init_mem(c, framed, 8 + segsz, 0);
}

static std::vector<uint8_t> frame_segs(const std::vector<std::vector<uint8_t>> &segs) {
	uint32_t n = (uint32_t) segs.size();
	uint32_t nwords = ((2 + n) / 2) * 2;
	std::vector<uint8_t> out(nwords * 4, 0);
	wr32(out.data(), n - 1);
	size_t body = 0;
	for (uint32_t i = 0; i < n; i++) {
		wr32(out.data() + 4 * (1 + i), (uint32_t) (segs[i].size() / 8));
		body += segs[i].size();
	}
	out.resize(out.size() + body);
	size_t off = nwords * 4;
	for (uint32_t i = 0; i < n; i++) {
		memcpy(out.data() + off, segs[i].data(), segs[i].size());
		off += segs[i].size();
	}
	return out;
}

static std::vector<uint8_t> canon_bytes(struct capn *src) {
	struct capn dst;
	std::vector<uint8_t> out;
	capn_init_malloc(&dst);
	if (capn_canonicalize(src, &dst) != 0 || dst.seglist == NULL) {
		capn_free(&dst);
		return out;
	}
	out.assign(dst.seglist->data, dst.seglist->data + dst.seglist->len);
	capn_free(&dst);
	return out;
}

static std::vector<uint8_t> write_frame(struct capn *c, int packed) {
	int64_t n = capn_size(c);
	std::vector<uint8_t> out;
	if (n < 0)
		return out;
	/* Packed output is at most a small constant bigger than the
	 * unpacked frame. capn_size is the unpacked size. */
	size_t cap = (size_t) n * 2 + 64;
	out.resize(cap);
	int64_t sz = capn_write_mem(c, out.data(), out.size(), packed);
	if (sz < 0) {
		out.clear();
		return out;
	}
	out.resize((size_t) sz);
	return out;
}

TEST(WireParity, CompositeTagMustBeAStruct) {
	/* Inline composite whose tag is a list pointer. C++ 1.4.0
	 * rejects it: the tag has to be a struct pointer. */
	uint8_t seg[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00,
		0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	};
	uint8_t framed[8 + sizeof seg];
	struct capn c;
	struct capn dst;

	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof seg, framed, sizeof framed));
	EXPECT_NE(0, capn_validate(&c));
	capn_clear_err(&c);
	capn_ptr root = capn_getp(capn_root(&c), 0, 1);
	ASSERT_EQ(CAPN_STRUCT, root.type);
	EXPECT_EQ(CAPN_NULL, capn_getp(root, 0, 1).type);
	EXPECT_EQ(0, capn_ok(&c));

	capn_init_malloc(&dst);
	EXPECT_NE(0, capn_canonicalize(&c, &dst));
	capn_free(&dst);
	capn_free(&c);
}

TEST(WireParity, ValidCompositeListStillReads) {
	/* One element, two data words, tag is a struct pointer. */
	uint8_t seg[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00,
		0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
		0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
	};
	uint8_t framed[8 + sizeof seg];
	struct capn c;

	ASSERT_EQ(0, init_one_seg(&c, seg, sizeof seg, framed, sizeof framed));
	EXPECT_EQ(0, capn_validate(&c));
	EXPECT_EQ(1, capn_ok(&c));
	capn_ptr root = capn_getp(capn_root(&c), 0, 1);
	ASSERT_EQ(CAPN_STRUCT, root.type);
	capn_ptr list = capn_getp(root, 0, 1);
	EXPECT_EQ(CAPN_LIST, list.type);
	EXPECT_EQ(1, list.is_composite_list);
	EXPECT_EQ(1, list.len);
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}

static void expect_cli_agrees(struct capn *c) {
	if (!capnp_present()) {
		/* The meson suite only builds this file when capnp exists.
		 * A hand-built tree without it still runs the pointer tests. */
		return;
	}
	std::vector<uint8_t> framed = write_frame(c, 0);
	ASSERT_FALSE(framed.empty());
	std::vector<uint8_t> ours = canon_bytes(c);
	ASSERT_FALSE(ours.empty());

	std::vector<std::string> argv;
	argv.push_back(capnp_bin());
	argv.push_back("convert");
	argv.push_back("binary:canonical");
	argv.push_back("--quiet");
	CmdResult got = run_argv(argv, framed.data(), framed.size());
	ASSERT_EQ(0, got.status) << got.err;
	ASSERT_EQ(ours.size(), got.out.size());
	EXPECT_EQ(0, memcmp(ours.data(), got.out.data(), ours.size()));

	std::vector<uint8_t> packed = write_frame(c, 1);
	argv[2] = "binary:packed";
	CmdResult pk = run_argv(argv, framed.data(), framed.size());
	ASSERT_EQ(0, pk.status) << pk.err;
	ASSERT_EQ(packed.size(), pk.out.size());
	EXPECT_EQ(0, memcmp(packed.data(), pk.out.data(), packed.size()));
}

TEST(WireParity, BuilderMessagesMatchTheCapnpTool) {
	std::mt19937 rng(0xC0FFEE);
	for (int n = 0; n < 20; n++) {
		struct capn c;
		capn_init_malloc(&c);
		struct capn_segment *seg = capn_root(&c).seg;
		int dw = (int) (rng() % 4) * 8;
		int pw = (int) (rng() % 3);
		capn_ptr st = capn_new_struct(seg, dw, pw);
		ASSERT_EQ(CAPN_STRUCT, st.type);
		if (dw >= 4)
			capn_write32(st, 0, rng());
		if (pw > 0) {
			char buf[8];
			int len = (int) (rng() % 6);
			for (int i = 0; i < len; i++)
				buf[i] = (char) ('a' + (rng() % 26));
			buf[len] = 0;
			capn_text tx;
			tx.str = buf;
			tx.len = len;
			tx.seg = NULL;
			ASSERT_EQ(0, capn_set_text(st, 0, tx));
		}
		if (pw > 1) {
			int count = (int) (rng() % 5);
			capn_ptr list = capn_new_struct_list(st.seg, count, 8, 0);
			ASSERT_EQ(CAPN_LIST, list.type);
			for (int i = 0; i < count; i++) {
				capn_ptr el = capn_getp(list, i, 0);
				capn_write32(el, 0, rng());
			}
			ASSERT_EQ(0, capn_setp(st, 1, list));
		}
		ASSERT_EQ(0, capn_set_root(&c, st));
		expect_cli_agrees(&c);
		capn_free(&c);
	}
}
