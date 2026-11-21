/* capn-dyn.h - reflection, text and JSON for messages of any schema.
 *
 * capnpc-c emits a descriptor per struct and enum, named <Name>_dyn,
 * when the schema file carries `$C.reflect;`. The functions here read
 * and build messages from those descriptors alone, which is what a
 * generic tool (a pretty printer, a JSON bridge, a debugger) needs and
 * what C++ offers as DynamicStruct, the text format and JsonCodec.
 *
 * Output matches the official tool: capn_dyn_to_text is the single-line
 * form of `capnp decode`, and capn_dyn_to_json the form of
 * `capnp convert binary:json`. Both decoders accept what those tools
 * accept for the types they cover.
 *
 * Not covered: Interface and AnyPointer fields are opaque (printed as
 * a placeholder, never decoded). Fields typed by a generic parameter are
 * AnyPointer until a brand binds them. Pointer defaults are returned by
 * capn_dyn_get_ptr and capn_dyn_get_text; capn_dyn_has stays false when
 * the wire pointer is null.
 */

#ifndef CAPN_DYN_H
#define CAPN_DYN_H

#include "capnp_c.h"

#ifdef __cplusplus
extern "C" {
#endif

enum capn_dyn_kind {
	CAPN_DYN_VOID,
	CAPN_DYN_BOOL,
	CAPN_DYN_INT8,
	CAPN_DYN_INT16,
	CAPN_DYN_INT32,
	CAPN_DYN_INT64,
	CAPN_DYN_UINT8,
	CAPN_DYN_UINT16,
	CAPN_DYN_UINT32,
	CAPN_DYN_UINT64,
	CAPN_DYN_FLOAT32,
	CAPN_DYN_FLOAT64,
	CAPN_DYN_TEXT,
	CAPN_DYN_DATA,
	CAPN_DYN_LIST,
	CAPN_DYN_ENUM,
	CAPN_DYN_STRUCT,
	CAPN_DYN_INTERFACE,
	CAPN_DYN_ANYPOINTER,
	/* A generic parameter, replaced by its binding when a value is read
	 * through a brand; opaque (AnyPointer) when nothing binds it. */
	CAPN_DYN_PARAM
};

struct capn_dyn_struct;
struct capn_dyn_enum;
struct capn_dyn_type;

/* The bindings for one generic scope: `params[i]` is the type bound to
 * the scope's i-th parameter, or `inherit` says the scope keeps whatever
 * the surrounding context binds. */
struct capn_dyn_scope {
	uint64_t scope_id;
	int inherit;
	int nparams;
	const struct capn_dyn_type *params;
};

struct capn_dyn_brand {
	int nscopes;
	const struct capn_dyn_scope *scopes;
};

/* A value type. `st` names the struct for STRUCT (and for a group), `en`
 * the enum for ENUM, `elem` the element type for LIST. A NULL `st` or
 * `en` means the type's schema file was not built by $C.reflect,
 * and values of it are opaque. */
struct capn_dyn_type {
	uint8_t kind;
	const struct capn_dyn_struct *st;
	const struct capn_dyn_enum *en;
	const struct capn_dyn_type *elem;
	/* STRUCT: the bindings its generic parameters take here, or NULL. */
	const struct capn_dyn_brand *brand;
	/* PARAM: which parameter of which scope. */
	uint64_t scope_id;
	uint32_t index;
};

#define CAPN_DYN_NO_DISCRIMINANT 0xffffu

struct capn_dyn_field {
	const char *name;
	struct capn_dyn_type type;
	/* Slot offset as the schema gives it: bits for Bool, elements of the
	 * type's size for other scalars, pointer index for pointer types. */
	uint32_t offset;
	/* Union tag that selects this field, or CAPN_DYN_NO_DISCRIMINANT. */
	uint32_t discriminant;
	/* A group shares the enclosing struct's storage; `type.st` names it. */
	uint32_t is_group;
	/* Scalar default as raw bits (floats as IEEE bits), xor-ed on read. */
	uint64_t default_bits;
	const char *default_text;
	int default_text_len;
	/* $Json.name. NULL means the schema name. */
	const char *json_name;
	/* $Json.flatten. json_prefix is NULL when there is no prefix. */
	uint8_t json_flatten;
	const char *json_prefix;
	/* $Json.base64 is 1, $Json.hex is 2, a JSON number array is 0. */
	uint8_t json_data;
	/* Struct, list, text or data default. NULL when the field has none.
	 * The pointer addresses a static segment. capn_dyn_has stays false
	 * while the wire pointer is null. */
	const capn_ptr *default_ptr;
};

struct capn_dyn_struct {
	const char *name;
	uint64_t id;
	uint16_t data_words;
	uint16_t pointers;
	/* Number of union members, and where the 16-bit tag lives (in
	 * 16-bit units from the start of the data section). */
	uint16_t union_count;
	uint32_t union_offset;
	int nfields;
	const struct capn_dyn_field *fields; /* in declaration order */
	/* $Json.discriminator. json_has_disc is 0 when the struct has none.
	 * json_disc_name is the tag field (NULL when the annotation names
	 * none). json_disc_value is valueName, or NULL. */
	const char *json_disc_name;
	const char *json_disc_value;
	uint8_t json_has_disc;
};

struct capn_dyn_enum {
	const char *name;
	uint64_t id;
	int n;
	const char *const *names; /* indexed by value */
	/* $Json.name per enumerant. NULL, or a parallel list whose NULL
	 * entries keep `names`. */
	const char *const *json_names;
};

/* --- reflection ---------------------------------------------------- */

CAPN_EXPORT const struct capn_dyn_field *capn_dyn_find_field(const struct capn_dyn_struct *s,
                                                 const char *name);

/* The active union member's tag, or -1 when the struct has no union. */
CAPN_EXPORT int capn_dyn_which(const struct capn_dyn_struct *s, capn_ptr st);

/* 1 unless the field is a union member that is not the active one. */
CAPN_EXPORT int capn_dyn_is_active(const struct capn_dyn_struct *s, capn_ptr st,
                       const struct capn_dyn_field *f);

/* Scalar fields as raw bits with the schema default applied: Bool is 0 or
 * 1, integers are zero-extended, floats are their IEEE bit pattern. */
CAPN_EXPORT uint64_t capn_dyn_get_bits(capn_ptr st, const struct capn_dyn_field *f);
CAPN_EXPORT int capn_dyn_set_bits(capn_ptr st, const struct capn_dyn_field *f, uint64_t bits);

/* Select a union member. Returns 0, or -1 if `f` is not a member. */
CAPN_EXPORT int capn_dyn_set_which(const struct capn_dyn_struct *s, capn_ptr st,
                       const struct capn_dyn_field *f);

/* Pointer fields (Text, Data, List, Struct, Interface, AnyPointer) as
 * the resolved pointer, CAPN_NULL when unset. */
CAPN_EXPORT capn_ptr capn_dyn_get_ptr(capn_ptr st, const struct capn_dyn_field *f);

/* Text with the schema default when unset. */
CAPN_EXPORT capn_text capn_dyn_get_text(capn_ptr st, const struct capn_dyn_field *f);

/* 1 when the field carries a value: scalars and groups always, pointers
 * when set, union members when active. */
CAPN_EXPORT int capn_dyn_has(const struct capn_dyn_struct *s, capn_ptr st,
                 const struct capn_dyn_field *f);

/* Float bits to a value and back. */
CAPN_EXPORT double capn_dyn_bits_to_f64(const struct capn_dyn_field *f, uint64_t bits);
CAPN_EXPORT int64_t capn_dyn_bits_to_i64(const struct capn_dyn_field *f, uint64_t bits);

/* --- text and JSON --------------------------------------------------- */

#define CAPN_DYN_COMPACT 1 /* JSON without spaces after ':' and ',' */

/* Single-line text format, as `capnp decode --short` prints it. Returns
 * a malloc'd string, or NULL on out of memory. */
CAPN_EXPORT char *capn_dyn_to_text(const struct capn_dyn_struct *s, capn_ptr st);

/* Multi-line text format, as `capnp decode` prints it (without --short).
 * Returns a malloc'd string, or NULL on out of memory. */
CAPN_EXPORT char *capn_dyn_to_text_pretty(const struct capn_dyn_struct *s,
                                          capn_ptr st);

/* JSON, as `capnp convert binary:json` prints it (`--short` is
 * CAPN_DYN_COMPACT). 64-bit integers are strings, Data is an array of
 * numbers, enums are names, non-finite floats are the strings "NaN",
 * "Infinity" and "-Infinity". Returns a malloc'd string, or NULL. */
CAPN_EXPORT char *capn_dyn_to_json(const struct capn_dyn_struct *s, capn_ptr st, int flags);

struct capn_dyn_error {
	char message[160];
	int offset; /* byte offset into the input, -1 if unknown */
};

/* Build a message root of type `s` from JSON or from the schema-language
 * text format into `seg` (a segment of a writable session, as
 * capn_root(&c).seg). On success returns 0 and stores the struct in
 * *out; the caller attaches it with capn_set_root. On failure returns -1
 * and fills `err` when it is not NULL. */
CAPN_EXPORT int capn_dyn_from_json(const struct capn_dyn_struct *s, const char *json,
                       size_t len, struct capn_segment *seg, capn_ptr *out,
                       struct capn_dyn_error *err);
CAPN_EXPORT int capn_dyn_from_text(const struct capn_dyn_struct *s, const char *text,
                       size_t len, struct capn_segment *seg, capn_ptr *out,
                       struct capn_dyn_error *err);

#ifdef __cplusplus
}
#endif

#endif /* CAPN_DYN_H */
