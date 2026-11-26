/* capn-dyn.c - reflection, text and JSON over generated descriptors.
 * See capn-dyn.h. */

#include "capn-dyn.h"

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
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

static capn_ptr wire_ptr(capn_ptr st, const struct capn_dyn_field *f)
{
	capn_resolve(&st);
	return capn_getp(st, (int) f->offset, 1);
}

/* Element size for capn_resolve_list. A struct element is -1: C++ reads
 * List(Struct) with readListPointer(INLINE_COMPOSITE), which is
 * capn_resolve_struct_list here. */
static int dyn_list_elem(const struct capn_dyn_type *elem)
{
	if (elem == NULL)
		return CAPN_ELEM_POINTER;
	switch (elem->kind) {
	case CAPN_DYN_VOID:
		return CAPN_ELEM_VOID;
	case CAPN_DYN_BOOL:
		return CAPN_ELEM_BIT;
	case CAPN_DYN_INT8:
	case CAPN_DYN_UINT8:
		return CAPN_ELEM_BYTE;
	case CAPN_DYN_INT16:
	case CAPN_DYN_UINT16:
	case CAPN_DYN_ENUM:
		return CAPN_ELEM_2_BYTES;
	case CAPN_DYN_INT32:
	case CAPN_DYN_UINT32:
	case CAPN_DYN_FLOAT32:
		return CAPN_ELEM_4_BYTES;
	case CAPN_DYN_INT64:
	case CAPN_DYN_UINT64:
	case CAPN_DYN_FLOAT64:
		return CAPN_ELEM_8_BYTES;
	case CAPN_DYN_STRUCT:
		return -1;
	default:
		return CAPN_ELEM_POINTER;
	}
}

/* layout.c++ readStructPointer / readListPointer / readCapabilityPointer.
 * A pointer of the wrong kind sets decode_err and becomes null. */
static void check_dyn_ptr(capn_ptr *p, const struct capn_dyn_type *t)
{
	int elem;

	if (p == NULL || t == NULL || p->type == CAPN_NULL)
		return;
	switch (t->kind) {
	case CAPN_DYN_STRUCT:
		capn_resolve_struct(p);
		break;
	case CAPN_DYN_INTERFACE:
		capn_resolve_interface(p);
		break;
	case CAPN_DYN_LIST:
		elem = dyn_list_elem(t->elem);
		if (elem < 0)
			capn_resolve_struct_list(p);
		else
			capn_resolve_list(p, elem);
		break;
	case CAPN_DYN_TEXT:
	case CAPN_DYN_DATA:
		if (p->type != CAPN_LIST || p->datasz != 1) {
			if (p->seg && p->seg->capn)
				p->seg->capn->decode_err = 1;
			memset(p, 0, sizeof *p);
		}
		break;
	default:
		break;
	}
}

capn_ptr capn_dyn_get_ptr(capn_ptr st, const struct capn_dyn_field *f)
{
	capn_ptr p = wire_ptr(st, f);
	if (p.type == CAPN_NULL && f->default_ptr != NULL)
		return *f->default_ptr;
	if (f->type.kind != CAPN_DYN_PARAM)
		check_dyn_ptr(&p, &f->type);
	return p;
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
		return wire_ptr(st, f).type != CAPN_NULL;
	case CAPN_DYN_STRUCT:
		if (f->is_group)
			return 1;
		return wire_ptr(st, f).type != CAPN_NULL;
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
	size_t need, cap;
	char *p;

	if (b->err)
		return;
	/* len + n + 1 can wrap. A wrapped need makes the doubling loop
	 * run until cap itself wraps to 0 and then never stop. */
	if (n > SIZE_MAX - 1 || b->len > SIZE_MAX - 1 - n) {
		b->err = 1;
		return;
	}
	need = b->len + n + 1;
	if (need <= b->cap)
		return;
	cap = b->cap ? b->cap : 256;
	while (need > cap) {
		if (cap > SIZE_MAX / 2) {
			b->err = 1;
			return;
		}
		cap *= 2;
	}
	p = (char *) realloc(b->buf, cap);
	if (p == NULL) {
		b->err = 1;
		return;
	}
	b->buf = p;
	b->cap = cap;
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
		if (f->type.kind == CAPN_DYN_PARAM)
			check_dyn_ptr(&v->p, rt);
		break;
	case CAPN_DYN_DATA:
		/* A parameter is checked only after the brand resolves it.
		 * Copy into v->data afterwards: a rejected pointer is null,
		 * and the text and JSON printers read v->data. */
		v->p = capn_dyn_get_ptr(st, f);
		if (f->type.kind == CAPN_DYN_PARAM)
			check_dyn_ptr(&v->p, rt);
		v->data.p = v->p;
		break;
	case CAPN_DYN_STRUCT:
		if (f->is_group)
			v->p = st;
		else {
			v->p = capn_dyn_get_ptr(st, f);
			if (f->type.kind == CAPN_DYN_PARAM)
				check_dyn_ptr(&v->p, rt);
		}
		break;
	default:
		if (is_pointer_kind(rt->kind)) {
			v->p = capn_dyn_get_ptr(st, f);
			if (f->type.kind == CAPN_DYN_PARAM)
				check_dyn_ptr(&v->p, rt);
		} else
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
	if (e != NULL && v < (uint64_t) e->n && e->names[v] != NULL) {
		sb_str(b, e->names[v]);
	} else {
		sb_str(b, "(");
		sb_int(b, CAPN_DYN_UINT16, v);
		sb_str(b, ")");
	}
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
		if (tmp == NULL) {
			b->err = 1;
			break;
		}
		l.p = v->data.p;
		for (i = 0; i < n; i++)
			tmp[i] = (char) capn_get8(l, i);
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
		return wire_ptr(st, f).type != CAPN_NULL;
	return 1;
}

/* `capnp decode` prints the active union member even when its pointer
 * is unset, unless it is the union's first member. */
static int text_shown(const struct capn_dyn_struct *s, capn_ptr st,
                      const struct capn_dyn_field *f, const struct capn_dyn_type *rt)
{
	if (field_has(s, st, f, rt))
		return 1;
	return f->discriminant != CAPN_DYN_NO_DISCRIMINANT && f->discriminant != 0 &&
	       capn_dyn_is_active(s, st, f);
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
		if (!text_shown(s, st, f, rt))
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

/* Multi-line text, as `capnp decode` prints it. Indent amount starts at
 * 1. A list or struct stays on one line when every item is at most 24
 * bytes, contains no newline, and a struct's items total at most 64. */

enum { PRETTY_BARE = 0, PRETTY_PREFIXED = 1, PRETTY_LIST = 0, PRETTY_RECORD = 1 };

struct piece {
	char *s;
	size_t n;
};

static struct piece piece_fail(void)
{
	struct piece p;
	p.s = NULL;
	p.n = 0;
	return p;
}

static struct piece piece_from_sb(struct sb *b)
{
	struct piece p;
	if (b->err) {
		free(b->buf);
		return piece_fail();
	}
	if (b->buf == NULL)
		b->buf = empty_string();
	if (b->buf == NULL)
		return piece_fail();
	p.s = b->buf;
	p.n = b->len;
	return p;
}

static int piece_inline(const struct piece *p)
{
	size_t i;
	if (p->s == NULL || p->n > 24)
		return 0;
	for (i = 0; i < p->n; i++)
		if (p->s[i] == '\n')
			return 0;
	return 1;
}

static struct piece pretty_value(const struct capn_dyn_type *t, const struct frame *fr,
                                 struct dv *v, int amount, int mode);

static struct piece pretty_delimit(struct piece *items, int n, int amount, int mode, int kind)
{
	struct sb b;
	int i, all = 1;
	size_t total = 0;
	char pad[80];
	int padn;

	memset(&b, 0, sizeof b);
	if (amount != 0) {
		for (i = 0; i < n; i++) {
			if (!piece_inline(&items[i])) {
				all = 0;
				break;
			}
			if (kind == PRETTY_RECORD) {
				total += items[i].n;
				if (total > 64) {
					all = 0;
					break;
				}
			}
		}
	}
	if (amount == 0 || all) {
		for (i = 0; i < n; i++) {
			if (i)
				sb_str(&b, ", ");
			sb_add(&b, items[i].s, items[i].n);
		}
	} else {
		padn = amount * 2;
		if (padn > (int) sizeof pad - 1)
			padn = (int) sizeof pad - 1;
		memset(pad, ' ', (size_t) padn);
		pad[padn] = '\0';
		if (mode == PRETTY_BARE)
			sb_str(&b, " ");
		else {
			sb_str(&b, "\n");
			sb_str(&b, pad);
		}
		for (i = 0; i < n; i++) {
			if (i) {
				sb_str(&b, ",\n");
				sb_str(&b, pad);
			}
			sb_add(&b, items[i].s, items[i].n);
		}
		sb_str(&b, " ");
	}
	return piece_from_sb(&b);
}

static struct piece pretty_list(const struct capn_dyn_type *t, const struct frame *fr,
                                capn_ptr list, int amount, int mode)
{
	const struct frame *efr = fr;
	const struct capn_dyn_type *et = resolve(t->elem, &efr);
	struct piece *items = NULL;
	struct piece body;
	struct sb b;
	int i, n;

	capn_resolve(&list);
	n = capn_ptr_len(list);
	if (n < 0)
		n = 0;
	if (n > 0) {
		items = (struct piece *) calloc((size_t) n, sizeof *items);
		if (items == NULL)
			return piece_fail();
	}
	for (i = 0; i < n; i++) {
		struct dv ev;
		get_elem_value(list, et, i, &ev);
		items[i] = pretty_value(et, efr, &ev, amount + 1, PRETTY_BARE);
		if (items[i].s == NULL)
			goto fail;
	}
	body = pretty_delimit(items, n, amount, mode, PRETTY_LIST);
	for (i = 0; i < n; i++)
		free(items[i].s);
	free(items);
	if (body.s == NULL)
		return piece_fail();
	memset(&b, 0, sizeof b);
	sb_str(&b, "[");
	sb_add(&b, body.s, body.n);
	sb_str(&b, "]");
	free(body.s);
	return piece_from_sb(&b);
fail:
	for (i = 0; i < n; i++)
		free(items[i].s);
	free(items);
	return piece_fail();
}

static struct piece pretty_struct(const struct capn_dyn_struct *s, capn_ptr st,
                                  const struct frame *fr, int amount, int mode)
{
	struct piece *items = NULL;
	struct piece body;
	struct sb b;
	int i, n = 0, slot = 0;
	capn_resolve(&st);
	if (s->nfields > 0) {
		items = (struct piece *) calloc((size_t) s->nfields, sizeof *items);
		if (items == NULL)
			return piece_fail();
	}
	for (i = 0; i < s->nfields; i++) {
		const struct capn_dyn_field *f = &s->fields[i];
		const struct frame *ffr = fr;
		const struct capn_dyn_type *rt = resolve(&f->type, &ffr);
		struct dv v;
		struct piece val;
		struct sb item;

		if (!text_shown(s, st, f, rt))
			continue;
		get_field_value(st, f, rt, &v);
		if (f->is_group)
			val = pretty_struct(rt->st, v.p, fr, amount + 1, PRETTY_PREFIXED);
		else
			val = pretty_value(rt, ffr, &v, amount + 1, PRETTY_PREFIXED);
		if (val.s == NULL)
			goto fail;
		memset(&item, 0, sizeof item);
		sb_str(&item, f->name);
		sb_str(&item, " = ");
		sb_add(&item, val.s, val.n);
		free(val.s);
		items[slot] = piece_from_sb(&item);
		if (items[slot].s == NULL)
			goto fail;
		slot++;
		n++;
	}
	body = pretty_delimit(items, n, amount, mode, PRETTY_RECORD);
	for (i = 0; i < n; i++)
		free(items[i].s);
	free(items);
	items = NULL;
	if (body.s == NULL)
		return piece_fail();
	memset(&b, 0, sizeof b);
	sb_str(&b, "(");
	sb_add(&b, body.s, body.n);
	sb_str(&b, ")");
	free(body.s);
	return piece_from_sb(&b);
fail:
	for (i = 0; i < n; i++)
		free(items[i].s);
	free(items);
	return piece_fail();
}

static struct piece pretty_value(const struct capn_dyn_type *t, const struct frame *fr,
                                 struct dv *v, int amount, int mode)
{
	struct sb b;

	switch (t->kind) {
	case CAPN_DYN_LIST:
		return pretty_list(t, fr, v->p, amount, mode);
	case CAPN_DYN_STRUCT:
		if (t->st != NULL) {
			struct frame nf;
			enter_struct(&nf, t, fr);
			return pretty_struct(t->st, v->p, &nf, amount, mode);
		}
		break;
	default:
		break;
	}
	memset(&b, 0, sizeof b);
	text_value(&b, t, fr, v);
	return piece_from_sb(&b);
}

char *capn_dyn_to_text_pretty(const struct capn_dyn_struct *s, capn_ptr st)
{
	struct piece p = pretty_struct(s, st, NULL, 1, PRETTY_BARE);
	return p.s;
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
	case CAPN_DYN_ENUM: {
		const char *en = NULL;
		if (t->en != NULL && v->bits < (uint64_t) t->en->n) {
			if (t->en->json_names != NULL && t->en->json_names[v->bits] != NULL)
				en = t->en->json_names[v->bits];
			else
				en = t->en->names[v->bits];
		}
		if (en != NULL)
			json_escape(b, en, (int) strlen(en));
		else
			sb_int(b, CAPN_DYN_UINT16, v->bits);
		break;
	}
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

static const char *json_key(const struct capn_dyn_field *f)
{
	return f->json_name != NULL ? f->json_name : f->name;
}

static void json_escape_prefixed(struct sb *b, const char *prefix, const char *name)
{
	size_t np = prefix != NULL ? strlen(prefix) : 0;
	size_t nn = strlen(name);
	char *tmp;

	if (np == 0) {
		json_escape(b, name, (int) nn);
		return;
	}
	tmp = (char *) malloc(np + nn + 1);
	if (tmp == NULL) {
		b->err = 1;
		return;
	}
	memcpy(tmp, prefix, np);
	memcpy(tmp + np, name, nn + 1);
	json_escape(b, tmp, (int) (np + nn));
	free(tmp);
}

static void json_bytes(struct jctx *j, const uint8_t *p, int n, int enc)
{
	static const char b64[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	static const char hexd[] = "0123456789abcdef";
	int i;

	sb_str(j->b, "\"");
	if (enc == 1) {
		for (i = 0; i < n; i += 3) {
			unsigned v = (unsigned) p[i] << 16;
			int left = n - i;
			if (left > 1)
				v |= (unsigned) p[i + 1] << 8;
			if (left > 2)
				v |= (unsigned) p[i + 2];
			sb_fmt(j->b, "%c", b64[(v >> 18) & 63]);
			sb_fmt(j->b, "%c", b64[(v >> 12) & 63]);
			sb_fmt(j->b, "%c", left > 1 ? b64[(v >> 6) & 63] : '=');
			sb_fmt(j->b, "%c", left > 2 ? b64[v & 63] : '=');
		}
	} else {
		for (i = 0; i < n; i++) {
			sb_fmt(j->b, "%c", hexd[p[i] >> 4]);
			sb_fmt(j->b, "%c", hexd[p[i] & 15]);
		}
	}
	sb_str(j->b, "\"");
}

static void json_data_value(struct jctx *j, struct dv *v, int enc)
{
	capn_list8 l;
	int n, i;
	uint8_t *tmp;

	if (enc == 0) {
		json_value(j, &(struct capn_dyn_type){CAPN_DYN_DATA}, NULL, v);
		return;
	}
	l.p = v->data.p;
	n = capn_len(v->data);
	if (n < 0)
		n = 0;
	tmp = n > 0 ? (uint8_t *) malloc((size_t) n) : NULL;
	if (n > 0 && tmp == NULL) {
		j->b->err = 1;
		return;
	}
	for (i = 0; i < n; i++)
		tmp[i] = capn_get8(l, i);
	json_bytes(j, tmp, n, enc);
	free(tmp);
}

static int json_fields(struct jctx *j, const struct capn_dyn_struct *s, capn_ptr st,
                       const struct frame *fr, const char *prefix, int first);

static const struct capn_dyn_field *active_union_field(const struct capn_dyn_struct *s, capn_ptr st)
{
	int which = capn_dyn_which(s, st);
	int i;
	if (which < 0)
		return NULL;
	for (i = 0; i < s->nfields; i++) {
		if (s->fields[i].discriminant == (uint32_t) which)
			return &s->fields[i];
	}
	return NULL;
}

/* `active` writes the field even when its pointer is unset, as the JSON
 * handler of `capnp convert` does for the active union member. */
static int json_emit_field(struct jctx *j, const struct capn_dyn_struct *s, capn_ptr st,
                           const struct capn_dyn_field *f, const struct frame *fr,
                           const char *prefix, const char *key, int first, int active)
{
	struct dv v;
	const struct frame *ffr = fr;
	const struct capn_dyn_type *rt = resolve(&f->type, &ffr);
	char nested[512];

	if (rt->kind == CAPN_DYN_INTERFACE || rt->kind == CAPN_DYN_ANYPOINTER)
		return first;
	if (rt->kind == CAPN_DYN_STRUCT && !f->is_group && rt->st == NULL)
		return first;
	if (!active && !field_has(s, st, f, rt))
		return first;
	get_field_value(st, f, rt, &v);
	if (f->json_flatten && rt->kind == CAPN_DYN_STRUCT && rt->st != NULL) {
		const char *pre = f->json_prefix != NULL ? f->json_prefix : "";
		const char *outer = prefix != NULL ? prefix : "";
		int nw = snprintf(nested, sizeof nested, "%s%s", outer, pre);
		if (nw < 0 || (size_t) nw >= sizeof nested) {
			j->b->err = 1;
			return first;
		}
		if (f->is_group)
			return json_fields(j, rt->st, st, fr, nested, first);
		{
			struct frame nf;
			enter_struct(&nf, rt, ffr);
			return json_fields(j, rt->st, v.p, &nf, nested, first);
		}
	}
	if (!first)
		sb_str(j->b, j->comma);
	json_escape_prefixed(j->b, prefix, key != NULL ? key : json_key(f));
	sb_str(j->b, j->colon);
	if (f->is_group)
		json_struct(j, rt->st, v.p, fr);
	else if (rt->kind == CAPN_DYN_DATA && f->json_data != 0)
		json_data_value(j, &v, f->json_data);
	else
		json_value(j, rt, ffr, &v);
	return 0;
}

static int json_fields(struct jctx *j, const struct capn_dyn_struct *s, capn_ptr st,
                       const struct frame *fr, const char *prefix, int first)
{
	int i;
	const struct capn_dyn_field *u;

	capn_resolve(&st);
	for (i = 0; i < s->nfields; i++) {
		if (s->fields[i].discriminant == CAPN_DYN_NO_DISCRIMINANT)
			first = json_emit_field(j, s, st, &s->fields[i], fr, prefix, NULL, first, 0);
	}
	u = active_union_field(s, st);
	if (u == NULL)
		return first;
	if (s->json_has_disc && s->json_disc_name != NULL && s->json_disc_name[0] != '\0') {
		struct dv tag;
		const struct capn_dyn_type text = {CAPN_DYN_TEXT};
		if (!first)
			sb_str(j->b, j->comma);
		json_escape_prefixed(j->b, prefix, s->json_disc_name);
		sb_str(j->b, j->colon);
		tag.text.str = json_key(u);
		tag.text.len = (int) strlen(json_key(u));
		tag.text.seg = NULL;
		json_value(j, &text, NULL, &tag);
		first = 0;
	}
	if (s->json_has_disc && u->type.kind == CAPN_DYN_VOID)
		return first;
	if (s->json_has_disc && s->json_disc_value != NULL)
		return json_emit_field(j, s, st, u, fr, prefix, s->json_disc_value, first, 1);
	return json_emit_field(j, s, st, u, fr, prefix, NULL, first, 1);
}

static void json_struct(struct jctx *j, const struct capn_dyn_struct *s, capn_ptr st,
                        const struct frame *fr)
{
	sb_str(j->b, "{");
	json_fields(j, s, st, fr, NULL, 1);
	sb_str(j->b, "}");
}

/* Pretty JSON follows JsonCodec::encodeList. One line uses ", " between
 * items. Several items break when one is already multi-line or any item
 * is longer than 50 bytes: a space or a newline-indent before the first,
 * ",\n" plus indent between items, and a space before the closer. */
static void pieces_free(struct piece *p, int n)
{
	int i;
	if (p == NULL)
		return;
	for (i = 0; i < n; i++)
		free(p[i].s);
	free(p);
}

static struct piece json_leaf(const struct capn_dyn_type *t, const struct frame *fr,
                              struct dv *v)
{
	struct sb b;
	struct jctx j;

	memset(&b, 0, sizeof b);
	memset(&j, 0, sizeof j);
	j.b = &b;
	j.colon = ": ";
	j.comma = ", ";
	json_value(&j, t, fr, v);
	return piece_from_sb(&b);
}

static struct piece json_join(struct piece *elems, int n, int child_multi, int indent,
                              int *multiline, int has_prefix, char open_ch, char close_ch)
{
	size_t maxc = 0;
	int i, brk, padn;
	struct sb b;
	char stackpad[256];
	char *pad = stackpad;
	char *heap = NULL;
	char open_s[2], close_s[2];

	for (i = 0; i < n; i++) {
		if (elems[i].s == NULL)
			return piece_fail();
		if (elems[i].n > maxc)
			maxc = elems[i].n;
	}
	brk = n > 1 && (child_multi || maxc > 50);
	memset(&b, 0, sizeof b);
	open_s[0] = open_ch;
	open_s[1] = '\0';
	close_s[0] = close_ch;
	close_s[1] = '\0';
	sb_str(&b, open_s);
	if (brk) {
		if (multiline != NULL)
			*multiline = 1;
		padn = (indent + 1) * 2;
		if (padn < 0)
			padn = 0;
		if (padn > (int) sizeof stackpad - 1) {
			heap = (char *) malloc((size_t) padn + 1);
			if (heap == NULL) {
				free(b.buf);
				return piece_fail();
			}
			pad = heap;
		}
		memset(pad, ' ', (size_t) padn);
		pad[padn] = '\0';
		if (has_prefix) {
			sb_str(&b, "\n");
			sb_str(&b, pad);
		} else {
			sb_str(&b, " ");
		}
		for (i = 0; i < n; i++) {
			if (i) {
				sb_str(&b, ",\n");
				sb_str(&b, pad);
			}
			sb_add(&b, elems[i].s, elems[i].n);
		}
		sb_str(&b, " ");
	} else {
		for (i = 0; i < n; i++) {
			if (i)
				sb_str(&b, ", ");
			sb_add(&b, elems[i].s, elems[i].n);
		}
	}
	sb_str(&b, close_s);
	free(heap);
	return piece_from_sb(&b);
}

static struct piece json_pretty_value(const struct capn_dyn_type *t, const struct frame *fr,
                                      struct dv *v, int indent, int *multiline, int has_prefix);
static struct piece json_pretty_struct(const struct capn_dyn_struct *s, capn_ptr st,
                                       const struct frame *fr, int indent, int *multiline,
                                       int has_prefix);
static int json_count_fields(const struct capn_dyn_struct *s, capn_ptr st,
                             const struct frame *fr);
static int json_fill_fields(const struct capn_dyn_struct *s, capn_ptr st,
                            const struct frame *fr, const char *prefix, int sub_indent,
                            struct piece *out, int cap, int *at, int *child_multi);

static int json_skip_field(const struct capn_dyn_struct *s, capn_ptr st,
                           const struct capn_dyn_field *f, const struct frame *fr,
                           const struct frame **ffr_out, const struct capn_dyn_type **rt_out,
                           struct dv *v, int active)
{
	const struct frame *ffr = fr;
	const struct capn_dyn_type *rt = resolve(&f->type, &ffr);

	*ffr_out = ffr;
	*rt_out = rt;
	if (rt->kind == CAPN_DYN_INTERFACE || rt->kind == CAPN_DYN_ANYPOINTER)
		return 1;
	if (rt->kind == CAPN_DYN_STRUCT && !f->is_group && rt->st == NULL)
		return 1;
	if (!active && !field_has(s, st, f, rt))
		return 1;
	get_field_value(st, f, rt, v);
	return 0;
}

static int json_count_one(const struct capn_dyn_struct *s, capn_ptr st,
                          const struct capn_dyn_field *f, const struct frame *fr, int active)
{
	const struct frame *ffr;
	const struct capn_dyn_type *rt;
	struct dv v;

	if (json_skip_field(s, st, f, fr, &ffr, &rt, &v, active))
		return 0;
	if (f->json_flatten && rt->kind == CAPN_DYN_STRUCT && rt->st != NULL) {
		if (f->is_group)
			return json_count_fields(rt->st, st, fr);
		{
			struct frame nf;
			enter_struct(&nf, rt, ffr);
			return json_count_fields(rt->st, v.p, &nf);
		}
	}
	return 1;
}

static int json_count_fields(const struct capn_dyn_struct *s, capn_ptr st,
                             const struct frame *fr)
{
	int i, n = 0;
	const struct capn_dyn_field *u;

	capn_resolve(&st);
	for (i = 0; i < s->nfields; i++) {
		if (s->fields[i].discriminant == CAPN_DYN_NO_DISCRIMINANT)
			n += json_count_one(s, st, &s->fields[i], fr, 0);
	}
	u = active_union_field(s, st);
	if (u == NULL)
		return n;
	if (s->json_has_disc && s->json_disc_name != NULL && s->json_disc_name[0] != '\0')
		n++;
	if (s->json_has_disc && u->type.kind == CAPN_DYN_VOID)
		return n;
	n += json_count_one(s, st, u, fr, 1);
	return n;
}

static int json_push_field(struct piece *out, int cap, int *at, const char *prefix,
                           const char *key, struct piece val)
{
	struct sb b;

	if (val.s == NULL)
		return -1;
	if (*at >= cap) {
		free(val.s);
		return -1;
	}
	memset(&b, 0, sizeof b);
	json_escape_prefixed(&b, prefix, key);
	sb_str(&b, ": ");
	sb_add(&b, val.s, val.n);
	free(val.s);
	out[*at] = piece_from_sb(&b);
	if (out[*at].s == NULL)
		return -1;
	(*at)++;
	return 0;
}

static int json_fill_one(const struct capn_dyn_struct *s, capn_ptr st,
                         const struct capn_dyn_field *f, const struct frame *fr,
                         const char *prefix, const char *key_over, int sub_indent,
                         struct piece *out, int cap, int *at, int *child_multi, int active)
{
	const struct frame *ffr;
	const struct capn_dyn_type *rt;
	struct dv v;
	struct piece val;

	if (json_skip_field(s, st, f, fr, &ffr, &rt, &v, active))
		return 0;
	if (f->json_flatten && rt->kind == CAPN_DYN_STRUCT && rt->st != NULL) {
		const char *pre = f->json_prefix != NULL ? f->json_prefix : "";
		const char *outer = prefix != NULL ? prefix : "";
		char nested[512];
		int nw = snprintf(nested, sizeof nested, "%s%s", outer, pre);
		if (nw < 0 || (size_t) nw >= sizeof nested)
			return -1;
		if (f->is_group)
			return json_fill_fields(rt->st, st, fr, nested, sub_indent, out, cap, at, child_multi);
		{
			struct frame nf;
			enter_struct(&nf, rt, ffr);
			return json_fill_fields(rt->st, v.p, &nf, nested, sub_indent, out, cap, at, child_multi);
		}
	}
	if (f->is_group)
		val = json_pretty_struct(rt->st, v.p, fr, sub_indent, child_multi, 1);
	else if (rt->kind == CAPN_DYN_DATA && f->json_data != 0) {
		struct sb b;
		struct jctx j;
		memset(&b, 0, sizeof b);
		memset(&j, 0, sizeof j);
		j.b = &b;
		j.colon = ":";
		j.comma = ",";
		json_data_value(&j, &v, f->json_data);
		val = piece_from_sb(&b);
	} else {
		val = json_pretty_value(rt, ffr, &v, sub_indent, child_multi, 1);
	}
	return json_push_field(out, cap, at, prefix, key_over != NULL ? key_over : json_key(f), val);
}

static int json_fill_fields(const struct capn_dyn_struct *s, capn_ptr st,
                            const struct frame *fr, const char *prefix, int sub_indent,
                            struct piece *out, int cap, int *at, int *child_multi)
{
	int i;
	const struct capn_dyn_field *u;

	capn_resolve(&st);
	for (i = 0; i < s->nfields; i++) {
		if (s->fields[i].discriminant != CAPN_DYN_NO_DISCRIMINANT)
			continue;
		if (json_fill_one(s, st, &s->fields[i], fr, prefix, NULL, sub_indent,
		                  out, cap, at, child_multi, 0) != 0)
			return -1;
	}
	u = active_union_field(s, st);
	if (u == NULL)
		return 0;
	if (s->json_has_disc && s->json_disc_name != NULL && s->json_disc_name[0] != '\0') {
		struct dv tag;
		struct piece val;
		const struct capn_dyn_type text = {CAPN_DYN_TEXT};
		memset(&tag, 0, sizeof tag);
		tag.text.str = json_key(u);
		tag.text.len = (int) strlen(tag.text.str);
		val = json_pretty_value(&text, NULL, &tag, sub_indent, child_multi, 1);
		if (json_push_field(out, cap, at, prefix, s->json_disc_name, val) != 0)
			return -1;
	}
	if (s->json_has_disc && u->type.kind == CAPN_DYN_VOID)
		return 0;
	if (s->json_has_disc && s->json_disc_value != NULL)
		return json_fill_one(s, st, u, fr, prefix, s->json_disc_value, sub_indent,
		                      out, cap, at, child_multi, 1);
	return json_fill_one(s, st, u, fr, prefix, NULL, sub_indent, out, cap, at, child_multi, 1);
}

static struct piece json_pretty_list(const struct capn_dyn_type *t, const struct frame *fr,
                                     capn_ptr list, int indent, int *multiline, int has_prefix)
{
	const struct frame *efr = fr;
	const struct capn_dyn_type *et = resolve(t->elem, &efr);
	struct piece *elems = NULL;
	struct piece body;
	int i, n, sub, child = 0;

	capn_resolve(&list);
	n = capn_ptr_len(list);
	if (n < 0)
		n = 0;
	sub = indent + (n > 1);
	if (n > 0) {
		elems = (struct piece *) calloc((size_t) n, sizeof *elems);
		if (elems == NULL)
			return piece_fail();
		for (i = 0; i < n; i++) {
			struct dv ev;
			get_elem_value(list, et, i, &ev);
			elems[i] = json_pretty_value(et, efr, &ev, sub, &child, 0);
			if (elems[i].s == NULL) {
				pieces_free(elems, n);
				return piece_fail();
			}
		}
	}
	body = json_join(elems, n, child, indent, multiline, has_prefix, '[', ']');
	pieces_free(elems, n);
	return body;
}

static struct piece json_pretty_value(const struct capn_dyn_type *t, const struct frame *fr,
                                      struct dv *v, int indent, int *multiline, int has_prefix)
{
	switch (t->kind) {
	case CAPN_DYN_LIST:
		return json_pretty_list(t, fr, v->p, indent, multiline, has_prefix);
	case CAPN_DYN_STRUCT:
		if (t->st == NULL)
			return json_leaf(t, fr, v);
		{
			struct frame nf;
			enter_struct(&nf, t, fr);
			return json_pretty_struct(t->st, v->p, &nf, indent, multiline, has_prefix);
		}
	default:
		return json_leaf(t, fr, v);
	}
}

static struct piece json_pretty_struct(const struct capn_dyn_struct *s, capn_ptr st,
                                       const struct frame *fr, int indent, int *multiline,
                                       int has_prefix)
{
	int n = json_count_fields(s, st, fr);
	int sub = indent + (n > 1);
	int child = 0, at = 0;
	struct piece *elems = NULL;
	struct piece body;

	if (n > 0) {
		elems = (struct piece *) calloc((size_t) n, sizeof *elems);
		if (elems == NULL)
			return piece_fail();
		if (json_fill_fields(s, st, fr, NULL, sub, elems, n, &at, &child) != 0 || at != n) {
			pieces_free(elems, n);
			return piece_fail();
		}
	}
	body = json_join(elems, n, child, indent, multiline, has_prefix, '{', '}');
	pieces_free(elems, n);
	return body;
}

char *capn_dyn_to_json(const struct capn_dyn_struct *s, capn_ptr st, int flags)
{
	struct sb b;
	struct jctx j;

	if ((flags & CAPN_DYN_COMPACT) == 0) {
		struct piece p;
		int multi = 0;
		p = json_pretty_struct(s, st, NULL, 0, &multi, 0);
		return p.s;
	}
	memset(&b, 0, sizeof b);
	j.b = &b;
	j.colon = ":";
	j.comma = ",";
	json_struct(&j, s, st, NULL);
	if (b.err) {
		free(b.buf);
		return NULL;
	}
	return b.buf ? b.buf : empty_string();
}

/* --- decoding ------------------------------------------------------------
 *
 * JSON and the text format are parsed into the same small tree, which one
 * builder then writes into a message using the descriptors. The two
 * syntaxes differ only in how leaves are spelled (an enum is a string in
 * JSON and a bare name in text), and the builder takes either.
 */

enum { V_NULL, V_BOOL, V_NUM, V_STR, V_IDENT, V_LIST, V_OBJ };

#define MAX_DEPTH 64

struct vnode {
	int kind;
	int off;
	char *s;
	size_t n;
	struct vnode **v;
	char **k;
	int count, cap;
	struct vnode *chain;
};

struct pctx {
	const char *p;
	size_t len, pos;
	struct vnode *all;
	struct capn_dyn_error *err;
	int failed;
	int depth;
};

static int perr(struct pctx *c, int off, const char *fmt, ...)
{
	va_list ap;
	if (!c->failed) {
		c->failed = 1;
		if (c->err) {
			va_start(ap, fmt);
			vsnprintf(c->err->message, sizeof c->err->message, fmt, ap);
			va_end(ap);
			c->err->offset = off;
		}
	}
	return -1;
}

static struct vnode *vnew(struct pctx *c, int kind, int off)
{
	struct vnode *n = (struct vnode *) calloc(1, sizeof *n);
	if (n == NULL) {
		perr(c, off, "out of memory");
		return NULL;
	}
	n->kind = kind;
	n->off = off;
	n->chain = c->all;
	c->all = n;
	return n;
}

static void vfree_all(struct pctx *c)
{
	struct vnode *n = c->all, *next;
	int i;
	for (; n != NULL; n = next) {
		next = n->chain;
		free(n->s);
		free(n->v);
		for (i = 0; i < n->count && n->k; i++)
			free(n->k[i]);
		free(n->k);
		free(n);
	}
	c->all = NULL;
}

static int vpush(struct pctx *c, struct vnode *parent, struct vnode *child, char *key)
{
	if (parent->count == parent->cap) {
		int cap;
		struct vnode **nv;
		char **nk;
		if (parent->cap > INT_MAX / 2)
			return perr(c, child->off, "out of memory");
		cap = parent->cap ? parent->cap * 2 : 4;
		if ((size_t) cap > SIZE_MAX / sizeof *nv)
			return perr(c, child->off, "out of memory");
		nv = (struct vnode **) realloc(parent->v, (size_t) cap * sizeof *nv);
		if (nv == NULL)
			return perr(c, child->off, "out of memory");
		parent->v = nv;
		if (parent->kind == V_OBJ) {
			nk = (char **) realloc(parent->k, (size_t) cap * sizeof *nk);
			if (nk == NULL)
				return perr(c, child->off, "out of memory");
			parent->k = nk;
		}
		parent->cap = cap;
	}
	parent->v[parent->count] = child;
	if (parent->kind == V_OBJ)
		parent->k[parent->count] = key;
	parent->count++;
	return 0;
}

static int append_utf8(char *out, size_t *n, unsigned long cp)
{
	if (cp < 0x80) {
		out[(*n)++] = (char) cp;
	} else if (cp < 0x800) {
		out[(*n)++] = (char) (0xc0 | (cp >> 6));
		out[(*n)++] = (char) (0x80 | (cp & 0x3f));
	} else if (cp < 0x10000) {
		out[(*n)++] = (char) (0xe0 | (cp >> 12));
		out[(*n)++] = (char) (0x80 | ((cp >> 6) & 0x3f));
		out[(*n)++] = (char) (0x80 | (cp & 0x3f));
	} else {
		out[(*n)++] = (char) (0xf0 | (cp >> 18));
		out[(*n)++] = (char) (0x80 | ((cp >> 12) & 0x3f));
		out[(*n)++] = (char) (0x80 | ((cp >> 6) & 0x3f));
		out[(*n)++] = (char) (0x80 | (cp & 0x3f));
	}
	return 0;
}

static int hexval(int ch)
{
	if (ch >= '0' && ch <= '9') return ch - '0';
	if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
	return -1;
}

static void skip_ws(struct pctx *c, int allow_comments)
{
	while (c->pos < c->len) {
		char ch = c->p[c->pos];
		if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
			c->pos++;
		} else if (allow_comments && ch == '#') {
			while (c->pos < c->len && c->p[c->pos] != '\n')
				c->pos++;
		} else {
			break;
		}
	}
}

/* A quoted string, JSON escapes when `json` else the schema language's. */
static struct vnode *parse_string(struct pctx *c, int json)
{
	int start = (int) c->pos;
	struct vnode *n;
	char *out;
	size_t o = 0;

	c->pos++; /* opening quote */
	out = (char *) malloc(c->len - c->pos + 5);
	if (out == NULL) {
		perr(c, start, "out of memory");
		return NULL;
	}
	while (c->pos < c->len && c->p[c->pos] != '"') {
		unsigned char ch = (unsigned char) c->p[c->pos++];
		if (ch != '\\') {
			if (json && ch < 0x20) {
				free(out);
				perr(c, (int) c->pos - 1, "control character in string");
				return NULL;
			}
			out[o++] = (char) ch;
			continue;
		}
		if (c->pos >= c->len)
			break;
		ch = (unsigned char) c->p[c->pos++];
		switch (ch) {
		case '"': out[o++] = '"'; break;
		case '\\': out[o++] = '\\'; break;
		case 'b': out[o++] = '\b'; break;
		case 'f': out[o++] = '\f'; break;
		case 'n': out[o++] = '\n'; break;
		case 'r': out[o++] = '\r'; break;
		case 't': out[o++] = '\t'; break;
		case '/':
			if (!json)
				goto bad;
			out[o++] = '/';
			break;
		case '\'':
			if (json)
				goto bad;
			out[o++] = '\'';
			break;
		case 'a':
			if (json)
				goto bad;
			out[o++] = '\a';
			break;
		case 'v':
			if (json)
				goto bad;
			out[o++] = '\v';
			break;
		case 'u': {
			unsigned long cp = 0;
			int k;
			/* Each \uXXXX is its own UTF-8 sequence, including a surrogate.
			 * A pair is two 3-byte sequences (ed a0 bd ed b8 80 for
			 * \uD83D\uDE00), the same bytes JsonCodec writes. */
			if (!json)
				goto bad;
			for (k = 0; k < 4; k++) {
				int hv = c->pos < c->len ? hexval(c->p[c->pos]) : -1;
				if (hv < 0)
					goto bad;
				cp = cp * 16 + (unsigned long) hv;
				c->pos++;
			}
			append_utf8(out, &o, cp);
			break;
		}
		case 'x': {
			int hv, v = 0, k;
			if (json)
				goto bad;
			for (k = 0; k < 2 && c->pos < c->len && (hv = hexval(c->p[c->pos])) >= 0; k++) {
				v = v * 16 + hv;
				c->pos++;
			}
			if (k == 0)
				goto bad;
			out[o++] = (char) v;
			break;
		}
		default:
			if (!json && ch >= '0' && ch <= '7') {
				int v = ch - '0', k;
				for (k = 0; k < 2 && c->pos < c->len && c->p[c->pos] >= '0' && c->p[c->pos] <= '7'; k++)
					v = v * 8 + (c->p[c->pos++] - '0');
				out[o++] = (char) v;
				break;
			}
			goto bad;
		}
		continue;
	bad:
		free(out);
		perr(c, (int) c->pos - 1, "bad escape in string");
		return NULL;
	}
	if (c->pos >= c->len) {
		free(out);
		perr(c, start, "unterminated string");
		return NULL;
	}
	c->pos++; /* closing quote */
	n = vnew(c, V_STR, start);
	if (n == NULL) {
		free(out);
		return NULL;
	}
	out[o] = '\0';
	n->s = out;
	n->n = o;
	return n;
}

static struct vnode *token_node(struct pctx *c, int kind, int start, size_t end)
{
	struct vnode *n = vnew(c, kind, start);
	if (n == NULL)
		return NULL;
	n->n = end - (size_t) start;
	n->s = (char *) malloc(n->n + 1);
	if (n->s == NULL) {
		perr(c, start, "out of memory");
		return NULL;
	}
	memcpy(n->s, c->p + start, n->n);
	n->s[n->n] = '\0';
	return n;
}

static struct vnode *parse_json(struct pctx *c)
{
	struct vnode *n;
	int start;

	skip_ws(c, 0);
	if (c->pos >= c->len) {
		perr(c, (int) c->pos, "unexpected end of JSON");
		return NULL;
	}
	start = (int) c->pos;
	switch (c->p[c->pos]) {
	case '{': {
		if (++c->depth > MAX_DEPTH) {
			perr(c, start, "JSON nests too deeply");
			return NULL;
		}
		n = vnew(c, V_OBJ, start);
		if (n == NULL)
			return NULL;
		c->pos++;
		skip_ws(c, 0);
		if (c->pos < c->len && c->p[c->pos] == '}') {
			c->pos++;
			c->depth--;
			return n;
		}
		for (;;) {
			struct vnode *key, *val;
			char *kcopy;
			skip_ws(c, 0);
			if (c->pos >= c->len || c->p[c->pos] != '"') {
				perr(c, (int) c->pos, "expected a string key");
				return NULL;
			}
			key = parse_string(c, 1);
			if (key == NULL)
				return NULL;
			skip_ws(c, 0);
			if (c->pos >= c->len || c->p[c->pos] != ':') {
				perr(c, (int) c->pos, "expected ':'");
				return NULL;
			}
			c->pos++;
			val = parse_json(c);
			if (val == NULL)
				return NULL;
			kcopy = (char *) malloc(key->n + 1);
			if (kcopy == NULL) {
				perr(c, key->off, "out of memory");
				return NULL;
			}
			memcpy(kcopy, key->s, key->n + 1);
			if (vpush(c, n, val, kcopy) != 0) {
				free(kcopy);
				return NULL;
			}
			skip_ws(c, 0);
			if (c->pos < c->len && c->p[c->pos] == ',') {
				c->pos++;
				continue;
			}
			if (c->pos < c->len && c->p[c->pos] == '}') {
				c->pos++;
				c->depth--;
				return n;
			}
			perr(c, (int) c->pos, "expected ',' or '}'");
			return NULL;
		}
	}
	case '[': {
		if (++c->depth > MAX_DEPTH) {
			perr(c, start, "JSON nests too deeply");
			return NULL;
		}
		n = vnew(c, V_LIST, start);
		if (n == NULL)
			return NULL;
		c->pos++;
		skip_ws(c, 0);
		if (c->pos < c->len && c->p[c->pos] == ']') {
			c->pos++;
			c->depth--;
			return n;
		}
		for (;;) {
			struct vnode *val = parse_json(c);
			if (val == NULL || vpush(c, n, val, NULL) != 0)
				return NULL;
			skip_ws(c, 0);
			if (c->pos < c->len && c->p[c->pos] == ',') {
				c->pos++;
				continue;
			}
			if (c->pos < c->len && c->p[c->pos] == ']') {
				c->pos++;
				c->depth--;
				return n;
			}
			perr(c, (int) c->pos, "expected ',' or ']'");
			return NULL;
		}
	}
	case '"':
		return parse_string(c, 1);
	case 't':
	case 'f':
	case 'n': {
		static const char *const lit[] = {"true", "false", "null"};
		int k;
		for (k = 0; k < 3; k++) {
			size_t l = strlen(lit[k]);
			if (c->p[c->pos] == lit[k][0] && c->len - c->pos >= l &&
			    memcmp(c->p + c->pos, lit[k], l) == 0) {
				c->pos += l;
				n = token_node(c, k == 2 ? V_NULL : V_BOOL, start, c->pos);
				return n;
			}
		}
		perr(c, start, "bad JSON literal");
		return NULL;
	}
	default:
		if (c->p[c->pos] == '-' || (c->p[c->pos] >= '0' && c->p[c->pos] <= '9')) {
			const char *dig;
			while (c->pos < c->len && (strchr("+-.eE", c->p[c->pos]) ||
			       (c->p[c->pos] >= '0' && c->p[c->pos] <= '9')))
				c->pos++;
			/* A second digit after a leading 0 rejects the JSON number. */
			dig = c->p + start;
			if (*dig == '+' || *dig == '-')
				dig++;
			if ((size_t) (dig - c->p) + 1 < c->pos && *dig == '0' &&
			    dig[1] >= '0' && dig[1] <= '9') {
				perr(c, (int) (dig - c->p), "unexpected character in JSON");
				return NULL;
			}
			return token_node(c, V_NUM, start, c->pos);
		}
		perr(c, start, "unexpected character in JSON");
		return NULL;
	}
}

/* The schema language's struct literal. */
static int is_ident_char(int ch)
{
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
	       (ch >= '0' && ch <= '9') || ch == '_';
}

static struct vnode *parse_text(struct pctx *c)
{
	struct vnode *n;
	int start;
	char ch;

	skip_ws(c, 1);
	if (c->pos >= c->len) {
		perr(c, (int) c->pos, "unexpected end of input");
		return NULL;
	}
	start = (int) c->pos;
	ch = c->p[c->pos];
	if (ch == '(') {
		if (++c->depth > MAX_DEPTH) {
			perr(c, start, "value nests too deeply");
			return NULL;
		}
		n = vnew(c, V_OBJ, start);
		if (n == NULL)
			return NULL;
		c->pos++;
		for (;;) {
			struct vnode *val;
			char *key;
			size_t ks;
			skip_ws(c, 1);
			if (c->pos < c->len && c->p[c->pos] == ')') {
				c->pos++;
				c->depth--;
				return n;
			}
			ks = c->pos;
			while (c->pos < c->len && is_ident_char(c->p[c->pos]))
				c->pos++;
			if (c->pos == ks) {
				perr(c, (int) c->pos, "expected a field name");
				return NULL;
			}
			key = (char *) malloc(c->pos - ks + 1);
			if (key == NULL) {
				perr(c, (int) ks, "out of memory");
				return NULL;
			}
			memcpy(key, c->p + ks, c->pos - ks);
			key[c->pos - ks] = '\0';
			skip_ws(c, 1);
			if (c->pos >= c->len || c->p[c->pos] != '=') {
				free(key);
				perr(c, (int) c->pos, "expected '='");
				return NULL;
			}
			c->pos++;
			val = parse_text(c);
			if (val == NULL || vpush(c, n, val, key) != 0) {
				free(key);
				return NULL;
			}
			skip_ws(c, 1);
			if (c->pos < c->len && c->p[c->pos] == ',') {
				c->pos++;
				continue;
			}
			if (c->pos < c->len && c->p[c->pos] == ')') {
				c->pos++;
				c->depth--;
				return n;
			}
			perr(c, (int) c->pos, "expected ',' or ')'");
			return NULL;
		}
	}
	if (ch == '[') {
		if (++c->depth > MAX_DEPTH) {
			perr(c, start, "value nests too deeply");
			return NULL;
		}
		n = vnew(c, V_LIST, start);
		if (n == NULL)
			return NULL;
		c->pos++;
		for (;;) {
			struct vnode *val;
			skip_ws(c, 1);
			if (c->pos < c->len && c->p[c->pos] == ']') {
				c->pos++;
				c->depth--;
				return n;
			}
			val = parse_text(c);
			if (val == NULL || vpush(c, n, val, NULL) != 0)
				return NULL;
			skip_ws(c, 1);
			if (c->pos < c->len && c->p[c->pos] == ',') {
				c->pos++;
				continue;
			}
			if (c->pos < c->len && c->p[c->pos] == ']') {
				c->pos++;
				c->depth--;
				return n;
			}
			perr(c, (int) c->pos, "expected ',' or ']'");
			return NULL;
		}
	}
	if (ch == '"')
		return parse_string(c, 0);
	if (ch == '0' && c->pos + 2 < c->len && (c->p[c->pos + 1] == 'x' || c->p[c->pos + 1] == 'X') &&
	    c->p[c->pos + 2] == '"') {
		/* 0x"de ad be ef": bytes as hex pairs. */
		char *out;
		size_t o = 0;
		c->pos += 3;
		out = (char *) malloc(c->len - c->pos + 1);
		if (out == NULL) {
			perr(c, start, "out of memory");
			return NULL;
		}
		while (c->pos < c->len && c->p[c->pos] != '"') {
			int hi, lo;
			if (c->p[c->pos] == ' ' || c->p[c->pos] == '\t' || c->p[c->pos] == '\n') {
				c->pos++;
				continue;
			}
			hi = hexval(c->p[c->pos]);
			lo = c->pos + 1 < c->len ? hexval(c->p[c->pos + 1]) : -1;
			if (hi < 0 || lo < 0) {
				free(out);
				perr(c, (int) c->pos, "bad hex data literal");
				return NULL;
			}
			out[o++] = (char) (hi * 16 + lo);
			c->pos += 2;
		}
		if (c->pos >= c->len) {
			free(out);
			perr(c, start, "unterminated data literal");
			return NULL;
		}
		c->pos++;
		n = vnew(c, V_STR, start);
		if (n == NULL) {
			free(out);
			return NULL;
		}
		out[o] = '\0';
		n->s = out;
		n->n = o;
		return n;
	}
	if (ch == '-' || ch == '+' || (ch >= '0' && ch <= '9')) {
		size_t b = c->pos;
		c->pos++;
		if (c->pos < c->len && is_ident_char(c->p[c->pos]) == 0 && c->p[c->pos] != '.')
			return token_node(c, V_NUM, start, c->pos);
		while (c->pos < c->len && (is_ident_char(c->p[c->pos]) || c->p[c->pos] == '.' ||
		       ((c->p[c->pos] == '-' || c->p[c->pos] == '+') &&
		        (c->p[c->pos - 1] == 'e' || c->p[c->pos - 1] == 'E') && c->p[b] != '0')))
			c->pos++;
		/* "-inf" and "inf" are names; every other run is a number. */
		if (c->pos - b >= 2 && c->p[b] == '-' && is_ident_char(c->p[b + 1]) &&
		    !(c->p[b + 1] >= '0' && c->p[b + 1] <= '9'))
			return token_node(c, V_IDENT, start, c->pos);
		return token_node(c, V_NUM, start, c->pos);
	}
	if (is_ident_char(ch)) {
		while (c->pos < c->len && is_ident_char(c->p[c->pos]))
			c->pos++;
		return token_node(c, V_IDENT, start, c->pos);
	}
	perr(c, start, "unexpected character");
	return NULL;
}

/* --- building a message from the tree ---------------------------------- */

struct bctx {
	struct capn_dyn_error *err;
	int failed;
	int json;
	/* Discriminant addresses whose JSON tag was present. A valueName
	 * with no tag leaves that union unset. */
	char **seen;
	int nseen, capseen;
};

static int berr(struct bctx *b, const struct vnode *at, const char *fmt, ...)
{
	va_list ap;
	if (!b->failed) {
		b->failed = 1;
		if (b->err) {
			va_start(ap, fmt);
			vsnprintf(b->err->message, sizeof b->err->message, fmt, ap);
			va_end(ap);
			b->err->offset = at ? at->off : -1;
		}
	}
	return -1;
}

static int str_is(const struct vnode *n, const char *lit)
{
	return n->s != NULL && strcmp(n->s, lit) == 0;
}

/* A JSON number is a double, then checked the way dynamic.c++ checks a
 * float-to-integer conversion: in range of the widened min and max, and
 * equal to that integer when read back as a double. */
static int json_number_as_int(struct bctx *b, const struct vnode *n, int kind,
                              uint64_t *bits)
{
	static const int width[] = {0, 0, 8, 16, 32, 64, 8, 16, 32, 64};
	int w = width[kind];
	int sgn = is_signed(kind);
	char *end;
	double d, minv, maxv;

	d = strtod(n->s, &end);
	if (end == n->s || *end != '\0')
		return berr(b, n, "expected an integer");
	if (sgn) {
		int64_t result;
		switch (w) {
		case 8: minv = (double) INT8_MIN; maxv = (double) INT8_MAX; break;
		case 16: minv = (double) INT16_MIN; maxv = (double) INT16_MAX; break;
		case 32: minv = (double) INT32_MIN; maxv = (double) INT32_MAX; break;
		default: minv = (double) INT64_MIN; maxv = (double) INT64_MAX; break;
		}
			/* (double)INT64_MAX is 2^63, so an inclusive compare lets that
			 * value through and the cast is out of range. */
			if (!(d >= minv) || !(w == 64 ? d < maxv : d <= maxv))
				return berr(b, n, "integer out of range");
			switch (w) {
			case 8: result = (int8_t) d; break;
			case 16: result = (int16_t) d; break;
			case 32: result = (int32_t) d; break;
			default: result = (int64_t) d; break;
			}
		if ((double) result != d)
			return berr(b, n, "integer out of range");
		*bits = (uint64_t) result;
	} else {
		uint64_t result;
		minv = 0;
		switch (w) {
		case 8: maxv = (double) UINT8_MAX; break;
		case 16: maxv = (double) UINT16_MAX; break;
		case 32: maxv = (double) UINT32_MAX; break;
		default: maxv = (double) UINT64_MAX; break;
		}
			/* (double)UINT64_MAX is 2^64, same hole as the signed bound. */
			if (!(d >= minv) || !(w == 64 ? d < maxv : d <= maxv))
				return berr(b, n, "integer out of range");
			switch (w) {
			case 8: result = (uint8_t) d; break;
			case 16: result = (uint16_t) d; break;
			case 32: result = (uint32_t) d; break;
			default: result = (uint64_t) d; break;
			}
		if ((double) result != d)
			return berr(b, n, "integer out of range");
		*bits = result;
	}
	if (w < 64)
		*bits &= (1ull << w) - 1;
	return 0;
}

static int parse_scalar(struct bctx *b, const struct capn_dyn_type *t,
                        const struct vnode *n, uint64_t *bits)
{
	int kind = t->kind;
	char *end;

	switch (kind) {
	case CAPN_DYN_BOOL:
		if (n->kind == V_BOOL || n->kind == V_IDENT) {
			if (str_is(n, "true")) { *bits = 1; return 0; }
			if (str_is(n, "false")) { *bits = 0; return 0; }
		}
		return berr(b, n, "expected true or false");
	case CAPN_DYN_INT8: case CAPN_DYN_INT16: case CAPN_DYN_INT32: case CAPN_DYN_INT64:
	case CAPN_DYN_UINT8: case CAPN_DYN_UINT16: case CAPN_DYN_UINT32: case CAPN_DYN_UINT64: {
		static const int width[] = {0, 0, 8, 16, 32, 64, 8, 16, 32, 64};
		int w = width[kind];
		int sgn = is_signed(kind);
		int neg;
		const char *s;
		unsigned long long mag;
		unsigned long long limit;

		if ((n->kind != V_NUM && n->kind != V_STR) || n->n == 0)
			return berr(b, n, "expected an integer");
		/* Text integers and JSON strings stay exact. A JSON number is a double. */
		if (b->json && n->kind == V_NUM)
			return json_number_as_int(b, n, kind, bits);
		s = n->s;
		neg = (*s == '-');
		if (*s == '-' || *s == '+')
			s++;
		if (!(*s >= '0' && *s <= '9'))
			return berr(b, n, "expected an integer");
		errno = 0;
		mag = strtoull(s, &end, (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) ? 16 : 10);
		if (*end != '\0' || errno == ERANGE)
			return berr(b, n, "integer out of range or malformed");
		limit = sgn ? (1ull << (w - 1)) : (w == 64 ? ~0ull : (1ull << w) - 1);
		if (neg) {
			if (!sgn || mag > limit)
				return berr(b, n, "integer out of range");
			*bits = (uint64_t) (0 - mag);
		} else {
			if (sgn ? mag > limit - 1 : mag > limit)
				return berr(b, n, "integer out of range");
			*bits = mag;
		}
		if (w < 64)
			*bits &= (1ull << w) - 1;
		return 0;
	}
	case CAPN_DYN_FLOAT32:
	case CAPN_DYN_FLOAT64: {
		double d;
		if (n->kind == V_NUM) {
			errno = 0;
			d = strtod(n->s, &end);
			if (*end != '\0')
				return berr(b, n, "expected a number");
		} else if (n->kind == V_STR || n->kind == V_IDENT) {
			if (str_is(n, "NaN") || str_is(n, "nan")) d = NAN;
			else if (str_is(n, "Infinity") || str_is(n, "inf")) d = INFINITY;
			else if (str_is(n, "-Infinity") || str_is(n, "-inf")) d = -INFINITY;
			else return berr(b, n, "expected a number");
		} else {
			return berr(b, n, "expected a number");
		}
		if (kind == CAPN_DYN_FLOAT32)
			*bits = capn_from_f32((float) d);
		else
			*bits = capn_from_f64(d);
		return 0;
	}
	case CAPN_DYN_ENUM: {
		int i;
		if ((n->kind == V_STR || n->kind == V_IDENT) && t->en != NULL) {
			for (i = 0; i < t->en->n; i++) {
				const char *jn = t->en->json_names != NULL ? t->en->json_names[i] : NULL;
				if (t->en->names[i] != NULL && strcmp(t->en->names[i], n->s) == 0) {
					*bits = (uint64_t) i;
					return 0;
				}
				if (jn != NULL && strcmp(jn, n->s) == 0) {
					*bits = (uint64_t) i;
					return 0;
				}
			}
			return berr(b, n, "no such enumerant: %s", n->s);
		}
		if (n->kind == V_NUM) {
			unsigned long v = strtoul(n->s, &end, 10);
			if (*end == '\0' && v < 65536) {
				*bits = v;
				return 0;
			}
		}
		return berr(b, n, "expected an enumerant name");
	}
	default:
		return berr(b, n, "not a scalar");
	}
}

static int fill_struct(struct bctx *b, const struct capn_dyn_struct *s,
                       const struct frame *fr, capn_ptr st, const struct vnode *obj);

static int build_data(struct bctx *b, struct capn_segment *seg, const struct vnode *n, capn_ptr *out)
{
	capn_list8 l;
	int i;

	if (n->kind == V_STR) {
		/* Text spells Data as a string or 0x"...". JSON accepts a string
		 * only through $Json.base64 and $Json.hex, which never reach here. */
		if (b->json)
			return berr(b, n, "expected data");
		if (n->n > 0x1fffffffu)
			return berr(b, n, "data too long");
		l = capn_new_list8(seg, (int) n->n);
		if (l.p.type == CAPN_NULL)
			return berr(b, n, "out of memory");
		if (n->n)
			capn_setv8(l, 0, (const uint8_t *) n->s, (int) n->n);
		*out = l.p;
		return 0;
	}
	if (n->kind == V_LIST) {
		l = capn_new_list8(seg, n->count);
		if (l.p.type == CAPN_NULL)
			return berr(b, n, "out of memory");
		for (i = 0; i < n->count; i++) {
			uint64_t v = 0;
			struct capn_dyn_type t = {CAPN_DYN_UINT8, NULL, NULL, NULL, NULL, 0, 0};
			if (parse_scalar(b, &t, n->v[i], &v) != 0)
				return -1;
			capn_set8(l, i, (uint8_t) v);
		}
		*out = l.p;
		return 0;
	}
	return berr(b, n, "expected data");
}

static int set_text_at(struct bctx *b, capn_ptr container, int off, const struct vnode *n)
{
	capn_text t;
	if (n->kind != V_STR)
		return berr(b, n, "expected text");
	if (n->n > (size_t) INT_MAX)
		return berr(b, n, "text too long");
	t.str = n->s;
	t.len = (int) n->n;
	t.seg = NULL;
	if (capn_set_text(container, off, t) != 0)
		return berr(b, n, "could not store text");
	return 0;
}

static int build_list(struct bctx *b, struct capn_segment *seg, const struct capn_dyn_type *lt,
                      const struct frame *fr, const struct vnode *n, capn_ptr *out)
{
	const struct frame *efr = fr;
	const struct capn_dyn_type *et = resolve(lt->elem, &efr);
	int i, cnt = n->count;
	capn_ptr list;

	if (n->kind != V_LIST)
		return berr(b, n, "expected a list");

	switch (et->kind) {
	case CAPN_DYN_VOID:
		list = capn_new_list(seg, cnt, 0, 0);
		break;
	case CAPN_DYN_BOOL: {
		capn_list1 l = capn_new_list1(seg, cnt);
		list = l.p;
		break;
	}
	case CAPN_DYN_INT8: case CAPN_DYN_UINT8: {
		capn_list8 l = capn_new_list8(seg, cnt);
		list = l.p;
		break;
	}
	case CAPN_DYN_INT16: case CAPN_DYN_UINT16: case CAPN_DYN_ENUM: {
		capn_list16 l = capn_new_list16(seg, cnt);
		list = l.p;
		break;
	}
	case CAPN_DYN_INT32: case CAPN_DYN_UINT32: case CAPN_DYN_FLOAT32: {
		capn_list32 l = capn_new_list32(seg, cnt);
		list = l.p;
		break;
	}
	case CAPN_DYN_INT64: case CAPN_DYN_UINT64: case CAPN_DYN_FLOAT64: {
		capn_list64 l = capn_new_list64(seg, cnt);
		list = l.p;
		break;
	}
	case CAPN_DYN_STRUCT:
		if (et->st == NULL)
			return berr(b, n, "list of a type without a descriptor");
		list = capn_new_struct_list(seg, cnt, et->st->data_words * 8, et->st->pointers);
		break;
	case CAPN_DYN_TEXT: case CAPN_DYN_DATA: case CAPN_DYN_LIST:
		list = capn_new_ptr_list(seg, cnt);
		break;
	default:
		return berr(b, n, "cannot build a list of this type");
	}
	if (list.type == CAPN_NULL)
		return berr(b, n, "could not allocate the list");

	for (i = 0; i < cnt; i++) {
		const struct vnode *el = n->v[i];
		uint64_t bits = 0;
		switch (et->kind) {
		case CAPN_DYN_VOID:
			break;
		case CAPN_DYN_BOOL: {
			capn_list1 l;
			l.p = list;
			if (parse_scalar(b, et, el, &bits) != 0)
				return -1;
			capn_set1(l, i, (int) bits);
			break;
		}
		case CAPN_DYN_INT8: case CAPN_DYN_UINT8: {
			capn_list8 l;
			l.p = list;
			if (parse_scalar(b, et, el, &bits) != 0)
				return -1;
			capn_set8(l, i, (uint8_t) bits);
			break;
		}
		case CAPN_DYN_INT16: case CAPN_DYN_UINT16: case CAPN_DYN_ENUM: {
			capn_list16 l;
			l.p = list;
			if (parse_scalar(b, et, el, &bits) != 0)
				return -1;
			capn_set16(l, i, (uint16_t) bits);
			break;
		}
		case CAPN_DYN_INT32: case CAPN_DYN_UINT32: case CAPN_DYN_FLOAT32: {
			capn_list32 l;
			l.p = list;
			if (parse_scalar(b, et, el, &bits) != 0)
				return -1;
			capn_set32(l, i, (uint32_t) bits);
			break;
		}
		case CAPN_DYN_INT64: case CAPN_DYN_UINT64: case CAPN_DYN_FLOAT64: {
			capn_list64 l;
			l.p = list;
			if (parse_scalar(b, et, el, &bits) != 0)
				return -1;
			capn_set64(l, i, bits);
			break;
		}
		case CAPN_DYN_TEXT:
			if (set_text_at(b, list, i, el) != 0)
				return -1;
			break;
		case CAPN_DYN_DATA: {
			capn_ptr d;
			if (build_data(b, seg, el, &d) != 0)
				return -1;
			capn_setp(list, i, d);
			break;
		}
		case CAPN_DYN_LIST: {
			capn_ptr sub;
			if (build_list(b, seg, et, efr, el, &sub) != 0)
				return -1;
			capn_setp(list, i, sub);
			break;
		}
		case CAPN_DYN_STRUCT: {
			struct frame nf;
			capn_ptr elt = capn_getp(list, i, 0);
			enter_struct(&nf, et, efr);
			if (fill_struct(b, et->st, &nf, elt, el) != 0)
				return -1;
			break;
		}
		}
	}
	*out = list;
	return 0;
}

static int b64_digit(int c)
{
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}

static int decode_base64(const char *s, size_t n, uint8_t **out, int *outn)
{
	size_t i = 0, o = 0;
	uint8_t *buf;
	if (n % 4 != 0)
		return -1;
	/* Output is a data list. A length past the list limit, or one
	 * that does not fit in the int the caller stores, is rejected. */
	if (n / 4 > (0x1fffffffu / 3) || n / 4 * 3 > (size_t) INT_MAX)
		return -1;
	buf = (uint8_t *) malloc(n / 4 * 3 + 1);
	if (buf == NULL)
		return -1;
	while (i < n) {
		int last = (i + 4 == n);
		int a, b, c, d;
		/* Padding is only `=` at the end of the last quartet, as
		 * `xx==` or `xxx=`. An earlier `=` or `AA=A` is not base64. */
		if (s[i] == '=' || s[i + 1] == '=' ||
		    (s[i + 2] == '=' && s[i + 3] != '=') ||
		    ((s[i + 2] == '=' || s[i + 3] == '=') && !last)) {
			free(buf);
			return -1;
		}
		a = b64_digit(s[i]);
		b = b64_digit(s[i + 1]);
		c = s[i + 2] == '=' ? 0 : b64_digit(s[i + 2]);
		d = s[i + 3] == '=' ? 0 : b64_digit(s[i + 3]);
		if (a < 0 || b < 0 || c < 0 || d < 0) {
			free(buf);
			return -1;
		}
		buf[o++] = (uint8_t) ((a << 2) | (b >> 4));
		if (s[i + 2] != '=')
			buf[o++] = (uint8_t) ((b << 4) | (c >> 2));
		if (s[i + 3] != '=')
			buf[o++] = (uint8_t) ((c << 6) | d);
		i += 4;
	}
	*out = buf;
	*outn = (int) o;
	return 0;
}

static int hex_digit(int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static int decode_hex(const char *s, size_t n, uint8_t **out, int *outn)
{
	size_t i;
	uint8_t *buf;
	if (n % 2 != 0)
		return -1;
	if (n / 2 > 0x1fffffffu || n / 2 > (size_t) INT_MAX)
		return -1;
	buf = (uint8_t *) malloc(n / 2 + 1);
	if (buf == NULL)
		return -1;
	for (i = 0; i < n; i += 2) {
		int hi = hex_digit(s[i]);
		int lo = hex_digit(s[i + 1]);
		if (hi < 0 || lo < 0) {
			free(buf);
			return -1;
		}
		buf[i / 2] = (uint8_t) ((hi << 4) | lo);
	}
	*out = buf;
	*outn = (int) (n / 2);
	return 0;
}

static int assign_field(struct bctx *b, const struct capn_dyn_struct *s, const struct frame *fr,
                        capn_ptr st, const struct capn_dyn_field *f,
                        const struct capn_dyn_type *rt, const struct frame *ffr,
                        const struct vnode *val)
{
	uint64_t bits = 0;
	(void) s;

	switch (rt->kind) {
	case CAPN_DYN_VOID:
		return 0;
	case CAPN_DYN_TEXT:
		if (val->kind == V_NULL) {
			if (b->json)
				return berr(b, val, "expected text");
			return 0;
		}
		return set_text_at(b, st, (int) f->offset, val);
	case CAPN_DYN_DATA: {
		capn_ptr d;
		if (val->kind == V_NULL) {
			if (b->json)
				return berr(b, val, "expected data");
			return 0;
		}
		if (b->json && f->json_data != 0) {
			uint8_t *raw = NULL;
			int rn = 0;
			capn_list8 l;
			if (val->kind != V_STR)
				return berr(b, val, "expected a string for %s", f->name);
			if (f->json_data == 1) {
				if (decode_base64(val->s, val->n, &raw, &rn) != 0)
					return berr(b, val, "invalid base64");
			} else if (decode_hex(val->s, val->n, &raw, &rn) != 0) {
				return berr(b, val, "invalid hex");
			}
			l = capn_new_list8(st.seg, rn);
			if (l.p.type == CAPN_NULL) {
				free(raw);
				return berr(b, val, "out of memory");
			}
			if (rn)
				capn_setv8(l, 0, raw, rn);
			free(raw);
			d = l.p;
		} else if (build_data(b, st.seg, val, &d) != 0) {
			return -1;
		}
		return capn_setp(st, (int) f->offset, d) ? berr(b, val, "could not store data") : 0;
	}
	case CAPN_DYN_LIST: {
		capn_ptr l;
		if (val->kind == V_NULL) {
			if (b->json)
				return berr(b, val, "expected a list");
			return 0;
		}
		if (build_list(b, st.seg, rt, ffr, val, &l) != 0)
			return -1;
		return capn_setp(st, (int) f->offset, l) ? berr(b, val, "could not store list") : 0;
	}
	case CAPN_DYN_STRUCT: {
		struct frame nf;
		capn_ptr sub;
		if (val->kind == V_NULL)
			return 0;
		if (rt->st == NULL)
			return berr(b, val, "field %s has no descriptor", f->name);
		sub = capn_new_struct(st.seg, rt->st->data_words * 8, rt->st->pointers);
		if (sub.type == CAPN_NULL)
			return berr(b, val, "could not allocate the struct");
		enter_struct(&nf, rt, ffr);
		if (fill_struct(b, rt->st, &nf, sub, val) != 0)
			return -1;
		return capn_setp(st, (int) f->offset, sub) ? berr(b, val, "could not store struct") : 0;
	}
	case CAPN_DYN_INTERFACE:
	case CAPN_DYN_ANYPOINTER:
	case CAPN_DYN_PARAM:
		return berr(b, val, "field %s cannot be set from text or JSON", f->name);
	default:
		if (parse_scalar(b, rt, val, &bits) != 0)
			return -1;
		(void) fr;
		return capn_dyn_set_bits(st, f, bits) ? berr(b, val, "field %s does not fit the struct", f->name) : 0;
	}
}

enum { J_FIELD = 0, J_TAG = 1, J_VALUE = 2 };

struct jstep {
	const struct capn_dyn_struct *owner;
	const struct capn_dyn_field *f;
};

struct jhit {
	struct jstep step[8];
	int n;
	int kind;
};

static int jfind(const struct capn_dyn_struct *s, const struct frame *fr,
                 const char *key, struct jhit *h, int depth)
{
	int i;
	if (s == NULL || depth > 8)
		return 0;
	if (s->json_has_disc && s->json_disc_name != NULL &&
	    strcmp(key, s->json_disc_name) == 0) {
		h->kind = J_TAG;
		return 1;
	}
	if (s->json_has_disc && s->json_disc_value != NULL &&
	    strcmp(key, s->json_disc_value) == 0) {
		h->kind = J_VALUE;
		return 1;
	}
	for (i = 0; i < s->nfields; i++) {
		const struct capn_dyn_field *f = &s->fields[i];
		const struct frame *ffr = fr;
		const struct capn_dyn_type *rt = resolve(&f->type, &ffr);
		if (f->json_flatten)
			continue;
		if (strcmp(json_key(f), key) == 0) {
			if (h->n >= 8)
				return 0;
			h->step[h->n].owner = s;
			h->step[h->n].f = f;
			h->n++;
			h->kind = J_FIELD;
			return 1;
		}
		(void) rt;
	}
	for (i = 0; i < s->nfields; i++) {
		const struct capn_dyn_field *f = &s->fields[i];
		const struct frame *ffr = fr;
		const struct capn_dyn_type *rt = resolve(&f->type, &ffr);
		const char *pre, *rest;
		size_t np;
		int saved;
		if (!f->json_flatten || rt->kind != CAPN_DYN_STRUCT || rt->st == NULL)
			continue;
		pre = f->json_prefix != NULL ? f->json_prefix : "";
		np = strlen(pre);
		if (np != 0 && strncmp(key, pre, np) != 0)
			continue;
		rest = key + np;
		if (h->n >= 8)
			return 0;
		h->step[h->n].owner = s;
		h->step[h->n].f = f;
		h->n++;
		saved = h->n;
		{
			struct frame nf;
			const struct frame *child_fr;
			if (f->is_group) {
				child_fr = fr;
			} else {
				enter_struct(&nf, rt, ffr);
				child_fr = &nf;
			}
			if (jfind(rt->st, child_fr, rest, h, depth + 1))
				return 1;
		}
		h->n = saved - 1;
	}
	return 0;
}

/* Walk flattened containers. *curs is the struct the remaining key
 * belongs to. */
static int jwalk(struct bctx *b, const struct vnode *at,
                 const struct capn_dyn_struct *s, const struct frame *fr, capn_ptr st,
                 const struct jhit *h, int ncont, struct frame *owned,
                 const struct capn_dyn_struct **curs, const struct frame **curf, capn_ptr *cur)
{
	int i;
	*curs = s;
	*curf = fr;
	*cur = st;
	for (i = 0; i < ncont; i++) {
		const struct jstep *step = &h->step[i];
		const struct frame *ffr = *curf;
		const struct capn_dyn_type *rt = resolve(&step->f->type, &ffr);
		if (step->f->discriminant != CAPN_DYN_NO_DISCRIMINANT &&
		    capn_dyn_set_which(step->owner, *cur, step->f) != 0)
			return berr(b, at, "cannot select %s", step->f->name);
		if (step->f->is_group) {
			*curs = rt->st;
			continue;
		}
		if (rt->st == NULL)
			return berr(b, at, "field %s has no descriptor", step->f->name);
		{
			capn_ptr sub = wire_ptr(*cur, step->f);
			if (sub.type == CAPN_NULL) {
				sub = capn_new_struct((*cur).seg, rt->st->data_words * 8, rt->st->pointers);
				if (sub.type == CAPN_NULL)
					return berr(b, at, "could not allocate the struct");
				if (capn_setp(*cur, (int) step->f->offset, sub))
					return berr(b, at, "could not store struct");
			}
			enter_struct(&owned[i], rt, ffr);
			*cur = sub;
			*curs = rt->st;
			*curf = &owned[i];
		}
	}
	return 0;
}

static char *union_disc_ptr(capn_ptr st, const struct capn_dyn_struct *s)
{
	capn_resolve(&st);
	if (s == NULL || s->union_count == 0 || st.type != CAPN_STRUCT || st.data == NULL)
		return NULL;
	return st.data + 2 * (int) s->union_offset;
}

static int union_seen_has(struct bctx *b, char *disc)
{
	int i;
	if (disc == NULL)
		return 0;
	for (i = 0; i < b->nseen; i++)
		if (b->seen[i] == disc)
			return 1;
	return 0;
}

static int union_seen_add(struct bctx *b, char *disc)
{
	int i;
	char **nbuf;
	int cap;
	if (disc == NULL)
		return 0;
	for (i = 0; i < b->nseen; i++)
		if (b->seen[i] == disc)
			return 0;
	if (b->nseen == b->capseen) {
		cap = b->capseen ? b->capseen * 2 : 4;
		nbuf = (char **) realloc(b->seen, (size_t) cap * sizeof *nbuf);
		if (nbuf == NULL)
			return -1;
		b->seen = nbuf;
		b->capseen = cap;
	}
	b->seen[b->nseen++] = disc;
	return 0;
}

static int japply_leaf(struct bctx *b, const struct capn_dyn_struct *s, const struct frame *fr,
                       capn_ptr st, const struct capn_dyn_field *f, const struct vnode *val)
{
	const struct frame *ffr = fr;
	const struct capn_dyn_type *rt = resolve(&f->type, &ffr);
	if (f->discriminant != CAPN_DYN_NO_DISCRIMINANT &&
	    capn_dyn_set_which(s, st, f) != 0)
		return berr(b, val, "cannot select %s", f->name);
	if (f->is_group) {
		if (rt->st == NULL)
			return berr(b, val, "group %s has no descriptor", f->name);
		return fill_struct(b, rt->st, fr, st, val);
	}
	return assign_field(b, s, fr, st, f, rt, ffr, val);
}

static int japply(struct bctx *b, const struct capn_dyn_struct *s, const struct frame *fr,
                  capn_ptr st, const struct jhit *h, const struct vnode *val)
{
	const struct capn_dyn_struct *curs;
	const struct frame *curf;
	struct frame owned[8];
	capn_ptr cur;
	int ncont = h->n;

	if (h->kind == J_FIELD)
		ncont = h->n - 1;
	if (jwalk(b, val, s, fr, st, h, ncont, owned, &curs, &curf, &cur) != 0)
		return -1;
	if (h->kind == J_TAG) {
		int i;
		const struct capn_dyn_field *mem = NULL;
		if (val->kind != V_STR)
			return berr(b, val, "discriminator must be a string");
		for (i = 0; i < curs->nfields; i++) {
			if (curs->fields[i].discriminant != CAPN_DYN_NO_DISCRIMINANT &&
			    strcmp(json_key(&curs->fields[i]), val->s) == 0) {
				mem = &curs->fields[i];
				break;
			}
		}
		if (mem == NULL)
			return berr(b, val, "no union member named %s", val->s);
		if (capn_dyn_set_which(curs, cur, mem) != 0)
			return berr(b, val, "cannot select %s", mem->name);
		if (union_seen_add(b, union_disc_ptr(cur, curs)) != 0)
			return berr(b, val, "out of memory");
		return 0;
	}
	if (h->kind == J_VALUE) {
		const struct capn_dyn_field *mem;
		/* The tag is applied in an earlier pass. With no tag, the
		 * value is left unread and the union stays at its wire default. */
		if (!union_seen_has(b, union_disc_ptr(cur, curs)))
			return 0;
		mem = active_union_field(curs, cur);
		if (mem == NULL)
			return berr(b, val, "union value without a selected member");
		return japply_leaf(b, curs, curf, cur, mem, val);
	}
	return japply_leaf(b, h->step[h->n - 1].owner, curf, cur, h->step[h->n - 1].f, val);
}

static int fill_struct(struct bctx *b, const struct capn_dyn_struct *s,
                       const struct frame *fr, capn_ptr st, const struct vnode *obj)
{
	int i;

	if (obj->kind != V_OBJ)
		return berr(b, obj, "expected an object for %s", s->name);
	if (!b->json) {
		for (i = 0; i < obj->count; i++) {
			const struct capn_dyn_field *f = capn_dyn_find_field(s, obj->k[i]);
			const struct frame *ffr = fr;
			const struct capn_dyn_type *rt;

			if (f == NULL)
				return berr(b, obj->v[i], "no field named %s in %s", obj->k[i], s->name);
			if (f->discriminant != CAPN_DYN_NO_DISCRIMINANT &&
			    capn_dyn_set_which(s, st, f) != 0)
				return berr(b, obj->v[i], "cannot select %s", f->name);
			rt = resolve(&f->type, &ffr);
			if (f->is_group) {
				if (rt->st == NULL)
					return berr(b, obj->v[i], "group %s has no descriptor", f->name);
				if (fill_struct(b, rt->st, fr, st, obj->v[i]) != 0)
					return -1;
			} else if (assign_field(b, s, fr, st, f, rt, ffr, obj->v[i]) != 0) {
				return -1;
			}
		}
		return 0;
	}

	for (i = 0; i < obj->count; i++) {
		struct jhit h;
		memset(&h, 0, sizeof h);
		if (!jfind(s, fr, obj->k[i], &h, 0))
			continue;
		if (h.kind == J_TAG && japply(b, s, fr, st, &h, obj->v[i]) != 0)
			return -1;
	}
	for (i = 0; i < obj->count; i++) {
		struct jhit h;
		memset(&h, 0, sizeof h);
		if (!jfind(s, fr, obj->k[i], &h, 0))
			continue;
		if (h.kind == J_TAG)
			continue;
		if (japply(b, s, fr, st, &h, obj->v[i]) != 0)
			return -1;
	}
	return 0;
}

static int decode(const struct capn_dyn_struct *s, const char *in, size_t len, int json,
                  struct capn_segment *seg, capn_ptr *out, struct capn_dyn_error *err)
{
	struct pctx c;
	struct bctx b;
	struct vnode *root;
	capn_ptr st;
	int rc = -1;

	memset(&c, 0, sizeof c);
	memset(&b, 0, sizeof b);
	b.json = json;
	c.err = err;
	b.err = err;
	if (err) {
		err->message[0] = '\0';
		err->offset = -1;
	}
	if (s == NULL || seg == NULL || out == NULL || (in == NULL && len > 0)) {
		perr(&c, -1, "bad arguments");
		return -1;
	}
	if (in == NULL)
		in = "";
	c.p = in;
	c.len = len;

	root = json ? parse_json(&c) : parse_text(&c);
	if (root != NULL) {
		skip_ws(&c, !json);
		if (c.pos != c.len) {
			perr(&c, (int) c.pos, "trailing characters after the value");
			root = NULL;
		}
	}
	if (root == NULL)
		goto done;

	st = capn_new_struct(seg, s->data_words * 8, s->pointers);
	if (st.type == CAPN_NULL) {
		berr(&b, root, "could not allocate the struct");
		goto done;
	}
	if (fill_struct(&b, s, NULL, st, root) != 0)
		goto done;
	*out = st;
	rc = 0;
done:
	free(b.seen);
	vfree_all(&c);
	return rc;
}

int capn_dyn_from_json(const struct capn_dyn_struct *s, const char *json,
                       size_t len, struct capn_segment *seg, capn_ptr *out,
                       struct capn_dyn_error *err)
{
	return decode(s, json, len, 1, seg, out, err);
}

int capn_dyn_from_text(const struct capn_dyn_struct *s, const char *text,
                       size_t len, struct capn_segment *seg, capn_ptr *out,
                       struct capn_dyn_error *err)
{
	return decode(s, text, len, 0, seg, out, err);
}
