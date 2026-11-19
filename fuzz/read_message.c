#include "capnp_c.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

/* Bound the walk so a pointer cycle or a huge list cannot hang the
 * process. The runtime's own nesting limit is the other backstop. */
enum { kMaxVisits = 4096, kMaxDepth = 64, kMaxFanout = 32 };

static int g_visits;

static void walk(capn_ptr p, int depth)
{
	int i, n;
	volatile uint8_t touch;

	if (g_visits >= kMaxVisits || depth > kMaxDepth)
		return;
	g_visits++;
	capn_resolve(&p);
	if (p.type == CAPN_NULL || p.data == NULL)
		return;

	if (p.type == CAPN_STRUCT) {
		if (p.datasz > 0) {
			touch = capn_read8(p, 0);
			if (p.datasz > 1)
				touch = capn_read8(p, p.datasz - 1);
			(void)touch;
		}
		n = p.ptrs;
		if (n > kMaxFanout)
			n = kMaxFanout;
		for (i = 0; i < n; i++)
			walk(capn_getp(p, i, 1), depth + 1);
		return;
	}

	if (p.type == CAPN_PTR_LIST) {
		n = p.len;
		if (n > kMaxFanout)
			n = kMaxFanout;
		for (i = 0; i < n; i++)
			walk(capn_getp(p, i, 1), depth + 1);
		return;
	}

	if (p.type == CAPN_LIST) {
		n = p.len;
		if (n > kMaxFanout)
			n = kMaxFanout;
		if (p.ptrs > 0 || p.is_composite_list) {
			for (i = 0; i < n; i++)
				walk(capn_getp(p, i, 0), depth + 1);
			return;
		}
		if (p.datasz > 0 && n > 0) {
			/* Primitive list: read the first and last byte the
			 * list claims, through the bounds-checked getters
			 * when the element is at least a byte. */
			if (p.datasz == 1) {
				capn_list8 l;
				l.p = p;
				touch = capn_get8(l, 0);
				touch = capn_get8(l, n - 1);
				(void)touch;
			} else if (p.data != NULL && p.datasz > 0) {
				size_t bytes = (size_t)p.datasz * (size_t)n;
				touch = (uint8_t)p.data[0];
				touch = (uint8_t)p.data[bytes - 1];
				(void)touch;
			}
		}
	}
}

int main(int argc, char **argv)
{
	struct stat st;
	struct capn message;
	FILE *fp;
	uint8_t *data;
	size_t len;
	capn_ptr root;

	if (argc != 2 || stat(argv[1], &st) != 0 || st.st_size < 0)
		return 1;
	if ((unsigned long long)st.st_size > 64ull * 1024ull * 1024ull)
		return 0;
	len = (size_t)st.st_size;
	data = malloc(len ? len : 1);
	if (data == NULL)
		return 1;
	fp = fopen(argv[1], "rb");
	if (fp == NULL || fread(data, 1, len, fp) != len) {
		if (fp != NULL)
			fclose(fp);
		free(data);
		return 1;
	}
	fclose(fp);
	if (capn_init_mem(&message, data, len, 0) == 0) {
		root = capn_getp(capn_root(&message), 0, 1);
		g_visits = 0;
		walk(root, 0);
		(void) capn_validate(&message);
		capn_free(&message);
	}
	free(data);
	return 0;
}
