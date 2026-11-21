/* The frame header is little endian. A big-endian load of these
 * bytes is a different integer, and the reader must not use it. */

#include <stdio.h>
#include <string.h>

#include "frame_le.h"

#ifndef RPC_CLIENT_SOURCE
#error RPC_CLIENT_SOURCE
#endif
#ifndef TAIL_CLIENT_SOURCE
#error TAIL_CLIENT_SOURCE
#endif

/* The clients have to decode the header with the helper. A host load
 * of the same bytes still returns 1 on a little-endian machine, so the
 * numeric check above cannot see that swap. */
static int source_calls(const char *path)
{
	char line[1024];
	FILE *f = fopen(path, "r");
	int hit = 0;

	if (!f) {
		perror(path);
		return 0;
	}
	while (fgets(line, sizeof line, f)) {
		if (strstr(line, "frame_le32(") != NULL)
			hit = 1;
	}
	fclose(f);
	return hit;
}

static int fail(const char *what)
{
	fprintf(stderr, "frame-le: %s\n", what);
	return 1;
}

int main(void)
{
	const uint8_t one[4] = {0x01, 0x00, 0x00, 0x00};
	const uint8_t twenty[4] = {0x14, 0x00, 0x00, 0x00};
	uint32_t be;

	if (frame_le32(one) != 1)
		return fail("size 1");
	if (frame_le32(twenty) != 20)
		return fail("size 20");

	/* The bytes a big-endian host would take from a plain load. */
	be = ((uint32_t) one[0] << 24) | ((uint32_t) one[1] << 16)
		| ((uint32_t) one[2] << 8) | (uint32_t) one[3];
	if (be == frame_le32(one))
		return fail("big-endian load matched");
	if (be != 0x01000000u)
		return fail("big-endian pattern");
	if (!source_calls(RPC_CLIENT_SOURCE))
		return fail("rpc client");
	if (!source_calls(TAIL_CLIENT_SOURCE))
		return fail("tail client");
	return 0;
}
