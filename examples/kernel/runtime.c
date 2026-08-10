/* Pull the userspace runtime into this kbuild directory.
 * kbuild does not like object paths outside M=.
 */
#include "../../lib/capn.c"
#include "../../lib/capn-malloc.c"
#include "../../lib/capn-stream.c"
#include "../../tests/addressbook.capnp.c"
