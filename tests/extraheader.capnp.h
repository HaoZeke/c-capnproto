#ifndef CAPN_BC91E0A4D3F8C217
#define CAPN_BC91E0A4D3F8C217
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>
#include "extraheader-probe.h"

#if CAPN_VERSION != 1
#error "version mismatch between capnp_c.h and generated code"
#endif

#ifndef capnp_nowarn
# ifdef __GNUC__
#  define capnp_nowarn __extension__
# else
#  define capnp_nowarn
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct EXTRAHEADER_EXTATTR Widget;

typedef struct {capn_ptr p;} Widget_ptr;

typedef struct {capn_ptr p;} Widget_list;

struct EXTRAHEADER_EXTATTR Widget {
	uint32_t id;
	capn_text name;
};

static const size_t Widget_word_count = 1;

static const size_t Widget_pointer_count = 1;

static const size_t Widget_struct_bytes_count = 16;


EXTRAHEADER_EXTATTR uint32_t Widget_get_id(Widget_ptr p);

EXTRAHEADER_EXTATTR capn_text Widget_get_name(Widget_ptr p);

EXTRAHEADER_EXTATTR void Widget_set_id(Widget_ptr p, uint32_t id);

EXTRAHEADER_EXTATTR void Widget_set_name(Widget_ptr p, capn_text name);

EXTRAHEADER_EXTATTR Widget_ptr new_Widget(struct capn_segment*);

EXTRAHEADER_EXTATTR Widget_list new_Widget_list(struct capn_segment*, int len);

EXTRAHEADER_EXTATTR void read_Widget(struct Widget*, Widget_ptr);

EXTRAHEADER_EXTATTR void write_Widget(const struct Widget*, Widget_ptr);

EXTRAHEADER_EXTATTR void get_Widget(struct Widget*, Widget_list, int i);

EXTRAHEADER_EXTATTR void set_Widget(const struct Widget*, Widget_list, int i);

#ifdef __cplusplus
}
#endif
#endif
