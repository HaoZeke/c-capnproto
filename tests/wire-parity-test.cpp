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

#include <stdio.h>
#include <unistd.h>

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

TEST(WireParity, DoubleFarIgnoresANonzeroTagOffset) {
	/* Pad is a far pointer to word 0 of segment 2. The tag is a
	 * struct pointer with offset 1. C++ 1.4.0 reads word 0. */
	std::vector<uint8_t> s0 = {0x06, 0, 0, 0, 0x01, 0, 0, 0};
	std::vector<uint8_t> s1 = {
		0x02, 0, 0, 0, 0x02, 0, 0, 0,
		0x04, 0, 0, 0, 0x01, 0, 0, 0,
	};
	std::vector<uint8_t> s2 = {
		0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0,
		0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
	};
	std::vector<uint8_t> fr = frame_segs({s0, s1, s2});
	struct capn c;
	uint8_t expect[16] = {
		0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00,
	};

	ASSERT_EQ(0, capn_init_mem(&c, fr.data(), fr.size(), 0));
	EXPECT_EQ(0, capn_validate(&c));
	capn_ptr root = capn_getp(capn_root(&c), 0, 1);
	ASSERT_EQ(CAPN_STRUCT, root.type);
	EXPECT_EQ(0x44332211u, capn_read32(root, 0));
	EXPECT_EQ(1, capn_ok(&c));
	std::vector<uint8_t> got = canon_bytes(&c);
	ASSERT_EQ(sizeof expect, got.size());
	EXPECT_EQ(0, memcmp(expect, got.data(), sizeof expect));
	capn_free(&c);
}

TEST(WireParity, DoubleFarCapabilityReadsTheIndex) {
	/* Double far whose tag is a capability, index 4. C++ 1.4.0
	 * reports pointer type capability. */
	std::vector<uint8_t> s0 = {0x06, 0, 0, 0, 0x01, 0, 0, 0};
	std::vector<uint8_t> s1 = {
		0x02, 0, 0, 0, 0x02, 0, 0, 0,
		0x03, 0, 0, 0, 0x04, 0, 0, 0,
	};
	std::vector<uint8_t> s2 = {0, 0, 0, 0, 0, 0, 0, 0};
	std::vector<uint8_t> fr = frame_segs({s0, s1, s2});
	struct capn c;

	ASSERT_EQ(0, capn_init_mem(&c, fr.data(), fr.size(), 0));
	EXPECT_EQ(0, capn_validate(&c));
	capn_ptr root = capn_getp(capn_root(&c), 0, 1);
	EXPECT_EQ(CAPN_INTERFACE, root.type);
	EXPECT_EQ(4, root.cap_idx);
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}

TEST(WireParity, CanonicalFormRefusesACapability) {
	/* Root capability, then a struct whose only pointer is one.
	 * C++ 1.4.0 refuses both with "Cannot create a canonical
	 * message with a capability". */
	const uint8_t root_cap[] = {0x03, 0, 0, 0, 0x04, 0, 0, 0};
	const uint8_t nested[] = {
		0x00, 0, 0, 0, 0x00, 0, 0x01, 0,
		0x03, 0, 0, 0, 0x04, 0, 0, 0,
	};
	uint8_t framed[64];
	struct capn c, dst;

	ASSERT_EQ(0, init_one_seg(&c, root_cap, sizeof root_cap, framed, sizeof framed));
	capn_init_malloc(&dst);
	EXPECT_NE(0, capn_canonicalize(&c, &dst));
	capn_free(&dst);
	capn_free(&c);

	ASSERT_EQ(0, init_one_seg(&c, nested, sizeof nested, framed, sizeof framed));
	capn_init_malloc(&dst);
	EXPECT_NE(0, capn_canonicalize(&c, &dst));
	capn_free(&dst);
	capn_free(&c);
}

static capn_text text_def(void) {
	capn_text t;
	memset(&t, 0, sizeof t);
	t.str = "hi";
	t.len = 2;
	return t;
}

TEST(WireParity, TextWithoutATerminatorSetsDecodeErr) {
	/* One byte, no NUL. C++ 1.4.0 throws. A wire-null pointer
	 * still returns the default and leaves capn_ok set. */
	const uint8_t bad[] = {
		0x00, 0, 0, 0, 0x00, 0, 0x01, 0,
		0x01, 0, 0, 0, 0x0a, 0, 0, 0,
		0x41, 0, 0, 0, 0, 0, 0, 0,
	};
	const uint8_t good[] = {
		0x00, 0, 0, 0, 0x00, 0, 0x01, 0,
		0x01, 0, 0, 0, 0x12, 0, 0, 0,
		0x41, 0, 0, 0, 0, 0, 0, 0,
	};
	const uint8_t absent[] = {
		0x00, 0, 0, 0, 0x00, 0, 0x01, 0,
		0x00, 0, 0, 0, 0x00, 0, 0, 0,
	};
	uint8_t framed[64];
	struct capn c;
	capn_text got;

	ASSERT_EQ(0, init_one_seg(&c, bad, sizeof bad, framed, sizeof framed));
	got = capn_get_text(capn_getp(capn_root(&c), 0, 1), 0, text_def());
	EXPECT_EQ(0, capn_ok(&c));
	EXPECT_STREQ("hi", got.str);
	EXPECT_EQ(2, got.len);
	capn_free(&c);

	ASSERT_EQ(0, init_one_seg(&c, good, sizeof good, framed, sizeof framed));
	got = capn_get_text(capn_getp(capn_root(&c), 0, 1), 0, text_def());
	EXPECT_EQ(1, capn_ok(&c));
	ASSERT_NE(nullptr, got.str);
	EXPECT_EQ(1, got.len);
	EXPECT_EQ('A', got.str[0]);
	capn_free(&c);

	ASSERT_EQ(0, init_one_seg(&c, absent, sizeof absent, framed, sizeof framed));
	got = capn_get_text(capn_getp(capn_root(&c), 0, 1), 0, text_def());
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_STREQ("hi", got.str);
	capn_free(&c);
}

TEST(WireParity, DataThatIsNotBytesSetsDecodeErr) {
	/* A struct where Data was expected. C++ 1.4.0 throws a schema
	 * mismatch. A wire-null pointer and a byte list stay quiet. */
	const uint8_t bad[] = {
		0x00, 0, 0, 0, 0x00, 0, 0x01, 0,
		0x00, 0, 0, 0, 0x01, 0, 0, 0,
		0x11, 0x22, 0x33, 0x44, 0, 0, 0, 0,
	};
	const uint8_t one[] = {
		0x00, 0, 0, 0, 0x00, 0, 0x01, 0,
		0x01, 0, 0, 0, 0x0a, 0, 0, 0,
		0x41, 0, 0, 0, 0, 0, 0, 0,
	};
	const uint8_t empty[] = {
		0x00, 0, 0, 0, 0x00, 0, 0x01, 0,
		0x01, 0, 0, 0, 0x02, 0, 0, 0,
	};
	const uint8_t absent[] = {
		0x00, 0, 0, 0, 0x00, 0, 0x01, 0,
		0x00, 0, 0, 0, 0x00, 0, 0, 0,
	};
	uint8_t framed[64];
	struct capn c;
	capn_data got;

	ASSERT_EQ(0, init_one_seg(&c, bad, sizeof bad, framed, sizeof framed));
	got = capn_get_data(capn_getp(capn_root(&c), 0, 1), 0);
	EXPECT_EQ(0, capn_ok(&c));
	EXPECT_EQ(CAPN_NULL, got.p.type);
	capn_free(&c);

	ASSERT_EQ(0, init_one_seg(&c, one, sizeof one, framed, sizeof framed));
	got = capn_get_data(capn_getp(capn_root(&c), 0, 1), 0);
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_EQ(CAPN_LIST, got.p.type);
	EXPECT_EQ(1, got.p.len);
	ASSERT_NE(nullptr, got.p.data);
	EXPECT_EQ(0x41, (unsigned char) got.p.data[0]);
	capn_free(&c);

	ASSERT_EQ(0, init_one_seg(&c, empty, sizeof empty, framed, sizeof framed));
	got = capn_get_data(capn_getp(capn_root(&c), 0, 1), 0);
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_EQ(CAPN_LIST, got.p.type);
	EXPECT_EQ(0, got.p.len);
	capn_free(&c);

	ASSERT_EQ(0, init_one_seg(&c, absent, sizeof absent, framed, sizeof framed));
	got = capn_get_data(capn_getp(capn_root(&c), 0, 1), 0);
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_EQ(CAPN_NULL, got.p.type);
	capn_free(&c);
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

static int load_frame(struct capn *c, const uint8_t *b, size_t n)
{
	return capn_init_mem(c, (uint8_t *) b, n, 0);
}

static void expect_empty_struct_canon(const uint8_t *frame, size_t n,
				       const uint8_t *canon, size_t cn)
{
	struct capn c, dst;
	capn_ptr root;

	ASSERT_EQ(0, load_frame(&c, frame, n));
	EXPECT_EQ(0, capn_validate(&c));
	EXPECT_EQ(1, capn_ok(&c));
	root = capn_getp(capn_root(&c), 0, 1);
	if (root.ptrs == 1 && root.datasz == 0) {
		capn_ptr child = capn_getp(root, 0, 1);
		EXPECT_EQ(CAPN_STRUCT, child.type);
		EXPECT_EQ(0u, child.datasz);
		EXPECT_EQ(0u, child.ptrs);
	} else {
		EXPECT_EQ(CAPN_STRUCT, root.type);
		EXPECT_EQ(0u, root.datasz);
		EXPECT_EQ(0u, root.ptrs);
	}
	capn_init_malloc(&dst);
	ASSERT_EQ(0, capn_canonicalize(&c, &dst));
	ASSERT_NE(nullptr, dst.seglist);
	ASSERT_EQ(cn, dst.seglist->len);
	EXPECT_EQ(0, memcmp(canon, dst.seglist->data, cn));
	capn_free(&dst);
	capn_free(&c);
}

TEST(WireParity, EmptyStructThroughAFarStaysAStruct) {
	/* Double far, empty struct tag, target is word zero.
	 * Offsets 0, 1 and -1. Root and a nested slot. A single
	 * far that lands on a zero word is the same object. */
	const uint8_t dfar0[] = {
		0x02,0,0,0, 0x01,0,0,0, 0x02,0,0,0, 0x01,0,0,0,
		0x06,0,0,0, 0x01,0,0,0,
		0x02,0,0,0, 0x02,0,0,0, 0x00,0,0,0, 0x00,0,0,0,
		0x00,0,0,0, 0x00,0,0,0,
	};
	const uint8_t dfar1[] = {
		0x02,0,0,0, 0x01,0,0,0, 0x02,0,0,0, 0x01,0,0,0,
		0x06,0,0,0, 0x01,0,0,0,
		0x02,0,0,0, 0x02,0,0,0, 0x04,0,0,0, 0x00,0,0,0,
		0x00,0,0,0, 0x00,0,0,0,
	};
	const uint8_t dfarm[] = {
		0x02,0,0,0, 0x01,0,0,0, 0x02,0,0,0, 0x01,0,0,0,
		0x06,0,0,0, 0x01,0,0,0,
		0x02,0,0,0, 0x02,0,0,0, 0xfc,0xff,0xff,0xff, 0x00,0,0,0,
		0x00,0,0,0, 0x00,0,0,0,
	};
	const uint8_t sfar[] = {
		0x01,0,0,0, 0x01,0,0,0, 0x01,0,0,0, 0x00,0,0,0,
		0x02,0,0,0, 0x01,0,0,0,
		0x00,0,0,0, 0x00,0,0,0,
	};
	const uint8_t nested[] = {
		0x01,0,0,0, 0x02,0,0,0, 0x01,0,0,0, 0x00,0,0,0,
		0x00,0,0,0, 0x00,0,0x01,0, 0x02,0,0,0, 0x01,0,0,0,
		0x00,0,0,0, 0x00,0,0,0,
	};
	const uint8_t empty_root[] = {
		0xfc,0xff,0xff,0xff, 0x00,0,0,0,
	};
	const uint8_t empty_nest[] = {
		0x00,0,0,0, 0x00,0,0x01,0,
		0xfc,0xff,0xff,0xff, 0x00,0,0,0,
	};

	expect_empty_struct_canon(dfar0, sizeof dfar0, empty_root, sizeof empty_root);
	expect_empty_struct_canon(dfar1, sizeof dfar1, empty_root, sizeof empty_root);
	expect_empty_struct_canon(dfarm, sizeof dfarm, empty_root, sizeof empty_root);
	expect_empty_struct_canon(sfar, sizeof sfar, empty_root, sizeof empty_root);
	expect_empty_struct_canon(nested, sizeof nested, empty_nest, sizeof empty_nest);
}

TEST(WireParity, EmptyStructInTheFirstSlotIsNotACycle) {
	/* Struct with no data word. Slot zero holds an empty struct
	 * whose offset is -1, so the child starts on the parent's word. */
	const uint8_t frame[] = {
		0x00,0,0,0, 0x02,0,0,0,
		0x00,0,0,0, 0x00,0,0x01,0,
		0xfc,0xff,0xff,0xff, 0x00,0,0,0,
	};
	const uint8_t canon[] = {
		0x00,0,0,0, 0x00,0,0x01,0,
		0xfc,0xff,0xff,0xff, 0x00,0,0,0,
	};
	expect_empty_struct_canon(frame, sizeof frame, canon, sizeof canon);
}

TEST(WireParity, ZeroSizeCompositeMatchesTheTraversalBoundary) {
	auto frame = [](uint32_t L) {
		std::vector<uint8_t> b(40, 0);
		wr32(b.data() + 4, 4);
		wr32(b.data() + 12, 0x00010000);
		wr32(b.data() + 16, 1);
		wr32(b.data() + 20, 0x0f);
		wr32(b.data() + 24, L << 2);
		return b;
	};
	struct capn c;
	std::vector<uint8_t> ok = frame(8388603);
	std::vector<uint8_t> over = frame(8388604);
	ASSERT_EQ(0, load_frame(&c, ok.data(), ok.size()));
	EXPECT_EQ(0, capn_validate(&c));
	capn_free(&c);
	ASSERT_EQ(0, load_frame(&c, over.data(), over.size()));
	EXPECT_NE(0, capn_validate(&c));
	capn_free(&c);
}

TEST(WireParity, ZeroSizeCompositeChargesTheTraversalLimit) {
	/* 16777216 zero-size composite elements and one padding word.
	 * The padding made the old charge look like eight bytes. */
	const uint8_t huge[] = {
		0x00,0,0,0, 0x04,0,0,0,
		0x00,0,0,0, 0x00,0,0x01,0,
		0x01,0,0,0, 0x0f,0,0,0,
		0x00,0,0,0x04, 0x00,0,0,0,
		0x00,0,0,0, 0x00,0,0,0,
	};
	/* Two elements, one padding word: still inside the limit. */
	const uint8_t small[] = {
		0x00,0,0,0, 0x04,0,0,0,
		0x00,0,0,0, 0x00,0,0x01,0,
		0x01,0,0,0, 0x0f,0,0,0,
		0x08,0,0,0, 0x00,0,0,0,
		0x00,0,0,0, 0x00,0,0,0,
	};
	struct capn c;
	capn_ptr root, list;

	ASSERT_EQ(0, load_frame(&c, huge, sizeof huge));
	EXPECT_NE(0, capn_validate(&c));
	EXPECT_EQ(0, capn_ok(&c));
	capn_free(&c);

	ASSERT_EQ(0, load_frame(&c, small, sizeof small));
	EXPECT_EQ(0, capn_validate(&c));
	EXPECT_EQ(1, capn_ok(&c));
	root = capn_getp(capn_root(&c), 0, 1);
	list = capn_getp(root, 0, 1);
	EXPECT_EQ(CAPN_LIST, list.type);
	EXPECT_EQ(2, list.len);
	capn_free(&c);
}

TEST(WireParity, HugeVoidListExceedsTheTraversalLimit) {
	/* Count 8388608 fills the default limit before the pointer
	 * word is counted, so the read refuses it. */
	const uint8_t frame[] = {
		0,0,0,0, 1,0,0,0,
		1,0,0,0, 0,0,0,4,
	};
	struct capn c;
	capn_ptr root;
	ASSERT_EQ(0, load_frame(&c, frame, sizeof frame));
	root = capn_getp(capn_root(&c), 0, 1);
	EXPECT_EQ(CAPN_NULL, root.type);
	EXPECT_EQ(0, capn_ok(&c));
	capn_free(&c);
}

TEST(WireParity, VoidListBoundaryMatchesTheRootRead) {
	/* 8388606 still fits. 8388607 is one word over a root read. */
	const uint8_t fit[] = {
		0,0,0,0, 1,0,0,0,
		1,0,0,0, 0xf0,0xff,0xff,0x03,
	};
	const uint8_t over[] = {
		0,0,0,0, 1,0,0,0,
		1,0,0,0, 0xf8,0xff,0xff,0x03,
	};
	struct capn c, dst;
	capn_ptr root;

	ASSERT_EQ(0, load_frame(&c, fit, sizeof fit));
	root = capn_getp(capn_root(&c), 0, 1);
	EXPECT_EQ(CAPN_LIST, root.type);
	EXPECT_EQ(8388606, root.len);
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);

	ASSERT_EQ(0, load_frame(&c, fit, sizeof fit));
	capn_init_malloc(&dst);
	EXPECT_EQ(0, capn_set_root(&dst, capn_root(&c)));
	capn_free(&dst);
	capn_free(&c);

	ASSERT_EQ(0, load_frame(&c, over, sizeof over));
	root = capn_getp(capn_root(&c), 0, 1);
	EXPECT_EQ(CAPN_NULL, root.type);
	EXPECT_EQ(0, capn_ok(&c));
	capn_free(&c);

	ASSERT_EQ(0, load_frame(&c, over, sizeof over));
	capn_init_malloc(&dst);
	EXPECT_NE(0, capn_set_root(&dst, capn_root(&c)));
	capn_free(&dst);
	capn_free(&c);
}

TEST(WireParity, SeveralEmptyVoidListsFit) {
	/* Three million empty void lists. Each used to add sixteen bytes. */
	const uint32_t n = 3000000;
	const uint32_t words = 1u + n;
	std::vector<uint8_t> b(8u + (size_t) words * 8u, 0);
	wr32(b.data() + 4, words);
	wr32(b.data() + 8, 1);
	wr32(b.data() + 12, (n << 3) | 6);
	for (uint32_t i = 0; i < n; i++)
		b[16 + (size_t) i * 8] = 1;
	struct capn c;
	capn_ptr root, elem;
	ASSERT_EQ(0, load_frame(&c, b.data(), b.size()));
	EXPECT_EQ(0, capn_validate(&c));
	root = capn_getp(capn_root(&c), 0, 1);
	EXPECT_EQ(CAPN_PTR_LIST, root.type);
	EXPECT_EQ((int) n, root.len);
	elem = capn_getp(root, 0, 1);
	EXPECT_EQ(CAPN_LIST, elem.type);
	EXPECT_EQ(0, elem.len);
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}

TEST(WireParity, TwoLargeVoidListsFit) {
	/* Two void lists of 4194302. The pair fills the default budget. */
	const uint8_t frame[] = {
		0,0,0,0, 3,0,0,0,
		0,0,0,0, 0,0,2,0,
		5,0,0,0, 0xf0,0xff,0xff,1,
		1,0,0,0, 0xf0,0xff,0xff,1,
	};
	struct capn c;
	capn_ptr root, a, b;
	ASSERT_EQ(0, load_frame(&c, frame, sizeof frame));
	EXPECT_EQ(0, capn_validate(&c));
	root = capn_getp(capn_root(&c), 0, 1);
	a = capn_getp(root, 0, 1);
	b = capn_getp(root, 1, 1);
	EXPECT_EQ(CAPN_LIST, a.type);
	EXPECT_EQ(4194302, a.len);
	EXPECT_EQ(CAPN_LIST, b.type);
	EXPECT_EQ(4194302, b.len);
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}

static int canon_words(const uint8_t *frame, size_t n, size_t words)
{
	struct capn src, dst;
	int rc;

	if (capn_init_mem(&src, (uint8_t *) frame, n, 0) != 0)
		return -2;
	if (words)
		src.traversal_limit = words * 8;
	src.traversal_used = 0;
	capn_init_malloc(&dst);
	rc = capn_canonicalize(&src, &dst);
	capn_free(&src);
	capn_free(&dst);
	return rc;
}

TEST(WireParity, CanonicalVoidListMatchesTheCopyBudget) {
	/* A struct holding a 22-element void list. C++ canonicalize
	 * needs 24 words: the root pointer and one word per element.
	 * The two longer frames are void and zero-stride lists that
	 * setRoot rejects and canonicalize accepts at the default limit. */
	const uint8_t small[] = {
		0,0,0,0, 2,0,0,0,
		0,0,0,0, 0,0,1,0,
		1,0,0,0, 0xb0,0,0,0,
	};
	const uint8_t voids[] = {
		0,0,0,0, 2,0,0,0,
		0,0,0,0, 0,0,1,0,
		1,0,0,0, 0xf0,0xff,0xff,0x03,
	};
	const uint8_t zcomp[] = {
		0,0,0,0, 3,0,0,0,
		0,0,0,0, 0,0,1,0,
		1,0,0,0, 7,0,0,0,
		0xf4,0xff,0xff,0x01, 0,0,0,0,
	};

	EXPECT_NE(0, canon_words(small, sizeof small, 23));
	EXPECT_EQ(0, canon_words(small, sizeof small, 24));
	EXPECT_EQ(0, canon_words(voids, sizeof voids, 0));
	EXPECT_EQ(0, canon_words(zcomp, sizeof zcomp, 0));
}

TEST(WireParity, TwoZeroStrideCompositeListsFit) {
	/* Two zero-stride composite lists of 4194301. */
	const uint8_t frame[] = {
		0,0,0,0, 5,0,0,0,
		0,0,0,0, 0,0,2,0,
		5,0,0,0, 7,0,0,0,
		5,0,0,0, 7,0,0,0,
		0xf4,0xff,0xff,0, 0,0,0,0,
		0xf4,0xff,0xff,0, 0,0,0,0,
	};
	struct capn c;
	capn_ptr root, a, b;
	ASSERT_EQ(0, load_frame(&c, frame, sizeof frame));
	EXPECT_EQ(0, capn_validate(&c));
	root = capn_getp(capn_root(&c), 0, 1);
	a = capn_getp(root, 0, 1);
	b = capn_getp(root, 1, 1);
	EXPECT_EQ(CAPN_LIST, a.type);
	EXPECT_EQ(4194301, a.len);
	EXPECT_EQ(CAPN_LIST, b.type);
	EXPECT_EQ(4194301, b.len);
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}

TEST(WireParity, CompositeTagCountUsesTwentyNineBits) {
	/* Bit 31 of the tag is outside the 29-bit element count. */
	const uint8_t frame[] = {
		0x00,0,0,0, 0x04,0,0,0,
		0x00,0,0,0, 0x00,0,0x01,0,
		0x01,0,0,0, 0x0f,0,0,0,
		0x04,0,0,0x80, 0x00,0,0,0,
		0x00,0,0,0, 0x00,0,0,0,
	};
	const uint8_t canon[] = {
		0x00,0,0,0, 0x00,0,0x01,0,
		0x01,0,0,0, 0x07,0,0,0,
		0x04,0,0,0, 0x00,0,0,0,
	};
	struct capn c, dst;
	capn_ptr list;

	ASSERT_EQ(0, load_frame(&c, frame, sizeof frame));
	list = capn_getp(capn_getp(capn_root(&c), 0, 1), 0, 1);
	EXPECT_EQ(1, list.len);
	EXPECT_EQ(0, capn_validate(&c));
	capn_init_malloc(&dst);
	ASSERT_EQ(0, capn_canonicalize(&c, &dst));
	ASSERT_NE(nullptr, dst.seglist);
	ASSERT_EQ(sizeof canon, dst.seglist->len);
	EXPECT_EQ(0, memcmp(canon, dst.seglist->data, sizeof canon));
	capn_free(&dst);
	capn_free(&c);
}

TEST(WireParity, LandingPadMayRepeatTheDoubleFarBit) {
	/* The landing far has its double-far bit set. The kind is
	 * still far, which is what C++ 1.4.0 checks. */
	const uint8_t frame[] = {
		0x02,0,0,0, 0x01,0,0,0, 0x02,0,0,0, 0x01,0,0,0,
		0x06,0,0,0, 0x01,0,0,0,
		0x06,0,0,0, 0x02,0,0,0, 0x00,0,0,0, 0x01,0,0,0,
		0x11,0x22,0x33,0x44, 0x00,0,0,0,
	};
	const uint8_t canon[] = {
		0x00,0,0,0, 0x01,0,0,0,
		0x11,0x22,0x33,0x44, 0x00,0,0,0,
	};
	struct capn c, dst;
	capn_ptr root;

	ASSERT_EQ(0, load_frame(&c, frame, sizeof frame));
	EXPECT_EQ(0, capn_validate(&c));
	root = capn_getp(capn_root(&c), 0, 1);
	EXPECT_EQ(CAPN_STRUCT, root.type);
	EXPECT_EQ(8u, root.datasz);
	EXPECT_EQ(0u, root.ptrs);
	ASSERT_NE(nullptr, root.data);
	EXPECT_EQ(0, memcmp(root.data, "\x11\x22\x33\x44", 4));
	capn_init_malloc(&dst);
	ASSERT_EQ(0, capn_canonicalize(&c, &dst));
	ASSERT_NE(nullptr, dst.seglist);
	ASSERT_EQ(sizeof canon, dst.seglist->len);
	EXPECT_EQ(0, memcmp(canon, dst.seglist->data, sizeof canon));
	capn_free(&dst);
	capn_free(&c);
}

TEST(WireParity, OutOfRangeEmptyListIsClamped) {
	const uint8_t byte_list[] = {
		0,0,0,0, 2,0,0,0,
		0,0,0,0, 0,0,1,0,
		0xf1,0xff,0xff,0x0f, 2,0,0,0,
	};
	const uint8_t void_list[] = {
		0,0,0,0, 2,0,0,0,
		0,0,0,0, 0,0,1,0,
		0xf1,0xff,0xff,0x0f, 0x28,0,0,0,
	};
	const uint8_t expect_byte[] = {
		0,0,0,0, 0,0,1,0,
		1,0,0,0, 2,0,0,0,
	};
	const uint8_t ptr_list[] = {
		0,0,0,0, 2,0,0,0,
		0,0,0,0, 0,0,1,0,
		0xf1,0xff,0xff,0x0f, 6,0,0,0,
	};
	const uint8_t expect_void[] = {
		0,0,0,0, 0,0,1,0,
		1,0,0,0, 0x28,0,0,0,
	};
	const uint8_t expect_ptr[] = {
		0,0,0,0, 0,0,1,0,
		1,0,0,0, 6,0,0,0,
	};
	struct capn c, dst;
	ASSERT_EQ(0, load_frame(&c, byte_list, sizeof byte_list));
	EXPECT_EQ(0, capn_validate(&c));
	capn_init_malloc(&dst);
	ASSERT_EQ(0, capn_canonicalize(&c, &dst));
	ASSERT_EQ(sizeof expect_byte, dst.seglist->len);
	EXPECT_EQ(0, memcmp(expect_byte, dst.seglist->data, sizeof expect_byte));
	capn_free(&dst);
	capn_free(&c);
	ASSERT_EQ(0, load_frame(&c, void_list, sizeof void_list));
	EXPECT_EQ(0, capn_validate(&c));
	capn_init_malloc(&dst);
	ASSERT_EQ(0, capn_canonicalize(&c, &dst));
	ASSERT_EQ(sizeof expect_void, dst.seglist->len);
	EXPECT_EQ(0, memcmp(expect_void, dst.seglist->data, sizeof expect_void));
	capn_free(&dst);
	capn_free(&c);
	ASSERT_EQ(0, load_frame(&c, ptr_list, sizeof ptr_list));
	EXPECT_EQ(0, capn_validate(&c));
	capn_init_malloc(&dst);
	ASSERT_EQ(0, capn_canonicalize(&c, &dst));
	ASSERT_EQ(sizeof expect_ptr, dst.seglist->len);
	EXPECT_EQ(0, memcmp(expect_ptr, dst.seglist->data, sizeof expect_ptr));
	capn_free(&dst);
	capn_free(&c);
}

TEST(WireParity, OutOfRangeEmptyStructIsClamped) {
	/* A zero-size struct whose offset leaves the segment. C++
	 * clamps that to the end, and a zero-length object there is
	 * in range. A struct that has bytes is still rejected. */
	const uint8_t direct[] = {
		0x00,0,0,0, 0x01,0,0,0,
		0xbc,0xff,0xfe,0xff, 0x00,0,0,0,
	};
	const uint8_t small[] = {
		0x00,0,0,0, 0x01,0,0,0,
		0xdc,0xff,0xff,0xff, 0x00,0,0,0,
	};
	const uint8_t farw[] = {
		0x01,0,0,0, 0x01,0,0,0, 0x01,0,0,0, 0x00,0,0,0,
		0x02,0,0,0, 0x01,0,0,0,
		0x00,0x04,0,0, 0x00,0,0,0,
	};
	const uint8_t nonempty[] = {
		0x00,0,0,0, 0x01,0,0,0,
		0xbc,0xff,0xfe,0xff, 0x01,0,0,0,
	};
	const uint8_t canon[] = { 0xfc,0xff,0xff,0xff, 0x00,0,0,0 };
	struct capn c;

	expect_empty_struct_canon(direct, sizeof direct, canon, sizeof canon);
	expect_empty_struct_canon(small, sizeof small, canon, sizeof canon);
	expect_empty_struct_canon(farw, sizeof farw, canon, sizeof canon);

	ASSERT_EQ(0, load_frame(&c, nonempty, sizeof nonempty));
	EXPECT_NE(0, capn_validate(&c));
	EXPECT_EQ(0, capn_ok(&c));
	capn_free(&c);
}

TEST(WireParity, EmptyFirstSegmentHasNoRoot) {
	const uint8_t frame[] = {
		1,0,0,0, 0,0,0,0, 1,0,0,0, 0,0,0,0,
		0,0,0,0, 0,0,0,0,
	};
	struct capn c;
	ASSERT_EQ(0, capn_init_mem(&c, frame, sizeof frame, 0));
	EXPECT_EQ(2, c.segnum);
	EXPECT_NE(0, capn_validate(&c));
	capn_free(&c);
}

TEST(WireParity, ASegmentWithNoRootPointerIsRejected) {
	/* One segment of zero words. The reader is built. The root
	 * read fails, and the packed form is the two zero words of
	 * the header. A one-word null root stays a message. */
	const uint8_t twoblank[] = {
		1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
	};
	const uint8_t empty[] = { 0,0,0,0, 0,0,0,0 };
	const uint8_t nullroot[] = {
		0,0,0,0, 1,0,0,0,
		0,0,0,0, 0,0,0,0,
	};
	const uint8_t packed_empty[] = { 0x00, 0x00 };
	uint8_t buf[32];
	struct capn c;
	int64_t n;

	ASSERT_EQ(0, capn_init_mem(&c, empty, sizeof empty, 0));
	EXPECT_EQ(1, capn_ok(&c));
	(void) capn_root(&c);
	EXPECT_EQ(0, capn_ok(&c));
	capn_clear_err(&c);
	n = capn_write_mem(&c, buf, sizeof buf, 1);
	ASSERT_EQ((int64_t) sizeof packed_empty, n);
	EXPECT_EQ(0, memcmp(buf, packed_empty, sizeof packed_empty));
	capn_free(&c);

	/* writeMessage of that segment is the 8-byte header, and the
	 * packed form is two zero bytes. The file and descriptor
	 * writers do the same. */
	ASSERT_EQ(0, capn_init_mem(&c, empty, sizeof empty, 0));
	EXPECT_EQ((int64_t) 8, capn_size(&c));
	{
		int fds[2];
		uint8_t got[8];
		ssize_t rd;
		FILE *fp;
		ASSERT_EQ(0, pipe(fds));
		n = capn_write_fd(&c, write, fds[1], 1);
		ASSERT_EQ((int64_t) 2, n);
		ASSERT_EQ(0, close(fds[1]));
		rd = read(fds[0], got, sizeof got);
		EXPECT_EQ((ssize_t) 2, rd);
		EXPECT_EQ(0, memcmp(got, packed_empty, 2));
		close(fds[0]);
		fp = tmpfile();
		ASSERT_NE(nullptr, fp);
		n = capn_write_fp(&c, fp, 0);
		ASSERT_EQ((int64_t) 8, n);
		ASSERT_EQ(0, fseek(fp, 0, SEEK_SET));
		ASSERT_EQ((size_t) 8, fread(got, 1, 8, fp));
		EXPECT_EQ(0, memcmp(got, empty, 8));
		fclose(fp);
	}
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
	ASSERT_EQ(0, capn_init_mem(&c, twoblank, sizeof twoblank, 0));
	EXPECT_EQ(2, c.segnum);
	capn_free(&c);
	ASSERT_EQ(0, capn_init_mem(&c, nullroot, sizeof nullroot, 0));
	EXPECT_EQ(0, capn_validate(&c));
	EXPECT_EQ(1, capn_ok(&c));
	capn_free(&c);
}
