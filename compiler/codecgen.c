/* codecgen.c
 *
 * Copyright (C) 2013 James McKaskill
 * Codec encode/decode/free generators: Rongsong Shen (shen390s/c-capnproto)
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 *
 * Adapted to HaoZeke capnpc-c process globals via a thin capnp_ctx_t
 * that points at HDR/SRC/g_nullused. find_node is the compiler global.
 */

#include "codecgen.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <stdio.h>

static void fail(int code, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  exit(code);
}

const char *get_text_annotation(Annotation_list l, unsigned long id) {
  int i;

  for (i = capn_len(l) - 1; i >= 0; i--) {
    struct Annotation a;
    struct Value v;

    get_Annotation(&a, l, i);
    read_Value(&v, a.value);

    if (a.id == id) {
      if (v.which != Value_text) {
        fail(2, "annotation is not string");
      }

      return v.text.str;
    }
  }

  return NULL;
}

const char *get_mapname(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPNAME);
}

const char *get_maplistcount(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPLISTCOUNT);
}

const char *get_mapuniontag(Annotation_list l) {
  return get_text_annotation(l, ANNOTATION_MAPUNIONTAG);
}


static void mk_simple_list_encoder(struct str *func, const char *tab,
                                   const char *list_type, const char *setf,
                                   const char *dvar, const char *cvar,
                                   const char *svar) {
  str_add(func, tab, -1);
  str_addf(func, "if (1) {\n");
  str_add(func, tab, -1);
  str_addf(func, "\tint i_;\n");
  if (strcmp(list_type, "text") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\td->%s = capn_new_ptr_list(cs, s->%s);\n", dvar, cvar);
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < s->%s; i_ ++) {\n", cvar);
    str_add(func, tab, -1);
    str_addf(func,
             "\t\tcapn_text text_ = {.str = s->%s[i_], .len = "
             "strlen(s->%s[i_]),.seg "
             "= NULL};\n",
             svar, svar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tcapn_set_text(d->%s, i_, text_);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  } else if (strcmp(list_type, "data") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\td->%s = capn_new_ptr_list(cs, s->%s);\n", dvar, cvar);
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < s->%s; i_ ++) {\n", cvar);
    str_add(func, tab, -1);
    str_addf(func,
             "\t\tcapn_list8 item_ = capn_new_list8(cs, s->%s[i_].len);\n",
             svar);
    str_add(func, tab, -1);
    str_addf(func,
             "\t\tcapn_setv8(item_, 0, s->%s[i_].data, s->%s[i_].len);\n",
             svar, svar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tcapn_setp(d->%s, i_, item_.p);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  } else {
    str_add(func, tab, -1);
    str_addf(func, "\td->%s = capn_new_%s(cs, s->%s);\n", dvar, list_type,
             cvar);
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < s->%s; i_ ++) {\n", cvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tcapn_%s(d->%s, i_, s->%s[i_]);\n", setf, dvar, svar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  }
  str_add(func, tab, -1);
  str_addf(func, "}\n");
}

static void mk_simple_list_decoder(struct str *func, const char *tab,
                                   const char *list_type, const char *getf,
                                   const char *dvar, const char *cvar,
                                   const char *svar) {
  str_add(func, tab, -1);
  str_addf(func, "if (1) {\n");
  str_add(func, tab, -1);
  str_addf(func, "\tint i_, nc_;\n");
  if (strcmp(list_type, "text") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\tcapn_resolve(&(s->%s));\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\tnc_ = s->%s.len;\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\tif (nc_ == 0) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s = NULL;\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "\telse {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s = (char **)calloc(nc_, sizeof(char *));\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tfor(i_ = 0; i_ < nc_; i_ ++) {\n");
    str_add(func, tab, -1);
    str_addf(func,
             "\t\t\tcapn_text text_ = capn_get_text(s->%s, i_, capn_val0);\n",
             svar);
    str_add(func, tab, -1);
    str_addf(func, "\t\t\td->%s[i_] = STRING_DUP(text_.str);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  } else if (strcmp(list_type, "data") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\tcapn_resolve(&(s->%s));\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\tnc_ = s->%s.len;\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\tif (nc_ == 0) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s = NULL;\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "\telse {\n");
    str_add(func, tab, -1);
    str_addf(func,
             "\t\td->%s = (capnp_data_t *)calloc(nc_, sizeof(capnp_data_t));\n",
             dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tfor(i_ = 0; i_ < nc_; i_ ++) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\t\tcapn_ptr item_ = capn_getp(s->%s, i_, 1);\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\t\t\td->%s[i_].len = item_.len;\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\t\tif (item_.len > 0) {\n");
    str_add(func, tab, -1);
    str_addf(func,
             "\t\t\t\td->%s[i_].data = (uint8_t *)malloc(item_.len);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func,
             "\t\t\t\tcapn_getv8((capn_list8){item_}, 0, d->%s[i_].data, "
             "item_.len);\n",
             dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\t\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  } else {
    str_add(func, tab, -1);
    str_addf(func, "\tcapn_resolve(&(s->%s.p));\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\tnc_ = s->%s.p.len;\n", svar);
    str_add(func, tab, -1);
    str_addf(func, "\tif (nc_ == 0) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s = NULL;\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "\telse {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s = (%s *)calloc(nc_, sizeof(%s));\n", dvar,
             list_type, list_type);
    str_add(func, tab, -1);
    str_addf(func, "\t\tfor(i_ = 0; i_ < nc_; i_ ++) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\t\td->%s[i_] = capn_%s(s->%s, i_);\n", dvar, getf, svar);
    str_add(func, tab, -1);
    str_addf(func, "\t\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  }

  str_add(func, tab, -1);
  str_addf(func, "d->%s = nc_;\n", cvar);
  str_add(func, tab, -1);
  str_addf(func, "}\n");
}

static void mk_simple_list_free(struct str *func, const char *tab,
                                const char *list_type, const char *getf,
                                const char *dvar, const char *cvar,
                                const char *svar) {
  str_add(func, tab, -1);
  str_addf(func, "if (1) {\n");
  str_add(func, tab, -1);
  str_addf(func, "\tint i_, nc_ = d->%s;\n", cvar);
  str_add(func, tab, -1);
  str_addf(func, "\tcapnp_use(i_);capnp_use(nc_);\n");
  if (strcmp(list_type, "text") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < nc_; i_ ++) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\tif (d->%s[i_] == NULL) continue;\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\tfree(d->%s[i_]);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  } else if (strcmp(list_type, "data") == 0) {
    str_add(func, tab, -1);
    str_addf(func, "\tfor(i_ = 0; i_ < nc_; i_ ++) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\tif (d->%s[i_].data != NULL) {\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\t\tfree(d->%s[i_].data);\n", dvar);
    str_add(func, tab, -1);
    str_addf(func, "\t\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
  }

  str_add(func, tab, -1);
  str_addf(func, "\tfree(d->%s);\n", dvar);
  str_add(func, tab, -1);
  str_addf(func, "}\n");
}

struct list_type_info {
  int type_which;
  const char *ctype;
  const char *list_type;
  const char *setf;
  const char *getf;
};

static const struct list_type_info list_types[] = {
    {Type__bool, "uint8_t", "list1", "set1", "get1"},
    {Type_int8, "int8_t", "list8", "set8", "get8"},
    {Type_uint8, "uint8_t", "list8", "set8", "get8"},
    {Type_int16, "int16_t", "list16", "set16", "get16"},
    {Type_uint16, "uint16_t", "list16", "set16", "get16"},
    {Type_int32, "int32_t", "list32", "set32", "get32"},
    {Type_uint32, "uint32_t", "list32", "set32", "get32"},
    {Type_float32, "float", "list32", "set32", "get32"},
    {Type_int64, "int64_t", "list64", "set64", "get64"},
    {Type_uint64, "uint64_t", "list64", "set64", "get64"},
    {Type_float64, "double", "list64", "set64", "get64"},
    {Type_text, "text", "text", NULL, NULL},
    {Type_data, "data", "data", NULL, NULL},
};

static const struct list_type_info *find_list_type(int which) {
  size_t i;
  for (i = 0; i < sizeof(list_types) / sizeof(list_types[0]); i++) {
    if (list_types[i].type_which == which)
      return &list_types[i];
  }
  return NULL;
}

static void gen_call_list_encoder(capnp_ctx_t *ctx, struct str *func,
                                  struct Type *type, const char *tab,
                                  const char *var, const char *countvar,
                                  const char *var2) {
  const struct list_type_info *info = find_list_type(type->which);

  str_add(func, tab, -1);

  if (info) {
    mk_simple_list_encoder(func, tab, info->list_type, info->setf, var,
                           countvar, var2);
  } else if (type->which == Type__struct) {
    struct node *n = find_node(type->_struct.typeId);

    if (n != NULL) {
      char *dtypename = n->name.str;

      str_addf(func, "encode_%s_list(cs, &(d->%s), s->%s, s->%s);\n",
               dtypename, var, countvar, var2);
    }
  }
}

static void gen_call_list_decoder(capnp_ctx_t *ctx, struct str *func,
                                  struct Type *type, const char *tab,
                                  const char *var, const char *countvar,
                                  const char *var2) {
  const struct list_type_info *info = find_list_type(type->which);

  str_add(func, tab, -1);

  if (info) {
    mk_simple_list_decoder(func, tab, info->ctype, info->getf, var, countvar,
                           var2);
  } else if (type->which == Type__struct) {
    struct node *n = find_node(type->_struct.typeId);
    if (n != NULL) {
      char *dtypename = n->name.str;

      str_addf(func, "decode_%s_list(&(d->%s), &(d->%s), s->%s);\n",
               dtypename, countvar, var, var2);
    }
  }
}

static void gen_call_list_free(capnp_ctx_t *ctx, struct str *func,
                               struct Type *type, const char *tab,
                               const char *var, const char *countvar,
                               const char *var2) {
  const struct list_type_info *info = find_list_type(type->which);

  str_add(func, tab, -1);

  if (info) {
    mk_simple_list_free(func, tab, info->ctype, info->getf, var, countvar,
                        var2);
  } else if (type->which == Type__struct) {
    struct node *n = find_node(type->_struct.typeId);
    if (n != NULL) {
      char *dtypename = n->name.str;

      str_addf(func, "free_%s_list(d->%s, d->%s);\n", dtypename, countvar,
               var);
    }
  }
}

void encode_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                   const char *tab, const char *var, const char *var2) {
  struct Type list_type;
  struct node *n = NULL;

  if (f->v.t.which == Type__void) {
    return;
  }

  if (var2 == NULL) {
    var2 = var;
  }

  switch (f->v.t.which) {
  case Type__bool:
  case Type_int8:
  case Type_int16:
  case Type_int32:
  case Type_int64:
  case Type_uint8:
  case Type_uint16:
  case Type_uint32:
  case Type_uint64:
  case Type_float32:
  case Type_float64:
  case Type__enum:
    str_add(func, tab, -1);
    str_addf(func, "d->%s = s->%s;\n", var, var2);
    break;
  case Type_text:
    str_add(func, tab, -1);
    str_addf(func, "if (s->%s != NULL) {\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.str = s->%s;\n", var, var2);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.len = strlen(s->%s);\n", var, var2);
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    str_add(func, tab, -1);
    str_addf(func, "else{\n");
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.str = \"\";\n", var);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.len = 0;\n", var);
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    str_add(func, tab, -1);
    str_addf(func, "d->%s.seg = NULL;\n", var);
    break;
  case Type_data: {
    char *ncount = (char *)get_maplistcount(f->f.annotations);
    struct str buf = STR_INIT;
    if (ncount != NULL) {
      strf(&buf, "%s", ncount);
    } else {
      strf(&buf, "n_%s", var2);
    }
    str_add(func, tab, -1);
    str_addf(func, "if (s->%s != NULL && s->%s > 0) {\n", var2, buf.str);
    str_add(func, tab, -1);
    str_addf(func, "\tcapn_list8 list_ = capn_new_list8(cs, s->%s);\n",
             buf.str);
    str_add(func, tab, -1);
    str_addf(func, "\tcapn_setv8(list_, 0, s->%s, s->%s);\n", var2, buf.str);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s.p = list_.p;\n", var);
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    str_release(&buf);
    break;
  }
  case Type__struct:
    n = find_node(f->v.t._struct.typeId);

    if (n != NULL) {
      str_add(func, tab, -1);
      str_addf(func, "encode_%s_ptr(cs, &(d->%s), s->%s);\n", n->name.str, var,
               var2);
    }
    break;
  case Type__list:
    read_Type(&list_type, f->v.t._list.elementType);
    if (list_type.which != Type__void) {
      char *name = NULL;
      char *ncount = NULL;
      struct str buf = STR_INIT;

      name = (char *)get_mapname(f->f.annotations);
      if (name == NULL) {
        var2 = var;
      } else {
        var2 = name;
      }

      ncount = (char *)get_maplistcount(f->f.annotations);
      if (ncount != NULL) {
        strf(&buf, "%s", ncount);
      } else {
        strf(&buf, "n_%s", var2);
      }

      gen_call_list_encoder(ctx, func, &list_type, tab, var, buf.str, var2);
      str_release(&buf);
    }
    break;
  default:
    str_add(func, tab, -1);
    str_addf(func, "\t /* %s %s */\n", var, var2);
    break;
  }
}

void decode_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                   const char *tab, const char *var, const char *var2) {
  struct Type list_type;
  struct node *n = NULL;

  if (f->v.t.which == Type__void) {
    return;
  }

  if (var2 == NULL) {
    var2 = var;
  }

  switch (f->v.t.which) {
  case Type__bool:
  case Type_int8:
  case Type_int16:
  case Type_int32:
  case Type_int64:
  case Type_uint8:
  case Type_uint16:
  case Type_uint32:
  case Type_uint64:
  case Type_float32:
  case Type_float64:
  case Type__enum:
    str_add(func, tab, -1);
    str_addf(func, "d->%s = s->%s;\n", var2, var);
    break;
  case Type_text:
    str_add(func, tab, -1);
    str_addf(func, "d->%s = STRING_DUP(s->%s.str);\n", var2, var);
    break;
  case Type_data: {
    char *ncount = (char *)get_maplistcount(f->f.annotations);
    struct str buf = STR_INIT;
    if (ncount != NULL) {
      strf(&buf, "%s", ncount);
    } else {
      strf(&buf, "n_%s", var2);
    }
    str_add(func, tab, -1);
    str_addf(func, "if (1) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\tint nc_;\n");
    str_add(func, tab, -1);
    str_addf(func, "\tcapn_resolve(&(s->%s.p));\n", var);
    str_add(func, tab, -1);
    str_addf(func, "\tnc_ = s->%s.p.len;\n", var);
    str_add(func, tab, -1);
    str_addf(func, "\td->%s = nc_;\n", buf.str);
    str_add(func, tab, -1);
    str_addf(func, "\tif (nc_ > 0) {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s = (uint8_t *)malloc(nc_);\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "\t\tcapn_getv8((capn_list8){s->%s.p}, 0, d->%s, nc_);\n",
             var, var2);
    str_add(func, tab, -1);
    str_addf(func, "\t} else {\n");
    str_add(func, tab, -1);
    str_addf(func, "\t\td->%s = NULL;\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "\t}\n");
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    str_release(&buf);
    break;
  }
  case Type__struct:
    n = find_node(f->v.t._struct.typeId);
    if (n != NULL) {
      str_add(func, tab, -1);
      str_addf(func, "decode_%s_ptr(&(d->%s), s->%s);\n", n->name.str, var2,
               var);
    }
    break;
  case Type__list:
    read_Type(&list_type, f->v.t._list.elementType);
    if (list_type.which != Type__void) {
      char *name = NULL;
      char *ncount = NULL;
      struct str buf = STR_INIT;

      name = (char *)get_mapname(f->f.annotations);
      if (name == NULL) {
        var2 = var;
      } else {
        var2 = name;
      }

      ncount = (char *)get_maplistcount(f->f.annotations);
      if (ncount != NULL) {
        strf(&buf, "%s", ncount);
      } else {
        struct str buf2 = STR_INIT;
        char *p;

        strf(&buf2, "%s", var2);
        p = strchr(buf2.str, '.');
        if (p != NULL) {
          *p = '\0';
          p++;
        }

        strf(&buf, "%s", buf2.str);
        if (p != NULL) {
          str_addf(&buf, ".n_%s", p);
        }
        str_release(&buf2);
      }

      gen_call_list_decoder(ctx, func, &list_type, tab, var2, buf.str, var);
      str_release(&buf);
    }
    break;
  default:
    str_add(func, tab, -1);
    str_addf(func, "\t /* %s %s */\n", var2, var);
    break;
  }
}

void free_member(capnp_ctx_t *ctx, struct str *func, struct field *f,
                 const char *tab, const char *var, const char *var2) {
  struct Type list_type;
  struct node *n = NULL;

  if (f->v.t.which == Type__void) {
    return;
  }

  if (var2 == NULL) {
    var2 = var;
  }

  switch (f->v.t.which) {
  case Type__bool:
  case Type_int8:
  case Type_int16:
  case Type_int32:
  case Type_int64:
  case Type_uint8:
  case Type_uint16:
  case Type_uint32:
  case Type_uint64:
  case Type_float32:
  case Type_float64:
  case Type__enum:
    break;
  case Type_text:
    str_add(func, tab, -1);
    str_addf(func, "if (d->%s != NULL) {\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "\tfree(d->%s);\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    break;
  case Type_data:
    str_add(func, tab, -1);
    str_addf(func, "if (d->%s != NULL) {\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "\tfree(d->%s);\n", var2);
    str_add(func, tab, -1);
    str_addf(func, "}\n");
    break;
  case Type__struct:
    n = find_node(f->v.t._struct.typeId);
    if (n != NULL) {
      str_add(func, tab, -1);
      str_addf(func, "free_%s_ptr(&(d->%s));\n", n->name.str, var2);
    }
    break;
  case Type__list:
    read_Type(&list_type, f->v.t._list.elementType);
    if (list_type.which != Type__void) {
      char *name = NULL;
      char *ncount = NULL;
      struct str buf = STR_INIT;

      name = (char *)get_mapname(f->f.annotations);
      if (name == NULL) {
        var2 = var;
      } else {
        var2 = name;
      }

      ncount = (char *)get_maplistcount(f->f.annotations);
      if (ncount != NULL) {
        strf(&buf, "%s", ncount);
      } else {
        struct str buf2 = STR_INIT;
        char *p;

        strf(&buf2, "%s", var2);
        p = strchr(buf2.str, '.');
        if (p != NULL) {
          *p = '\0';
          p++;
        }

        strf(&buf, "%s", buf2.str);
        if (p != NULL) {
          str_addf(&buf, ".n_%s", p);
        }
        str_release(&buf2);
      }

      gen_call_list_free(ctx, func, &list_type, tab, var2, buf.str, var);
      str_release(&buf);
    }
    break;
  default:
    str_add(func, tab, -1);
    str_addf(func, "\t /* %s %s */\n", var2, var);
    break;
  }
}

void mk_struct_list_encoder(capnp_ctx_t *ctx, struct node *n) {
  if (n == NULL) {
    return;
  }

  if (1) {
    char *mapname = (char *)get_mapname(n->n.annotations);
    struct str buf = STR_INIT;

    if (mapname == NULL) {
      strf(&buf, "struct %s_", n->name.str);
    } else {
      strf(&buf, "%s", mapname);
    }

    str_addf(ctx->SRC,
             "void encode_%s_list(struct capn_segment *cs, %s_list *l,int "
             "count,%s **s) {\n",
             n->name.str, n->name.str, buf.str);
    str_addf(ctx->SRC, "\t%s_list lst;\n", n->name.str);
    str_addf(ctx->SRC, "\tint i;\n");
    str_addf(ctx->SRC, "\tlst = new_%s_list(cs, count);\n", n->name.str);
    str_addf(ctx->SRC, "\tfor(i = 0; i < count; i ++) {\n");
    str_addf(ctx->SRC, "\t\tstruct %s d;\n", n->name.str);
    str_addf(ctx->SRC, "\t\tencode_%s(cs, &d, s[i]);\n", n->name.str);
    str_addf(ctx->SRC, "\t\tset_%s(&d, lst, i);\n", n->name.str);
    str_addf(ctx->SRC, "\t}\n");
    str_addf(ctx->SRC, "\t(*l) = lst;\n");
    str_addf(ctx->SRC, "}\n");
    str_release(&buf);
  }
}

void mk_struct_ptr_encoder(capnp_ctx_t *ctx, struct node *n) {
  char *mapname;
  struct str buf = STR_INIT;

  if (n == NULL) {
    return;
  }

  mapname = (char *)get_mapname(n->n.annotations);

  if (mapname == NULL) {
    strf(&buf, "struct %s_", n->name.str);
  } else {
    strf(&buf, "%s", mapname);
  }

  str_addf(ctx->SRC,
           "void encode_%s_ptr(struct capn_segment *cs, %s_ptr *p,"
           "%s *s) {\n",
           n->name.str, n->name.str, buf.str);
  str_addf(ctx->SRC, "\t%s_ptr ptr;\n", n->name.str);
  str_addf(ctx->SRC, "\tstruct %s d;\n", n->name.str);
  str_addf(ctx->SRC, "\tptr = new_%s(cs);\n", n->name.str);
  str_addf(ctx->SRC, "\tif (s == NULL) {\n");
  str_addf(ctx->SRC, "\t\tptr.p = capn_null;\n");
  str_addf(ctx->SRC, "\t}\n");
  str_addf(ctx->SRC, "\telse{\n");
  str_addf(ctx->SRC, "\t\tencode_%s(cs, &d, s);\n", n->name.str);
  str_addf(ctx->SRC, "\t\twrite_%s(&d, ptr);\n", n->name.str);
  str_addf(ctx->SRC, "\t}\n");
  str_addf(ctx->SRC, "\t(*p) = ptr;\n");
  str_addf(ctx->SRC, "}\n");
  str_release(&buf);
  *(ctx->g_nullused) = 1;
}

void mk_struct_list_decoder(capnp_ctx_t *ctx, struct node *n) {
  if (n == NULL) {
    return;
  }

  if (1) {
    char *mapname = (char *)get_mapname(n->n.annotations);
    struct str buf = STR_INIT;

    if (mapname == NULL) {
      strf(&buf, "struct %s_", n->name.str);
    } else {
      strf(&buf, "%s", mapname);
    }

    str_addf(ctx->SRC,
             "void decode_%s_list(int *pcount, %s ***d, %s_list list) {\n",
             n->name.str, buf.str, n->name.str);
    str_addf(ctx->SRC, "\tint i;\n");
    str_addf(ctx->SRC, "\tint nc;\n");
    str_addf(ctx->SRC, "\t%s **ptr;\n", buf.str);
    str_addf(ctx->SRC, "\tcapn_resolve(&(list.p));\n");
    str_addf(ctx->SRC, "\tnc = list.p.len;\n");
    str_addf(ctx->SRC, "\tif (nc == 0) {\n");
    str_addf(ctx->SRC, "\t\t(*d) = NULL;\n");
    str_addf(ctx->SRC, "\t\t(*pcount) = 0;\n");
    str_addf(ctx->SRC, "\t\treturn;\n");
    str_addf(ctx->SRC, "\t}\n");
    str_addf(ctx->SRC, "\tptr = (%s **)calloc(nc, sizeof(%s *));\n", buf.str,
             buf.str);
    str_addf(ctx->SRC, "\tfor(i = 0; i < nc; i ++) {\n");
    str_addf(ctx->SRC, "\t\tstruct %s s;\n", n->name.str);
    str_addf(ctx->SRC, "\t\tget_%s(&s, list, i);\n", n->name.str);
    str_addf(ctx->SRC, "\t\tptr[i] = (%s *)calloc(1, sizeof(%s));\n", buf.str,
             buf.str);
    str_addf(ctx->SRC, "\t\tdecode_%s(ptr[i], &s);\n", n->name.str);
    str_addf(ctx->SRC, "\t}\n");
    str_addf(ctx->SRC, "\t(*d) = ptr;\n");
    str_addf(ctx->SRC, "\t(*pcount) = nc;\n");
    str_addf(ctx->SRC, "}\n");
    str_release(&buf);
  }
}

void mk_struct_ptr_decoder(capnp_ctx_t *ctx, struct node *n) {
  char *mapname;
  struct str buf = STR_INIT;

  if (n == NULL) {
    return;
  }

  mapname = (char *)get_mapname(n->n.annotations);

  if (mapname == NULL) {
    strf(&buf, "struct %s_", n->name.str);
  } else {
    strf(&buf, "%s", mapname);
  }

  str_addf(ctx->SRC,
           "void decode_%s_ptr(%s **d,"
           "%s_ptr p) {\n",
           n->name.str, buf.str, n->name.str);
  str_addf(ctx->SRC, "\tstruct %s s;\n", n->name.str);
  str_addf(ctx->SRC, "\tcapn_resolve(&(p.p));\n");
  str_addf(ctx->SRC, "\tif (p.p.type == CAPN_NULL) {\n");
  str_addf(ctx->SRC, "\t\t(*d) = NULL;\n");
  str_addf(ctx->SRC, "\t\treturn;\n");
  str_addf(ctx->SRC, "\t}\n");
  str_addf(ctx->SRC, "\t*d = (%s *)calloc(1, sizeof(%s));\n", buf.str, buf.str);
  str_addf(ctx->SRC, "\tread_%s(&s, p);\n", n->name.str);
  str_addf(ctx->SRC, "\tdecode_%s(*d, &s);\n", n->name.str);
  str_addf(ctx->SRC, "}\n");
  str_release(&buf);
  *(ctx->g_nullused) = 1;
}

void mk_struct_list_free(capnp_ctx_t *ctx, struct node *n) {
  if (n == NULL) {
    return;
  }

  if (1) {
    char *mapname = (char *)get_mapname(n->n.annotations);
    struct str buf = STR_INIT;

    if (mapname == NULL) {
      strf(&buf, "struct %s_", n->name.str);
    } else {
      strf(&buf, "%s", mapname);
    }

    str_addf(ctx->SRC, "void free_%s_list(int pcount, %s **d) {\n",
             n->name.str, buf.str);
    str_addf(ctx->SRC, "\tint i;\n");
    str_addf(ctx->SRC, "\tint nc = pcount;\n");
    str_addf(ctx->SRC, "\t%s **ptr = d;\n", buf.str);
    str_addf(ctx->SRC, "\tif (ptr == NULL) return;\n");
    str_addf(ctx->SRC, "\tfor(i = 0; i < nc; i ++) {\n");
    str_addf(ctx->SRC, "\t\tif(ptr[i] == NULL) continue;\n");
    str_addf(ctx->SRC, "\t\tfree_%s(ptr[i]);\n", n->name.str);
    str_addf(ctx->SRC, "\t\tfree(ptr[i]);\n");
    str_addf(ctx->SRC, "\t}\n");
    str_addf(ctx->SRC, "\tfree(ptr);\n");
    str_addf(ctx->SRC, "}\n");
    str_release(&buf);
  }
}

void mk_struct_ptr_free(capnp_ctx_t *ctx, struct node *n) {
  char *mapname;
  struct str buf = STR_INIT;

  if (n == NULL) {
    return;
  }

  mapname = (char *)get_mapname(n->n.annotations);

  if (mapname == NULL) {
    strf(&buf, "struct %s_", n->name.str);
  } else {
    strf(&buf, "%s", mapname);
  }

  str_addf(ctx->SRC, "void free_%s_ptr(%s **d){\n", n->name.str, buf.str);
  str_addf(ctx->SRC, "\tif((*d) == NULL) return;\n");
  str_addf(ctx->SRC, "\tfree_%s(*d);\n", n->name.str);
  str_addf(ctx->SRC, "\tfree(*d);\n");
  str_addf(ctx->SRC, "\t(*d) = NULL;\n");
  str_addf(ctx->SRC, "}\n");
  str_release(&buf);
}

static void mk_codec_declares(capnp_ctx_t *ctx, const char *n1,
                              const char *n2) {
  str_addf(ctx->HDR,
           "void encode_%s(struct capn_segment *,struct %s *, %s *);\n", n1, n1,
           n2);
  str_addf(ctx->HDR, "void decode_%s(%s *, struct %s *);\n", n1, n2, n1);
  str_addf(ctx->HDR, "void free_%s(%s *);\n", n1, n2);
  str_addf(
      ctx->HDR,
      "void encode_%s_list(struct capn_segment *,%s_list *, int, %s **);\n", n1,
      n1, n2);
  str_addf(ctx->HDR, "void decode_%s_list(int *, %s ***, %s_list);\n", n1,
           n2, n1);
  str_addf(ctx->HDR, "void free_%s_list(int, %s **);\n", n1, n2);
  str_addf(ctx->HDR,
           "void encode_%s_ptr(struct capn_segment*, %s_ptr *, %s *);\n", n1,
           n1, n2);
  str_addf(ctx->HDR, "void decode_%s_ptr(%s **, %s_ptr);\n", n1, n2, n1);
  str_addf(ctx->HDR, "void free_%s_ptr(%s **);\n", n1, n2);
}

void declare_codec(capnp_ctx_t *ctx, struct node *file_node) {
  struct node *n;
  str_addf(ctx->HDR, "\n");
  for (n = file_node->file_nodes; n != NULL; n = n->next_file_node) {
    if (n->n.which == Node__struct && !n->n._struct.isGroup) {
      const char *mapname = get_mapname(n->n.annotations);

      if (mapname == NULL) {
        mk_codec_declares(ctx, n->name.str, n->name.str);
      } else {
        mk_codec_declares(ctx, n->name.str, mapname);
      }
      str_addf(ctx->HDR, "\n");
    }
  }
}
