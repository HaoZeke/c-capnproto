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
	struct capn_dyn_struct f64s, f32s, i8s, i64s, u64s, ens, datas, lists;
	struct capn_dyn_struct cell, outer, l32;
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
	set_struct(&s.datas, "DT", 0, 1, 1, &s.df);
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

