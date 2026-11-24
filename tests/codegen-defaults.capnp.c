#include "codegen-defaults.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

static const char capn_val0_str[1] = {0};
static const capn_text capn_val0 = {0,capn_val0_str,0};
static const capn_ptr capn_null = {CAPN_NULL,0,0,0,0,0,0,0,0,0,0,0};
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
static capn_data capn_val1 = {{CAPN_NULL,0,0,0,0,0,0,0,0,0,0,0}};
static capn_ptr_list capn_val2 = {{CAPN_NULL,0,0,0,0,0,0,0,0,0,0,0}};

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
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->info = capn_get_data(p.p, 0);
	if (!s->info.p.type) {
		s->info = capn_val1;
	}
	s->tags.p = capn_getp(p.p, 1, 0);
if (s->tags.p.type == CAPN_FAR_POINTER && s->tags.p.data != NULL && *(const uint64_t*)s->tags.p.data == 0) s->tags.p.type = CAPN_NULL;
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
	capn_resolve_struct_list(&l.p);
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
	capn_resolve_struct(&p.p);
	capn_data info;
	info = capn_get_data(p.p, 0);
	if (!info.p.type) {
		info = capn_val1;
	}
	return info;
}

int Rec_has_info(Rec_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr info;
	info = capn_getp(p.p, 0, 1);
	return info.type != CAPN_NULL;
}

capn_ptr_list Rec_get_tags(Rec_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr_list tags;
	tags.p = capn_getp(p.p, 1, 1);
	capn_resolve_list(&tags.p, CAPN_ELEM_POINTER);
	if (!tags.p.type) {
		tags = capn_val2;
	}
	return tags;
}

int Rec_has_tags(Rec_ptr p)
{
	capn_resolve_struct(&p.p);
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
	capn_resolve_struct(&p.p);
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
	capn_resolve_struct_list(&l.p);
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
	capn_resolve_struct(&p.p);
	capn_text hello;
	hello = capn_get_text(p.p, 0, capn_val3);
	return hello;
}

int Greeting_has_hello(Greeting_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr hello;
	hello = capn_getp(p.p, 0, 1);
	return hello.type != CAPN_NULL;
}

capn_text Greeting_get_plain(Greeting_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text plain;
	plain = capn_get_text(p.p, 1, capn_val0);
	return plain;
}

int Greeting_has_plain(Greeting_ptr p)
{
	capn_resolve_struct(&p.p);
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

Names_ptr new_Names(struct capn_segment *s) {
	Names_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Names_list new_Names_list(struct capn_segment *s, int len) {
	Names_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Names(struct Names *s capnp_unused, Names_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->_which = capn_read8(p.p, 0);
	s->_errno = capn_read32(p.p, 4);
	s->_linux = (capn_read8(p.p, 1) & 1) != 0;
	s->_stdin = capn_get_text(p.p, 0, capn_val0);
	s->voids_which = (enum Names_voids_which)(int) capn_read16(p.p, 2);
	switch (s->voids_which) {
	default:
		break;
	}
}
void write_Names(const struct Names *s capnp_unused, Names_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write8(p.p, 0, s->_which);
	capn_write32(p.p, 4, s->_errno);
	capn_write1(p.p, 8, s->_linux != 0);
	capn_set_text(p.p, 0, (s->_stdin.str != capn_val0.str) ? s->_stdin : capn_text_null);
	capn_write16(p.p, 2, s->voids_which);
	switch (s->voids_which) {
	default:
		break;
	}
}
void get_Names(struct Names *s, Names_list l, int i) {
	Names_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Names(s, p);
}
void set_Names(const struct Names *s, Names_list l, int i) {
	Names_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Names(s, p);
}

uint8_t Names_get__which(Names_ptr p)
{
	capn_resolve_struct(&p.p);
	uint8_t _which;
	_which = capn_read8(p.p, 0);
	return _which;
}

uint32_t Names_get__errno(Names_ptr p)
{
	capn_resolve_struct(&p.p);
	uint32_t _errno;
	_errno = capn_read32(p.p, 4);
	return _errno;
}

unsigned Names_get__linux(Names_ptr p)
{
	capn_resolve_struct(&p.p);
	unsigned _linux;
	_linux = (capn_read8(p.p, 1) & 1) != 0;
	return _linux;
}

capn_text Names_get__stdin(Names_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text _stdin;
	_stdin = capn_get_text(p.p, 0, capn_val0);
	return _stdin;
}

int Names_has__stdin(Names_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr _stdin;
	_stdin = capn_getp(p.p, 0, 1);
	return _stdin.type != CAPN_NULL;
}

void Names_set__which(Names_ptr p, uint8_t _which)
{
	capn_resolve(&p.p);
	capn_write8(p.p, 0, _which);
}

void Names_set__errno(Names_ptr p, uint32_t _errno)
{
	capn_resolve(&p.p);
	capn_write32(p.p, 4, _errno);
}

void Names_set__linux(Names_ptr p, unsigned _linux)
{
	capn_resolve(&p.p);
	capn_write1(p.p, 8, _linux != 0);
}

void Names_set__stdin(Names_ptr p, capn_text _stdin)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (_stdin.str != capn_val0.str) ? _stdin : capn_text_null);
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
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->args.p = capn_getp(p.p, 0, 0);
if (s->args.p.type == CAPN_FAR_POINTER && s->args.p.data != NULL && *(const uint64_t*)s->args.p.data == 0) s->args.p.type = CAPN_NULL;
}
void write_Event(const struct Event *s capnp_unused, Event_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->args.p.type != CAPN_NULL) ? s->args.p : capn_null);
}
void get_Event(struct Event *s, Event_list l, int i) {
	Event_ptr p;
	capn_resolve_struct_list(&l.p);
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
	capn_resolve_struct(&p.p);
	capn_ptr_list args;
	args.p = capn_getp(p.p, 0, 1);
	capn_resolve_list(&args.p, CAPN_ELEM_POINTER);
	return args;
}

int Event_has_args(Event_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr args;
	args = capn_getp(p.p, 0, 1);
	return args.type != CAPN_NULL;
}

void Event_set_args(Event_ptr p, capn_ptr_list args)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, (args.p.type != CAPN_NULL) ? args.p : capn_null);
}
