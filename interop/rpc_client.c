/* Live-peer RPC interop: this vat against capnp-C++.
 *
 * Bootstraps the Adder served by interop/rpc_peer_server.c++ over
 * rpc-twoparty on 127.0.0.1:<argv[1]> and calls add(), which proves
 * protocol-level compatibility with the reference implementation rather
 * than only wire-format byte equality against our own encoders.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "adder.capnp.h"
#include "capn-rpc.h"
#include "capnp_c.h"
#include "rpc.capnp.h"

static int g_fd = -1;

static int send_fd(void *ctx, const uint8_t *data, size_t len)
{
	size_t off = 0;
	(void)ctx;
	while (off < len) {
		ssize_t n = write(g_fd, data + off, len - off);
		if (n <= 0)
			return -1;
		off += (size_t)n;
	}
	return 0;
}

static ssize_t read_all(int fd, void *p, size_t count)
{
	size_t off = 0;
	while (off < count) {
		ssize_t n = read(fd, (char *)p + off, count - off);
		if (n <= 0)
			return n == 0 && off > 0 ? (ssize_t)off : n;
		off += (size_t)n;
	}
	return (ssize_t)off;
}

/* Read one stream-framed message off the socket into `buf`. The segment
 * table says how much body follows, so the frame boundary is recoverable
 * from the stream without a length prefix of our own. */
static ssize_t read_frame(int fd, uint8_t *buf, size_t cap)
{
	uint32_t nsegs, i, table_bytes, total_words = 0;
	if (read_all(fd, buf, 4) != 4)
		return -1;
	memcpy(&nsegs, buf, 4);
	nsegs += 1;
	if (nsegs > 64)
		return -1;
	table_bytes = 4 + nsegs * 4;
	if (table_bytes % 8)
		table_bytes += 4;
	if (table_bytes > cap)
		return -1;
	if (read_all(fd, buf + 4, table_bytes - 4) != (ssize_t)(table_bytes - 4))
		return -1;
	for (i = 0; i < nsegs; i++) {
		uint32_t w;
		memcpy(&w, buf + 4 + i * 4, 4);
		total_words += w;
	}
	if (table_bytes + total_words * 8 > cap)
		return -1;
	if (read_all(fd, buf + table_bytes, total_words * 8) !=
	    (ssize_t)(total_words * 8))
		return -1;
	return (ssize_t)(table_bytes + total_words * 8);
}

static void fill_add(void *ctx, capn_ptr params)
{
	const int64_t *ab = (const int64_t *)ctx;
	capn_write64(params, 0, (uint64_t)ab[0]);
	capn_write64(params, 8, (uint64_t)ab[1]);
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	struct capn_rpc_conn conn;
	uint8_t buf[16384];
	uint32_t qboot, qcall;
	int port = argc > 1 ? atoi(argv[1]) : 43117;
	int tries;
	int64_t ab[2];

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	/* The peer may still be binding when this starts. */
	for (tries = 0; tries < 100; tries++) {
		g_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (g_fd >= 0 &&
		    connect(g_fd, (struct sockaddr *)&addr, sizeof addr) == 0)
			break;
		if (g_fd >= 0)
			close(g_fd);
		g_fd = -1;
		{
			/* usleep left POSIX in 2008; nanosleep is the portable
			 * spelling of "wait a moment and retry". */
			struct timespec ts;
			ts.tv_sec = 0;
			ts.tv_nsec = 50000000L;
			nanosleep(&ts, NULL);
		}
	}
	if (g_fd < 0) {
		fprintf(stderr, "could not connect to 127.0.0.1:%d\n", port);
		return 1;
	}

	capn_rpc_init(&conn, send_fd, NULL);

	qboot = capn_rpc_send_bootstrap(&conn);
	if (qboot == (uint32_t)-1) {
		fprintf(stderr, "bootstrap not sent\n");
		return 1;
	}
	{
		ssize_t n = read_frame(g_fd, buf, sizeof buf);
		if (n <= 0 || capn_rpc_handle(&conn, buf, (size_t)n) != 0) {
			fprintf(stderr, "bootstrap reply not handled\n");
			return 1;
		}
	}
	if (!capn_rpc_is_answered(&conn, qboot) ||
	    capn_rpc_is_failed(&conn, qboot)) {
		fprintf(stderr, "capnp-C++ did not answer the bootstrap\n");
		return 1;
	}

	/* The bootstrap capability lands in the peer's export table; a
	 * two-party server hands out id 0 for it. */
	ab[0] = 20;
	ab[1] = 22;
	/* The generated descriptor carries the interface id, the ordinal and
	 * the parameter struct's shape, so none of those is written out
	 * here. Getting the shape wrong drops arguments silently. */
	qcall = capn_rpc_send_call(&conn, 0, Adder_INTERFACE_ID,
	                           Adder_add_method.ordinal,
	                           Adder_add_method.params_datasz,
	                           Adder_add_method.params_ptrs,
	                           fill_add, ab);
	if (qcall == (uint32_t)-1) {
		fprintf(stderr, "call not sent\n");
		return 1;
	}
	{
		ssize_t n = read_frame(g_fd, buf, sizeof buf);
		if (n <= 0 || capn_rpc_handle(&conn, buf, (size_t)n) != 0) {
			fprintf(stderr, "call reply not handled\n");
			return 1;
		}
	}
	if (!capn_rpc_is_answered(&conn, qcall)) {
		fprintf(stderr, "capnp-C++ did not answer the call\n");
		return 1;
	}
	if (capn_rpc_is_failed(&conn, qcall)) {
		fprintf(stderr, "capnp-C++ returned an exception for add()\n");
		return 1;
	}
	{
		struct capn m;
		capn_ptr content;
		int64_t sum;
		if (capn_rpc_answer_content(&conn, qcall, &m, &content) != 0) {
			fprintf(stderr, "no results in the add() answer\n");
			return 1;
		}
		sum = (int64_t)capn_read64(content, 0);
		capn_free(&m);
		if (sum != 42) {
			fprintf(stderr, "add(20, 22) returned %lld, expected 42\n",
			        (long long)sum);
			return 1;
		}
	}

	/* tag() has a data word and a pointer. The stub must pass the data
	 * size only; counting the pointer section shifts the text. */
	{
		struct Adder_tag_params in;
		struct Adder_tag_results out;
		struct capn m;
		memset(&in, 0, sizeof in);
		in.n = 9;
		in.label.str = "wire";
		in.label.len = 4;
		qcall = Adder_call_tag(&conn, 0, &in);
		if (qcall == (uint32_t)-1) {
			fprintf(stderr, "tag call not sent\n");
			return 1;
		}
		{
			ssize_t n = read_frame(g_fd, buf, sizeof buf);
			if (n <= 0 || capn_rpc_handle(&conn, buf, (size_t)n) != 0) {
				fprintf(stderr, "tag reply not handled\n");
				return 1;
			}
		}
		if (!capn_rpc_is_answered(&conn, qcall) ||
		    capn_rpc_is_failed(&conn, qcall)) {
			fprintf(stderr, "capnp-C++ did not answer tag()\n");
			return 1;
		}
		memset(&out, 0, sizeof out);
		if (Adder_results_tag(&conn, qcall, &out, &m) != 0) {
			fprintf(stderr, "no results in the tag() answer\n");
			return 1;
		}
		if (out.n != 9 || out.label.len != 4 || out.label.str == NULL ||
		    memcmp(out.label.str, "wire", 4) != 0) {
			fprintf(stderr, "tag(9, \"wire\") came back as n=%d label=%.*s\n",
			        out.n, out.label.len,
			        out.label.str ? out.label.str : "");
			capn_free(&m);
			return 1;
		}
		capn_free(&m);
	}

	/* Pipeline add() through defer()'s promise, then fulfill it. The
	 * Return for the pipelined call has to arrive, and the promise id
	 * from defer() has to resolve, before this side is done. */
	{
		uint16_t path = 0;
		uint32_t qdefer, qpipe, qful;
		int64_t later[2];
		int prom_id = -1;
		int guard;

		qdefer = capn_rpc_send_call(&conn, 0, Adder_INTERFACE_ID,
		                            Adder_defer_method.ordinal,
		                            Adder_defer_method.params_datasz,
		                            Adder_defer_method.params_ptrs,
		                            NULL, NULL);
		later[0] = 7;
		later[1] = 8;
		qpipe = capn_rpc_send_pipelined_call(&conn, qdefer, &path, 1,
		                                     Adder_INTERFACE_ID,
		                                     Adder_add_method.ordinal,
		                                     Adder_add_method.params_datasz,
		                                     Adder_add_method.params_ptrs,
		                                     fill_add, later);
		if (qdefer == (uint32_t)-1 || qpipe == (uint32_t)-1) {
			fprintf(stderr, "promise calls not sent\n");
			return 1;
		}
		guard = 0;
		while (!capn_rpc_is_answered(&conn, qdefer) && guard++ < 16) {
			ssize_t n = read_frame(g_fd, buf, sizeof buf);
			if (n <= 0 || capn_rpc_handle(&conn, buf, (size_t)n) != 0) {
				fprintf(stderr, "defer reply not handled\n");
				return 1;
			}
		}
		if (!capn_rpc_is_answered(&conn, qdefer) ||
		    capn_rpc_is_failed(&conn, qdefer)) {
			fprintf(stderr, "capnp-C++ did not answer defer()\n");
			return 1;
		}
		{
			struct capn m;
			capn_ptr content, cap;
			int is_export = -1;
			if (capn_rpc_answer_content(&conn, qdefer, &m, &content) != 0) {
				fprintf(stderr, "defer() returned no content\n");
				return 1;
			}
			cap = capn_getp(content, 0, 1);
			prom_id = capn_rpc_answer_cap(&conn, qdefer, cap, &is_export);
			capn_free(&m);
			if (prom_id < 0 || is_export != 0) {
				fprintf(stderr, "defer() did not return a promise cap\n");
				return 1;
			}
		}

		qful = capn_rpc_send_call(&conn, 0, Adder_INTERFACE_ID,
		                          Adder_fulfill_method.ordinal,
		                          Adder_fulfill_method.params_datasz,
		                          Adder_fulfill_method.params_ptrs,
		                          NULL, NULL);
		if (qful == (uint32_t)-1) {
			fprintf(stderr, "fulfill not sent\n");
			return 1;
		}
		guard = 0;
		while (guard++ < 16) {
			int resolved = capn_rpc_promise_state(&conn, (uint32_t)prom_id,
			                                      NULL, NULL, NULL);
			if (capn_rpc_is_answered(&conn, qpipe) &&
			    capn_rpc_is_answered(&conn, qful) && resolved)
				break;
			{
				ssize_t n = read_frame(g_fd, buf, sizeof buf);
				if (n <= 0 || capn_rpc_handle(&conn, buf, (size_t)n) != 0) {
					fprintf(stderr, "promise resolution not handled\n");
					return 1;
				}
			}
		}
		if (capn_rpc_is_failed(&conn, qpipe) || capn_rpc_is_failed(&conn, qful)) {
			fprintf(stderr, "capnp-C++ failed the promised add()\n");
			return 1;
		}
		if (!capn_rpc_is_answered(&conn, qpipe)) {
			fprintf(stderr, "promised add() was not answered\n");
			return 1;
		}
		{
			struct capn m;
			capn_ptr content;
			int64_t sum;
			int broken = 0;
			if (capn_rpc_answer_content(&conn, qpipe, &m, &content) != 0) {
				fprintf(stderr, "no results in the promised add()\n");
				return 1;
			}
			sum = (int64_t)capn_read64(content, 0);
			capn_free(&m);
			if (sum != 15) {
				fprintf(stderr, "promised add(7, 8) returned %lld, expected 15\n",
				        (long long)sum);
				return 1;
			}
			if (!capn_rpc_promise_state(&conn, (uint32_t)prom_id, &broken,
			                            NULL, NULL) || broken) {
				fprintf(stderr, "promise was not resolved\n");
				return 1;
			}
		}
	}

	capn_rpc_free(&conn);
	close(g_fd);
	printf("All rpc interop assertions passed.\n");
	return 0;
}
