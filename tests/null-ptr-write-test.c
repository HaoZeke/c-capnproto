/* Encode a null pointer. The writer must not apply an offset to a
 * null data address. */
#include "capnp_c.h"

#include <stdio.h>
#include <string.h>

int main(void) {
	struct capn c;
	capn_ptr obj;
	capn_ptr nullp;
	capn_ptr got;
	uint8_t buf[256];
	int64_t sz;
	int64_t wr;

	capn_init_malloc(&c);
	obj = capn_new_struct(capn_root(&c).seg, 0, 1);
	if (obj.type != CAPN_STRUCT || obj.ptrs < 1) {
		fprintf(stderr, "new_struct failed\n");
		return 1;
	}
	memset(&nullp, 0, sizeof(nullp));
	if (capn_setp(obj, 0, nullp) != 0) {
		fprintf(stderr, "capn_setp failed\n");
		return 1;
	}
	if (capn_set_root(&c, obj) != 0) {
		fprintf(stderr, "capn_set_root failed\n");
		return 1;
	}
	got = capn_getp(obj, 0, 1);
	if (got.type != CAPN_NULL) {
		fprintf(stderr, "pointer was not null\n");
		return 1;
	}
	sz = capn_size(&c);
	if (sz <= 0 || (size_t)sz > sizeof(buf)) {
		fprintf(stderr, "unexpected size\n");
		return 1;
	}
	wr = capn_write_mem(&c, buf, (size_t)sz, 0);
	if (wr != sz) {
		fprintf(stderr, "write failed\n");
		return 1;
	}
	capn_free(&c);
	printf("null pointer write ok\n");
	return 0;
}
