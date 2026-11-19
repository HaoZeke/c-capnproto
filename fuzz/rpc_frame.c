#include "capn-rpc.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static int discard_frame(void *ctx, const uint8_t *data, size_t len)
{
	(void)ctx;
	(void)data;
	(void)len;
	return 0;
}

int main(int argc, char **argv)
{
	struct stat st;
	struct capn_rpc_conn conn;
	FILE *fp;
	uint8_t *data;
	size_t len;

	if (argc != 2 || stat(argv[1], &st) != 0 || st.st_size < 0)
		return 1;
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
	capn_rpc_init(&conn, discard_frame, NULL);
	(void)capn_rpc_handle(&conn, data, len);
	capn_rpc_free(&conn);
	free(data);
	return 0;
}
