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
static const capn_text capn_text_null = {0,0,0};

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
