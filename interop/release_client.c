/* Live-peer capability release: this vat against capnp-C++.
 *
 * interop/release_server.c++ lends a new Adder on each borrow() and
 * counts the live ones in tag(). rpc.c++ drops a returned cap once its
 * caller releases it, so after each Finish the count says whether this
 * vat let go of what it did not keep. Run as `release_client <port>`.
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
#include "capnp_c.h"
#include "frame_le.h"
#include "rpc.capnp.h"

static int g_fd = -1;
static int g_finishes;
static int g_release_result_caps = -1;
static int g_releases;

/* Count what goes out, so the checks can see Finish and Release. */
static void note_sent(const uint8_t *data, size_t len)
{
	struct capn m;
	struct Message msg;
	Message_ptr mp;

	if (capn_init_mem(&m, data, len, 0) != 0)
		return;
	mp.p = capn_getp(capn_root(&m), 0, 1);
	read_Message(&msg, mp);
	if (msg.which == Message_finish) {
		struct Finish f;
		read_Finish(&f, msg.finish);
		g_finishes++;
		g_release_result_caps = f.releaseResultCaps;
	} else if (msg.which == Message_release) {
		g_releases++;
	}
	capn_free(&m);
}

static int send_fd(void *ctx, const uint8_t *data, size_t len)
{
	size_t off = 0;
	(void)ctx;
	note_sent(data, len);
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

static ssize_t read_frame(int fd, uint8_t *buf, size_t cap)
{
	uint32_t count, i, table_bytes;
	size_t nsegs, total_words = 0;
	if (read_all(fd, buf, 4) != 4)
		return -1;
	count = frame_le32(buf);
	if (count >= 64u)
		return -1;
	nsegs = (size_t) count + 1u;
	table_bytes = 4u + (uint32_t) nsegs * 4u;
	if (table_bytes % 8)
		table_bytes += 4;
	if ((size_t) table_bytes > cap)
		return -1;
	if (read_all(fd, buf + 4, table_bytes - 4) != (ssize_t)(table_bytes - 4))
		return -1;
	for (i = 0; i < nsegs; i++)
		total_words += frame_le32(buf + 4 + i * 4);
	if ((size_t) table_bytes + total_words * 8 > cap)
		return -1;
	if (read_all(fd, buf + table_bytes, total_words * 8) !=
	    (ssize_t)(total_words * 8))
		return -1;
	return (ssize_t)((size_t) table_bytes + total_words * 8);
}

static uint8_t g_buf[16384];

static int wait_answer(struct capn_rpc_conn *c, uint32_t q)
{
	while (!capn_rpc_is_answered(c, q)) {
		ssize_t n = read_frame(g_fd, g_buf, sizeof g_buf);
		if (n <= 0 || capn_rpc_handle(c, g_buf, (size_t)n) != 0)
			return -1;
	}
	return capn_rpc_is_failed(c, q) ? -1 : 0;
}

static void fill_n(void *ctx, capn_ptr params)
{
	capn_write32(params, 0, (uint32_t) *(const int32_t *)ctx);
}

static void fill_add(void *ctx, capn_ptr params)
{
	const int64_t *ab = (const int64_t *)ctx;
	capn_write64(params, 0, (uint64_t)ab[0]);
	capn_write64(params, 8, (uint64_t)ab[1]);
}

static uint32_t call_borrow(struct capn_rpc_conn *c, int32_t n)
{
	return capn_rpc_send_call(c, 0, Adder_INTERFACE_ID,
	                          Adder_borrow_method.ordinal,
	                          Adder_borrow_method.params_datasz,
	                          Adder_borrow_method.params_ptrs, fill_n, &n);
}

/* How many borrowed Adders the server still has. tag() returns no cap,
 * so rpc.c++ says noFinishNeeded and no Finish may follow. */
static int live(struct capn_rpc_conn *c)
{
	int32_t zero = 0;
	int before = g_finishes;
	struct capn m;
	capn_ptr content;
	int n;
	uint32_t q = capn_rpc_send_call(c, 0, Adder_INTERFACE_ID,
	                                Adder_tag_method.ordinal,
	                                Adder_tag_method.params_datasz,
	                                Adder_tag_method.params_ptrs, fill_n, &zero);

	if (q == (uint32_t)-1 || wait_answer(c, q) != 0)
		return -1;
	if (capn_rpc_answer_content(c, q, &m, &content) != 0)
		return -1;
	n = (int) capn_read32(content, 0);
	capn_free(&m);
	capn_rpc_send_finish(c, q);
	if (g_finishes != before) {
		fprintf(stderr, "a Finish followed a Return with noFinishNeeded\n");
		return -1;
	}
	return n;
}

static int expect_live(struct capn_rpc_conn *c, int want, const char *what)
{
	int got = live(c);
	if (got != want) {
		fprintf(stderr, "%s: %d borrowed caps alive, expected %d\n", what, got, want);
		return -1;
	}
	return 0;
}

static int connect_port(int port)
{
	struct sockaddr_in addr;
	int tries;

	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	for (tries = 0; tries < 100; tries++) {
		struct timespec ts;
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd >= 0 && connect(fd, (struct sockaddr *)&addr, sizeof addr) == 0)
			return fd;
		if (fd >= 0)
			close(fd);
		ts.tv_sec = 0;
		ts.tv_nsec = 50000000L;
		nanosleep(&ts, NULL);
	}
	return -1;
}

int main(int argc, char **argv)
{
	struct capn_rpc_conn conn;
	uint32_t q;
	int port = argc > 1 ? atoi(argv[1]) : 43117;

	g_fd = connect_port(port);
	if (g_fd < 0) {
		fprintf(stderr, "could not connect to 127.0.0.1:%d\n", port);
		return 1;
	}
	capn_rpc_init(&conn, send_fd, NULL);

	q = capn_rpc_send_bootstrap(&conn);
	if (q == (uint32_t)-1 || wait_answer(&conn, q) != 0) {
		fprintf(stderr, "capnp-C++ did not answer the bootstrap\n");
		goto fail;
	}
	/* Finishing the bootstrap keeps import 0. */
	capn_rpc_send_finish(&conn, q);
	if (expect_live(&conn, 0, "after bootstrap") != 0)
		goto fail;

	/* A cap nobody read is released with the Finish. */
	q = call_borrow(&conn, 1);
	if (q == (uint32_t)-1 || wait_answer(&conn, q) != 0) {
		fprintf(stderr, "borrow(1) failed\n");
		goto fail;
	}
	if (expect_live(&conn, 1, "borrow(1) answered") != 0)
		goto fail;
	g_releases = 0;
	capn_rpc_send_finish(&conn, q);
	if (g_releases != 1 || g_release_result_caps != 0) {
		fprintf(stderr, "finish after borrow(1): %d releases, releaseResultCaps %d\n",
		        g_releases, g_release_result_caps);
		goto fail;
	}
	if (expect_live(&conn, 0, "unread cap after Finish") != 0)
		goto fail;

	/* A cap the application read stays until it releases it. */
	q = call_borrow(&conn, 2);
	if (q == (uint32_t)-1 || wait_answer(&conn, q) != 0) {
		fprintf(stderr, "borrow(2) failed\n");
		goto fail;
	}
	{
		struct capn m;
		capn_ptr content, cap;
		int is_export = -1;
		int64_t id, ab[2] = {20, 22};
		uint32_t qa;

		if (capn_rpc_answer_content(&conn, q, &m, &content) != 0)
			goto fail;
		cap = capn_getp(content, 0, 1);
		id = capn_rpc_answer_cap(&conn, q, cap, &is_export);
		capn_free(&m);
		if (id < 0 || is_export != 0) {
			fprintf(stderr, "borrow(2) returned no import\n");
			goto fail;
		}
		g_releases = 0;
		capn_rpc_send_finish(&conn, q);
		if (g_releases != 0) {
			fprintf(stderr, "Finish released a cap the application read\n");
			goto fail;
		}
		if (expect_live(&conn, 1, "read cap after Finish") != 0)
			goto fail;
		qa = capn_rpc_send_call(&conn, (uint32_t) id, Adder_INTERFACE_ID,
		                        Adder_add_method.ordinal,
		                        Adder_add_method.params_datasz,
		                        Adder_add_method.params_ptrs, fill_add, ab);
		if (qa == (uint32_t)-1 || wait_answer(&conn, qa) != 0) {
			fprintf(stderr, "add() on the kept cap failed\n");
			goto fail;
		}
		capn_rpc_send_finish(&conn, qa);
		capn_rpc_send_release(&conn, (uint32_t) id, 1);
		if (expect_live(&conn, 0, "read cap after Release") != 0)
			goto fail;
	}

	/* A Finish sent before the Return is read asks the peer to drop
	 * the results' caps itself. */
	q = call_borrow(&conn, 3);
	if (q == (uint32_t)-1) {
		fprintf(stderr, "borrow(3) not sent\n");
		goto fail;
	}
	{
		ssize_t n = read_frame(g_fd, g_buf, sizeof g_buf);
		if (n <= 0) {
			fprintf(stderr, "no Return for borrow(3)\n");
			goto fail;
		}
		capn_rpc_send_finish(&conn, q);
		if (g_release_result_caps != 1) {
			fprintf(stderr, "a cancelling Finish left releaseResultCaps unset\n");
			goto fail;
		}
		if (capn_rpc_handle(&conn, g_buf, (size_t)n) != 0)
			goto fail;
	}
	if (expect_live(&conn, 0, "cancelled borrow(3)") != 0)
		goto fail;

	printf("All rpc release interop assertions passed.\n");
	capn_rpc_free(&conn);
	close(g_fd);
	return 0;

fail:
	capn_rpc_free(&conn);
	close(g_fd);
	return 1;
}
