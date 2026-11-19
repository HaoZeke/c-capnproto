#include "capnp_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
	struct stat st;
	struct capn message;
	FILE *fp;
	uint8_t *data;
	size_t len;
	capn_ptr root;

	if (argc != 2 || stat(argv[1], &st) != 0 || st.st_size < 0)
		return 0;
	len = (size_t)st.st_size;
	data = malloc(len ? len : 1);
	if (data == NULL)
		return 0;
	fp = fopen(argv[1], "rb");
	if (fp == NULL || fread(data, 1, len, fp) != len) {
		if (fp != NULL)
			fclose(fp);
		free(data);
		return 0;
	}
	fclose(fp);
	if (capn_init_mem(&message, data, len, 0) == 0) {
		root = capn_getp(capn_root(&message), 0, 1);
		capn_resolve(&root);
		capn_free(&message);
	}
	free(data);
	return 0;
}
