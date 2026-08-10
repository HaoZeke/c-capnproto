#include "codegen-defaults.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

static const capn_ptr capn_null = {CAPN_NULL};
int32_t answer = 42;
unsigned flag = 1;
uint16_t count = 7;
uint64_t big = ((uint64_t) 0x1u << 32) | 0u;
enum Color hue = (enum Color) 1u;
union capn_conv_f32 ratio = {0x3fc00000u};
int32_t answerCamel = 99;
static capn_data capn_val1 = {{CAPN_NULL}};
static capn_ptr_list capn_val2 = {{CAPN_NULL}};

Rec_ptr new_Rec(struct capn_segment *s) {
	Rec_ptr p;
	p.p = capn_new_struct(s, 0, 2);
	return p;
}
Rec_list new_Rec_list(struct capn_segment *s, int len) {
	Rec_list p;
	p.p = capn_new_list(s, len, 0, 2);
	return p;
}
void read_Rec(struct Rec *s capnp_unused, Rec_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->info = capn_get_data(p.p, 0);
	if (!s->info.p.type) {
		s->info = capn_val1;
	}
	s->tags.p = capn_getp(p.p, 1, 0);
	if (!s->tags.p.type) {
		s->tags = capn_val2;
	}
}
void write_Rec(const struct Rec *s capnp_unused, Rec_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->info.p.data != capn_val1.p.data) ? s->info.p : capn_null);
	capn_setp(p.p, 1, (s->tags.p.data != capn_val2.p.data) ? s->tags.p : capn_null);
}
void get_Rec(struct Rec *s, Rec_list l, int i) {
	Rec_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Rec(s, p);
}
void set_Rec(const struct Rec *s, Rec_list l, int i) {
	Rec_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Rec(s, p);
}

capn_data Rec_get_info(Rec_ptr p)
{
	capn_data info;
	info = capn_get_data(p.p, 0);
if (!info.p.type) {
	info = capn_val1;
}
	return info;
}

capn_ptr_list Rec_get_tags(Rec_ptr p)
{
	capn_ptr_list tags;
	tags.p = capn_getp(p.p, 1, 0);
if (!tags.p.type) {
	tags = capn_val2;
}
	return tags;
}

void Rec_set_info(Rec_ptr p, capn_data info)
{
	capn_setp(p.p, 0, (info.p.data != capn_val1.p.data) ? info.p : capn_null);
}

void Rec_set_tags(Rec_ptr p, capn_ptr_list tags)
{
	capn_setp(p.p, 1, (tags.p.data != capn_val2.p.data) ? tags.p : capn_null);
}

Event_ptr new_Event(struct capn_segment *s) {
	Event_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Event_list new_Event_list(struct capn_segment *s, int len) {
	Event_list p;
	p.p = capn_new_list(s, len, 0, 1);
	return p;
}
void read_Event(struct Event *s capnp_unused, Event_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->args.p = capn_getp(p.p, 0, 0);
}
void write_Event(const struct Event *s capnp_unused, Event_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, s->args.p);
}
void get_Event(struct Event *s, Event_list l, int i) {
	Event_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Event(s, p);
}
void set_Event(const struct Event *s, Event_list l, int i) {
	Event_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Event(s, p);
}

capn_ptr_list Event_get_args(Event_ptr p)
{
	capn_ptr_list args;
	args.p = capn_getp(p.p, 0, 0);
	return args;
}

void Event_set_args(Event_ptr p, capn_ptr_list args)
{
	capn_setp(p.p, 0, args.p);
}
