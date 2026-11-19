#ifndef CAPN_BF5E831AC9F0D2A1
#define CAPN_BF5E831AC9F0D2A1
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>
#include <capn-rpc.h>

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

struct Adder_tag_results;
struct Adder_tag_params;
struct Adder_add_results;
struct Adder_add_params;

typedef struct {capn_ptr p;} Adder_tag_results_ptr;
typedef struct {capn_ptr p;} Adder_tag_params_ptr;
typedef struct {capn_ptr p;} Adder_add_results_ptr;
typedef struct {capn_ptr p;} Adder_add_params_ptr;

typedef struct {capn_ptr p;} Adder_tag_results_list;
typedef struct {capn_ptr p;} Adder_tag_params_list;
typedef struct {capn_ptr p;} Adder_add_results_list;
typedef struct {capn_ptr p;} Adder_add_params_list;

typedef struct {capn_ptr p;} Adder_ptr;
typedef struct {capn_ptr p;} Adder_list;

struct Adder_tag_results {
	int32_t n;
	capn_text label;
};

static const size_t Adder_tag_results_word_count = 1;

static const size_t Adder_tag_results_pointer_count = 1;

static const size_t Adder_tag_results_struct_bytes_count = 16;


struct Adder_tag_params {
	int32_t n;
	capn_text label;
};

static const size_t Adder_tag_params_word_count = 1;

static const size_t Adder_tag_params_pointer_count = 1;

static const size_t Adder_tag_params_struct_bytes_count = 16;


struct Adder_add_results {
	int64_t sum;
};

static const size_t Adder_add_results_word_count = 1;

static const size_t Adder_add_results_pointer_count = 0;

static const size_t Adder_add_results_struct_bytes_count = 8;


struct Adder_add_params {
	int64_t a;
	int64_t b;
};

static const size_t Adder_add_params_word_count = 2;

static const size_t Adder_add_params_pointer_count = 0;

static const size_t Adder_add_params_struct_bytes_count = 16;


/* Interface Adder: id and method descriptors. */
#define Adder_INTERFACE_ID 0xea01e10cbc414411ULL
struct Adder_method {
	uint16_t ordinal;
	int params_datasz;   /* bytes */
	int params_ptrs;
	int results_datasz;
	int results_ptrs;
};
static const struct Adder_method Adder_add_method = {0, 16, 0, 8, 0};
static const struct Adder_method Adder_tag_method = {1, 8, 1, 8, 1};

/* RPC stubs for Adder. */
struct Adder_server {
	void *self;
	int (*on_add)(void *self, const struct Adder_add_params *in, struct Adder_add_results *out, struct capn_segment *seg);
	int (*on_tag)(void *self, const struct Adder_tag_params *in, struct Adder_tag_results *out, struct capn_segment *seg);
};
/* Pass these to capn_rpc_set_result_size so results fit. */
#define Adder_RESULT_DATASZ 8
#define Adder_RESULT_PTRS 1
int Adder_dispatch(void *server, uint64_t interface_id, uint16_t method_id, capn_ptr params, capn_ptr results);
uint32_t Adder_call_add(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_add_params *in);
int Adder_results_add(struct capn_rpc_conn *c, uint32_t question, struct Adder_add_results *out, struct capn *msg);
uint32_t Adder_call_tag(struct capn_rpc_conn *c, uint32_t cap, const struct Adder_tag_params *in);
int Adder_results_tag(struct capn_rpc_conn *c, uint32_t question, struct Adder_tag_results *out, struct capn *msg);

Adder_tag_results_ptr new_Adder_tag_results(struct capn_segment*);
Adder_tag_params_ptr new_Adder_tag_params(struct capn_segment*);
Adder_add_results_ptr new_Adder_add_results(struct capn_segment*);
Adder_add_params_ptr new_Adder_add_params(struct capn_segment*);

Adder_tag_results_list new_Adder_tag_results_list(struct capn_segment*, int len);
Adder_tag_params_list new_Adder_tag_params_list(struct capn_segment*, int len);
Adder_add_results_list new_Adder_add_results_list(struct capn_segment*, int len);
Adder_add_params_list new_Adder_add_params_list(struct capn_segment*, int len);

Adder_ptr new_Adder(struct capn_segment*);
Adder_list new_Adder_list(struct capn_segment*, int len);

void read_Adder_tag_results(struct Adder_tag_results*, Adder_tag_results_ptr);
void read_Adder_tag_params(struct Adder_tag_params*, Adder_tag_params_ptr);
void read_Adder_add_results(struct Adder_add_results*, Adder_add_results_ptr);
void read_Adder_add_params(struct Adder_add_params*, Adder_add_params_ptr);

void write_Adder_tag_results(const struct Adder_tag_results*, Adder_tag_results_ptr);
void write_Adder_tag_params(const struct Adder_tag_params*, Adder_tag_params_ptr);
void write_Adder_add_results(const struct Adder_add_results*, Adder_add_results_ptr);
void write_Adder_add_params(const struct Adder_add_params*, Adder_add_params_ptr);

void get_Adder_tag_results(struct Adder_tag_results*, Adder_tag_results_list, int i);
void get_Adder_tag_params(struct Adder_tag_params*, Adder_tag_params_list, int i);
void get_Adder_add_results(struct Adder_add_results*, Adder_add_results_list, int i);
void get_Adder_add_params(struct Adder_add_params*, Adder_add_params_list, int i);

void set_Adder_tag_results(const struct Adder_tag_results*, Adder_tag_results_list, int i);
void set_Adder_tag_params(const struct Adder_tag_params*, Adder_tag_params_list, int i);
void set_Adder_add_results(const struct Adder_add_results*, Adder_add_results_list, int i);
void set_Adder_add_params(const struct Adder_add_params*, Adder_add_params_list, int i);

#ifdef __cplusplus
}
#endif
#endif
