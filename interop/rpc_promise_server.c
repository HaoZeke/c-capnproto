/* C vat that serves Adder for a capnp-C++ client.
 *
 * defer() exports a promise. fulfill() resolves it to this adder, after
 * delivering calls that arrived while it was still a promise. The C++
 * client pipelines add() through that promise.
 */

#include "posix_net.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "adder.capnp.h"
#include "capn-rpc.h"

static struct capn_rpc_conn *g_conn;
static int g_promise = -1;
static int g_adder;
static int g_watched = -1;

static int adder_dispatch(void *server, uint64_t interface_id, uint16_t method,
                          capn_ptr params, capn_ptr results)
{
	capn_ptr cap;
	(void)server;
	(void)interface_id;
	if (method == Adder_add_method.ordinal) {
		int64_t a, b;
		capn_resolve(&params);
		a = (int64_t)capn_read64(params, 0);
		b = (int64_t)capn_read64(params, 8);
		capn_write64(results, 0, (uint64_t)(a + b));
		return 0;
	}
	if (method == Adder_defer_method.ordinal) {
		g_promise = capn_rpc_export_promise(g_conn, &g_adder, adder_dispatch);
		if (g_promise < 0)
			return -1;
		cap = capn_rpc_new_promise_cap(g_conn, results.seg, &g_adder,
		                               adder_dispatch);
		if (cap.type != CAPN_INTERFACE)
			return -1;
		return capn_setp(results, 0, cap);
	}
	if (method == Adder_fulfill_method.ordinal) {
		if (g_promise < 0)
			return -1;
		/* watch() handed us a capability the caller hosts. Resolve to
		 * that import so pipelined calls are reflected, not answered
		 * here. rpc.c++ resolveExportedPromise writes receiverHosted
		 * for an ImportClient. */
		if (g_watched >= 0)
			return capn_rpc_resolve_import(g_conn, g_promise,
			                               (uint32_t)g_watched);
		return capn_rpc_resolve(g_conn, g_promise, &g_adder, adder_dispatch);
	}
	if (method == Adder_watch_method.ordinal) {
		int is_export = -1;
		capn_resolve(&params);
		cap = capn_getp(params, 0, 1);
		g_watched = capn_rpc_param_cap(g_conn, cap, &is_export);
		if (g_watched < 0 || is_export != 0)
			return -1;
		return 0;
	}
	return -1;
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	struct capn_rpc_conn conn;
	int port = argc > 1 ? atoi(argv[1]) : 43117;
	int lst, fd, one = 1;

	lst = socket(AF_INET, SOCK_STREAM, 0);
	if (lst < 0)
		return 1;
	setsockopt(lst, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = posix_loopback_addr();
	if (bind(lst, (struct sockaddr *)&addr, sizeof addr) != 0 ||
	    listen(lst, 1) != 0) {
		perror("listen");
		return 1;
	}
	fd = accept(lst, NULL, NULL);
	if (fd < 0)
		return 1;

	g_conn = &conn;
	capn_rpc_init(&conn, capn_rpc_fd_send, &fd);
	capn_rpc_set_result_size(&conn, 8, 1);
	capn_rpc_set_bootstrap(&conn, &g_adder, adder_dispatch);

	while (capn_rpc_fd_pump(&conn, fd) == 0)
		;
	capn_rpc_free(&conn);
	close(fd);
	close(lst);
	return 0;
}
