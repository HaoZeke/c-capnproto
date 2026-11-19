/* capn-dyn.c - reflection, text and JSON over generated descriptors.
 * See capn-dyn.h. */

#include "capn-dyn.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- reflection ---------------------------------------------------- */

const struct capn_dyn_field *capn_dyn_find_field(const struct capn_dyn_struct *s,
                                                 const char *name)
{
	int i;
	for (i = 0; i < s->nfields; i++)
		if (strcmp(s->fields[i].name, name) == 0)
			return &s->fields[i];
	return NULL;
}

int capn_dyn_which(const struct capn_dyn_struct *s, capn_ptr st)
{
	if (s->union_count == 0)
		return -1;
	capn_resolve(&st);
	return capn_read16(st, 2 * (int) s->union_offset);
}

int capn_dyn_is_active(const struct capn_dyn_struct *s, capn_ptr st,
                       const struct capn_dyn_field *f)
{
	if (f->discriminant == CAPN_DYN_NO_DISCRIMINANT)
		return 1;
	return capn_dyn_which(s, st) == (int) f->discriminant;
}

static int scalar_width(int kind)
{
	switch (kind) {
	case CAPN_DYN_BOOL: return 1;
	case CAPN_DYN_INT8: case CAPN_DYN_UINT8: return 8;
	case CAPN_DYN_INT16: case CAPN_DYN_UINT16: case CAPN_DYN_ENUM: return 16;
	case CAPN_DYN_INT32: case CAPN_DYN_UINT32: case CAPN_DYN_FLOAT32: return 32;
	case CAPN_DYN_INT64: case CAPN_DYN_UINT64: case CAPN_DYN_FLOAT64: return 64;
	default: return 0;
	}
}

static uint64_t read_scalar(capn_ptr st, int kind, uint32_t offset)
{
	switch (scalar_width(kind)) {
	case 1: return (capn_read8(st, (int) (offset / 8)) >> (offset % 8)) & 1u;
	case 8: return capn_read8(st, (int) offset);
	case 16: return capn_read16(st, (int) offset * 2);
	case 32: return capn_read32(st, (int) offset * 4);
	case 64: return capn_read64(st, (int) offset * 8);
	default: return 0;
	}
}

uint64_t capn_dyn_get_bits(capn_ptr st, const struct capn_dyn_field *f)
{
	capn_resolve(&st);
	return read_scalar(st, f->type.kind, f->offset) ^ f->default_bits;
}

int capn_dyn_set_bits(capn_ptr st, const struct capn_dyn_field *f, uint64_t bits)
{
	uint64_t raw = bits ^ f->default_bits;
	capn_resolve(&st);
	switch (scalar_width(f->type.kind)) {
	case 1: return capn_write1(st, (int) f->offset, (int) (raw & 1));
	case 8: return capn_write8(st, (int) f->offset, (uint8_t) raw);
	case 16: return capn_write16(st, (int) f->offset * 2, (uint16_t) raw);
	case 32: return capn_write32(st, (int) f->offset * 4, (uint32_t) raw);
	case 64: return capn_write64(st, (int) f->offset * 8, raw);
	default: return 0; /* Void has no storage */
	}
}

int capn_dyn_set_which(const struct capn_dyn_struct *s, capn_ptr st,
                       const struct capn_dyn_field *f)
{
	if (f->discriminant == CAPN_DYN_NO_DISCRIMINANT || s->union_count == 0)
		return -1;
	capn_resolve(&st);
	return capn_write16(st, 2 * (int) s->union_offset, (uint16_t) f->discriminant);
}

capn_ptr capn_dyn_get_ptr(capn_ptr st, const struct capn_dyn_field *f)
{
	capn_resolve(&st);
	return capn_getp(st, (int) f->offset, 1);
}

capn_text capn_dyn_get_text(capn_ptr st, const struct capn_dyn_field *f)
{
	capn_text def;
	def.str = f->default_text ? f->default_text : "";
	def.len = f->default_text ? f->default_text_len : 0;
	def.seg = NULL;
	return capn_get_text(st, (int) f->offset, def);
}

int capn_dyn_has(const struct capn_dyn_struct *s, capn_ptr st,
                 const struct capn_dyn_field *f)
{
	if (!capn_dyn_is_active(s, st, f))
		return 0;
	switch (f->type.kind) {
	case CAPN_DYN_TEXT:
	case CAPN_DYN_DATA:
	case CAPN_DYN_LIST:
	case CAPN_DYN_INTERFACE:
	case CAPN_DYN_ANYPOINTER:
	case CAPN_DYN_PARAM:
		return capn_dyn_get_ptr(st, f).type != CAPN_NULL;
	case CAPN_DYN_STRUCT:
		if (f->is_group)
			return 1;
		return capn_dyn_get_ptr(st, f).type != CAPN_NULL;
	default:
		return 1;
	}
}

double capn_dyn_bits_to_f64(const struct capn_dyn_field *f, uint64_t bits)
{
	if (f->type.kind == CAPN_DYN_FLOAT32)
		return capn_to_f32((uint32_t) bits);
	return capn_to_f64(bits);
}

int64_t capn_dyn_bits_to_i64(const struct capn_dyn_field *f, uint64_t bits)
{
	switch (f->type.kind) {
	case CAPN_DYN_INT8: return (int8_t) bits;
	case CAPN_DYN_INT16: return (int16_t) bits;
	case CAPN_DYN_INT32: return (int32_t) bits;
	default: return (int64_t) bits;
	}
}

/* --- output buffer --------------------------------------------------- */

static char *empty_string(void)
{
	char *p = (char *) malloc(1);
	if (p)
		*p = '\0';
	return p;
}

struct sb {
	char *buf;
	size_t len, cap;
	int err;
};

static void sb_reserve(struct sb *b, size_t n)
{
	if (b->err)
		return;
	if (b->len + n + 1 > b->cap) {
		size_t cap = b->cap ? b->cap : 256;
		char *p;
		while (b->len + n + 1 > cap)
			cap *= 2;
		p = (char *) realloc(b->buf, cap);
		if (p == NULL) {
			b->err = 1;
			return;
		}
		b->buf = p;
		b->cap = cap;
	}
}

static void sb_add(struct sb *b, const char *s, size_t n)
{
	sb_reserve(b, n);
	if (b->err)
		return;
	memcpy(b->buf + b->len, s, n);
	b->len += n;
	b->buf[b->len] = '\0';
}

static void sb_str(struct sb *b, const char *s)
{
	sb_add(b, s, strlen(s));
}

static void sb_fmt(struct sb *b, const char *fmt, ...)
{
	char tmp[64];
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(tmp, sizeof tmp, fmt, ap);
	va_end(ap);
	if (n > 0 && (size_t) n < sizeof tmp)
		sb_add(b, tmp, (size_t) n);
}

/* %g as KJ writes it: the exponent keeps its zero padding but loses '+',
 * and the shortest of 15 and 17 significant digits that round-trips. */
static void sb_double(struct sb *b, double v)
{
	char tmp[64], out[64];
	char *e, *w;

	if (isnan(v)) {
		sb_str(b, "nan");
		return;
	}
	if (isinf(v)) {
		sb_str(b, v < 0 ? "-inf" : "inf");
		return;
	}
	snprintf(tmp, sizeof tmp, "%.15g", v);
	if (strtod(tmp, NULL) != v)
		snprintf(tmp, sizeof tmp, "%.17g", v);
	e = strchr(tmp, 'e');
	w = out;
	for (char *r = tmp; *r; r++) {
		if (e != NULL && r == e + 1 && *r == '+')
			continue;
		*w++ = *r;
	}
	*w = '\0';
	sb_str(b, out);
}

static void sb_float(struct sb *b, float v)
{
	char tmp[64], out[64];
	char *e, *w;

	if (isnan(v)) {
		sb_str(b, "nan");
		return;
	}
	if (isinf(v)) {
		sb_str(b, v < 0 ? "-inf" : "inf");
		return;
	}
	/* Six digits when they read back as the same float, else eight;
	 * denormals always take eight, which is what the reference prints. */
	snprintf(tmp, sizeof tmp, "%.6g", (double) v);
	if (strtof(tmp, NULL) != v || (v != 0 && fabsf(v) < 1.17549435e-38f))
		snprintf(tmp, sizeof tmp, "%.8g", (double) v);
	e = strchr(tmp, 'e');
	w = out;
	for (char *r = tmp; *r; r++) {
		if (e != NULL && r == e + 1 && *r == '+')
			continue;
		*w++ = *r;
	}
	*w = '\0';
	sb_str(b, out);
}

/* --- generic parameters ----------------------------------------------- */

/* The bindings in force at some point of a walk down a message. */
struct frame {
	const struct capn_dyn_scope *scopes;
	int n;
	const struct frame *parent;
};

static const struct capn_dyn_type any_type = {CAPN_DYN_ANYPOINTER, NULL, NULL, NULL, NULL, 0, 0};

/* Replace a parameter by what the frames bind it to, until a concrete
 * type remains. A parameter nothing binds is AnyPointer. `*frp` becomes
 * the frame the returned type is to be read in. */
static const struct capn_dyn_type *resolve(const struct capn_dyn_type *t,
                                           const struct frame **frp)
{
	const struct frame *fr = *frp;
	int guard = 0;

	while (t->kind == CAPN_DYN_PARAM) {
		const struct capn_dyn_type *bound = NULL;
		const struct frame *at = NULL, *f;
		int k;

		if (++guard > 64)
			return &any_type;
		for (f = fr; f != NULL && bound == NULL; f = f->parent) {
			for (k = 0; k < f->n; k++) {
				const struct capn_dyn_scope *sc = &f->scopes[k];
				if (sc->scope_id != t->scope_id || sc->inherit)
					continue;
				if (t->index < (uint32_t) sc->nparams) {
					bound = &sc->params[t->index];
					at = f->parent;
				}
				break;
			}
		}
		if (bound == NULL)
			return &any_type;
		t = bound;
		fr = at;
	}
	*frp = fr;
	return t;
}

static void enter_struct(struct frame *nf, const struct capn_dyn_type *t,
                         const struct frame *fr)
{
	nf->scopes = t->brand ? t->brand->scopes : NULL;
	nf->n = t->brand ? t->brand->nscopes : 0;
	nf->parent = fr;
}

/* --- values read from a message ---------------------------------------- */

/* One value as read: scalars as raw bits, pointer kinds as the resolved
 * pointer (CAPN_NULL when unset). */
struct dv {
	uint64_t bits;
	capn_ptr p;
	capn_text text;
	capn_data data;
};

static int is_pointer_kind(int kind)
{
	switch (kind) {
	case CAPN_DYN_TEXT: case CAPN_DYN_DATA: case CAPN_DYN_LIST:
	case CAPN_DYN_STRUCT: case CAPN_DYN_INTERFACE: case CAPN_DYN_ANYPOINTER:
	case CAPN_DYN_PARAM:
		return 1;
	default:
		return 0;
	}
}

static void get_field_value(capn_ptr st, const struct capn_dyn_field *f,
                            const struct capn_dyn_type *rt, struct dv *v)
{
	memset(v, 0, sizeof *v);
	switch (rt->kind) {
	case CAPN_DYN_VOID:
		break;
	case CAPN_DYN_TEXT:
		v->text = capn_dyn_get_text(st, f);
		v->p = capn_dyn_get_ptr(st, f);
		break;
	case CAPN_DYN_DATA:
		v->p = capn_dyn_get_ptr(st, f);
		v->data = capn_get_data(st, (int) f->offset);
		break;
	case CAPN_DYN_STRUCT:
		if (f->is_group)
			v->p = st;
		else
			v->p = capn_dyn_get_ptr(st, f);
		break;
	default:
		if (is_pointer_kind(rt->kind))
			v->p = capn_dyn_get_ptr(st, f);
		else
			v->bits = capn_dyn_get_bits(st, f);
		break;
	}
}

static void get_elem_value(capn_ptr list, const struct capn_dyn_type *t, int i, struct dv *v)
{
	capn_list8 l8;
	capn_list16 l16;
	capn_list32 l32;
	capn_list64 l64;
	capn_list1 l1;

	memset(v, 0, sizeof *v);
	switch (t->kind) {
	case CAPN_DYN_VOID:
		break;
	case CAPN_DYN_BOOL:
		l1.p = list;
		v->bits = capn_get1(l1, i) ? 1 : 0;
		break;
	case CAPN_DYN_INT8:
	case CAPN_DYN_UINT8:
		l8.p = list;
		v->bits = capn_get8(l8, i);
		break;
	case CAPN_DYN_INT16:
	case CAPN_DYN_UINT16:
	case CAPN_DYN_ENUM:
		l16.p = list;
		v->bits = capn_get16(l16, i);
		break;
	case CAPN_DYN_INT32:
	case CAPN_DYN_UINT32:
	case CAPN_DYN_FLOAT32:
		l32.p = list;
		v->bits = capn_get32(l32, i);
		break;
	case CAPN_DYN_INT64:
	case CAPN_DYN_UINT64:
	case CAPN_DYN_FLOAT64:
		l64.p = list;
		v->bits = capn_get64(l64, i);
		break;
	case CAPN_DYN_TEXT: {
		capn_text def = {0, "", NULL};
		v->p = capn_getp(list, i, 1);
		v->text = capn_get_text(list, i, def);
		break;
	}
	case CAPN_DYN_DATA:
		v->p = capn_getp(list, i, 1);
		v->data = capn_get_data(list, i);
		break;
	case CAPN_DYN_STRUCT:
		v->p = capn_getp(list, i, 0);
		break;
	default:
		v->p = capn_getp(list, i, 1);
		break;
	}
}

static int is_signed(int kind)
{
	return kind == CAPN_DYN_INT8 || kind == CAPN_DYN_INT16 ||
	       kind == CAPN_DYN_INT32 || kind == CAPN_DYN_INT64;
}

static int64_t sign_extend(int kind, uint64_t bits)
{
	switch (kind) {
	case CAPN_DYN_INT8: return (int8_t) bits;
	case CAPN_DYN_INT16: return (int16_t) bits;
	case CAPN_DYN_INT32: return (int32_t) bits;
	default: return (int64_t) bits;
	}
}

static void sb_int(struct sb *b, int kind, uint64_t bits)
{
	char tmp[32];
	if (is_signed(kind))
		snprintf(tmp, sizeof tmp, "%lld", (long long) sign_extend(kind, bits));
	else
		snprintf(tmp, sizeof tmp, "%llu", (unsigned long long) bits);
	sb_str(b, tmp);
}

/* --- text format ------------------------------------------------------- */

/* kj::encodeCEscape: Text keeps bytes >= 0x80 (UTF-8), Data escapes them. */
static void text_escape(struct sb *b, const char *p, int len, int binary)
{
	int i;
	sb_str(b, "\"");
	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char) p[i];
		switch (c) {
		case '\a': sb_str(b, "\\a"); break;
		case '\b': sb_str(b, "\\b"); break;
		case '\f': sb_str(b, "\\f"); break;
		case '\n': sb_str(b, "\\n"); break;
		case '\r': sb_str(b, "\\r"); break;
		case '\t': sb_str(b, "\\t"); break;
		case '\v': sb_str(b, "\\v"); break;
		case '\'': sb_str(b, "\\'"); break;
		case '"': sb_str(b, "\\\""); break;
		case '\\': sb_str(b, "\\\\"); break;
		default:
			if (c < 0x20 || c == 0x7f || (binary && c >= 0x80))
				sb_fmt(b, "\\%03o", c);
			else
				sb_add(b, (const char *) &c, 1);
		}
	}
	sb_str(b, "\"");
}

static void text_struct(struct sb *b, const struct capn_dyn_struct *s, capn_ptr st,
                        const struct frame *fr);

static void text_enum(struct sb *b, const struct capn_dyn_enum *e, uint64_t v)
{
	if (e != NULL && v < (uint64_t) e->n && e->names[v] != NULL)
		sb_str(b, e->names[v]);
	else
		sb_int(b, CAPN_DYN_UINT16, v);
}

static void text_value(struct sb *b, const struct capn_dyn_type *t,
                       const struct frame *fr, struct dv *v);

static void text_list(struct sb *b, const struct capn_dyn_type *t,
                      const struct frame *fr, capn_ptr list)
{
	int i, n;
	struct dv ev;
	const struct frame *efr = fr;
	const struct capn_dyn_type *et = resolve(t->elem, &efr);

	capn_resolve(&list);
	n = capn_ptr_len(list);
	sb_str(b, "[");
	for (i = 0; i < n; i++) {
		if (i)
			sb_str(b, ", ");
		get_elem_value(list, et, i, &ev);
		text_value(b, et, efr, &ev);
	}
	sb_str(b, "]");
}

static void text_value(struct sb *b, const struct capn_dyn_type *t,
                       const struct frame *fr, struct dv *v)
{
	switch (t->kind) {
	case CAPN_DYN_VOID:
		sb_str(b, "void");
		break;
	case CAPN_DYN_BOOL:
		sb_str(b, v->bits ? "true" : "false");
		break;
	case CAPN_DYN_INT8: case CAPN_DYN_INT16: case CAPN_DYN_INT32: case CAPN_DYN_INT64:
	case CAPN_DYN_UINT8: case CAPN_DYN_UINT16: case CAPN_DYN_UINT32: case CAPN_DYN_UINT64:
		sb_int(b, t->kind, v->bits);
		break;
	case CAPN_DYN_FLOAT32:
		sb_float(b, capn_to_f32((uint32_t) v->bits));
		break;
	case CAPN_DYN_FLOAT64:
		sb_double(b, capn_to_f64(v->bits));
		break;
	case CAPN_DYN_ENUM:
		text_enum(b, t->en, v->bits);
		break;
	case CAPN_DYN_TEXT:
		text_escape(b, v->text.str ? v->text.str : "", v->text.len, 0);
		break;
	case CAPN_DYN_DATA: {
		capn_list8 l;
		int i, n = capn_len(v->data);
		char *tmp = (char *) malloc(n > 0 ? (size_t) n : 1);
		l.p = v->data.p;
		for (i = 0; tmp && i < n; i++)
			tmp[i] = (char) capn_get8(l, i);
		if (tmp)
			text_escape(b, tmp, n, 1);
		free(tmp);
		break;
	}
	case CAPN_DYN_LIST:
		text_list(b, t, fr, v->p);
		break;
	case CAPN_DYN_STRUCT:
		if (t->st != NULL) {
			struct frame nf;
			enter_struct(&nf, t, fr);
			text_struct(b, t->st, v->p, &nf);
		} else {
			sb_str(b, "<opaque pointer>");
		}
		break;
	case CAPN_DYN_INTERFACE:
		sb_str(b, "<external capability>");
		break;
	default:
		sb_str(b, "<opaque pointer>");
		break;
	}
}

/* Whether a field has a value, reading generic parameters through `fr`. */
static int field_has(const struct capn_dyn_struct *s, capn_ptr st,
                     const struct capn_dyn_field *f, const struct capn_dyn_type *rt)
{
	if (!capn_dyn_is_active(s, st, f))
		return 0;
	if (rt->kind == CAPN_DYN_STRUCT && f->is_group)
		return 1;
	if (is_pointer_kind(rt->kind))
		return capn_dyn_get_ptr(st, f).type != CAPN_NULL;
	return 1;
}

static void text_struct(struct sb *b, const struct capn_dyn_struct *s, capn_ptr st,
                        const struct frame *fr)
{
	int i, first = 1;
	struct dv v;

	capn_resolve(&st);
	sb_str(b, "(");
	for (i = 0; i < s->nfields; i++) {
		const struct capn_dyn_field *f = &s->fields[i];
		const struct frame *ffr = fr;
		const struct capn_dyn_type *rt = resolve(&f->type, &ffr);
		if (!field_has(s, st, f, rt))
			continue;
		if (!first)
			sb_str(b, ", ");
		first = 0;
		sb_str(b, f->name);
		sb_str(b, " = ");
		get_field_value(st, f, rt, &v);
		/* A group is read in the struct's own frame, not a new one. */
		if (f->is_group)
			text_struct(b, rt->st, v.p, fr);
		else
			text_value(b, rt, ffr, &v);
	}
	sb_str(b, ")");
}

char *capn_dyn_to_text(const struct capn_dyn_struct *s, capn_ptr st)
{
	struct sb b;
	memset(&b, 0, sizeof b);
	text_struct(&b, s, st, NULL);
	if (b.err) {
		free(b.buf);
		return NULL;
	}
	return b.buf ? b.buf : empty_string();
}

/* --- JSON output ------------------------------------------------------- */

struct jctx {
	struct sb *b;
	const char *colon;
	const char *comma;
};

static void json_escape(struct sb *b, const char *p, int len)
{
	int i;
	sb_str(b, "\"");
	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char) p[i];
		switch (c) {
		case '"': sb_str(b, "\\\""); break;
		case '\\': sb_str(b, "\\\\"); break;
		case '\b': sb_str(b, "\\b"); break;
		case '\f': sb_str(b, "\\f"); break;
		case '\n': sb_str(b, "\\n"); break;
		case '\r': sb_str(b, "\\r"); break;
		case '\t': sb_str(b, "\\t"); break;
		default:
			if (c < 0x20)
				sb_fmt(b, "\\u%04x", c);
			else
				sb_add(b, (const char *) &c, 1);
		}
	}
	sb_str(b, "\"");
}

static void json_struct(struct jctx *j, const struct capn_dyn_struct *s, capn_ptr st,
                        const struct frame *fr);
static void json_value(struct jctx *j, const struct capn_dyn_type *t,
                       const struct frame *fr, struct dv *v);

static void json_float(struct sb *b, double d)
{
	if (isnan(d)) {
		sb_str(b, "\"NaN\"");
	} else if (isinf(d)) {
		sb_str(b, d < 0 ? "\"-Infinity\"" : "\"Infinity\"");
	} else {
		sb_double(b, d);
	}
}

static void json_value(struct jctx *j, const struct capn_dyn_type *t,
                       const struct frame *fr, struct dv *v)
{
	struct sb *b = j->b;

	switch (t->kind) {
	case CAPN_DYN_VOID:
		sb_str(b, "null");
		break;
	case CAPN_DYN_BOOL:
		sb_str(b, v->bits ? "true" : "false");
		break;
	case CAPN_DYN_INT8: case CAPN_DYN_INT16: case CAPN_DYN_INT32:
	case CAPN_DYN_UINT8: case CAPN_DYN_UINT16: case CAPN_DYN_UINT32:
		sb_int(b, t->kind, v->bits);
		break;
	case CAPN_DYN_INT64:
	case CAPN_DYN_UINT64:
		sb_str(b, "\"");
		sb_int(b, t->kind, v->bits);
		sb_str(b, "\"");
		break;
	case CAPN_DYN_FLOAT32:
		json_float(b, (double) capn_to_f32((uint32_t) v->bits));
		break;
	case CAPN_DYN_FLOAT64:
		json_float(b, capn_to_f64(v->bits));
		break;
	case CAPN_DYN_ENUM:
		if (t->en != NULL && v->bits < (uint64_t) t->en->n && t->en->names[v->bits] != NULL)
			json_escape(b, t->en->names[v->bits], (int) strlen(t->en->names[v->bits]));
		else
			sb_int(b, CAPN_DYN_UINT16, v->bits);
		break;
	case CAPN_DYN_TEXT:
		json_escape(b, v->text.str ? v->text.str : "", v->text.len);
		break;
	case CAPN_DYN_DATA: {
		capn_list8 l;
		int i, n = capn_len(v->data);
		l.p = v->data.p;
		sb_str(b, "[");
		for (i = 0; i < n; i++) {
			if (i)
				sb_str(b, j->comma);
			sb_fmt(b, "%u", (unsigned) capn_get8(l, i));
		}
		sb_str(b, "]");
		break;
	}
	case CAPN_DYN_LIST: {
		capn_ptr list = v->p;
		int i, n;
		struct dv ev;
		const struct frame *efr = fr;
		const struct capn_dyn_type *et = resolve(t->elem, &efr);
		capn_resolve(&list);
		n = capn_ptr_len(list);
		sb_str(b, "[");
		for (i = 0; i < n; i++) {
			if (i)
				sb_str(b, j->comma);
			get_elem_value(list, et, i, &ev);
			json_value(j, et, efr, &ev);
		}
		sb_str(b, "]");
		break;
	}
	case CAPN_DYN_STRUCT:
		if (t->st != NULL) {
			struct frame nf;
			enter_struct(&nf, t, fr);
			json_struct(j, t->st, v->p, &nf);
		} else {
			sb_str(b, "null");
		}
		break;
	default:
		sb_str(b, "null");
		break;
	}
}

static int json_member(struct jctx *j, const struct capn_dyn_struct *s, capn_ptr st,
                       const struct capn_dyn_field *f, const struct frame *fr, int first)
{
	struct dv v;
	const struct frame *ffr = fr;
	const struct capn_dyn_type *rt = resolve(&f->type, &ffr);

	/* Interface and AnyPointer have no JSON form here: leave them out. */
	if (rt->kind == CAPN_DYN_INTERFACE || rt->kind == CAPN_DYN_ANYPOINTER)
		return first;
	if (rt->kind == CAPN_DYN_STRUCT && !f->is_group && rt->st == NULL)
		return first;
	if (!field_has(s, st, f, rt))
		return first;
	if (!first)
		sb_str(j->b, j->comma);
	json_escape(j->b, f->name, (int) strlen(f->name));
	sb_str(j->b, j->colon);
	get_field_value(st, f, rt, &v);
	if (f->is_group)
		json_struct(j, rt->st, v.p, fr);
	else
		json_value(j, rt, ffr, &v);
	return 0;
}

static void json_struct(struct jctx *j, const struct capn_dyn_struct *s, capn_ptr st,
                        const struct frame *fr)
{
	int i, first = 1, which;

	capn_resolve(&st);
	sb_str(j->b, "{");
	/* Fields outside a union in declaration order, then the active member. */
	for (i = 0; i < s->nfields; i++) {
		if (s->fields[i].discriminant == CAPN_DYN_NO_DISCRIMINANT)
			first = json_member(j, s, st, &s->fields[i], fr, first);
	}
	which = capn_dyn_which(s, st);
	if (which >= 0) {
		for (i = 0; i < s->nfields; i++) {
			if (s->fields[i].discriminant == (uint32_t) which) {
				first = json_member(j, s, st, &s->fields[i], fr, first);
				break;
			}
		}
	}
	sb_str(j->b, "}");
}

char *capn_dyn_to_json(const struct capn_dyn_struct *s, capn_ptr st, int flags)
{
	struct sb b;
	struct jctx j;

	memset(&b, 0, sizeof b);
	j.b = &b;
	j.colon = (flags & CAPN_DYN_COMPACT) ? ":" : ": ";
	j.comma = (flags & CAPN_DYN_COMPACT) ? "," : ", ";
	json_struct(&j, s, st, NULL);
	if (b.err) {
		free(b.buf);
		return NULL;
	}
	return b.buf ? b.buf : empty_string();
}

/* decoders are added in follow-up code below */

int capn_dyn_from_json(const struct capn_dyn_struct *s, const char *json,
                       size_t len, struct capn_segment *seg, capn_ptr *out,
                       struct capn_dyn_error *err)
{
	(void) s; (void) json; (void) len; (void) seg; (void) out;
	if (err) {
		snprintf(err->message, sizeof err->message, "not implemented");
		err->offset = -1;
	}
	return -1;
}

int capn_dyn_from_text(const struct capn_dyn_struct *s, const char *text,
                       size_t len, struct capn_segment *seg, capn_ptr *out,
                       struct capn_dyn_error *err)
{
	(void) s; (void) text; (void) len; (void) seg; (void) out;
	if (err) {
		snprintf(err->message, sizeof err->message, "not implemented");
		err->offset = -1;
	}
	return -1;
}
