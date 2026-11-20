/* C vat serving CountLeaf, for the capnp-C++ client in rpc_peer_client.c++.
 * The leaf extends CountMid and Adder. CountMid extends CountBase, so a
 * call that names CountBase is forwarded through the middle server. */

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

static int on_inc(void *self, const struct CountBase_inc_params *in,
                  struct CountBase_inc_results *out, struct capn_segment *seg)
{
	(void) self;
	(void) seg;
	out->n = in->n + 1;
	return 0;
}

static int on_twice(void *self, const struct CountMid_twice_params *in,
                    struct CountMid_twice_results *out, struct capn_segment *seg)
{
	(void) self;
	(void) seg;
	out->n = in->n * 2;
	return 0;
}

static int on_dec(void *self, const struct CountLeaf_dec_params *in,
                  struct CountLeaf_dec_results *out, struct capn_segment *seg)
{
	(void) self;
	(void) seg;
	out->n = in->n - 1;
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
	struct Adder_server adder;
	struct CountBase_server base;
	struct CountMid_server mid;
	struct CountLeaf_server leaf;
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

	memset(&adder, 0, sizeof adder);
	memset(&base, 0, sizeof base);
	memset(&mid, 0, sizeof mid);
	memset(&leaf, 0, sizeof leaf);
	adder.on_add = on_add;
	adder.on_tag = on_tag;
	base.on_inc = on_inc;
	mid.on_twice = on_twice;
	mid.super_CountBase = &base;
	leaf.on_dec = on_dec;
	leaf.super_CountMid = &mid;
	leaf.super_Adder = &adder;
	capn_rpc_init(&conn, capn_rpc_fd_send, &fd);
	capn_rpc_set_bootstrap(&conn, &leaf, CountLeaf_dispatch);
	capn_rpc_set_result_size(&conn, CountLeaf_RESULT_DATASZ, CountLeaf_RESULT_PTRS);

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
