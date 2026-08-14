#include "adder.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif


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
