#include "extraheader.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

static const capn_text capn_val0 = {0,"",0};

EXTRAHEADER_EXTATTR Widget_ptr new_Widget(struct capn_segment *s) {
	Widget_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
EXTRAHEADER_EXTATTR Widget_list new_Widget_list(struct capn_segment *s, int len) {
	Widget_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
EXTRAHEADER_EXTATTR void read_Widget(struct Widget *s capnp_unused, Widget_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->id = capn_read32(p.p, 0);
	s->name = capn_get_text(p.p, 0, capn_val0);
}
EXTRAHEADER_EXTATTR void write_Widget(const struct Widget *s capnp_unused, Widget_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->id);
	capn_set_text(p.p, 0, s->name);
}
EXTRAHEADER_EXTATTR void get_Widget(struct Widget *s, Widget_list l, int i) {
	Widget_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Widget(s, p);
}
EXTRAHEADER_EXTATTR void set_Widget(const struct Widget *s, Widget_list l, int i) {
	Widget_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Widget(s, p);
}

EXTRAHEADER_EXTATTR uint32_t Widget_get_id(Widget_ptr p)
{
	uint32_t id;
	id = capn_read32(p.p, 0);
	return id;
}

EXTRAHEADER_EXTATTR capn_text Widget_get_name(Widget_ptr p)
{
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

EXTRAHEADER_EXTATTR void Widget_set_id(Widget_ptr p, uint32_t id)
{
	capn_write32(p.p, 0, id);
}

EXTRAHEADER_EXTATTR void Widget_set_name(Widget_ptr p, capn_text name)
{
	capn_set_text(p.p, 0, name);
}
