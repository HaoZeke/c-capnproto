/* SPDX-License-Identifier: MIT */
#include "str.h"

#include <string.h>

int main(void)
{
	struct str value = STR_INIT;

	str_add(&value, NULL, 0);
	if (value.len != 0 || strcmp(value.str, "") != 0)
		return 1;

	str_add(&value, "capnp", 5);
	if (value.len != 5 || strcmp(value.str, "capnp") != 0)
		return 1;

	str_release(&value);
	return 0;
}
