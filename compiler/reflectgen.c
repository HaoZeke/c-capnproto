/* reflectgen.c - schema descriptor tables for $C.reflect
 *
 * For every struct, group and enum in a file, emit a const descriptor the
 * runtime in lib/capn-dyn.c walks to read, print and build messages
 * without generated per-type code. Descriptors hold names, types, slot
 * offsets, union discriminants, scalar defaults and the bytes of struct,
 * list, text and data defaults. An unset pointer still counts as absent
 * when printing, which is what capnp decode does.
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codecgen.h"
#include "reflectgen.h"

static int g_tcount;
static int g_dcount;

static struct node *file_of(struct node *n) {
	while (n && n->n.which != Node_file && n->n.scopeId) {
		n = find_node(n->n.scopeId);
	}
	return n;
}

int reflect_file_enabled(struct node *file) {
	int i;
	if (file == NULL)
		return 0;
	for (i = 0; i < capn_len(file->n.annotations); i++) {
		struct Annotation a;
		get_Annotation(&a, file->n.annotations, i);
		if (a.id == ANNOTATION_REFLECT)
			return 1;
	}
	return 0;
}

#define JSON_NAME_ID 0xfa5b1fd61c2e7c3dull
#define JSON_FLATTEN_ID 0x82d3e852af0336bfull
#define JSON_DISC_ID 0xcfa794e8d19a0162ull
#define JSON_BASE64_ID 0xd7d879450a253e4bull
#define JSON_HEX_ID 0xf061e22f0ae5c7b5ull

static int find_anno(Annotation_list list, uint64_t id, struct Annotation *a)
{
	int i, n = capn_len(list);
	for (i = 0; i < n; i++) {
		get_Annotation(a, list, i);
		if (a->id == id)
			return 1;
	}
	return 0;
}

/* 1 when the annotation value is Text and not empty. */
static int anno_text(struct Annotation *a, capn_text *out)
{
	struct Value v;
	memset(&v, 0, sizeof v);
	read_Value(&v, a->value);
	if (v.which != Value_text || v.text.str == NULL || v.text.len <= 0)
		return 0;
	*out = v.text;
	return 1;
}

/* Annotation value is a struct. *set is 1 when pointer `idx` is present. */
static int anno_field_text(struct Annotation *a, int idx, capn_text *out, int *set)
{
	struct Value v;
	capn_ptr p;
	capn_text def = {0, "", NULL};
	memset(&v, 0, sizeof v);
	read_Value(&v, a->value);
	*set = 0;
	if (v.which != Value__struct)
		return 0;
	p = capn_getp(v._struct, idx, 1);
	if (p.type == CAPN_NULL)
		return 1;
	*out = capn_get_text(v._struct, idx, def);
	*set = out->str != NULL;
	return 1;
}

static struct field *group_field_in(struct node *scope, uint64_t gid)
{
	int i, nf;
	if (scope == NULL || scope->n.which != Node__struct)
		return NULL;
	nf = capn_len(scope->n._struct.fields);
	for (i = 0; i < nf; i++) {
		struct field *f = &scope->fields[i];
		struct field *inner;
		if (f->f.which == Field_group && f->f.group.typeId == gid)
			return f;
		inner = group_field_in(f->group, gid);
		if (inner)
			return inner;
	}
	return NULL;
}

static struct field *find_group_field(struct node *g)
{
	struct node *file = file_of(g);
	struct node *n;
	if (file == NULL)
		return NULL;
	for (n = file->file_nodes; n != NULL; n = n->next_file_node) {
		struct field *f = group_field_in(n, g->n.id);
		if (f)
			return f;
	}
	return NULL;
}

static void add_cstr(struct str *s, const char *p, int len) {
	int i;
	str_addf(s, "\"");
	for (i = 0; i < len; i++) {
		unsigned char c = (unsigned char) p[i];
		if (c == '"' || c == '\\') {
			str_addf(s, "\\%c", c);
		} else if (c >= 0x20 && c < 0x7f && c != '?') {
			str_addf(s, "%c", c);
		} else {
			/* three octal digits cannot swallow a following digit */
			str_addf(s, "\\%03o", c);
		}
	}
	str_addf(s, "\"");
}

static const char *kind_name(enum Type_which w) {
	switch (w) {
	case Type__void: return "CAPN_DYN_VOID";
	case Type__bool: return "CAPN_DYN_BOOL";
	case Type_int8: return "CAPN_DYN_INT8";
	case Type_int16: return "CAPN_DYN_INT16";
	case Type_int32: return "CAPN_DYN_INT32";
	case Type_int64: return "CAPN_DYN_INT64";
	case Type_uint8: return "CAPN_DYN_UINT8";
	case Type_uint16: return "CAPN_DYN_UINT16";
	case Type_uint32: return "CAPN_DYN_UINT32";
	case Type_uint64: return "CAPN_DYN_UINT64";
	case Type_float32: return "CAPN_DYN_FLOAT32";
	case Type_float64: return "CAPN_DYN_FLOAT64";
	case Type_text: return "CAPN_DYN_TEXT";
	case Type_data: return "CAPN_DYN_DATA";
	case Type__list: return "CAPN_DYN_LIST";
	case Type__enum: return "CAPN_DYN_ENUM";
	case Type__struct: return "CAPN_DYN_STRUCT";
	case Type__interface: return "CAPN_DYN_INTERFACE";
	default: return "CAPN_DYN_ANYPOINTER";
	}
}

/* The C expression naming a struct descriptor, or NULL when that type's
 * file does not carry $C.reflect and so has no descriptor to link to. */
static const char *struct_ref(struct str *buf, uint64_t id) {
	struct node *n = find_node(id);
	struct node *file = file_of(n);
	if (!reflect_file_enabled(file))
		return "NULL";
	if (n->n._struct.isGroup)
		return strf(buf, "&capn_dyn_g%" PRIx64, id);
	return strf(buf, "&%s_dyn", n->name.str);
}

static const char *enum_ref(struct str *buf, uint64_t id) {
	struct node *n = find_node(id);
	if (!reflect_file_enabled(file_of(n)))
		return "NULL";
	return strf(buf, "&%s_dyn", n->name.str);
}

static void emit_type(struct str *pre, struct str *lit, Type_ptr tp);

/* Emit the binding tables for a brand and return the name of the brand
 * object, or "NULL" when the type is not parameterised. */
static const char *emit_brand(struct str *pre, struct str *name, Brand_ptr bp) {
	struct Brand br;
	int i, ns;

	if (bp.p.type == CAPN_NULL)
		return "NULL";
	read_Brand(&br, bp);
	ns = capn_len(br.scopes);
	if (ns == 0)
		return "NULL";

	{
		struct str scopes = STR_INIT;
		for (i = 0; i < ns; i++) {
			struct Brand_Scope sc;
			struct str params = STR_INIT;
			int nb = 0, j;
			int n = ++g_tcount;

			get_Brand_Scope(&sc, br.scopes, i);
			if (sc.which == Brand_Scope_bind) {
				nb = capn_len(sc.bind);
				for (j = 0; j < nb; j++) {
					struct Brand_Binding bd;
					struct str lit = STR_INIT;
					get_Brand_Binding(&bd, sc.bind, j);
					if (bd.which == Brand_Binding_type)
						emit_type(pre, &lit, bd.type);
					else
						str_addf(&lit, "{CAPN_DYN_ANYPOINTER, NULL, NULL, NULL, NULL, 0ull, 0u}");
					str_addf(&params, "\t%s,\n", lit.str);
					str_release(&lit);
				}
				if (nb)
					str_addf(pre, "static const struct capn_dyn_type capn_dyn_bp%d[] = {\n%s};\n", n, params.str);
			}
			str_addf(&scopes, "\t{0x%" PRIx64 "ull, %d, %d, %s},\n", sc.scopeId,
					sc.which == Brand_Scope_inherit, nb, nb ? strf(&params, "capn_dyn_bp%d", n) : "NULL");
			str_release(&params);
		}
		{
			int n = ++g_tcount;
			str_addf(pre, "static const struct capn_dyn_scope capn_dyn_bs%d[] = {\n%s};\n", n, scopes.str);
			str_addf(pre, "static const struct capn_dyn_brand capn_dyn_b%d = {%d, capn_dyn_bs%d};\n", n, ns, n);
			strf(name, "&capn_dyn_b%d", n);
		}
		str_release(&scopes);
	}
	return name->str;
}

/* Emit a type literal into `lit`, and any tables it points at into `pre`. */
static void emit_type(struct str *pre, struct str *lit, Type_ptr tp) {
	struct Type t;
	struct str a = STR_INIT, b = STR_INIT, br = STR_INIT;
	const char *st = "NULL", *en = "NULL", *elem = "NULL", *brand = "NULL";
	const char *kind;
	uint64_t scope = 0;
	unsigned index = 0;

	read_Type(&t, tp);
	kind = kind_name(t.which);
	switch (t.which) {
	case Type__struct:
		st = struct_ref(&a, t._struct.typeId);
		brand = emit_brand(pre, &br, t._struct.brand);
		break;
	case Type__enum:
		en = enum_ref(&b, t._enum.typeId);
		break;
	case Type__list: {
		struct str inner = STR_INIT;
		struct str name = STR_INIT;
		emit_type(pre, &inner, t._list.elementType);
		strf(&name, "capn_dyn_t%d", ++g_tcount);
		str_addf(pre, "static const struct capn_dyn_type %s = %s;\n", name.str, inner.str);
		elem = strf(&b, "&%s", name.str);
		str_release(&inner);
		str_release(&name);
		break;
	}
	case Type_anyPointer:
		if (t.anyPointer.which == Type_anyPointer_parameter) {
			kind = "CAPN_DYN_PARAM";
			scope = t.anyPointer.parameter.scopeId;
			index = t.anyPointer.parameter.parameterIndex;
		}
		break;
	default:
		break;
	}
	str_addf(lit, "{%s, %s, %s, %s, %s, 0x%" PRIx64 "ull, %uu}", kind, st, en, elem, brand, scope, index);
	str_release(&a);
	str_release(&b);
	str_release(&br);
}

static uint64_t default_bits(struct Type *t, struct Value *v) {
	switch (t->which) {
	case Type__bool: return v->_bool ? 1 : 0;
	case Type_int8: return (uint8_t) v->int8;
	case Type_uint8: return v->uint8;
	case Type_int16: return (uint16_t) v->int16;
	case Type_uint16: return v->uint16;
	case Type__enum: return v->_enum;
	case Type_int32: return (uint32_t) v->int32;
	case Type_uint32: return v->uint32;
	case Type_float32: return capn_from_f32(v->float32);
	case Type_int64: return (uint64_t) v->int64;
	case Type_uint64: return v->uint64;
	case Type_float64: return capn_from_f64(v->float64);
	default: return 0;
	}
}

/* Copy a pointer default into its own message and emit a capn_ptr that
 * addresses those bytes. expr receives "&capn_dyn_dpN" or "NULL". */
static void emit_default_ptr(struct str *pre, struct str *expr, int is_text,
                             capn_text text, capn_ptr src)
{
	struct capn dst;
	capn_ptr root, out;
	int n, i, off;

	str_reset(expr);
	str_addf(expr, "NULL");
	memset(&dst, 0, sizeof dst);
	capn_init_malloc(&dst);
	root = capn_root(&dst);
	if (is_text) {
		if (capn_set_text(root, 0, text))
			goto done;
	} else if (capn_setp(root, 0, src)) {
		goto done;
	}
	out = capn_getp(root, 0, 1);
	if (out.type == CAPN_NULL)
		goto done;

	n = ++g_dcount;
	if ((out.type == CAPN_LIST || out.type == CAPN_PTR_LIST || out.type == CAPN_BIT_LIST) &&
	    out.len == 0 && out.data == NULL) {
		str_reset(expr);
		str_addf(pre, "static const capn_ptr capn_dyn_dp%d = {%u,0,0,%u,%u,%u,0,NULL,NULL,0,0};\n",
		         n, out.type, out.is_composite_list, out.datasz, out.ptrs);
		str_addf(expr, "&capn_dyn_dp%d", n);
		goto done;
	}
	if (out.seg == NULL || out.data == NULL || out.seg->data == NULL)
		goto done;
	off = (int) (out.data - out.seg->data);
	if (off < 0 || (size_t) off > out.seg->len)
		goto done;

	str_reset(expr);
	str_addf(pre, "static const uint8_t capn_dyn_db%d[] = {", n);
	for (i = 0; i < (int) out.seg->len; i++) {
		if (i)
			str_addf(pre, ",");
		str_addf(pre, "%u", (unsigned char) out.seg->data[i]);
	}
	str_addf(pre, "};\n");
	str_addf(pre,
	         "static struct capn_segment capn_dyn_ds%d = {{0},0,0,0,(char*)capn_dyn_db%d,%zu,%zu,0};\n",
	         n, n, out.seg->len, out.seg->len);
	str_addf(pre,
	         "static const capn_ptr capn_dyn_dp%d = {%u,%u,%u,%u,%u,%u,%d,(char*)capn_dyn_db%d+%d,&capn_dyn_ds%d,0,0};\n",
	         n, out.type, out.has_ptr_tag, out.is_list_member, out.is_composite_list,
	         out.datasz, out.ptrs, out.len, n, off, n);
	str_addf(expr, "&capn_dyn_dp%d", n);
done:
	capn_free(&dst);
}

static void emit_struct_forward(struct str *out, struct node *n, int group) {
	if (group) {
		str_addf(out, "static const struct capn_dyn_struct capn_dyn_g%" PRIx64 ";\n", n->n.id);
	}
}

static void emit_struct(struct str *src, struct node *n, int group) {
	struct str pre = STR_INIT;
	struct str fields = STR_INIT;
	int i, nf = capn_len(n->n._struct.fields);
	const char *name = n->n.displayName.str + n->n.displayNamePrefixLength;

	for (i = 0; i < nf; i++) {
		struct field *f = &n->fields[i];
		struct str lit = STR_INIT;
		struct str tb = STR_INIT;
		const char *dtext = "NULL";
		struct str dtbuf = STR_INIT;
		struct str dexpr = STR_INIT;
		struct str jnbuf = STR_INIT, jpbuf = STR_INIT;
		const char *jname = "NULL";
		const char *jprefix = "NULL";
		int jflatten = 0, jdata = 0;
		uint64_t bits = 0;
		int dlen = 0;
		unsigned is_group = 0;
		uint32_t offset = 0;
		struct Annotation ann;
		capn_text jt;

		if (f->f.which == Field_group) {
			is_group = 1;
			strf(&tb, "&capn_dyn_g%" PRIx64, f->f.group.typeId);
			str_addf(&lit, "{CAPN_DYN_STRUCT, %s, NULL, NULL, NULL, 0ull, 0u}", tb.str);
		} else {
			struct Type t;
			struct Value v;
			emit_type(&pre, &lit, f->f.slot.type);
			read_Type(&t, f->f.slot.type);
			read_Value(&v, f->f.slot.defaultValue);
			bits = default_bits(&t, &v);
			offset = f->f.slot.offset;
			str_addf(&dexpr, "NULL");
			if (t.which == Type_text && v.which == Value_text && v.text.len > 0) {
				capn_ptr none = {CAPN_NULL};
				add_cstr(&dtbuf, v.text.str, v.text.len);
				dtext = dtbuf.str;
				dlen = v.text.len;
				emit_default_ptr(&pre, &dexpr, 1, v.text, none);
			} else if (t.which == Type_data && v.which == Value_data && v.data.p.type != CAPN_NULL) {
				capn_text none = {0, "", NULL};
				emit_default_ptr(&pre, &dexpr, 0, none, v.data.p);
			} else if (t.which == Type__list && v.which == Value__list && v._list.type != CAPN_NULL) {
				capn_text none = {0, "", NULL};
				emit_default_ptr(&pre, &dexpr, 0, none, v._list);
			} else if (t.which == Type__struct && v.which == Value__struct && v._struct.type != CAPN_NULL) {
				capn_text none = {0, "", NULL};
				emit_default_ptr(&pre, &dexpr, 0, none, v._struct);
			}
		}

		if (find_anno(f->f.annotations, JSON_NAME_ID, &ann) && anno_text(&ann, &jt)) {
			add_cstr(&jnbuf, jt.str, jt.len);
			jname = jnbuf.str;
		}
		if (find_anno(f->f.annotations, JSON_FLATTEN_ID, &ann)) {
			int set = 0;
			jflatten = 1;
			if (anno_field_text(&ann, 0, &jt, &set) && set && jt.len > 0) {
				add_cstr(&jpbuf, jt.str, jt.len);
				jprefix = jpbuf.str;
			}
		}
		if (find_anno(f->f.annotations, JSON_BASE64_ID, &ann))
			jdata = 1;
		else if (find_anno(f->f.annotations, JSON_HEX_ID, &ann))
			jdata = 2;

		str_addf(&fields, "\t{");
		add_cstr(&fields, f->f.name.str, f->f.name.len);
		str_addf(&fields, ", %s, %" PRIu32 "u, %uu, %u, 0x%" PRIx64 "ull, %s, %d, %s, %d, %s, %d, %s},\n",
				lit.str, offset, (unsigned) f->f.discriminantValue, is_group, bits, dtext, dlen,
				jname, jflatten, jprefix, jdata, dexpr.str ? dexpr.str : "NULL");
		str_release(&lit);
		str_release(&tb);
		str_release(&dtbuf);
		str_release(&dexpr);
		str_release(&jnbuf);
		str_release(&jpbuf);
	}

	if (nf == 0) {
		/* ISO C has no empty initialiser list; keep one inert entry that
		 * nfields excludes. */
		str_addf(&fields, "\t{\"\", {CAPN_DYN_VOID, NULL, NULL, NULL, NULL, 0ull, 0u}, 0u, 0xffffu, 0, 0ull, NULL, 0, NULL, 0, NULL, 0, NULL}\n");
	}

	str_addf(src, "%s", pre.str);
	str_addf(src, "static const struct capn_dyn_field capn_dyn_f%" PRIx64 "[] = {\n%s};\n", n->n.id, fields.str);
	if (group) {
		str_addf(src, "static const struct capn_dyn_struct capn_dyn_g%" PRIx64 " = {", n->n.id);
	} else {
		str_addf(src, "const struct capn_dyn_struct %s_dyn = {", n->name.str);
	}
	add_cstr(src, name, (int) strlen(name));
	{
		struct field *gf = group ? find_group_field(n) : NULL;
		struct Annotation ann;
		struct str dname = STR_INIT, dval = STR_INIT;
		const char *dn = "NULL", *dv = "NULL";
		int has_disc = 0;
		capn_text jt;
		int set = 0;

		if (gf && find_anno(gf->f.annotations, JSON_DISC_ID, &ann))
			has_disc = 1;
		else if (find_anno(n->n.annotations, JSON_DISC_ID, &ann))
			has_disc = 1;
		if (has_disc) {
			if (anno_field_text(&ann, 0, &jt, &set) && set && jt.len > 0) {
				add_cstr(&dname, jt.str, jt.len);
				dn = dname.str;
			} else if (gf && find_anno(gf->f.annotations, JSON_FLATTEN_ID, &ann)) {
				add_cstr(&dname, gf->f.name.str, gf->f.name.len);
				dn = dname.str;
			}
			if (gf)
				find_anno(gf->f.annotations, JSON_DISC_ID, &ann);
			else
				find_anno(n->n.annotations, JSON_DISC_ID, &ann);
			set = 0;
			if (anno_field_text(&ann, 1, &jt, &set) && set && jt.len > 0) {
				add_cstr(&dval, jt.str, jt.len);
				dv = dval.str;
			}
		}
		str_addf(src, ", 0x%" PRIx64 "ull, %u, %u, %u, %" PRIu32 "u, %d, capn_dyn_f%" PRIx64 ", %s, %s, %d};\n\n",
				n->n.id, (unsigned) n->n._struct.dataWordCount, (unsigned) n->n._struct.pointerCount,
				(unsigned) n->n._struct.discriminantCount, (uint32_t) n->n._struct.discriminantOffset,
				nf, n->n.id, dn, dv, has_disc);
		str_release(&dname);
		str_release(&dval);
	}
	str_release(&pre);
	str_release(&fields);
}

static void emit_enum(struct str *src, struct node *n) {
	int i, ne = capn_len(n->n._enum.enumerants);
	int any_json = 0;
	const char *name = n->n.displayName.str + n->n.displayNamePrefixLength;
	struct str jnames = STR_INIT;

	str_addf(src, "static const char *const capn_dyn_e%" PRIx64 "[] = {\n", n->n.id);
	for (i = 0; i < ne; i++) {
		struct Enumerant e;
		struct Annotation ann;
		capn_text jt;
		get_Enumerant(&e, n->n._enum.enumerants, i);
		str_addf(src, "\t");
		add_cstr(src, e.name.str, e.name.len);
		str_addf(src, ",\n");
		str_addf(&jnames, "\t");
		if (find_anno(e.annotations, JSON_NAME_ID, &ann) && anno_text(&ann, &jt)) {
			add_cstr(&jnames, jt.str, jt.len);
			any_json = 1;
		} else {
			str_addf(&jnames, "NULL");
		}
		str_addf(&jnames, ",\n");
	}
	if (ne == 0)
		str_addf(src, "\tNULL\n");
	str_addf(src, "};\n");
	if (any_json) {
		str_addf(src, "static const char *const capn_dyn_ej%" PRIx64 "[] = {\n%s};\n",
		         n->n.id, jnames.str);
	}
	str_addf(src, "const struct capn_dyn_enum %s_dyn = {", n->name.str);
	add_cstr(src, name, (int) strlen(name));
	if (any_json)
		str_addf(src, ", 0x%" PRIx64 "ull, %d, capn_dyn_e%" PRIx64 ", capn_dyn_ej%" PRIx64 "};\n\n",
		         n->n.id, ne, n->n.id, n->n.id);
	else
		str_addf(src, ", 0x%" PRIx64 "ull, %d, capn_dyn_e%" PRIx64 ", NULL};\n\n",
		         n->n.id, ne, n->n.id);
	str_release(&jnames);
}

static void collect_groups(struct node *n, struct node ***list, int *count, int *cap) {
	int i, nf = capn_len(n->n._struct.fields);
	for (i = 0; i < nf; i++) {
		if (n->fields[i].group) {
			struct node *g = n->fields[i].group;
			if (*count == *cap) {
				*cap = *cap ? *cap * 2 : 16;
				*list = realloc(*list, *cap * sizeof(**list));
			}
			(*list)[(*count)++] = g;
			collect_groups(g, list, count, cap);
		}
	}
}

void declare_reflect(capnp_ctx_t *ctx, struct node *file_node) {
	struct node *n;
	str_addf(ctx->HDR, "\n");
	for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
		if (n->n.which == Node__struct && !n->n._struct.isGroup) {
			str_addf(ctx->HDR, "extern const struct capn_dyn_struct %s_dyn;\n", n->name.str);
		} else if (n->n.which == Node__enum) {
			str_addf(ctx->HDR, "extern const struct capn_dyn_enum %s_dyn;\n", n->name.str);
		}
	}
}

void define_reflect(capnp_ctx_t *ctx, struct node *file_node) {
	struct node *n;
	struct node **groups = NULL;
	int ngroups = 0, cap = 0, i;

	g_tcount = 0;
	g_dcount = 0;

	for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
		if (n->n.which == Node__struct && !n->n._struct.isGroup)
			collect_groups(n, &groups, &ngroups, &cap);
	}

	/* Everything refers to everything, so declare before defining. */
	for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
		if (n->n.which == Node__struct && !n->n._struct.isGroup)
			str_addf(ctx->SRC, "extern const struct capn_dyn_struct %s_dyn;\n", n->name.str);
		else if (n->n.which == Node__enum)
			str_addf(ctx->SRC, "extern const struct capn_dyn_enum %s_dyn;\n", n->name.str);
	}
	for (i = 0; i < ngroups; i++)
		emit_struct_forward(ctx->SRC, groups[i], 1);
	str_addf(ctx->SRC, "\n");

	for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
		if (n->n.which == Node__enum)
			emit_enum(ctx->SRC, n);
	}
	for (i = 0; i < ngroups; i++)
		emit_struct(ctx->SRC, groups[i], 1);
	for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
		if (n->n.which == Node__struct && !n->n._struct.isGroup)
			emit_struct(ctx->SRC, n, 0);
	}
	free(groups);
}
