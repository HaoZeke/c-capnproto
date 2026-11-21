/* read-defaults-test.cpp
 *
 * read_ substitutes a list or struct default when the pointer is
 * null, and keeps a pointer that is present.
 */

#include <gtest/gtest.h>

#include <cstring>

#include "capnp_c.h"
#include "read-defaults.capnp.h"

TEST(ReadDefaults, NullPointersTakeTheSchemaDefault) {
	struct capn c;
	struct WithDefaults s;
	struct Inner in;
	capn_init_malloc(&c);
	WithDefaults_ptr p = new_WithDefaults(capn_root(&c).seg);
	memset(&s, 0, sizeof s);
	read_WithDefaults(&s, p);

	EXPECT_EQ(3, s.l.p.len);
	EXPECT_EQ(10u, capn_get32(s.l, 0));
	EXPECT_EQ(20u, capn_get32(s.l, 1));
	EXPECT_EQ(30u, capn_get32(s.l, 2));

	memset(&in, 0, sizeof in);
	read_Inner(&in, s.st);
	EXPECT_EQ(99u, in.x);
	ASSERT_NE(nullptr, in.s.str);
	EXPECT_STREQ("custom", in.s.str);
	capn_free(&c);
}

TEST(ReadDefaults, APresentPointerIsNotReplaced) {
	struct capn c;
	struct WithDefaults w, got;
	struct Inner in, gotin;
	capn_init_malloc(&c);
	struct capn_segment *seg = capn_root(&c).seg;
	WithDefaults_ptr p = new_WithDefaults(seg);

	memset(&w, 0, sizeof w);
	w.l = capn_new_list32(seg, 1);
	capn_set32(w.l, 0, 7);
	memset(&in, 0, sizeof in);
	in.x = 1;
	in.s.str = "z";
	in.s.len = 1;
	in.s.seg = NULL;
	Inner_ptr ip = new_Inner(seg);
	write_Inner(&in, ip);
	w.st = ip;
	write_WithDefaults(&w, p);

	memset(&got, 0, sizeof got);
	read_WithDefaults(&got, p);
	capn_resolve(&got.l.p);
	EXPECT_EQ(1, got.l.p.len);
	EXPECT_EQ(7u, capn_get32(got.l, 0));
	memset(&gotin, 0, sizeof gotin);
	capn_resolve(&got.st.p);
	read_Inner(&gotin, got.st);
	EXPECT_EQ(1u, gotin.x);
	ASSERT_NE(nullptr, gotin.s.str);
	EXPECT_STREQ("z", gotin.s.str);
	capn_free(&c);
}

TEST(ReadDefaults, AnUnreadPointerDoesNotFailTheStruct) {
	const uint8_t frame[] = {
		0,0,0,0, 3,0,0,0,
		0,0,0,0, 1,0,1,0,
		7,0,0,0, 0,0,0,0,
		0,0,0,0, 1,0,0,0,
	};
	struct capn c;
	struct Outer o;
	Outer_ptr p;

	ASSERT_EQ(0, capn_init_mem(&c, frame, sizeof frame, 0));
	p.p = capn_getp(capn_root(&c), 0, 1);
	memset(&o, 0, sizeof o);
	read_Outer(&o, p);
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_EQ(7u, o.x);
	capn_resolve(&o.inner.p);
	EXPECT_EQ(0, capn_ok(&c));
	capn_free(&c);
}
