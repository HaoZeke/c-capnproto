/* Serve Mirror for a C++ client that pipelines on a reflected cap
 * and checks that a dropped parameter cap is released.
 *
 * mirror.capnp: Mirror @0xb14a11ee00000003
 *   reflect @0 (cap :Seq) -> (cap :Seq)
 *   drop    @1 (cap :Seq) -> (cap :Seq)
 * Both structs are one pointer and no data. */

#include "posix_net.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "capn-rpc.h"

#define MIRROR_ID 0xb14a11ee00000003ULL

static struct capn_rpc_conn *g_conn;

static int mirror_dispatch(void *server, uint64_t interface_id,
                           uint16_t method_id, capn_ptr params,
                           capn_ptr results)
{
	int is_export = 1;
	int64_t id;
	capn_ptr cap;

	(void) server;
	if (interface_id != MIRROR_ID)
		return -1;
	if (method_id == 1)
		return 0;
	if (method_id != 0)
		return -1;
	cap = capn_getp(params, 0, 1);
	id = capn_rpc_param_cap(g_conn, cap, &is_export);
	if (id < 0 || is_export)
		return -1;
	cap = capn_rpc_new_import_cap(g_conn, results.seg, (uint32_t) id);
	if (cap.type == CAPN_NULL)
		return -1;
	return capn_setp(results, 0, cap);
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	struct capn_rpc_conn conn;
	int port = argc > 1 ? atoi(argv[1]) : 43120;
	int lfd, fd, one = 1;

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd < 0)
		return 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t) port);
	addr.sin_addr.s_addr = posix_loopback_addr();
	if (bind(lfd, (struct sockaddr *) &addr, sizeof addr) != 0 ||
	    listen(lfd, 1) != 0) {
		close(lfd);
		return 1;
	}
	fd = accept(lfd, NULL, NULL);
	if (fd < 0) {
		close(lfd);
		return 1;
	}

	capn_rpc_init(&conn, capn_rpc_fd_send, &fd);
	g_conn = &conn;
	capn_rpc_set_bootstrap(&conn, &conn, mirror_dispatch);
	capn_rpc_set_result_size(&conn, 0, 1);

	for (;;) {
		if (capn_rpc_fd_pump(&conn, fd) != 0)
			break;
	}
	capn_rpc_free(&conn);
	close(fd);
	close(lfd);
	return 0;
}
