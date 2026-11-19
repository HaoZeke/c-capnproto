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
struct CountBase_inc_results;
struct CountBase_inc_params;
struct CountMid_twice_results;
struct CountMid_twice_params;
struct CountLeaf_dec_results;
struct CountLeaf_dec_params;

typedef struct {capn_ptr p;} Adder_tag_results_ptr;
typedef struct {capn_ptr p;} Adder_tag_params_ptr;
typedef struct {capn_ptr p;} Adder_add_results_ptr;
typedef struct {capn_ptr p;} Adder_add_params_ptr;
typedef struct {capn_ptr p;} CountBase_inc_results_ptr;
typedef struct {capn_ptr p;} CountBase_inc_params_ptr;
typedef struct {capn_ptr p;} CountMid_twice_results_ptr;
typedef struct {capn_ptr p;} CountMid_twice_params_ptr;
typedef struct {capn_ptr p;} CountLeaf_dec_results_ptr;
typedef struct {capn_ptr p;} CountLeaf_dec_params_ptr;

typedef struct {capn_ptr p;} Adder_tag_results_list;
typedef struct {capn_ptr p;} Adder_tag_params_list;
typedef struct {capn_ptr p;} Adder_add_results_list;
typedef struct {capn_ptr p;} Adder_add_params_list;
typedef struct {capn_ptr p;} CountBase_inc_results_list;
typedef struct {capn_ptr p;} CountBase_inc_params_list;
typedef struct {capn_ptr p;} CountMid_twice_results_list;
typedef struct {capn_ptr p;} CountMid_twice_params_list;
typedef struct {capn_ptr p;} CountLeaf_dec_results_list;
typedef struct {capn_ptr p;} CountLeaf_dec_params_list;

typedef struct {capn_ptr p;} Adder_ptr;
typedef struct {capn_ptr p;} Adder_list;
typedef struct {capn_ptr p;} CountBase_ptr;
typedef struct {capn_ptr p;} CountBase_list;
typedef struct {capn_ptr p;} CountMid_ptr;
typedef struct {capn_ptr p;} CountMid_list;
typedef struct {capn_ptr p;} CountLeaf_ptr;
typedef struct {capn_ptr p;} CountLeaf_list;

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


struct CountBase_inc_results {
	int32_t n;
};

static const size_t CountBase_inc_results_word_count = 1;

static const size_t CountBase_inc_results_pointer_count = 0;

static const size_t CountBase_inc_results_struct_bytes_count = 8;


struct CountBase_inc_params {
	int32_t n;
};

static const size_t CountBase_inc_params_word_count = 1;

static const size_t CountBase_inc_params_pointer_count = 0;

static const size_t CountBase_inc_params_struct_bytes_count = 8;


struct CountMid_twice_results {
	int32_t n;
};

static const size_t CountMid_twice_results_word_count = 1;

static const size_t CountMid_twice_results_pointer_count = 0;

static const size_t CountMid_twice_results_struct_bytes_count = 8;


struct CountMid_twice_params {
	int32_t n;
};

static const size_t CountMid_twice_params_word_count = 1;

static const size_t CountMid_twice_params_pointer_count = 0;

static const size_t CountMid_twice_params_struct_bytes_count = 8;


struct CountLeaf_dec_results {
	int32_t n;
};

static const size_t CountLeaf_dec_results_word_count = 1;

static const size_t CountLeaf_dec_results_pointer_count = 0;

static const size_t CountLeaf_dec_results_struct_bytes_count = 8;


struct CountLeaf_dec_params {
	int32_t n;
};

static const size_t CountLeaf_dec_params_word_count = 1;

static const size_t CountLeaf_dec_params_pointer_count = 0;

static const size_t CountLeaf_dec_params_struct_bytes_count = 8;


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
static const struct Adder_method Adder_defer_method = {2, 0, 0, 0, 1};
static const struct Adder_method Adder_fulfill_method = {3, 0, 0, 0, 0};

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

/* Interface CountBase: id and method descriptors. */
#define CountBase_INTERFACE_ID 0xdbbc11dde380ab36ULL
struct CountBase_method {
	uint16_t ordinal;
	int params_datasz;   /* bytes */
	int params_ptrs;
	int results_datasz;
	int results_ptrs;
};
static const struct CountBase_method CountBase_inc_method = {0, 8, 0, 8, 0};

/* RPC stubs for CountBase. */
struct CountBase_server {
	void *self;
	int (*on_inc)(void *self, const struct CountBase_inc_params *in, struct CountBase_inc_results *out, struct capn_segment *seg);
};
/* Pass these to capn_rpc_set_result_size so results fit. */
#define CountBase_RESULT_DATASZ 8
#define CountBase_RESULT_PTRS 0
int CountBase_dispatch(void *server, uint64_t interface_id, uint16_t method_id, capn_ptr params, capn_ptr results);
uint32_t CountBase_call_inc(struct capn_rpc_conn *c, uint32_t cap, const struct CountBase_inc_params *in);
int CountBase_results_inc(struct capn_rpc_conn *c, uint32_t question, struct CountBase_inc_results *out, struct capn *msg);

/* Interface CountMid: id and method descriptors. */
#define CountMid_INTERFACE_ID 0xf6154401a1ef6c03ULL
struct CountMid_method {
	uint16_t ordinal;
	int params_datasz;   /* bytes */
	int params_ptrs;
	int results_datasz;
	int results_ptrs;
};
static const struct CountMid_method CountMid_twice_method = {0, 8, 0, 8, 0};

/* RPC stubs for CountMid. */
struct CountMid_server {
	void *self;
	const struct CountBase_server *super_CountBase;
	int (*on_twice)(void *self, const struct CountMid_twice_params *in, struct CountMid_twice_results *out, struct capn_segment *seg);
};
/* Pass these to capn_rpc_set_result_size so results fit. */
#define CountMid_RESULT_DATASZ 8
#define CountMid_RESULT_PTRS 0
int CountMid_dispatch(void *server, uint64_t interface_id, uint16_t method_id, capn_ptr params, capn_ptr results);
uint32_t CountMid_call_twice(struct capn_rpc_conn *c, uint32_t cap, const struct CountMid_twice_params *in);
int CountMid_results_twice(struct capn_rpc_conn *c, uint32_t question, struct CountMid_twice_results *out, struct capn *msg);

/* Interface CountLeaf: id and method descriptors. */
#define CountLeaf_INTERFACE_ID 0xc308e96fb95171f9ULL
struct CountLeaf_method {
	uint16_t ordinal;
	int params_datasz;   /* bytes */
	int params_ptrs;
	int results_datasz;
	int results_ptrs;
};
static const struct CountLeaf_method CountLeaf_dec_method = {0, 8, 0, 8, 0};

/* RPC stubs for CountLeaf. */
struct CountLeaf_server {
	void *self;
	const struct CountMid_server *super_CountMid;
	const struct Adder_server *super_Adder;
	int (*on_dec)(void *self, const struct CountLeaf_dec_params *in, struct CountLeaf_dec_results *out, struct capn_segment *seg);
};
/* Pass these to capn_rpc_set_result_size so results fit. */
#define CountLeaf_RESULT_DATASZ 8
#define CountLeaf_RESULT_PTRS 1
int CountLeaf_dispatch(void *server, uint64_t interface_id, uint16_t method_id, capn_ptr params, capn_ptr results);
uint32_t CountLeaf_call_dec(struct capn_rpc_conn *c, uint32_t cap, const struct CountLeaf_dec_params *in);
int CountLeaf_results_dec(struct capn_rpc_conn *c, uint32_t question, struct CountLeaf_dec_results *out, struct capn *msg);

Adder_tag_results_ptr new_Adder_tag_results(struct capn_segment*);
Adder_tag_params_ptr new_Adder_tag_params(struct capn_segment*);
Adder_add_results_ptr new_Adder_add_results(struct capn_segment*);
Adder_add_params_ptr new_Adder_add_params(struct capn_segment*);
CountBase_inc_results_ptr new_CountBase_inc_results(struct capn_segment*);
CountBase_inc_params_ptr new_CountBase_inc_params(struct capn_segment*);
CountMid_twice_results_ptr new_CountMid_twice_results(struct capn_segment*);
CountMid_twice_params_ptr new_CountMid_twice_params(struct capn_segment*);
CountLeaf_dec_results_ptr new_CountLeaf_dec_results(struct capn_segment*);
CountLeaf_dec_params_ptr new_CountLeaf_dec_params(struct capn_segment*);

Adder_tag_results_list new_Adder_tag_results_list(struct capn_segment*, int len);
Adder_tag_params_list new_Adder_tag_params_list(struct capn_segment*, int len);
Adder_add_results_list new_Adder_add_results_list(struct capn_segment*, int len);
Adder_add_params_list new_Adder_add_params_list(struct capn_segment*, int len);
CountBase_inc_results_list new_CountBase_inc_results_list(struct capn_segment*, int len);
CountBase_inc_params_list new_CountBase_inc_params_list(struct capn_segment*, int len);
CountMid_twice_results_list new_CountMid_twice_results_list(struct capn_segment*, int len);
CountMid_twice_params_list new_CountMid_twice_params_list(struct capn_segment*, int len);
CountLeaf_dec_results_list new_CountLeaf_dec_results_list(struct capn_segment*, int len);
CountLeaf_dec_params_list new_CountLeaf_dec_params_list(struct capn_segment*, int len);

Adder_ptr new_Adder(struct capn_segment*);
Adder_list new_Adder_list(struct capn_segment*, int len);
CountBase_ptr new_CountBase(struct capn_segment*);
CountBase_list new_CountBase_list(struct capn_segment*, int len);
CountMid_ptr new_CountMid(struct capn_segment*);
CountMid_list new_CountMid_list(struct capn_segment*, int len);
CountLeaf_ptr new_CountLeaf(struct capn_segment*);
CountLeaf_list new_CountLeaf_list(struct capn_segment*, int len);

void read_Adder_tag_results(struct Adder_tag_results*, Adder_tag_results_ptr);
void read_Adder_tag_params(struct Adder_tag_params*, Adder_tag_params_ptr);
void read_Adder_add_results(struct Adder_add_results*, Adder_add_results_ptr);
void read_Adder_add_params(struct Adder_add_params*, Adder_add_params_ptr);
void read_CountBase_inc_results(struct CountBase_inc_results*, CountBase_inc_results_ptr);
void read_CountBase_inc_params(struct CountBase_inc_params*, CountBase_inc_params_ptr);
void read_CountMid_twice_results(struct CountMid_twice_results*, CountMid_twice_results_ptr);
void read_CountMid_twice_params(struct CountMid_twice_params*, CountMid_twice_params_ptr);
void read_CountLeaf_dec_results(struct CountLeaf_dec_results*, CountLeaf_dec_results_ptr);
void read_CountLeaf_dec_params(struct CountLeaf_dec_params*, CountLeaf_dec_params_ptr);

void write_Adder_tag_results(const struct Adder_tag_results*, Adder_tag_results_ptr);
void write_Adder_tag_params(const struct Adder_tag_params*, Adder_tag_params_ptr);
void write_Adder_add_results(const struct Adder_add_results*, Adder_add_results_ptr);
void write_Adder_add_params(const struct Adder_add_params*, Adder_add_params_ptr);
void write_CountBase_inc_results(const struct CountBase_inc_results*, CountBase_inc_results_ptr);
void write_CountBase_inc_params(const struct CountBase_inc_params*, CountBase_inc_params_ptr);
void write_CountMid_twice_results(const struct CountMid_twice_results*, CountMid_twice_results_ptr);
void write_CountMid_twice_params(const struct CountMid_twice_params*, CountMid_twice_params_ptr);
void write_CountLeaf_dec_results(const struct CountLeaf_dec_results*, CountLeaf_dec_results_ptr);
void write_CountLeaf_dec_params(const struct CountLeaf_dec_params*, CountLeaf_dec_params_ptr);

void get_Adder_tag_results(struct Adder_tag_results*, Adder_tag_results_list, int i);
void get_Adder_tag_params(struct Adder_tag_params*, Adder_tag_params_list, int i);
void get_Adder_add_results(struct Adder_add_results*, Adder_add_results_list, int i);
void get_Adder_add_params(struct Adder_add_params*, Adder_add_params_list, int i);
void get_CountBase_inc_results(struct CountBase_inc_results*, CountBase_inc_results_list, int i);
void get_CountBase_inc_params(struct CountBase_inc_params*, CountBase_inc_params_list, int i);
void get_CountMid_twice_results(struct CountMid_twice_results*, CountMid_twice_results_list, int i);
void get_CountMid_twice_params(struct CountMid_twice_params*, CountMid_twice_params_list, int i);
void get_CountLeaf_dec_results(struct CountLeaf_dec_results*, CountLeaf_dec_results_list, int i);
void get_CountLeaf_dec_params(struct CountLeaf_dec_params*, CountLeaf_dec_params_list, int i);

void set_Adder_tag_results(const struct Adder_tag_results*, Adder_tag_results_list, int i);
void set_Adder_tag_params(const struct Adder_tag_params*, Adder_tag_params_list, int i);
void set_Adder_add_results(const struct Adder_add_results*, Adder_add_results_list, int i);
void set_Adder_add_params(const struct Adder_add_params*, Adder_add_params_list, int i);
void set_CountBase_inc_results(const struct CountBase_inc_results*, CountBase_inc_results_list, int i);
void set_CountBase_inc_params(const struct CountBase_inc_params*, CountBase_inc_params_list, int i);
void set_CountMid_twice_results(const struct CountMid_twice_results*, CountMid_twice_results_list, int i);
void set_CountMid_twice_params(const struct CountMid_twice_params*, CountMid_twice_params_list, int i);
void set_CountLeaf_dec_results(const struct CountLeaf_dec_results*, CountLeaf_dec_results_list, int i);
void set_CountLeaf_dec_params(const struct CountLeaf_dec_params*, CountLeaf_dec_params_list, int i);

#ifdef __cplusplus
}
#endif
#endif
