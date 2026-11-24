/* Each generated `capn_ptr` constant names every field, which the
 * build checks with -Werror=missing-field-initializers. */
#include <string.h>

#include "ptr-const.capnp.h"

int main(void)
{
	struct Box_Text b;
	struct Plain s;

	read_Box_Text(&b, boxed);
	if (b.val.len != 2 || memcmp(b.val.str, "hi", 2) != 0)
		return 1;
	read_Plain(&s, plain);
	if (s.n != 3 || s.t.len != 1 || s.t.str[0] != 'x')
		return 2;
	if (capn_len(nums) != 2 || capn_get32(nums, 1) != 2)
		return 3;
	get_Plain(&s, plains, 1);
	if (capn_len(plains) != 2 || s.n != 5)
		return 4;
	return 0;
}
