/* Consumes the installed/added c-capnproto runtime through its public
 * header only: frame a one-segment message, read it back. */
#include <stdio.h>
#include <string.h>

#include "capnp_c.h"

int main(void) {
	struct capn c;
	capn_ptr root, st;
	uint8_t buf[512];
	ssize_t sz;
	struct capn c2;
	capn_ptr root2, st2;

	capn_init_malloc(&c);
	root = capn_root(&c);
	st = capn_new_struct(root.seg, 8, 0);
	capn_write32(st, 0, 42u);
	capn_setp(root, 0, st);
	sz = capn_write_mem(&c, buf, sizeof buf, 0);
	capn_free(&c);
	if (sz <= 0) {
		fprintf(stderr, "write_mem failed\n");
		return 1;
	}

	if (capn_init_mem(&c2, buf, (size_t)sz, 0) != 0) {
		fprintf(stderr, "init_mem failed\n");
		return 1;
	}
	root2 = capn_root(&c2);
	st2 = capn_getp(root2, 0, 1);
	if (capn_read32(st2, 0) != 42u) {
		fprintf(stderr, "u32 mismatch\n");
		capn_free(&c2);
		return 1;
	}
	capn_free(&c2);
	printf("c-capnproto consumer smoke ok, %d framed bytes\n", (int)sz);
	return 0;
}
