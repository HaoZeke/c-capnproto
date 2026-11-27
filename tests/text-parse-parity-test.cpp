/* Text parsing matches Cap'n Proto C++ 1.4.0 for these inputs.
 * JSON leaves of the same helpers keep the values they had before. */

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

extern "C" {
#include "capn-dyn.h"
#include "capnp_c.h"
}

namespace {

struct Schemas {
	struct capn_dyn_enum kind;
	struct capn_dyn_type i32, u32, cell_type, list_cell, list_i32, list_u32;
	struct capn_dyn_field f64f, f32f, i8f, i64f, u64f, ef, df, lstf;
	struct capn_dyn_field cell_a, xs, y, l32xs;
	struct capn_dyn_field outer_fields[2];
	struct capn_dyn_field tf;
	struct capn_dyn_field box_fields[3];
	struct capn_dyn_field nest_f;
	struct capn_dyn_type text_st, data_st, f64_st;
	struct capn_dyn_struct f64s, f32s, i8s, i64s, u64s, ens, data_values, lists;
	struct capn_dyn_struct cell, outer, l32, texts, box, nest;
};

static struct capn_dyn_type scalar(uint8_t kind)
{
	struct capn_dyn_type t;
	memset(&t, 0, sizeof t);
	t.kind = kind;
	return t;
}

static void set_field(struct capn_dyn_field *f, const char *name,
                      struct capn_dyn_type type, uint32_t offset)
{
	memset(f, 0, sizeof *f);
	f->name = name;
	f->type = type;
	f->offset = offset;
	f->discriminant = CAPN_DYN_NO_DISCRIMINANT;
}

static void set_struct(struct capn_dyn_struct *s, const char *name,
                       uint16_t words, uint16_t ptrs, int nfields,
                       const struct capn_dyn_field *fields)
{
	memset(s, 0, sizeof *s);
	s->name = name;
	s->data_words = words;
	s->pointers = ptrs;
	s->nfields = nfields;
	s->fields = fields;
}

static Schemas &schemas()
{
	static Schemas s;
	static int ready = 0;
	static const char *const names[] = {"alpha", "beta", "gamma"};
	if (ready)
		return s;
	memset(&s, 0, sizeof s);
	s.kind.name = "Kind";
	s.kind.n = 3;
	s.kind.names = names;

	s.i32 = scalar(CAPN_DYN_INT32);
	s.u32 = scalar(CAPN_DYN_UINT32);

	set_field(&s.f64f, "f64", scalar(CAPN_DYN_FLOAT64), 0);
	set_field(&s.f32f, "f32", scalar(CAPN_DYN_FLOAT32), 0);
	set_field(&s.i8f, "i8", scalar(CAPN_DYN_INT8), 0);
	set_field(&s.i64f, "i64", scalar(CAPN_DYN_INT64), 0);
	set_field(&s.u64f, "u64", scalar(CAPN_DYN_UINT64), 0);
	set_field(&s.ef, "e", scalar(CAPN_DYN_ENUM), 0);
	s.ef.type.en = &s.kind;
	set_field(&s.df, "d", scalar(CAPN_DYN_DATA), 0);
	s.list_i32 = scalar(CAPN_DYN_LIST);
	s.list_i32.elem = &s.i32;
	set_field(&s.lstf, "lst", s.list_i32, 0);

	set_struct(&s.f64s, "F64", 1, 0, 1, &s.f64f);
	set_struct(&s.f32s, "F32", 1, 0, 1, &s.f32f);
	set_struct(&s.i8s, "I8", 1, 0, 1, &s.i8f);
	set_struct(&s.i64s, "I64", 1, 0, 1, &s.i64f);
	set_struct(&s.u64s, "U64", 1, 0, 1, &s.u64f);
	set_struct(&s.ens, "EN", 1, 0, 1, &s.ef);
	set_struct(&s.data_values, "DT", 0, 1, 1, &s.df);
	set_struct(&s.lists, "LI", 0, 1, 1, &s.lstf);

	set_field(&s.cell_a, "a", scalar(CAPN_DYN_INT32), 0);
	set_struct(&s.cell, "Cell", 1, 0, 1, &s.cell_a);
	s.cell_type = scalar(CAPN_DYN_STRUCT);
	s.cell_type.st = &s.cell;
	s.list_cell = scalar(CAPN_DYN_LIST);
	s.list_cell.elem = &s.cell_type;
	set_field(&s.outer_fields[0], "xs", s.list_cell, 0);
	set_field(&s.outer_fields[1], "y", s.cell_type, 1);
	set_struct(&s.outer, "Outer", 0, 2, 2, s.outer_fields);

	s.list_u32 = scalar(CAPN_DYN_LIST);
	s.list_u32.elem = &s.u32;
	set_field(&s.l32xs, "xs", s.list_u32, 0);
	set_struct(&s.l32, "L32", 0, 1, 1, &s.l32xs);

	set_field(&s.tf, "t", scalar(CAPN_DYN_TEXT), 0);
	set_struct(&s.texts, "Tx", 0, 1, 1, &s.tf);
	s.text_st = scalar(CAPN_DYN_STRUCT);
	s.text_st.st = &s.texts;
	s.data_st = scalar(CAPN_DYN_STRUCT);
	s.data_st.st = &s.data_values;
	set_field(&s.box_fields[0], "t", s.text_st, 0);
	set_field(&s.box_fields[1], "d", s.data_st, 1);
	set_field(&s.box_fields[2], "i", s.cell_type, 2);
	set_struct(&s.box, "Box", 0, 3, 3, s.box_fields);

	s.f64_st = scalar(CAPN_DYN_STRUCT);
	s.f64_st.st = &s.f64s;
	set_field(&s.nest_f, "f", s.f64_st, 0);
	set_struct(&s.nest, "Nest", 0, 1, 1, &s.nest_f);

	ready = 1;
	return s;
}

struct Session {
	struct capn c;
	Session() { capn_init_malloc(&c); }
	~Session() { capn_free(&c); }

	int text(const struct capn_dyn_struct *s, const char *in, capn_ptr *st,
	         struct capn_dyn_error *err)
	{
		return capn_dyn_from_text(s, in, strlen(in), capn_root(&c).seg, st, err);
	}
	int json(const struct capn_dyn_struct *s, const char *in, capn_ptr *st,
	         struct capn_dyn_error *err)
	{
		return capn_dyn_from_json(s, in, strlen(in), capn_root(&c).seg, st, err);
	}
};

static int64_t ival(capn_ptr st, const struct capn_dyn_struct *s, const char *name)
{
	const struct capn_dyn_field *f = capn_dyn_find_field(s, name);
	return capn_dyn_bits_to_i64(f, capn_dyn_get_bits(st, f));
}

static double fval(capn_ptr st, const struct capn_dyn_struct *s, const char *name)
{
	const struct capn_dyn_field *f = capn_dyn_find_field(s, name);
	return capn_dyn_bits_to_f64(f, capn_dyn_get_bits(st, f));
}

static std::string rendered(const struct capn_dyn_struct *s, capn_ptr st)
{
	char *t = capn_dyn_to_text(s, st);
	std::string out = t ? t : "";
	free(t);
	return out;
}

}  // namespace

TEST(TextParse, SignedExponentAfterLeadingZero) {
	Schemas &s = schemas();
	Session sess;
	capn_ptr st;
	struct capn_dyn_error err;
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 0.1e-5)", &st, &err)) << err.message;
	EXPECT_EQ(strtod("0.1e-5", nullptr), fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 0e-1)", &st, &err)) << err.message;
	EXPECT_EQ(0.0, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 0.5e+2)", &st, &err)) << err.message;
	EXPECT_EQ(50.0, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f32s, "(f32 = 0.1e-5)", &st, &err)) << err.message;
	EXPECT_EQ(static_cast<float>(strtod("0.1e-5", nullptr)),
	          static_cast<float>(fval(st, &s.f32s, "f32")));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 1e-5)", &st, &err)) << err.message;
	EXPECT_EQ(strtod("1e-5", nullptr), fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 10e-1)", &st, &err)) << err.message;
	EXPECT_EQ(1.0, fval(st, &s.f64s, "f64"));
}

TEST(TextParse, RejectSpellingsCppRejects) {
	Schemas &s = schemas();
	const char *rows[] = {
	    "(i8 = +7)",       "(f64 = +1.5)",    "(f64 = +0.1e-5)", "(f64 = +inf)",
	    "(f64 = NaN)",     "(f64 = Infinity)", "(f64 = -Infinity)", "(i64 = 0X10)",
	    "(i8 = 08)",       "(f64 = 08.5)",    "(e = 2)",         "(e = 0)",
	    "(e = \"gamma\")", "(d = [171])",
	};
	const struct capn_dyn_struct *types[] = {
	    &s.i8s,  &s.f64s, &s.f64s, &s.f64s, &s.f64s, &s.f64s, &s.f64s,
	    &s.i64s, &s.i8s,  &s.f64s, &s.ens,  &s.ens,  &s.ens,  &s.data_values,
	};
	for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
		Session sess;
		capn_ptr st;
		struct capn_dyn_error err;
		EXPECT_NE(0, sess.text(types[i], rows[i], &st, &err)) << rows[i];
		EXPECT_NE('\0', err.message[0]) << rows[i];
	}
}

TEST(TextParse, KeptSpellings) {
	Schemas &s = schemas();
	Session sess;
	capn_ptr st;
	struct capn_dyn_error err;
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = nan)", &st, &err)) << err.message;
	EXPECT_TRUE(std::isnan(fval(st, &s.f64s, "f64")));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = inf)", &st, &err)) << err.message;
	EXPECT_TRUE(std::isinf(fval(st, &s.f64s, "f64")) && fval(st, &s.f64s, "f64") > 0);
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = -inf)", &st, &err)) << err.message;
	EXPECT_TRUE(std::isinf(fval(st, &s.f64s, "f64")) && fval(st, &s.f64s, "f64") < 0);
	ASSERT_EQ(0, sess.text(&s.i64s, "(i64 = 0x10)", &st, &err)) << err.message;
	EXPECT_EQ(16, ival(st, &s.i64s, "i64"));
	ASSERT_EQ(0, sess.text(&s.ens, "(e = gamma)", &st, &err)) << err.message;
	EXPECT_EQ(2, ival(st, &s.ens, "e"));
	ASSERT_EQ(0, sess.text(&s.data_values, "(d = 0x\"ab\")", &st, &err)) << err.message;
	capn_list8 bytes;
	bytes.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.data_values, "d"));
	ASSERT_EQ(1, capn_len(bytes));
	EXPECT_EQ(0xab, capn_get8(bytes, 0));
	ASSERT_EQ(0, sess.text(&s.data_values, "(d = \"ab\")", &st, &err)) << err.message;
	bytes.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.data_values, "d"));
	ASSERT_EQ(2, capn_len(bytes));
	EXPECT_EQ('a', capn_get8(bytes, 0));
	EXPECT_EQ('b', capn_get8(bytes, 1));
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = 7)", &st, &err)) << err.message;
	EXPECT_EQ(7, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = -5)", &st, &err)) << err.message;
	EXPECT_EQ(-5, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = -0.0)", &st, &err)) << err.message;
	EXPECT_TRUE(std::signbit(fval(st, &s.f64s, "f64")));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 10.)", &st, &err)) << err.message;
	EXPECT_EQ(10.0, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 00.5)", &st, &err)) << err.message;
	EXPECT_EQ(0.5, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.u64s, "(u64 = 0xffffffffffffffff)", &st, &err)) << err.message;
	EXPECT_EQ(~0ull, capn_dyn_get_bits(st, capn_dyn_find_field(&s.u64s, "u64")));
	ASSERT_EQ(0, sess.text(&s.i64s, "(i64 = -9223372036854775808)", &st, &err)) << err.message;
	EXPECT_EQ(INT64_MIN, ival(st, &s.i64s, "i64"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 1e9999)", &st, &err)) << err.message;
	EXPECT_TRUE(std::isinf(fval(st, &s.f64s, "f64")));
	EXPECT_NE(0, sess.text(&s.i8s, "(i8 = 0x80)", &st, &err));
	EXPECT_NE(0, sess.text(&s.f64s, "(f64 = 0.1e)", &st, &err));
	EXPECT_NE(0, sess.text(&s.f64s, "(f64 = 1e+)", &st, &err));
}

TEST(TextParse, LeadingZeroIsOctal) {
	Schemas &s = schemas();
	Session sess;
	capn_ptr st;
	struct capn_dyn_error err;
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = 010)", &st, &err)) << err.message;
	EXPECT_EQ(8, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.i64s, "(i64 = 010)", &st, &err)) << err.message;
	EXPECT_EQ(8, ival(st, &s.i64s, "i64"));
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = 0)", &st, &err)) << err.message;
	EXPECT_EQ(0, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = 00)", &st, &err)) << err.message;
	EXPECT_EQ(0, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = 01)", &st, &err)) << err.message;
	EXPECT_EQ(1, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.lists, "(lst = [01, 010, 8])", &st, &err)) << err.message;
	capn_list32 lst;
	lst.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.lists, "lst"));
	ASSERT_EQ(3, capn_len(lst));
	EXPECT_EQ(1, capn_get32(lst, 0));
	EXPECT_EQ(8, capn_get32(lst, 1));
	EXPECT_EQ(8, capn_get32(lst, 2));
	ASSERT_EQ(0, sess.text(&s.l32, "(xs = [1, 2, 010])", &st, &err)) << err.message;
	lst.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.l32, "xs"));
	ASSERT_EQ(3, capn_len(lst));
	EXPECT_EQ(1, capn_get32(lst, 0));
	EXPECT_EQ(2, capn_get32(lst, 1));
	EXPECT_EQ(8, capn_get32(lst, 2));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 010)", &st, &err)) << err.message;
	EXPECT_EQ(8.0, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 0x10)", &st, &err)) << err.message;
	EXPECT_EQ(16.0, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.lists, "(lst = [1, 2, 10])", &st, &err)) << err.message;
	lst.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.lists, "lst"));
	EXPECT_EQ(10, capn_get32(lst, 2));
}

TEST(TextParse, StructValueShorthand) {
	Schemas &s = schemas();
	Session sess;
	capn_ptr st;
	struct capn_dyn_error err;
	ASSERT_EQ(0, sess.text(&s.outer, "(xs = [1, 2])", &st, &err)) << err.message;
	EXPECT_EQ("(xs = [(a = 1), (a = 2)])", rendered(&s.outer, st));
	ASSERT_EQ(0, sess.text(&s.outer, "(y = 3)", &st, &err)) << err.message;
	EXPECT_EQ("(y = (a = 3))", rendered(&s.outer, st));
	ASSERT_EQ(0, sess.text(&s.outer, "(xs = [(a = 1)])", &st, &err)) << err.message;
	EXPECT_EQ("(xs = [(a = 1)])", rendered(&s.outer, st));
	ASSERT_EQ(0, sess.text(&s.outer, "(y = (a = 3))", &st, &err)) << err.message;
	EXPECT_EQ("(y = (a = 3))", rendered(&s.outer, st));
	EXPECT_NE(0, sess.text(&s.outer, "3", &st, &err));
}

TEST(TextParse, JsonLeavesStay) {
	Schemas &s = schemas();
	Session sess;
	capn_ptr st;
	struct capn_dyn_error err;
	ASSERT_EQ(0, sess.json(&s.f64s, "{\"f64\": 0.1e-5}", &st, &err)) << err.message;
	EXPECT_EQ(strtod("0.1e-5", nullptr), fval(st, &s.f64s, "f64"));
	EXPECT_NE(0, sess.json(&s.i8s, "{\"i8\": 010}", &st, &err));
	ASSERT_EQ(0, sess.json(&s.i8s, "{\"i8\": \"+7\"}", &st, &err)) << err.message;
	EXPECT_EQ(7, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.json(&s.i64s, "{\"i64\": \"0X10\"}", &st, &err)) << err.message;
	EXPECT_EQ(16, ival(st, &s.i64s, "i64"));
	ASSERT_EQ(0, sess.json(&s.i64s, "{\"i64\": \"010\"}", &st, &err)) << err.message;
	EXPECT_EQ(10, ival(st, &s.i64s, "i64"));
	ASSERT_EQ(0, sess.json(&s.f64s, "{\"f64\": \"NaN\"}", &st, &err)) << err.message;
	EXPECT_TRUE(std::isnan(fval(st, &s.f64s, "f64")));
	ASSERT_EQ(0, sess.json(&s.f64s, "{\"f64\": \"Infinity\"}", &st, &err)) << err.message;
	EXPECT_TRUE(std::isinf(fval(st, &s.f64s, "f64")) && fval(st, &s.f64s, "f64") > 0);
	ASSERT_EQ(0, sess.json(&s.f64s, "{\"f64\": \"-Infinity\"}", &st, &err)) << err.message;
	EXPECT_TRUE(std::isinf(fval(st, &s.f64s, "f64")) && fval(st, &s.f64s, "f64") < 0);
	ASSERT_EQ(0, sess.json(&s.ens, "{\"e\": 2}", &st, &err)) << err.message;
	EXPECT_EQ(2, ival(st, &s.ens, "e"));
	ASSERT_EQ(0, sess.json(&s.ens, "{\"e\": 0}", &st, &err)) << err.message;
	EXPECT_EQ(0, ival(st, &s.ens, "e"));
	ASSERT_EQ(0, sess.json(&s.ens, "{\"e\": \"gamma\"}", &st, &err)) << err.message;
	EXPECT_EQ(2, ival(st, &s.ens, "e"));
	ASSERT_EQ(0, sess.json(&s.data_values, "{\"d\": [171]}", &st, &err)) << err.message;
	capn_list8 bytes;
	bytes.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.data_values, "d"));
	ASSERT_EQ(1, capn_len(bytes));
	EXPECT_EQ(171, capn_get8(bytes, 0));
	ASSERT_EQ(0, sess.json(&s.data_values, "{\"d\": [1, 2]}", &st, &err)) << err.message;
	bytes.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.data_values, "d"));
	ASSERT_EQ(2, capn_len(bytes));
	EXPECT_EQ(1, capn_get8(bytes, 0));
	EXPECT_EQ(2, capn_get8(bytes, 1));
	EXPECT_NE(0, sess.json(&s.data_values, "{\"d\": \"ab\"}", &st, &err));
	EXPECT_NE(0, sess.json(&s.outer, "{\"y\": 3}", &st, &err));
}

TEST(TextParse, BareValueMatchesFirstField) {
	Schemas &s = schemas();
	Session sess;
	capn_ptr st;
	struct capn_dyn_error err;
	int rc;

	rc = sess.text(&s.box, "(t = 0x\"61\")", &st, &err);
	EXPECT_NE(0, rc);
	rc = sess.text(&s.box, "(t = \"a\")", &st, &err);
	EXPECT_EQ(0, rc) << err.message;
	if (rc == 0) {
		EXPECT_EQ("(t = (t = \"a\"))", rendered(&s.box, st));
	}
	rc = sess.text(&s.box, "(d = 0x\"61\")", &st, &err);
	EXPECT_NE(0, rc);
	rc = sess.text(&s.box, "(d = \"a\")", &st, &err);
	EXPECT_NE(0, rc);
	rc = sess.text(&s.box, "(i = \"a\")", &st, &err);
	EXPECT_NE(0, rc);
	rc = sess.text(&s.texts, "(t = 0x\"61\")", &st, &err);
	EXPECT_NE(0, rc);
	rc = sess.text(&s.data_values, "(d = 0x\"61\")", &st, &err);
	EXPECT_EQ(0, rc) << err.message;
	rc = sess.text(&s.data_values, "(d = \"a\")", &st, &err);
	EXPECT_EQ(0, rc) << err.message;
}

TEST(TextParse, BareIntRejectsStringAndBinary) {
	Schemas &s = schemas();
	const char *rows[] = {
	    "(y = \"3\")",
	    "(y = 0x\"31\")",
	    "(xs = [\"3\"])",
	    "(xs = [0x\"31\"])",
	};
	for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
		Session sess;
		capn_ptr st;
		struct capn_dyn_error err;
		EXPECT_NE(0, sess.text(&s.outer, rows[i], &st, &err)) << rows[i];
		EXPECT_NE('\0', err.message[0]) << rows[i];
	}
	Session sess;
	capn_ptr st;
	struct capn_dyn_error err;
	capn_ptr inner;
	ASSERT_EQ(0, sess.text(&s.outer, "(y = 010)", &st, &err)) << err.message;
	EXPECT_EQ("(y = (a = 8))", rendered(&s.outer, st));
	ASSERT_EQ(0, sess.text(&s.nest, "(f = nan)", &st, &err)) << err.message;
	inner = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.nest, "f"));
	EXPECT_TRUE(std::isnan(fval(inner, &s.f64s, "f64")));
	ASSERT_EQ(0, sess.text(&s.nest, "(f = inf)", &st, &err)) << err.message;
	inner = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.nest, "f"));
	EXPECT_TRUE(std::isinf(fval(inner, &s.f64s, "f64")) && fval(inner, &s.f64s, "f64") > 0);
	ASSERT_EQ(0, sess.text(&s.nest, "(f = -inf)", &st, &err)) << err.message;
	inner = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.nest, "f"));
	EXPECT_TRUE(std::isinf(fval(inner, &s.f64s, "f64")) && fval(inner, &s.f64s, "f64") < 0);
	ASSERT_EQ(0, sess.text(&s.nest, "(f = 010)", &st, &err)) << err.message;
	inner = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.nest, "f"));
	EXPECT_EQ(8.0, fval(inner, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.nest, "(f = 1.5)", &st, &err)) << err.message;
	inner = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.nest, "f"));
	EXPECT_EQ(1.5, fval(inner, &s.f64s, "f64"));
	EXPECT_NE(0, sess.text(&s.nest, "(f = \"nan\")", &st, &err));
	EXPECT_NE(0, sess.text(&s.nest, "(f = 0x\"31\")", &st, &err));
}

TEST(TextParse, NamedNumberRejectsStringAndBinary) {
	Schemas &s = schemas();
	const char *rows[] = {
	    "(i8 = \"5\")",
	    "(i8 = \"010\")",
	    "(i8 = \"0x10\")",
	    "(i8 = \"+7\")",
	    "(i8 = \"-5\")",
	    "(i8 = 0x\"35\")",
	    "(i64 = \"5\")",
	    "(i64 = \"-1\")",
	    "(u64 = \"1\")",
	    "(u64 = 0x\"31\")",
	    "(f64 = \"5\")",
	    "(f64 = \"1.5\")",
	    "(f64 = \"nan\")",
	    "(f64 = \"inf\")",
	    "(f64 = \"-inf\")",
	    "(f64 = \"NaN\")",
	    "(f64 = \"Infinity\")",
	    "(f64 = \"-Infinity\")",
	    "(f64 = 0x\"00\")",
	    "(f64 = 0x\"6e616e\")",
	    "(f32 = \"nan\")",
	    "(f32 = 0x\"00\")",
	    "(f32 = 0x\"6e616e\")",
	    "(y = (a = \"5\"))",
	    "(y = (a = 0x\"31\"))",
	    "(xs = [(a = \"5\")])",
	    "(xs = [(a = 0x\"31\")])",
	    "(lst = [\"8\"])",
	    "(lst = [0x\"38\"])",
	    "(lst = [1, \"2\"])",
	    "(f = (f64 = \"nan\"))",
	    "(f = (f64 = \"1.5\"))",
	    "(f = (f64 = 0x\"00\"))",
	    "(f = (f64 = 0x\"6e616e\"))",
	};
	const struct capn_dyn_struct *types[] = {
	    &s.i8s,  &s.i8s,  &s.i8s,  &s.i8s,  &s.i8s,  &s.i8s,  &s.i64s, &s.i64s,
	    &s.u64s, &s.u64s, &s.f64s, &s.f64s, &s.f64s, &s.f64s, &s.f64s, &s.f64s,
	    &s.f64s, &s.f64s, &s.f64s, &s.f64s, &s.f32s, &s.f32s, &s.f32s, &s.outer,
	    &s.outer, &s.outer, &s.outer, &s.lists, &s.lists, &s.lists, &s.nest,
	    &s.nest, &s.nest, &s.nest,
	};
	static_assert(sizeof rows / sizeof rows[0] == sizeof types / sizeof types[0],
	              "row and type counts differ");
	for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
		Session sess;
		capn_ptr st;
		struct capn_dyn_error err;
		EXPECT_NE(0, sess.text(types[i], rows[i], &st, &err)) << rows[i];
		EXPECT_NE('\0', err.message[0]) << rows[i];
	}

	Session sess;
	capn_ptr st;
	struct capn_dyn_error err;
	capn_ptr inner;
	capn_list32 lst;
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = 5)", &st, &err)) << err.message;
	EXPECT_EQ(5, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = 010)", &st, &err)) << err.message;
	EXPECT_EQ(8, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = 0x10)", &st, &err)) << err.message;
	EXPECT_EQ(16, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.i8s, "(i8 = -5)", &st, &err)) << err.message;
	EXPECT_EQ(-5, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.text(&s.u64s, "(u64 = 0xffffffffffffffff)", &st, &err)) << err.message;
	EXPECT_EQ(~0ull, capn_dyn_get_bits(st, capn_dyn_find_field(&s.u64s, "u64")));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = nan)", &st, &err)) << err.message;
	EXPECT_TRUE(std::isnan(fval(st, &s.f64s, "f64")));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = inf)", &st, &err)) << err.message;
	EXPECT_TRUE(std::isinf(fval(st, &s.f64s, "f64")) && fval(st, &s.f64s, "f64") > 0);
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = -inf)", &st, &err)) << err.message;
	EXPECT_TRUE(std::isinf(fval(st, &s.f64s, "f64")) && fval(st, &s.f64s, "f64") < 0);
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 1.5)", &st, &err)) << err.message;
	EXPECT_EQ(1.5, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 010)", &st, &err)) << err.message;
	EXPECT_EQ(8.0, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f64s, "(f64 = 0x10)", &st, &err)) << err.message;
	EXPECT_EQ(16.0, fval(st, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.f32s, "(f32 = nan)", &st, &err)) << err.message;
	EXPECT_TRUE(std::isnan(fval(st, &s.f32s, "f32")));
	ASSERT_EQ(0, sess.text(&s.outer, "(y = (a = 010))", &st, &err)) << err.message;
	EXPECT_EQ("(y = (a = 8))", rendered(&s.outer, st));
	ASSERT_EQ(0, sess.text(&s.nest, "(f = (f64 = nan))", &st, &err)) << err.message;
	inner = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.nest, "f"));
	EXPECT_TRUE(std::isnan(fval(inner, &s.f64s, "f64")));
	ASSERT_EQ(0, sess.text(&s.nest, "(f = (f64 = 1.5))", &st, &err)) << err.message;
	inner = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.nest, "f"));
	EXPECT_EQ(1.5, fval(inner, &s.f64s, "f64"));
	ASSERT_EQ(0, sess.text(&s.lists, "(lst = [010])", &st, &err)) << err.message;
	lst.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.lists, "lst"));
	ASSERT_EQ(1, capn_len(lst));
	EXPECT_EQ(8, capn_get32(lst, 0));
	ASSERT_EQ(0, sess.text(&s.lists, "(lst = [1, 2, 0x10])", &st, &err)) << err.message;
	lst.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.lists, "lst"));
	ASSERT_EQ(3, capn_len(lst));
	EXPECT_EQ(1, capn_get32(lst, 0));
	EXPECT_EQ(2, capn_get32(lst, 1));
	EXPECT_EQ(16, capn_get32(lst, 2));

	ASSERT_EQ(0, sess.json(&s.i8s, "{\"i8\": \"5\"}", &st, &err)) << err.message;
	EXPECT_EQ(5, ival(st, &s.i8s, "i8"));
	ASSERT_EQ(0, sess.json(&s.i64s, "{\"i64\": \"0x10\"}", &st, &err)) << err.message;
	EXPECT_EQ(16, ival(st, &s.i64s, "i64"));
	ASSERT_EQ(0, sess.json(&s.f64s, "{\"f64\": \"NaN\"}", &st, &err)) << err.message;
	EXPECT_TRUE(std::isnan(fval(st, &s.f64s, "f64")));
	ASSERT_EQ(0, sess.json(&s.f64s, "{\"f64\": \"Infinity\"}", &st, &err)) << err.message;
	EXPECT_TRUE(std::isinf(fval(st, &s.f64s, "f64")) && fval(st, &s.f64s, "f64") > 0);
	ASSERT_EQ(0, sess.json(&s.lists, "{\"lst\": [\"010\"]}", &st, &err)) << err.message;
	lst.p = capn_dyn_get_ptr(st, capn_dyn_find_field(&s.lists, "lst"));
	ASSERT_EQ(1, capn_len(lst));
	EXPECT_EQ(10, capn_get32(lst, 0));
}
