/*
 * Copyright (C) 2017 Alex Helfet
 * Copyright (C) 2018 Angelo Haller
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <capnp_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "addressbook.capnp.h"

static int str_is(const char *want, const char *got)
{
	if (want == NULL || got == NULL)
		return want == got;
	return strcmp(want, got) == 0;
}

int
main(int argc, char **argv)
{
	const char *name = "Firstname Lastname";
	const char *email = "username@domain.com";
	const char *school = "of life";

	int rv = 1;
	struct capn rc;
	if (argc != 2)
		goto err;

	struct stat stat;
	if (lstat(argv[1], &stat) != 0)
		goto err;

	uint8_t *buf = malloc(stat.st_size);
	if (buf == NULL)
		goto err;

	FILE *fp = fopen(argv[1], "rb");
	if (fp == NULL)
		goto err_free;

	if (fread(buf, 1, stat.st_size, fp) != stat.st_size)
		goto err_close;

	int init_mem_ret = capn_init_mem(&rc, buf, stat.st_size, 0 /* packed */);
	if (init_mem_ret != 0)
		goto err_close;

	Person_ptr rroot;
	struct Person rp;
	/* read_Person leaves the inactive union arms unwritten. */
	memset(&rp, 0, sizeof rp);
	rroot.p = capn_getp(capn_root(&rc), 0 /* off */, 1 /* resolve */);
	read_Person(&rp, rroot);

	// Assert deserialized values in `rp`
	if (rp.id != 17)
		++rv;
	if (!str_is(name, rp.name.str))
		++rv;
	if (!str_is(email, rp.email.str))
		++rv;

	if (rp.employment_which != Person_employment_school)
		++rv;
	else if (!str_is(school, rp.employment.school.str))
		++rv;

	if (capn_len(rp.phones) != 2)
		++rv;

	size_t i;
	for (i = 0; i < capn_len(rp.phones); ++i) {
		struct Person_PhoneNumber rpn;
		get_Person_PhoneNumber(&rpn, rp.phones, i);
		if (i == 0) {
			if (!str_is("123", rpn.number.str))
				++rv;
			if (rpn.type != Person_PhoneNumber_Type_work)
				++rv;
		}
		if (i == 1) {
			if (!str_is("234", rpn.number.str))
				++rv;
			if (rpn.type != Person_PhoneNumber_Type_home)
				++rv;
		}
	}

	--rv;

	capn_free(&rc);

err_close:
	fclose(fp);
err_free:
	free(buf);
err:
	return rv;
}
