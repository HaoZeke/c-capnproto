/* C vat that lends caps to a capnp-C++ client.
 *
 * borrow(n) returns a new export: senderHosted for odd n, senderPromise
 * for even n. tag() reports in `n` how many references to those exports
 * remain, so the client can see what its Finish released. borrow(n)
 * for n <= 4 answers after 20ms, so the client cancels it first. Run as
 * `release_c_server <port>`.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "adder.capnp.h"
#include "capn-rpc.h"

static struct capn_rpc_conn *g_conn;
static int g_lender;
static int g_borrowed;

static int borrowed_refs(void)
{
	int i, n = 0;

	for (i = 0; i < g_conn->exports_cap; i++)
		if (g_conn->exports[i].used && g_conn->exports[i].server == &g_borrowed)
			n += g_conn->exports[i].refcount;
	return n;
}

static int lender_dispatch(void *server, uint64_t interface_id, uint16_t method,
                           capn_ptr params, capn_ptr results)
{
	capn_ptr cap;
	int32_t n;
	(void)server;
	(void)interface_id;
	capn_resolve(&params);
	if (method == Adder_add_method.ordinal) {
		int64_t a = (int64_t)capn_read64(params, 0);
		int64_t b = (int64_t)capn_read64(params, 8);
		capn_write64(results, 0, (uint64_t)(a + b));
		return 0;
	}
	if (method == Adder_tag_method.ordinal) {
		capn_write32(results, 0, (uint32_t)borrowed_refs());
		return 0;
	}
	if (method == Adder_borrow_method.ordinal) {
		n = (int32_t)capn_read32(params, 0);
		if (n <= 4) {
			struct timespec ts = {0, 20000000L};
			nanosleep(&ts, NULL);
		}
		if (n % 2)
			cap = capn_rpc_new_export_cap(g_conn, results.seg, &g_borrowed,
			                              lender_dispatch);
		else
			cap = capn_rpc_new_promise_cap(g_conn, results.seg, &g_borrowed,
			                               lender_dispatch);
		if (cap.type != CAPN_INTERFACE)
			return -1;
		capn_write32(results, 0, (uint32_t)n);
		return capn_setp(results, 0, cap);
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
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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
	capn_rpc_set_bootstrap(&conn, &g_lender, lender_dispatch);

	while (capn_rpc_fd_pump(&conn, fd) == 0)
		;
	capn_rpc_free(&conn);
	close(fd);
	close(lst);
	return 0;
}
