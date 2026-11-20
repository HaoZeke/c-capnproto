#include "adder.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

#include <string.h>
static const char capn_val0_str[1] = {0};
static const capn_text capn_val0 = {0,capn_val0_str,0};
static const capn_ptr capn_null = {CAPN_NULL};
static const capn_text capn_text_null = {0,0,0};

Adder_borrow_results_ptr new_Adder_borrow_results(struct capn_segment *s) {
	Adder_borrow_results_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Adder_borrow_results_list new_Adder_borrow_results_list(struct capn_segment *s, int len) {
	Adder_borrow_results_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Adder_borrow_results(struct Adder_borrow_results *s capnp_unused, Adder_borrow_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
	s->cap.p = capn_getp(p.p, 0, 0);
}
void write_Adder_borrow_results(const struct Adder_borrow_results *s capnp_unused, Adder_borrow_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
	capn_setp(p.p, 0, (s->cap.p.type != CAPN_NULL) ? s->cap.p : capn_null);
}
void get_Adder_borrow_results(struct Adder_borrow_results *s, Adder_borrow_results_list l, int i) {
	Adder_borrow_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_borrow_results(s, p);
}
void set_Adder_borrow_results(const struct Adder_borrow_results *s, Adder_borrow_results_list l, int i) {
	Adder_borrow_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_borrow_results(s, p);
}

Adder_borrow_params_ptr new_Adder_borrow_params(struct capn_segment *s) {
	Adder_borrow_params_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
Adder_borrow_params_list new_Adder_borrow_params_list(struct capn_segment *s, int len) {
	Adder_borrow_params_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_Adder_borrow_params(struct Adder_borrow_params *s capnp_unused, Adder_borrow_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
}
void write_Adder_borrow_params(const struct Adder_borrow_params *s capnp_unused, Adder_borrow_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
}
void get_Adder_borrow_params(struct Adder_borrow_params *s, Adder_borrow_params_list l, int i) {
	Adder_borrow_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_borrow_params(s, p);
}
void set_Adder_borrow_params(const struct Adder_borrow_params *s, Adder_borrow_params_list l, int i) {
	Adder_borrow_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_borrow_params(s, p);
}

Adder_seq_results_ptr new_Adder_seq_results(struct capn_segment *s) {
	Adder_seq_results_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
Adder_seq_results_list new_Adder_seq_results_list(struct capn_segment *s, int len) {
	Adder_seq_results_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_Adder_seq_results(struct Adder_seq_results *s capnp_unused, Adder_seq_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->prev = (int64_t) ((int64_t)(capn_read64(p.p, 0)));
}
void write_Adder_seq_results(const struct Adder_seq_results *s capnp_unused, Adder_seq_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, (uint64_t) (s->prev));
}
void get_Adder_seq_results(struct Adder_seq_results *s, Adder_seq_results_list l, int i) {
	Adder_seq_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_seq_results(s, p);
}
void set_Adder_seq_results(const struct Adder_seq_results *s, Adder_seq_results_list l, int i) {
	Adder_seq_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_seq_results(s, p);
}

Adder_seq_params_ptr new_Adder_seq_params(struct capn_segment *s) {
	Adder_seq_params_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
Adder_seq_params_list new_Adder_seq_params_list(struct capn_segment *s, int len) {
	Adder_seq_params_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_Adder_seq_params(struct Adder_seq_params *s capnp_unused, Adder_seq_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int64_t) ((int64_t)(capn_read64(p.p, 0)));
}
void write_Adder_seq_params(const struct Adder_seq_params *s capnp_unused, Adder_seq_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, (uint64_t) (s->n));
}
void get_Adder_seq_params(struct Adder_seq_params *s, Adder_seq_params_list l, int i) {
	Adder_seq_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_seq_params(s, p);
}
void set_Adder_seq_params(const struct Adder_seq_params *s, Adder_seq_params_list l, int i) {
	Adder_seq_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_seq_params(s, p);
}

Adder_echo_results_ptr new_Adder_echo_results(struct capn_segment *s) {
	Adder_echo_results_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Adder_echo_results_list new_Adder_echo_results_list(struct capn_segment *s, int len) {
	Adder_echo_results_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
	return p;
}
void read_Adder_echo_results(struct Adder_echo_results *s capnp_unused, Adder_echo_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->cap.p = capn_getp(p.p, 0, 0);
}
void write_Adder_echo_results(const struct Adder_echo_results *s capnp_unused, Adder_echo_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->cap.p.type != CAPN_NULL) ? s->cap.p : capn_null);
}
void get_Adder_echo_results(struct Adder_echo_results *s, Adder_echo_results_list l, int i) {
	Adder_echo_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_echo_results(s, p);
}
void set_Adder_echo_results(const struct Adder_echo_results *s, Adder_echo_results_list l, int i) {
	Adder_echo_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_echo_results(s, p);
}

Adder_echo_params_ptr new_Adder_echo_params(struct capn_segment *s) {
	Adder_echo_params_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Adder_echo_params_list new_Adder_echo_params_list(struct capn_segment *s, int len) {
	Adder_echo_params_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
	return p;
}
void read_Adder_echo_params(struct Adder_echo_params *s capnp_unused, Adder_echo_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->cap.p = capn_getp(p.p, 0, 0);
}
void write_Adder_echo_params(const struct Adder_echo_params *s capnp_unused, Adder_echo_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->cap.p.type != CAPN_NULL) ? s->cap.p : capn_null);
}
void get_Adder_echo_params(struct Adder_echo_params *s, Adder_echo_params_list l, int i) {
	Adder_echo_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_echo_params(s, p);
}
void set_Adder_echo_params(const struct Adder_echo_params *s, Adder_echo_params_list l, int i) {
	Adder_echo_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_echo_params(s, p);
}

Adder_watch_results_ptr new_Adder_watch_results(struct capn_segment *s) {
	Adder_watch_results_ptr p;
	p.p = capn_new_struct(s, 0, 0);
	return p;
}
Adder_watch_results_list new_Adder_watch_results_list(struct capn_segment *s, int len) {
	Adder_watch_results_list p;
	p.p = capn_new_struct_list(s, len, 0, 0);
	return p;
}
void read_Adder_watch_results(struct Adder_watch_results *s capnp_unused, Adder_watch_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void write_Adder_watch_results(const struct Adder_watch_results *s capnp_unused, Adder_watch_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void get_Adder_watch_results(struct Adder_watch_results *s, Adder_watch_results_list l, int i) {
	Adder_watch_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_watch_results(s, p);
}
void set_Adder_watch_results(const struct Adder_watch_results *s, Adder_watch_results_list l, int i) {
	Adder_watch_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_watch_results(s, p);
}

Adder_watch_params_ptr new_Adder_watch_params(struct capn_segment *s) {
	Adder_watch_params_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Adder_watch_params_list new_Adder_watch_params_list(struct capn_segment *s, int len) {
	Adder_watch_params_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
	return p;
}
void read_Adder_watch_params(struct Adder_watch_params *s capnp_unused, Adder_watch_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->cap.p = capn_getp(p.p, 0, 0);
}
void write_Adder_watch_params(const struct Adder_watch_params *s capnp_unused, Adder_watch_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->cap.p.type != CAPN_NULL) ? s->cap.p : capn_null);
}
void get_Adder_watch_params(struct Adder_watch_params *s, Adder_watch_params_list l, int i) {
	Adder_watch_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_watch_params(s, p);
}
void set_Adder_watch_params(const struct Adder_watch_params *s, Adder_watch_params_list l, int i) {
	Adder_watch_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_watch_params(s, p);
}

Adder_fulfill_results_ptr new_Adder_fulfill_results(struct capn_segment *s) {
	Adder_fulfill_results_ptr p;
	p.p = capn_new_struct(s, 0, 0);
	return p;
}
Adder_fulfill_results_list new_Adder_fulfill_results_list(struct capn_segment *s, int len) {
	Adder_fulfill_results_list p;
	p.p = capn_new_struct_list(s, len, 0, 0);
	return p;
}
void read_Adder_fulfill_results(struct Adder_fulfill_results *s capnp_unused, Adder_fulfill_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void write_Adder_fulfill_results(const struct Adder_fulfill_results *s capnp_unused, Adder_fulfill_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void get_Adder_fulfill_results(struct Adder_fulfill_results *s, Adder_fulfill_results_list l, int i) {
	Adder_fulfill_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_fulfill_results(s, p);
}
void set_Adder_fulfill_results(const struct Adder_fulfill_results *s, Adder_fulfill_results_list l, int i) {
	Adder_fulfill_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_fulfill_results(s, p);
}

Adder_fulfill_params_ptr new_Adder_fulfill_params(struct capn_segment *s) {
	Adder_fulfill_params_ptr p;
	p.p = capn_new_struct(s, 0, 0);
	return p;
}
Adder_fulfill_params_list new_Adder_fulfill_params_list(struct capn_segment *s, int len) {
	Adder_fulfill_params_list p;
	p.p = capn_new_struct_list(s, len, 0, 0);
	return p;
}
void read_Adder_fulfill_params(struct Adder_fulfill_params *s capnp_unused, Adder_fulfill_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void write_Adder_fulfill_params(const struct Adder_fulfill_params *s capnp_unused, Adder_fulfill_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void get_Adder_fulfill_params(struct Adder_fulfill_params *s, Adder_fulfill_params_list l, int i) {
	Adder_fulfill_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_fulfill_params(s, p);
}
void set_Adder_fulfill_params(const struct Adder_fulfill_params *s, Adder_fulfill_params_list l, int i) {
	Adder_fulfill_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_fulfill_params(s, p);
}

Adder_defer_results_ptr new_Adder_defer_results(struct capn_segment *s) {
	Adder_defer_results_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Adder_defer_results_list new_Adder_defer_results_list(struct capn_segment *s, int len) {
	Adder_defer_results_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
	return p;
}
void read_Adder_defer_results(struct Adder_defer_results *s capnp_unused, Adder_defer_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->cap.p = capn_getp(p.p, 0, 0);
}
void write_Adder_defer_results(const struct Adder_defer_results *s capnp_unused, Adder_defer_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->cap.p.type != CAPN_NULL) ? s->cap.p : capn_null);
}
void get_Adder_defer_results(struct Adder_defer_results *s, Adder_defer_results_list l, int i) {
	Adder_defer_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_defer_results(s, p);
}
void set_Adder_defer_results(const struct Adder_defer_results *s, Adder_defer_results_list l, int i) {
	Adder_defer_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_defer_results(s, p);
}

Adder_defer_params_ptr new_Adder_defer_params(struct capn_segment *s) {
	Adder_defer_params_ptr p;
	p.p = capn_new_struct(s, 0, 0);
	return p;
}
Adder_defer_params_list new_Adder_defer_params_list(struct capn_segment *s, int len) {
	Adder_defer_params_list p;
	p.p = capn_new_struct_list(s, len, 0, 0);
	return p;
}
void read_Adder_defer_params(struct Adder_defer_params *s capnp_unused, Adder_defer_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void write_Adder_defer_params(const struct Adder_defer_params *s capnp_unused, Adder_defer_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void get_Adder_defer_params(struct Adder_defer_params *s, Adder_defer_params_list l, int i) {
	Adder_defer_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_defer_params(s, p);
}
void set_Adder_defer_params(const struct Adder_defer_params *s, Adder_defer_params_list l, int i) {
	Adder_defer_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_defer_params(s, p);
}

Adder_tag_results_ptr new_Adder_tag_results(struct capn_segment *s) {
	Adder_tag_results_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Adder_tag_results_list new_Adder_tag_results_list(struct capn_segment *s, int len) {
	Adder_tag_results_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Adder_tag_results(struct Adder_tag_results *s capnp_unused, Adder_tag_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
	s->label = capn_get_text(p.p, 0, capn_val0);
}
void write_Adder_tag_results(const struct Adder_tag_results *s capnp_unused, Adder_tag_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
	capn_set_text(p.p, 0, (s->label.str != capn_val0.str) ? s->label : capn_text_null);
}
void get_Adder_tag_results(struct Adder_tag_results *s, Adder_tag_results_list l, int i) {
	Adder_tag_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_tag_results(s, p);
}
void set_Adder_tag_results(const struct Adder_tag_results *s, Adder_tag_results_list l, int i) {
	Adder_tag_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_tag_results(s, p);
}

Adder_tag_params_ptr new_Adder_tag_params(struct capn_segment *s) {
	Adder_tag_params_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Adder_tag_params_list new_Adder_tag_params_list(struct capn_segment *s, int len) {
	Adder_tag_params_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Adder_tag_params(struct Adder_tag_params *s capnp_unused, Adder_tag_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
	s->label = capn_get_text(p.p, 0, capn_val0);
}
void write_Adder_tag_params(const struct Adder_tag_params *s capnp_unused, Adder_tag_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
	capn_set_text(p.p, 0, (s->label.str != capn_val0.str) ? s->label : capn_text_null);
}
void get_Adder_tag_params(struct Adder_tag_params *s, Adder_tag_params_list l, int i) {
	Adder_tag_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_tag_params(s, p);
}
void set_Adder_tag_params(const struct Adder_tag_params *s, Adder_tag_params_list l, int i) {
	Adder_tag_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_tag_params(s, p);
}

Adder_add_results_ptr new_Adder_add_results(struct capn_segment *s) {
	Adder_add_results_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
Adder_add_results_list new_Adder_add_results_list(struct capn_segment *s, int len) {
	Adder_add_results_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_Adder_add_results(struct Adder_add_results *s capnp_unused, Adder_add_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->sum = (int64_t) ((int64_t)(capn_read64(p.p, 0)));
}
void write_Adder_add_results(const struct Adder_add_results *s capnp_unused, Adder_add_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, (uint64_t) (s->sum));
}
void get_Adder_add_results(struct Adder_add_results *s, Adder_add_results_list l, int i) {
	Adder_add_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_add_results(s, p);
}
void set_Adder_add_results(const struct Adder_add_results *s, Adder_add_results_list l, int i) {
	Adder_add_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_add_results(s, p);
}

Adder_add_params_ptr new_Adder_add_params(struct capn_segment *s) {
	Adder_add_params_ptr p;
	p.p = capn_new_struct(s, 16, 0);
	return p;
}
Adder_add_params_list new_Adder_add_params_list(struct capn_segment *s, int len) {
	Adder_add_params_list p;
	p.p = capn_new_struct_list(s, len, 16, 0);
	return p;
}
void read_Adder_add_params(struct Adder_add_params *s capnp_unused, Adder_add_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->a = (int64_t) ((int64_t)(capn_read64(p.p, 0)));
	s->b = (int64_t) ((int64_t)(capn_read64(p.p, 8)));
}
void write_Adder_add_params(const struct Adder_add_params *s capnp_unused, Adder_add_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, (uint64_t) (s->a));
	capn_write64(p.p, 8, (uint64_t) (s->b));
}
void get_Adder_add_params(struct Adder_add_params *s, Adder_add_params_list l, int i) {
	Adder_add_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Adder_add_params(s, p);
}
void set_Adder_add_params(const struct Adder_add_params *s, Adder_add_params_list l, int i) {
	Adder_add_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Adder_add_params(s, p);
}

CountBase_inc_results_ptr new_CountBase_inc_results(struct capn_segment *s) {
	CountBase_inc_results_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
CountBase_inc_results_list new_CountBase_inc_results_list(struct capn_segment *s, int len) {
	CountBase_inc_results_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_CountBase_inc_results(struct CountBase_inc_results *s capnp_unused, CountBase_inc_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
}
void write_CountBase_inc_results(const struct CountBase_inc_results *s capnp_unused, CountBase_inc_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
}
void get_CountBase_inc_results(struct CountBase_inc_results *s, CountBase_inc_results_list l, int i) {
	CountBase_inc_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_CountBase_inc_results(s, p);
}
void set_CountBase_inc_results(const struct CountBase_inc_results *s, CountBase_inc_results_list l, int i) {
	CountBase_inc_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CountBase_inc_results(s, p);
}

CountBase_inc_params_ptr new_CountBase_inc_params(struct capn_segment *s) {
	CountBase_inc_params_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
CountBase_inc_params_list new_CountBase_inc_params_list(struct capn_segment *s, int len) {
	CountBase_inc_params_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_CountBase_inc_params(struct CountBase_inc_params *s capnp_unused, CountBase_inc_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
}
void write_CountBase_inc_params(const struct CountBase_inc_params *s capnp_unused, CountBase_inc_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
}
void get_CountBase_inc_params(struct CountBase_inc_params *s, CountBase_inc_params_list l, int i) {
	CountBase_inc_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_CountBase_inc_params(s, p);
}
void set_CountBase_inc_params(const struct CountBase_inc_params *s, CountBase_inc_params_list l, int i) {
	CountBase_inc_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CountBase_inc_params(s, p);
}

CountMid_twice_results_ptr new_CountMid_twice_results(struct capn_segment *s) {
	CountMid_twice_results_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
CountMid_twice_results_list new_CountMid_twice_results_list(struct capn_segment *s, int len) {
	CountMid_twice_results_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_CountMid_twice_results(struct CountMid_twice_results *s capnp_unused, CountMid_twice_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
}
void write_CountMid_twice_results(const struct CountMid_twice_results *s capnp_unused, CountMid_twice_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
}
void get_CountMid_twice_results(struct CountMid_twice_results *s, CountMid_twice_results_list l, int i) {
	CountMid_twice_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_CountMid_twice_results(s, p);
}
void set_CountMid_twice_results(const struct CountMid_twice_results *s, CountMid_twice_results_list l, int i) {
	CountMid_twice_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CountMid_twice_results(s, p);
}

CountMid_twice_params_ptr new_CountMid_twice_params(struct capn_segment *s) {
	CountMid_twice_params_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
CountMid_twice_params_list new_CountMid_twice_params_list(struct capn_segment *s, int len) {
	CountMid_twice_params_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_CountMid_twice_params(struct CountMid_twice_params *s capnp_unused, CountMid_twice_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
}
void write_CountMid_twice_params(const struct CountMid_twice_params *s capnp_unused, CountMid_twice_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
}
void get_CountMid_twice_params(struct CountMid_twice_params *s, CountMid_twice_params_list l, int i) {
	CountMid_twice_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_CountMid_twice_params(s, p);
}
void set_CountMid_twice_params(const struct CountMid_twice_params *s, CountMid_twice_params_list l, int i) {
	CountMid_twice_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CountMid_twice_params(s, p);
}

CountLeaf_dec_results_ptr new_CountLeaf_dec_results(struct capn_segment *s) {
	CountLeaf_dec_results_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
CountLeaf_dec_results_list new_CountLeaf_dec_results_list(struct capn_segment *s, int len) {
	CountLeaf_dec_results_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_CountLeaf_dec_results(struct CountLeaf_dec_results *s capnp_unused, CountLeaf_dec_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
}
void write_CountLeaf_dec_results(const struct CountLeaf_dec_results *s capnp_unused, CountLeaf_dec_results_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
}
void get_CountLeaf_dec_results(struct CountLeaf_dec_results *s, CountLeaf_dec_results_list l, int i) {
	CountLeaf_dec_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_CountLeaf_dec_results(s, p);
}
void set_CountLeaf_dec_results(const struct CountLeaf_dec_results *s, CountLeaf_dec_results_list l, int i) {
	CountLeaf_dec_results_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CountLeaf_dec_results(s, p);
}

CountLeaf_dec_params_ptr new_CountLeaf_dec_params(struct capn_segment *s) {
	CountLeaf_dec_params_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
CountLeaf_dec_params_list new_CountLeaf_dec_params_list(struct capn_segment *s, int len) {
	CountLeaf_dec_params_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_CountLeaf_dec_params(struct CountLeaf_dec_params *s capnp_unused, CountLeaf_dec_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->n = (int32_t) ((int32_t)capn_read32(p.p, 0));
}
void write_CountLeaf_dec_params(const struct CountLeaf_dec_params *s capnp_unused, CountLeaf_dec_params_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->n));
}
void get_CountLeaf_dec_params(struct CountLeaf_dec_params *s, CountLeaf_dec_params_list l, int i) {
	CountLeaf_dec_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_CountLeaf_dec_params(s, p);
}
void set_CountLeaf_dec_params(const struct CountLeaf_dec_params *s, CountLeaf_dec_params_list l, int i) {
	CountLeaf_dec_params_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CountLeaf_dec_params(s, p);
}

Adder_ptr new_Adder(struct capn_segment *s) {
	Adder_ptr p;
	p.p = capn_new_interface(s, 0, 0);
	return p;
}
Adder_list new_Adder_list(struct capn_segment *s, int len) {
	Adder_list p;
	p.p = capn_new_ptr_list(s, len);
	return p;
}

int Adder_dispatch(void *server, uint64_t interface_id, uint16_t method_id, capn_ptr params, capn_ptr results) {
	const struct Adder_server *srv = (const struct Adder_server *) server;
	capnp_use(srv);
	capnp_use(results);
	if (interface_id != Adder_INTERFACE_ID) {
		return -1;
	}
	switch (method_id) {
	case 0: {
		struct Adder_add_params in;
		Adder_add_params_ptr pp;
		int rc;
		struct Adder_add_results out;
		Adder_add_results_ptr rp;
		if (srv->on_add == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_Adder_add_params(&in, pp);
		rc = srv->on_add(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_Adder_add_results(&out, rp);
		return 0;
	}
	case 1: {
		struct Adder_tag_params in;
		Adder_tag_params_ptr pp;
		int rc;
		struct Adder_tag_results out;
		Adder_tag_results_ptr rp;
		if (srv->on_tag == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_Adder_tag_params(&in, pp);
		rc = srv->on_tag(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_Adder_tag_results(&out, rp);
		return 0;
	}
	case 2: {
		struct Adder_defer_params in;
		Adder_defer_params_ptr pp;
		int rc;
		struct Adder_defer_results out;
		Adder_defer_results_ptr rp;
		if (srv->on_defer == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_Adder_defer_params(&in, pp);
		rc = srv->on_defer(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_Adder_defer_results(&out, rp);
		return 0;
	}
	case 3: {
		struct Adder_fulfill_params in;
		Adder_fulfill_params_ptr pp;
		int rc;
		struct Adder_fulfill_results out;
		Adder_fulfill_results_ptr rp;
		if (srv->on_fulfill == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_Adder_fulfill_params(&in, pp);
		rc = srv->on_fulfill(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_Adder_fulfill_results(&out, rp);
		return 0;
	}
	case 4: {
		struct Adder_watch_params in;
		Adder_watch_params_ptr pp;
		int rc;
		struct Adder_watch_results out;
		Adder_watch_results_ptr rp;
		if (srv->on_watch == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_Adder_watch_params(&in, pp);
		rc = srv->on_watch(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_Adder_watch_results(&out, rp);
		return 0;
	}
	case 5: {
		struct Adder_echo_params in;
		Adder_echo_params_ptr pp;
		int rc;
		struct Adder_echo_results out;
		Adder_echo_results_ptr rp;
		if (srv->on_echo == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_Adder_echo_params(&in, pp);
		rc = srv->on_echo(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_Adder_echo_results(&out, rp);
		return 0;
	}
	case 6: {
		struct Adder_seq_params in;
		Adder_seq_params_ptr pp;
		int rc;
		struct Adder_seq_results out;
		Adder_seq_results_ptr rp;
		if (srv->on_seq == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_Adder_seq_params(&in, pp);
		rc = srv->on_seq(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_Adder_seq_results(&out, rp);
		return 0;
	}
	case 7: {
		struct Adder_borrow_params in;
		Adder_borrow_params_ptr pp;
		int rc;
		struct Adder_borrow_results out;
		Adder_borrow_results_ptr rp;
		if (srv->on_borrow == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_Adder_borrow_params(&in, pp);
		rc = srv->on_borrow(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_Adder_borrow_results(&out, rp);
		return 0;
	}
	default:
		return -1;
	}
}

static void Adder_add_fill(void *ctx, capn_ptr params) {
	Adder_add_params_ptr pp;
	pp.p = params;
	write_Adder_add_params((const struct Adder_add_params *) ctx, pp);
}
uint32_t Adder_call_add(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_add_params *in) {
	return capn_rpc_send_call(c, cap, Adder_INTERFACE_ID, 0, (int) (Adder_add_params_word_count * 8), (int) Adder_add_params_pointer_count, Adder_add_fill, (void *) in);
}
int Adder_results_add(struct capn_rpc_conn *c, uint32_t question, struct Adder_add_results *out, struct capn *msg) {
	capn_ptr content;
	Adder_add_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_Adder_add_results(out, rp);
	return 0;
}
int Adder_return_add(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct Adder_add_results *out) {
	capn_ptr results;
	Adder_add_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_Adder_add_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

static void Adder_tag_fill(void *ctx, capn_ptr params) {
	Adder_tag_params_ptr pp;
	pp.p = params;
	write_Adder_tag_params((const struct Adder_tag_params *) ctx, pp);
}
uint32_t Adder_call_tag(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_tag_params *in) {
	return capn_rpc_send_call(c, cap, Adder_INTERFACE_ID, 1, (int) (Adder_tag_params_word_count * 8), (int) Adder_tag_params_pointer_count, Adder_tag_fill, (void *) in);
}
int Adder_results_tag(struct capn_rpc_conn *c, uint32_t question, struct Adder_tag_results *out, struct capn *msg) {
	capn_ptr content;
	Adder_tag_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_Adder_tag_results(out, rp);
	return 0;
}
int Adder_return_tag(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct Adder_tag_results *out) {
	capn_ptr results;
	Adder_tag_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_Adder_tag_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

static void Adder_defer_fill(void *ctx, capn_ptr params) {
	Adder_defer_params_ptr pp;
	pp.p = params;
	write_Adder_defer_params((const struct Adder_defer_params *) ctx, pp);
}
uint32_t Adder_call_defer(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_defer_params *in) {
	return capn_rpc_send_call(c, cap, Adder_INTERFACE_ID, 2, (int) (Adder_defer_params_word_count * 8), (int) Adder_defer_params_pointer_count, Adder_defer_fill, (void *) in);
}
int Adder_results_defer(struct capn_rpc_conn *c, uint32_t question, struct Adder_defer_results *out, struct capn *msg) {
	capn_ptr content;
	Adder_defer_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_Adder_defer_results(out, rp);
	return 0;
}
int Adder_return_defer(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct Adder_defer_results *out) {
	capn_ptr results;
	Adder_defer_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_Adder_defer_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

static void Adder_fulfill_fill(void *ctx, capn_ptr params) {
	Adder_fulfill_params_ptr pp;
	pp.p = params;
	write_Adder_fulfill_params((const struct Adder_fulfill_params *) ctx, pp);
}
uint32_t Adder_call_fulfill(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_fulfill_params *in) {
	return capn_rpc_send_call(c, cap, Adder_INTERFACE_ID, 3, (int) (Adder_fulfill_params_word_count * 8), (int) Adder_fulfill_params_pointer_count, Adder_fulfill_fill, (void *) in);
}
int Adder_results_fulfill(struct capn_rpc_conn *c, uint32_t question, struct Adder_fulfill_results *out, struct capn *msg) {
	capn_ptr content;
	Adder_fulfill_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_Adder_fulfill_results(out, rp);
	return 0;
}
int Adder_return_fulfill(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct Adder_fulfill_results *out) {
	capn_ptr results;
	Adder_fulfill_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_Adder_fulfill_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

static void Adder_watch_fill(void *ctx, capn_ptr params) {
	Adder_watch_params_ptr pp;
	pp.p = params;
	write_Adder_watch_params((const struct Adder_watch_params *) ctx, pp);
}
uint32_t Adder_call_watch(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_watch_params *in) {
	return capn_rpc_send_call(c, cap, Adder_INTERFACE_ID, 4, (int) (Adder_watch_params_word_count * 8), (int) Adder_watch_params_pointer_count, Adder_watch_fill, (void *) in);
}
int Adder_results_watch(struct capn_rpc_conn *c, uint32_t question, struct Adder_watch_results *out, struct capn *msg) {
	capn_ptr content;
	Adder_watch_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_Adder_watch_results(out, rp);
	return 0;
}
int Adder_return_watch(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct Adder_watch_results *out) {
	capn_ptr results;
	Adder_watch_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_Adder_watch_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

static void Adder_echo_fill(void *ctx, capn_ptr params) {
	Adder_echo_params_ptr pp;
	pp.p = params;
	write_Adder_echo_params((const struct Adder_echo_params *) ctx, pp);
}
uint32_t Adder_call_echo(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_echo_params *in) {
	return capn_rpc_send_call(c, cap, Adder_INTERFACE_ID, 5, (int) (Adder_echo_params_word_count * 8), (int) Adder_echo_params_pointer_count, Adder_echo_fill, (void *) in);
}
int Adder_results_echo(struct capn_rpc_conn *c, uint32_t question, struct Adder_echo_results *out, struct capn *msg) {
	capn_ptr content;
	Adder_echo_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_Adder_echo_results(out, rp);
	return 0;
}
int Adder_return_echo(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct Adder_echo_results *out) {
	capn_ptr results;
	Adder_echo_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_Adder_echo_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

static void Adder_seq_fill(void *ctx, capn_ptr params) {
	Adder_seq_params_ptr pp;
	pp.p = params;
	write_Adder_seq_params((const struct Adder_seq_params *) ctx, pp);
}
uint32_t Adder_call_seq(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_seq_params *in) {
	return capn_rpc_send_call(c, cap, Adder_INTERFACE_ID, 6, (int) (Adder_seq_params_word_count * 8), (int) Adder_seq_params_pointer_count, Adder_seq_fill, (void *) in);
}
int Adder_results_seq(struct capn_rpc_conn *c, uint32_t question, struct Adder_seq_results *out, struct capn *msg) {
	capn_ptr content;
	Adder_seq_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_Adder_seq_results(out, rp);
	return 0;
}
int Adder_return_seq(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct Adder_seq_results *out) {
	capn_ptr results;
	Adder_seq_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_Adder_seq_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

static void Adder_borrow_fill(void *ctx, capn_ptr params) {
	Adder_borrow_params_ptr pp;
	pp.p = params;
	write_Adder_borrow_params((const struct Adder_borrow_params *) ctx, pp);
}
uint32_t Adder_call_borrow(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_borrow_params *in) {
	return capn_rpc_send_call(c, cap, Adder_INTERFACE_ID, 7, (int) (Adder_borrow_params_word_count * 8), (int) Adder_borrow_params_pointer_count, Adder_borrow_fill, (void *) in);
}
int Adder_results_borrow(struct capn_rpc_conn *c, uint32_t question, struct Adder_borrow_results *out, struct capn *msg) {
	capn_ptr content;
	Adder_borrow_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_Adder_borrow_results(out, rp);
	return 0;
}
int Adder_return_borrow(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct Adder_borrow_results *out) {
	capn_ptr results;
	Adder_borrow_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_Adder_borrow_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

CountBase_ptr new_CountBase(struct capn_segment *s) {
	CountBase_ptr p;
	p.p = capn_new_interface(s, 0, 0);
	return p;
}
CountBase_list new_CountBase_list(struct capn_segment *s, int len) {
	CountBase_list p;
	p.p = capn_new_ptr_list(s, len);
	return p;
}

int CountBase_dispatch(void *server, uint64_t interface_id, uint16_t method_id, capn_ptr params, capn_ptr results) {
	const struct CountBase_server *srv = (const struct CountBase_server *) server;
	capnp_use(srv);
	capnp_use(results);
	if (interface_id != CountBase_INTERFACE_ID) {
		return -1;
	}
	switch (method_id) {
	case 0: {
		struct CountBase_inc_params in;
		CountBase_inc_params_ptr pp;
		int rc;
		struct CountBase_inc_results out;
		CountBase_inc_results_ptr rp;
		if (srv->on_inc == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_CountBase_inc_params(&in, pp);
		rc = srv->on_inc(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_CountBase_inc_results(&out, rp);
		return 0;
	}
	default:
		return -1;
	}
}

static void CountBase_inc_fill(void *ctx, capn_ptr params) {
	CountBase_inc_params_ptr pp;
	pp.p = params;
	write_CountBase_inc_params((const struct CountBase_inc_params *) ctx, pp);
}
uint32_t CountBase_call_inc(struct capn_rpc_conn *c, uint32_t cap, const struct CountBase_inc_params *in) {
	return capn_rpc_send_call(c, cap, CountBase_INTERFACE_ID, 0, (int) (CountBase_inc_params_word_count * 8), (int) CountBase_inc_params_pointer_count, CountBase_inc_fill, (void *) in);
}
int CountBase_results_inc(struct capn_rpc_conn *c, uint32_t question, struct CountBase_inc_results *out, struct capn *msg) {
	capn_ptr content;
	CountBase_inc_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_CountBase_inc_results(out, rp);
	return 0;
}
int CountBase_return_inc(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct CountBase_inc_results *out) {
	capn_ptr results;
	CountBase_inc_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_CountBase_inc_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

CountMid_ptr new_CountMid(struct capn_segment *s) {
	CountMid_ptr p;
	p.p = capn_new_interface(s, 0, 0);
	return p;
}
CountMid_list new_CountMid_list(struct capn_segment *s, int len) {
	CountMid_list p;
	p.p = capn_new_ptr_list(s, len);
	return p;
}

int CountMid_dispatch(void *server, uint64_t interface_id, uint16_t method_id, capn_ptr params, capn_ptr results) {
	const struct CountMid_server *srv = (const struct CountMid_server *) server;
	capnp_use(srv);
	capnp_use(results);
	if (interface_id != CountMid_INTERFACE_ID) {
		if (srv->super_CountBase != NULL && (interface_id == CountBase_INTERFACE_ID))
			return CountBase_dispatch((void *) srv->super_CountBase, interface_id, method_id, params, results);
		return -1;
	}
	switch (method_id) {
	case 0: {
		struct CountMid_twice_params in;
		CountMid_twice_params_ptr pp;
		int rc;
		struct CountMid_twice_results out;
		CountMid_twice_results_ptr rp;
		if (srv->on_twice == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_CountMid_twice_params(&in, pp);
		rc = srv->on_twice(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_CountMid_twice_results(&out, rp);
		return 0;
	}
	default:
		return -1;
	}
}

static void CountMid_twice_fill(void *ctx, capn_ptr params) {
	CountMid_twice_params_ptr pp;
	pp.p = params;
	write_CountMid_twice_params((const struct CountMid_twice_params *) ctx, pp);
}
uint32_t CountMid_call_twice(struct capn_rpc_conn *c, uint32_t cap, const struct CountMid_twice_params *in) {
	return capn_rpc_send_call(c, cap, CountMid_INTERFACE_ID, 0, (int) (CountMid_twice_params_word_count * 8), (int) CountMid_twice_params_pointer_count, CountMid_twice_fill, (void *) in);
}
int CountMid_results_twice(struct capn_rpc_conn *c, uint32_t question, struct CountMid_twice_results *out, struct capn *msg) {
	capn_ptr content;
	CountMid_twice_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_CountMid_twice_results(out, rp);
	return 0;
}
int CountMid_return_twice(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct CountMid_twice_results *out) {
	capn_ptr results;
	CountMid_twice_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_CountMid_twice_results(out, rp);
	return capn_rpc_return(c, question, generation);
}

CountLeaf_ptr new_CountLeaf(struct capn_segment *s) {
	CountLeaf_ptr p;
	p.p = capn_new_interface(s, 0, 0);
	return p;
}
CountLeaf_list new_CountLeaf_list(struct capn_segment *s, int len) {
	CountLeaf_list p;
	p.p = capn_new_ptr_list(s, len);
	return p;
}

int CountLeaf_dispatch(void *server, uint64_t interface_id, uint16_t method_id, capn_ptr params, capn_ptr results) {
	const struct CountLeaf_server *srv = (const struct CountLeaf_server *) server;
	capnp_use(srv);
	capnp_use(results);
	if (interface_id != CountLeaf_INTERFACE_ID) {
		if (srv->super_CountMid != NULL && (interface_id == CountMid_INTERFACE_ID || interface_id == CountBase_INTERFACE_ID))
			return CountMid_dispatch((void *) srv->super_CountMid, interface_id, method_id, params, results);
		if (srv->super_Adder != NULL && (interface_id == Adder_INTERFACE_ID))
			return Adder_dispatch((void *) srv->super_Adder, interface_id, method_id, params, results);
		return -1;
	}
	switch (method_id) {
	case 0: {
		struct CountLeaf_dec_params in;
		CountLeaf_dec_params_ptr pp;
		int rc;
		struct CountLeaf_dec_results out;
		CountLeaf_dec_results_ptr rp;
		if (srv->on_dec == NULL)
			return -1;
		memset(&in, 0, sizeof in);
		memset(&out, 0, sizeof out);
		pp.p = params;
		read_CountLeaf_dec_params(&in, pp);
		rc = srv->on_dec(srv->self, &in, &out, results.seg);
		if (rc != 0)
			return rc;
		rp.p = results;
		write_CountLeaf_dec_results(&out, rp);
		return 0;
	}
	default:
		return -1;
	}
}

static void CountLeaf_dec_fill(void *ctx, capn_ptr params) {
	CountLeaf_dec_params_ptr pp;
	pp.p = params;
	write_CountLeaf_dec_params((const struct CountLeaf_dec_params *) ctx, pp);
}
uint32_t CountLeaf_call_dec(struct capn_rpc_conn *c, uint32_t cap, const struct CountLeaf_dec_params *in) {
	return capn_rpc_send_call(c, cap, CountLeaf_INTERFACE_ID, 0, (int) (CountLeaf_dec_params_word_count * 8), (int) CountLeaf_dec_params_pointer_count, CountLeaf_dec_fill, (void *) in);
}
int CountLeaf_results_dec(struct capn_rpc_conn *c, uint32_t question, struct CountLeaf_dec_results *out, struct capn *msg) {
	capn_ptr content;
	CountLeaf_dec_results_ptr rp;
	if (capn_rpc_answer_content(c, question, msg, &content) != 0)
		return -1;
	rp.p = content;
	read_CountLeaf_dec_results(out, rp);
	return 0;
}
int CountLeaf_return_dec(struct capn_rpc_conn *c, uint32_t question, uint32_t generation, const struct CountLeaf_dec_results *out) {
	capn_ptr results;
	CountLeaf_dec_results_ptr rp;
	if (out == NULL)
		return -1;
	results = capn_rpc_pending_results(c, question, generation);
	if (results.type == CAPN_NULL)
		return -1;
	rp.p = results;
	write_CountLeaf_dec_results(out, rp);
	return capn_rpc_return(c, question, generation);
}
