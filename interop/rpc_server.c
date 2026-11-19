/* C vat serving Adder, for the capnp-C++ client in rpc_peer_client.c++. */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "adder.capnp.h"
#include "capn-rpc.h"

static char g_label[64];

static int on_add(void *self, const struct Adder_add_params *in,
                  struct Adder_add_results *out, struct capn_segment *seg)
{
	(void) self;
	(void) seg;
	out->sum = in->a + in->b;
	return 0;
}

static int on_tag(void *self, const struct Adder_tag_params *in,
                  struct Adder_tag_results *out, struct capn_segment *seg)
{
	int n = in->label.len;
	(void) self;
	(void) seg;
	if (n < 0)
		n = 0;
	if (n > (int) sizeof g_label - 1)
		n = (int) sizeof g_label - 1;
	if (in->label.str != NULL && n > 0)
		memcpy(g_label, in->label.str, (size_t) n);
	g_label[n] = '\0';
	out->n = in->n;
	out->label.str = g_label;
	out->label.len = n;
	out->label.seg = NULL;
	return 0;
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	struct capn_rpc_conn conn;
	struct Adder_server server;
	int port = argc > 1 ? atoi(argv[1]) : 43118;
	int lfd, fd, one = 1;

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd < 0)
		return 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t) port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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

	memset(&server, 0, sizeof server);
	server.on_add = on_add;
	server.on_tag = on_tag;
	capn_rpc_init(&conn, capn_rpc_fd_send, &fd);
	capn_rpc_set_bootstrap(&conn, &server, Adder_dispatch);
	capn_rpc_set_result_size(&conn, Adder_RESULT_DATASZ, Adder_RESULT_PTRS);

	for (;;) {
		int rc = capn_rpc_fd_pump(&conn, fd);
		if (rc != 0)
			break;
	}
	capn_rpc_free(&conn);
	close(fd);
	close(lfd);
	return 0;
}
