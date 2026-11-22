/* Call a C++ EzRpcServer method that tail-calls back to this vat.
 * The original question stays unanswered until this vat returns the
 * inner call, then the payload shows up as that question's results.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "capn-rpc.h"
#include "capnp_c.h"

/* interop/tail.capnp Hub @0xe401c0ffee000002 relay @0 (cb, n :Int32). */
#define HUB_INTERFACE_ID 0xe401c0ffee000002ULL

static int g_fd = -1;
static struct capn_rpc_conn *g_conn;
static capn_ptr g_results;
static uint32_t g_question;
static uint32_t g_generation;
static int g_n;
static int g_hits;

static int send_fd(void *ctx, const uint8_t *data, size_t len)
{
	size_t off = 0;
	(void) ctx;
	while (off < len) {
		ssize_t n = write(g_fd, data + off, len - off);
		if (n <= 0)
			return -1;
		off += (size_t) n;
	}
	return 0;
}

static ssize_t read_all(int fd, void *p, size_t count)
{
	size_t off = 0;
	while (off < count) {
		ssize_t n = read(fd, (char *) p + off, count - off);
		if (n <= 0)
			return n == 0 && off > 0 ? (ssize_t) off : n;
		off += (size_t) n;
	}
	return (ssize_t) off;
}

static ssize_t read_frame(int fd, uint8_t *buf, size_t cap)
{
	uint32_t nsegs, i, table_bytes, total_words = 0;
	if (read_all(fd, buf, 4) != 4)
		return -1;
	memcpy(&nsegs, buf, 4);
	nsegs = capn_flip32(nsegs);
	if (nsegs >= 64)
		return -1;
	nsegs += 1;
	table_bytes = 4 + nsegs * 4;
	if (table_bytes % 8)
		table_bytes += 4;
	if (table_bytes > cap)
		return -1;
	if (read_all(fd, buf + 4, table_bytes - 4) != (ssize_t) (table_bytes - 4))
		return -1;
	for (i = 0; i < nsegs; i++) {
		uint32_t w;
		memcpy(&w, buf + 4 + i * 4, 4);
		w = capn_flip32(w);
		total_words += w;
	}
	if (table_bytes + (size_t) total_words * 8 > cap)
		return -1;
	if (read_all(fd, buf + table_bytes, (size_t) total_words * 8) !=
	    (ssize_t) ((size_t) total_words * 8))
		return -1;
	return (ssize_t) (table_bytes + (size_t) total_words * 8);
}

static int echo_dispatch(void *server, uint64_t iface, uint16_t method,
			 capn_ptr params, capn_ptr results)
{
	(void) server;
	(void) iface;
	(void) method;
	g_hits++;
	capn_resolve(&params);
	g_n = (int) capn_read32(params, 0);
	g_question = capn_rpc_current_question(g_conn);
	g_generation = capn_rpc_current_generation(g_conn);
	g_results = results;
	return CAPN_RPC_PENDING;
}

static void fill_relay(void *ctx, capn_ptr params)
{
	int n = *(int *) ctx;
	capn_ptr cap = capn_rpc_new_export_cap(g_conn, params.seg, &g_hits, echo_dispatch);
	capn_write32(params, 0, (uint32_t) n);
	capn_setp(params, 0, cap);
}

static int pump(struct capn_rpc_conn *conn, int until_hit)
{
	uint8_t buf[16384];
	int i;
	for (i = 0; i < 8; i++) {
		struct pollfd pfd;
		pfd.fd = g_fd;
		pfd.events = POLLIN;
		if (poll(&pfd, 1, 200) <= 0)
			break;
		{
			ssize_t n = read_frame(g_fd, buf, sizeof buf);
			if (n <= 0 || capn_rpc_handle(conn, buf, (size_t) n) != 0)
				return -1;
		}
		if (until_hit && g_hits > 0 && i > 0)
			continue;
	}
	return g_hits > 0 ? 0 : -1;
}

int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	struct capn_rpc_conn conn;
	uint32_t qboot, qcall;
	int port = argc > 1 ? atoi(argv[1]) : 43118;
	int tries;
	int n_in = 41;
	struct capn got;
	capn_ptr content;

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t) port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	for (tries = 0; tries < 100; tries++) {
		g_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (g_fd >= 0 && connect(g_fd, (struct sockaddr *) &addr, sizeof addr) == 0)
			break;
		if (g_fd >= 0)
			close(g_fd);
		g_fd = -1;
		{
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
	g_conn = &conn;
	capn_rpc_set_result_size(&conn, 8, 0);

	qboot = capn_rpc_send_bootstrap(&conn);
	{
		uint8_t buf[16384];
		ssize_t n = read_frame(g_fd, buf, sizeof buf);
		if (qboot == (uint32_t) -1 || n <= 0 ||
		    capn_rpc_handle(&conn, buf, (size_t) n) != 0 ||
		    !capn_rpc_is_answered(&conn, qboot)) {
			fprintf(stderr, "bootstrap failed\n");
			return 1;
		}
	}

	qcall = capn_rpc_send_call(&conn, 0, HUB_INTERFACE_ID, 0, 8, 1,
				   fill_relay, &n_in);
	if (qcall == (uint32_t) -1) {
		fprintf(stderr, "relay not sent\n");
		return 1;
	}
	if (pump(&conn, 1) != 0) {
		fprintf(stderr, "tail call did not arrive\n");
		return 1;
	}
	if (capn_rpc_is_answered(&conn, qcall) || capn_rpc_is_failed(&conn, qcall)) {
		fprintf(stderr, "relay was answered before the inner call returned\n");
		return 1;
	}
	capn_write32(g_results, 0, (uint32_t) g_n);
	if (capn_rpc_return(&conn, g_question, g_generation) != 0) {
		fprintf(stderr, "inner return failed\n");
		return 1;
	}
	if (!capn_rpc_is_answered(&conn, qcall) || capn_rpc_is_failed(&conn, qcall)) {
		fprintf(stderr, "relay was not adopted\n");
		return 1;
	}
	if (capn_rpc_answer_content(&conn, qcall, &got, &content) != 0) {
		fprintf(stderr, "no relay content\n");
		return 1;
	}
	if ((int) capn_read32(content, 0) != 41) {
		fprintf(stderr, "relay content %u\n", capn_read32(content, 0));
		capn_free(&got);
		return 1;
	}
	capn_free(&got);
	capn_rpc_free(&conn);
	close(g_fd);
	return 0;
}
