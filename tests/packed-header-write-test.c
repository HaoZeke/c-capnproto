/* Packed framing stores the segment header at a 2-byte offset.
 * Those words must round-trip without an aligned uint32_t store. */
#include "capnp_c.h"

#include <stdint.h>
#include <stdio.h>

int main(void) {
	struct capn c;
	struct capn out;
	capn_ptr obj;
	capn_ptr got;
	uint8_t buf[4096];
	int64_t wr;
	uint64_t val;

	capn_init_malloc(&c);
	obj = capn_new_struct(capn_root(&c).seg, 8, 0);
	if (obj.type != CAPN_STRUCT) {
		fprintf(stderr, "new_struct failed\n");
		return 1;
	}
	if (capn_write64(obj, 0, 0x402) != 0) {
		fprintf(stderr, "write64 failed\n");
		return 1;
	}
	if (capn_set_root(&c, obj) != 0) {
		fprintf(stderr, "capn_set_root failed\n");
		return 1;
	}
	wr = capn_write_mem(&c, buf, sizeof(buf), 1);
	if (wr <= 0) {
		fprintf(stderr, "packed write failed\n");
		return 1;
	}
	if (capn_init_mem(&out, buf, (size_t)wr, 1) != 0) {
		fprintf(stderr, "packed read failed\n");
		return 1;
	}
	got = capn_getp(capn_root(&out), 0, 1);
	val = capn_read64(got, 0);
	if (val != 0x402) {
		fprintf(stderr, "round trip mismatch\n");
		return 1;
	}
	capn_free(&c);
	capn_free(&out);
	printf("packed header write ok\n");
	return 0;
}
