/* list-read-parity-test.cpp
 *
 * List(T) bound to a one-word struct reads back in text and compact JSON.
 * A composite list of { t @0 :Text } reads as List(Text).
 * The other direction, a pointer list read as a struct element, stays.
 * A composite list stored behind a far pointer reads the same way.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "capn-dyn.h"
}

static capn_text tlit(const char *s) {
  capn_text t;
  t.len = (int) strlen(s);
  t.str = s;
  t.seg = NULL;
  return t;
}

static void set_field(struct capn_dyn_field *f, const char *name,
                      const struct capn_dyn_type &type, uint32_t offset) {
  memset(f, 0, sizeof *f);
  f->name = name;
  f->type = type;
  f->offset = offset;
  f->discriminant = CAPN_DYN_NO_DISCRIMINANT;
}

/* Trio(A, B, T) { items @0 :List(T) } and UsesBrandLists, with T bound
 * once to a data-only struct and once to Text. */
struct BrandSchema {
  struct capn_dyn_struct leaf;
  struct capn_dyn_struct trio;
  struct capn_dyn_struct uses;
};

static BrandSchema *brand_schema(void) {
  static BrandSchema s;
  static int ready;
  static struct capn_dyn_field leaf_fields[1];
  static struct capn_dyn_field trio_fields[1];
  static struct capn_dyn_field uses_fields[2];
  static struct capn_dyn_type text_ty;
  static struct capn_dyn_type leaf_ty;
  static struct capn_dyn_type param_t;
  static struct capn_dyn_type list_ty;
  static struct capn_dyn_type leaf_params[3];
  static struct capn_dyn_type text_params[3];
  static struct capn_dyn_scope leaf_scope;
  static struct capn_dyn_scope text_scope;
  static struct capn_dyn_brand leaf_brand;
  static struct capn_dyn_brand text_brand;
  static struct capn_dyn_type trio_leaf;
  static struct capn_dyn_type trio_text;
  const uint64_t scope = UINT64_C(0x0b2c3d4e5f607101);

  if (ready)
    return &s;
  memset(&s, 0, sizeof s);
  memset(&text_ty, 0, sizeof text_ty);
  text_ty.kind = CAPN_DYN_TEXT;

  memset(&leaf_ty, 0, sizeof leaf_ty);
  leaf_ty.kind = CAPN_DYN_STRUCT;
  leaf_ty.st = &s.leaf;
  set_field(&leaf_fields[0], "x",
            (struct capn_dyn_type){CAPN_DYN_UINT32, NULL, NULL, NULL, NULL, 0, 0},
            0);
  s.leaf.name = "BrandLeaf";
  s.leaf.data_words = 1;
  s.leaf.pointers = 0;
  s.leaf.nfields = 1;
  s.leaf.fields = leaf_fields;

  memset(&param_t, 0, sizeof param_t);
  param_t.kind = CAPN_DYN_PARAM;
  param_t.scope_id = scope;
  param_t.index = 2;
  memset(&list_ty, 0, sizeof list_ty);
  list_ty.kind = CAPN_DYN_LIST;
  list_ty.elem = &param_t;
  set_field(&trio_fields[0], "items", list_ty, 0);
  s.trio.name = "Trio";
  s.trio.data_words = 0;
  s.trio.pointers = 1;
  s.trio.nfields = 1;
  s.trio.fields = trio_fields;

  leaf_params[0] = text_ty;
  leaf_params[1] = text_ty;
  leaf_params[2] = leaf_ty;
  memset(&leaf_scope, 0, sizeof leaf_scope);
  leaf_scope.scope_id = scope;
  leaf_scope.nparams = 3;
  leaf_scope.params = leaf_params;
  memset(&leaf_brand, 0, sizeof leaf_brand);
  leaf_brand.nscopes = 1;
  leaf_brand.scopes = &leaf_scope;
  memset(&trio_leaf, 0, sizeof trio_leaf);
  trio_leaf.kind = CAPN_DYN_STRUCT;
  trio_leaf.st = &s.trio;
  trio_leaf.brand = &leaf_brand;

  text_params[0] = text_ty;
  text_params[1] = text_ty;
  text_params[2] = text_ty;
  memset(&text_scope, 0, sizeof text_scope);
  text_scope.scope_id = scope;
  text_scope.nparams = 3;
  text_scope.params = text_params;
  memset(&text_brand, 0, sizeof text_brand);
  text_brand.nscopes = 1;
  text_brand.scopes = &text_scope;
  memset(&trio_text, 0, sizeof trio_text);
  trio_text.kind = CAPN_DYN_STRUCT;
  trio_text.st = &s.trio;
  trio_text.brand = &text_brand;

  set_field(&uses_fields[0], "leaves", trio_leaf, 0);
  set_field(&uses_fields[1], "texts", trio_text, 1);
  s.uses.name = "UsesBrandLists";
  s.uses.data_words = 0;
  s.uses.pointers = 2;
  s.uses.nfields = 2;
  s.uses.fields = uses_fields;
  ready = 1;
  return &s;
}

TEST(ListReadParity, BoundDataStructListReadsBack) {
  BrandSchema *s = brand_schema();
  const char *in =
      "(leaves = (items = [(x = 1)]), texts = (items = [\"a\"]))";
  struct capn c;
  struct capn_dyn_error err;
  capn_ptr st;
  char *text;
  char *json;

  capn_init_malloc(&c);
  memset(&err, 0, sizeof err);
  ASSERT_EQ(0, capn_dyn_from_text(&s->uses, in, strlen(in), capn_root(&c).seg,
                                  &st, &err))
      << err.message;
  capn_clear_err(&c);
  text = capn_dyn_to_text(&s->uses, st);
  ASSERT_NE(nullptr, text);
  EXPECT_STREQ(in, text);
  EXPECT_EQ(1, capn_ok(&c));
  free(text);

  capn_clear_err(&c);
  json = capn_dyn_to_json(&s->uses, st, CAPN_DYN_COMPACT);
  ASSERT_NE(nullptr, json);
  EXPECT_STREQ("{\"leaves\":{\"items\":[{\"x\":1}]},\"texts\":{\"items\":[\"a\"]}}",
               json);
  EXPECT_EQ(1, capn_ok(&c));
  free(json);
  capn_free(&c);
}

TEST(ListReadParity, StructListReadsAsText) {
  struct capn c;
  struct capn_segment *seg;
  capn_ptr list, e0, e1;
  capn_text a, bb;

  capn_init_malloc(&c);
  seg = capn_root(&c).seg;
  list = capn_new_struct_list(seg, 2, 0, 1);
  ASSERT_EQ(CAPN_LIST, list.type);
  ASSERT_EQ(1, list.is_composite_list);
  ASSERT_EQ(0u, list.datasz);
  ASSERT_EQ(1u, list.ptrs);
  e0 = capn_getp(list, 0, 0);
  e1 = capn_getp(list, 1, 0);
  ASSERT_EQ(CAPN_STRUCT, e0.type);
  ASSERT_EQ(0, capn_set_text(e0, 0, tlit("a")));
  ASSERT_EQ(0, capn_set_text(e1, 0, tlit("bb")));

  capn_clear_err(&c);
  a = capn_get_text(list, 0, tlit(""));
  bb = capn_get_text(list, 1, tlit(""));
  EXPECT_EQ(1, a.len);
  EXPECT_STREQ("a", a.str);
  EXPECT_EQ(2, bb.len);
  EXPECT_STREQ("bb", bb.str);
  EXPECT_EQ(1, capn_ok(&c));

  /* Pointer list read as a struct element stays a struct whose text is set. */
  {
    capn_ptr plist = capn_new_ptr_list(seg, 1);
    capn_ptr el;
    ASSERT_EQ(0, capn_set_text(plist, 0, tlit("a")));
    el = capn_getp(plist, 0, 0);
    EXPECT_EQ(CAPN_STRUCT, el.type);
    EXPECT_EQ(0, el.datasz);
    EXPECT_EQ(1u, el.ptrs);
    EXPECT_STREQ("a", capn_get_text(el, 0, tlit("")).str);
    EXPECT_STREQ("a", capn_get_text(plist, 0, tlit("")).str);
  }
  capn_free(&c);
}

TEST(ListReadParity, StructListReadsAsData) {
  struct capn c;
  struct capn_segment *seg;
  capn_ptr list, el;
  capn_list8 bytes;
  capn_data in, got;

  capn_init_malloc(&c);
  seg = capn_root(&c).seg;
  list = capn_new_struct_list(seg, 1, 0, 1);
  el = capn_getp(list, 0, 0);
  bytes = capn_new_list8(seg, 2);
  ASSERT_EQ(0, capn_set8(bytes, 0, 0xab));
  ASSERT_EQ(0, capn_set8(bytes, 1, 0xcd));
  in.p = bytes.p;
  ASSERT_EQ(0, capn_set_data(el, 0, in));
  capn_clear_err(&c);
  got = capn_get_data(list, 0);
  EXPECT_EQ(1, capn_ok(&c));
  ASSERT_EQ(2, capn_len(got));
  {
    capn_list8 view;
    view.p = got.p;
    EXPECT_EQ(0xab, capn_get8(view, 0));
    EXPECT_EQ(0xcd, capn_get8(view, 1));
  }
  capn_free(&c);
}

TEST(ListReadParity, DataOnlyStructListIsNotText) {
  struct capn c;
  capn_ptr list;
  capn_text got;

  capn_init_malloc(&c);
  list = capn_new_struct_list(capn_root(&c).seg, 1, 8, 0);
  ASSERT_EQ(1, list.is_composite_list);
  ASSERT_EQ(0u, list.ptrs);
  capn_clear_err(&c);
  got = capn_get_text(list, 0, tlit(""));
  EXPECT_EQ(0, capn_ok(&c));
  EXPECT_STREQ("", got.str);
  capn_free(&c);
}

/* Each allocation fills its segment, so the composite list lands in
 * another segment and the struct slot is a far pointer. */
static struct capn_segment *create_tight(void *, uint32_t, int sz)
{
  struct capn_segment *s;

  sz += (int)sizeof(struct capn_segment);
  s = (struct capn_segment *)calloc(1, (size_t)sz);
  if (s == NULL)
    return NULL;
  s->data = (char *)(s + 1);
  s->cap = (size_t)sz - sizeof(*s);
  s->user = s;
  return s;
}

TEST(ListReadParity, FarCompositeListReadsAsText) {
  struct capn c;
  capn_ptr root, list, e0, e1, slot;
  capn_text a, bb;

  capn_init_malloc(&c);
  c.create = create_tight;

  root = capn_new_struct(capn_root(&c).seg, 0, 1);
  ASSERT_EQ(CAPN_STRUCT, root.type);
  ASSERT_EQ(0, capn_set_root(&c, root));

  list = capn_new_struct_list(root.seg, 2, 0, 1);
  ASSERT_EQ(CAPN_LIST, list.type);
  ASSERT_EQ(1, list.is_composite_list);
  ASSERT_EQ(1u, list.ptrs);
  e0 = capn_getp(list, 0, 0);
  e1 = capn_getp(list, 1, 0);
  ASSERT_EQ(0, capn_set_text(e0, 0, tlit("a")));
  ASSERT_EQ(0, capn_set_text(e1, 0, tlit("bb")));
  ASSERT_EQ(0, capn_setp(root, 0, list));

  slot = capn_getp(root, 0, 0);
  ASSERT_EQ(CAPN_FAR_POINTER, slot.type);

  capn_clear_err(&c);
  a = capn_get_text(slot, 0, tlit(""));
  bb = capn_get_text(slot, 1, tlit(""));
  EXPECT_EQ(1, capn_ok(&c));
  EXPECT_EQ(1, a.len);
  EXPECT_STREQ("a", a.str);
  EXPECT_EQ(2, bb.len);
  EXPECT_STREQ("bb", bb.str);
  capn_free(&c);
}
