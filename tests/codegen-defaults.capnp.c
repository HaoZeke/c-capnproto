#include "codegen-defaults.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

static const capn_text capn_val0 = {0,"",0};
static const capn_ptr capn_null = {CAPN_NULL};
static const capn_text capn_text_null = {0,0,0};
static const uint8_t capn_buf[8] = {
	104,105,0,0,0,0,0,0
};
static const struct capn_segment capn_seg = {{0},0,0,0,(char*)&capn_buf[0],8,8,0};
const int32_t answer = ANSWER;
const unsigned flag = FLAG;
const uint16_t count = COUNT;
const uint64_t big = BIG;
const enum Color hue = (enum Color) HUE;
const union capn_conv_f32 ratio = {0x3fc00000u};
const int32_t answerCamel = ANSWER_CAMEL;
static capn_data capn_val1 = {{CAPN_NULL}};
static capn_ptr_list capn_val2 = {{CAPN_NULL}};

Rec_ptr new_Rec(struct capn_segment *s) {
	Rec_ptr p;
	p.p = capn_new_struct(s, 0, 2);
	return p;
}
Rec_list new_Rec_list(struct capn_segment *s, int len) {
	Rec_list p;
	p.p = capn_new_struct_list(s, len, 0, 2);
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
	capn_resolve(&p.p);
	capn_data info;
	info = capn_get_data(p.p, 0);
if (!info.p.type) {
	info = capn_val1;
}
	return info;
}

int Rec_has_info(Rec_ptr p)
{
	capn_resolve(&p.p);
	capn_ptr info;
	info = capn_getp(p.p, 0, 1);
	return info.type != CAPN_NULL;
}

capn_ptr_list Rec_get_tags(Rec_ptr p)
{
	capn_resolve(&p.p);
	capn_ptr_list tags;
	tags.p = capn_getp(p.p, 1, 1);
if (!tags.p.type) {
	tags = capn_val2;
}
	return tags;
}

int Rec_has_tags(Rec_ptr p)
{
	capn_resolve(&p.p);
	capn_ptr tags;
	tags = capn_getp(p.p, 1, 1);
	return tags.type != CAPN_NULL;
}

void Rec_set_info(Rec_ptr p, capn_data info)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, (info.p.data != capn_val1.p.data) ? info.p : capn_null);
}

void Rec_set_tags(Rec_ptr p, capn_ptr_list tags)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (tags.p.data != capn_val2.p.data) ? tags.p : capn_null);
}
static capn_text capn_val3 = {2,(char*)&capn_buf[0],(struct capn_segment*)&capn_seg};

Greeting_ptr new_Greeting(struct capn_segment *s) {
	Greeting_ptr p;
	p.p = capn_new_struct(s, 0, 2);
	return p;
}
Greeting_list new_Greeting_list(struct capn_segment *s, int len) {
	Greeting_list p;
	p.p = capn_new_struct_list(s, len, 0, 2);
	return p;
}
void read_Greeting(struct Greeting *s capnp_unused, Greeting_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->hello = capn_get_text(p.p, 0, capn_val3);
	s->plain = capn_get_text(p.p, 1, capn_val0);
}
void write_Greeting(const struct Greeting *s capnp_unused, Greeting_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, (s->hello.str != capn_val3.str) ? s->hello : capn_text_null);
	capn_set_text(p.p, 1, (s->plain.str != capn_val0.str) ? s->plain : capn_text_null);
}
void get_Greeting(struct Greeting *s, Greeting_list l, int i) {
	Greeting_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Greeting(s, p);
}
void set_Greeting(const struct Greeting *s, Greeting_list l, int i) {
	Greeting_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Greeting(s, p);
}

capn_text Greeting_get_hello(Greeting_ptr p)
{
	capn_resolve(&p.p);
	capn_text hello;
	hello = capn_get_text(p.p, 0, capn_val3);
	return hello;
}

int Greeting_has_hello(Greeting_ptr p)
{
	capn_resolve(&p.p);
	capn_ptr hello;
	hello = capn_getp(p.p, 0, 1);
	return hello.type != CAPN_NULL;
}

capn_text Greeting_get_plain(Greeting_ptr p)
{
	capn_resolve(&p.p);
	capn_text plain;
	plain = capn_get_text(p.p, 1, capn_val0);
	return plain;
}

int Greeting_has_plain(Greeting_ptr p)
{
	capn_resolve(&p.p);
	capn_ptr plain;
	plain = capn_getp(p.p, 1, 1);
	return plain.type != CAPN_NULL;
}

void Greeting_set_hello(Greeting_ptr p, capn_text hello)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (hello.str != capn_val3.str) ? hello : capn_text_null);
}

void Greeting_set_plain(Greeting_ptr p, capn_text plain)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 1, (plain.str != capn_val0.str) ? plain : capn_text_null);
}

Event_ptr new_Event(struct capn_segment *s) {
	Event_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Event_list new_Event_list(struct capn_segment *s, int len) {
	Event_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
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
	capn_setp(p.p, 0, (s->args.p.type != CAPN_NULL) ? s->args.p : capn_null);
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
	capn_resolve(&p.p);
	capn_ptr_list args;
	args.p = capn_getp(p.p, 0, 1);
	return args;
}

int Event_has_args(Event_ptr p)
{
	capn_resolve(&p.p);
	capn_ptr args;
	args = capn_getp(p.p, 0, 1);
	return args.type != CAPN_NULL;
}

void Event_set_args(Event_ptr p, capn_ptr_list args)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, (args.p.type != CAPN_NULL) ? args.p : capn_null);
}
