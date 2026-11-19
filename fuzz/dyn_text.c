/* Decode a message and print it through the dynamic text and JSON
 * walkers. A capability pointer used to be reported as a list of
 * length equal to its table index, so a 16-byte input never returned. */

#include "capn-dyn.h"
#include "capnp_c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
	struct stat st;
	struct capn message, dst;
	struct capn_dyn_type u8t;
	struct capn_dyn_field fields[2];
	struct capn_dyn_struct box;
	FILE *fp;
	uint8_t *data;
	size_t len;
	capn_ptr root, cap, boxp;
	char *text, *json;

	if (argc != 2 || stat(argv[1], &st) != 0 || st.st_size < 0)
		return 0;
	len = (size_t) st.st_size;
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
	if (capn_init_mem(&message, data, len, 0) != 0) {
		free(data);
		return 0;
	}
	cap = capn_getp(capn_root(&message), 0, 1);

	memset(&u8t, 0, sizeof u8t);
	memset(fields, 0, sizeof fields);
	memset(&box, 0, sizeof box);
	u8t.kind = CAPN_DYN_UINT8;
	fields[0].name = "items";
	fields[0].type.kind = CAPN_DYN_LIST;
	fields[0].type.elem = &u8t;
	fields[0].discriminant = CAPN_DYN_NO_DISCRIMINANT;
	fields[1].name = "blob";
	fields[1].type.kind = CAPN_DYN_DATA;
	fields[1].offset = 1;
	fields[1].discriminant = CAPN_DYN_NO_DISCRIMINANT;
	box.name = "Box";
	box.pointers = 2;
	box.nfields = 2;
	box.fields = fields;

	capn_init_malloc(&dst);
	root = capn_root(&dst);
	if (root.seg != NULL) {
		boxp = capn_new_struct(root.seg, 0, 2);
		if (boxp.type == CAPN_STRUCT) {
			capn_setp(boxp, 0, cap);
			capn_setp(boxp, 1, cap);
			text = capn_dyn_to_text(&box, boxp);
			json = capn_dyn_to_json(&box, boxp, 0);
			free(text);
			free(json);
		}
	}
	capn_free(&dst);
	capn_free(&message);
	free(data);
	return 0;
}
