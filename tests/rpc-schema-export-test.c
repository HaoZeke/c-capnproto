/* SPDX-License-Identifier: MIT */
#include "capnp_c.h"
#include "capn-rpc.h"
#include "rpc-threeparty.capnp.h"

int main(void)
{
	struct capn c;
	struct capn_segment *s;
	CapDescriptor_ptr cd;
	VatId_ptr vid;

	capn_init_malloc(&c);
	s = capn_root(&c).seg;
	cd = new_CapDescriptor(s);
	vid = new_VatId(s);
	if (!cd.p.type || !vid.p.type) {
		capn_free(&c);
		return 1;
	}
	capn_free(&c);
	return 0;
}
