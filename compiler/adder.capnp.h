#ifndef CAPN_BF5E831AC9F0D2A1
#define CAPN_BF5E831AC9F0D2A1
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>

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




typedef struct {capn_ptr p;} Adder_ptr;
typedef struct {capn_ptr p;} Adder_list;

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



Adder_ptr new_Adder(struct capn_segment*);
Adder_list new_Adder_list(struct capn_segment*, int len);





#ifdef __cplusplus
}
#endif
#endif
