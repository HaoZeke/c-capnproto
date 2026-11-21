#include <string.h>

#include "uses-nested.capnp.h"

int main(void)
{
	struct capn c;
	struct Uses u;
	Uses_ptr p;

	capn_init_malloc(&c);
	p = new_Uses(capn_root(&c).seg);
	memset(&u, 0, sizeof u);
	write_Uses(&u, p);
	capn_free(&c);
	return p.p.type == CAPN_STRUCT ? 0 : 1;
}
