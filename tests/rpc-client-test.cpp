/* rpc-client-test.cpp
 *
 * The client half of level 1: asking questions rather than only
 * answering them, and the `-> stream` flow-control window.
 *
 * Two vats share a pair of outboxes, so every message is a real frame
 * and the two are pumped in lockstep.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <deque>
#include <vector>

#include "capn-rpc.h"
#include "capnp_c.h"
#include "rpc.capnp.h"

namespace {

/* A frame queue standing in for a socket. */
struct Wire {
	std::deque<std::vector<uint8_t> > to_client;
	std::deque<std::vector<uint8_t> > to_vat;
};

Wire *g_wire;

int send_to_vat(void *, const uint8_t *d, size_t n)
{
	g_wire->to_vat.push_back(std::vector<uint8_t>(d, d + n));
	return 0;
}

int send_to_client(void *, const uint8_t *d, size_t n)
{
	g_wire->to_client.push_back(std::vector<uint8_t>(d, d + n));
	return 0;
}

/* Deliver everything queued for one side. */
int pump(struct capn_rpc_conn *c, std::deque<std::vector<uint8_t> > &inbox)
{
	int n = 0;
	while (!inbox.empty()) {
		std::vector<uint8_t> f = inbox.front();
		inbox.pop_front();
		capn_rpc_handle(c, f.data(), f.size());
		n++;
	}
	return n;
}

int doubler_calls;
int doubler_fail_from;

int doubler_dispatch(void *, uint64_t, uint16_t, capn_ptr params,
                     capn_ptr results)
{
	doubler_calls++;
	if (doubler_calls >= doubler_fail_from)
		return -1;
	capn_resolve(&params);
	uint32_t in = params.type == CAPN_STRUCT ? capn_read32(params, 0) : 0;
	capn_write32(results, 0, in * 2);
	return 0;
}

void fill_u32(void *ctx, capn_ptr params)
{
	capn_write32(params, 0, *static_cast<uint32_t *>(ctx));
}

class RpcClient : public ::testing::Test {
 protected:
	void SetUp() override
	{
		wire = Wire();
		g_wire = &wire;
		doubler_calls = 0;
		doubler_fail_from = 1 << 30;
		capn_rpc_init(&client, send_to_vat, NULL);
		capn_rpc_init(&vat, send_to_client, NULL);
		capn_rpc_set_bootstrap(&vat, &doubler_calls, doubler_dispatch);
	}

	void settle()
	{
		pump(&vat, wire.to_vat);
		pump(&client, wire.to_client);
	}

	Wire wire;
	struct capn_rpc_conn client;
	struct capn_rpc_conn vat;
};

TEST_F(RpcClient, BootstrapResolvesToThePeersCapability)
{
	uint32_t q = capn_rpc_send_bootstrap(&client);
	ASSERT_NE((uint32_t)-1, q);
	EXPECT_FALSE(capn_rpc_is_answered(&client, q));

	settle();
	EXPECT_TRUE(capn_rpc_is_answered(&client, q));
	EXPECT_FALSE(capn_rpc_is_failed(&client, q));

	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	/* The bootstrap answer's content is the capability itself. */
	EXPECT_EQ(CAPN_INTERFACE, content.type);
	capn_free(&m);
}

TEST_F(RpcClient, CallReturnsTheServersResults)
{
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t arg = 21;
	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 1, 8, 1, fill_u32, &arg);
	ASSERT_NE((uint32_t)-1, q);
	settle();

	EXPECT_TRUE(capn_rpc_is_answered(&client, q));
	EXPECT_FALSE(capn_rpc_is_failed(&client, q));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	EXPECT_EQ(42u, capn_read32(content, 0));
	capn_free(&m);
	EXPECT_EQ(1, doubler_calls);
}

TEST_F(RpcClient, UnroutableCallComesBackFailedNotSilent)
{
	uint32_t q = capn_rpc_send_call(&client, 99, 0, 0, 8, 1, NULL, NULL);
	settle();
	EXPECT_TRUE(capn_rpc_is_answered(&client, q));
	EXPECT_TRUE(capn_rpc_is_failed(&client, q));
	struct capn m;
	capn_ptr content;
	EXPECT_NE(0, capn_rpc_answer_content(&client, q, &m, &content));
}

TEST_F(RpcClient, ReturnForAQuestionWeNeverAskedIsIgnored)
{
	/* The vat answers question 77, which this client never sent. */
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct Return r;
	memset(&r, 0, sizeof r);
	r.answerId = 77;
	r.which = Return_results;
	r.results = new_Payload(cs);
	Return_ptr rp = new_Return(cs);
	write_Return(&r, rp);
	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message__return;
	m._return = rp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	uint8_t buf[2048];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	capn_rpc_handle(&client, buf, static_cast<size_t>(sz));

	/* Recording it would let a peer plant answers to questions we never
	 * asked, which later pipelining would then trust. */
	EXPECT_FALSE(capn_rpc_is_answered(&client, 77));
}

TEST_F(RpcClient, StreamWindowBoundsOutstandingCalls)
{
	capn_rpc_send_bootstrap(&client);
	settle();

	struct capn_rpc_stream s;
	capn_rpc_stream_init(&s, 2);
	uint32_t a = 1, b = 2, c3 = 3;
	ASSERT_EQ(0, capn_rpc_stream_send(&client, &s, 0, 0, 0, 8, 1, fill_u32, &a));
	ASSERT_EQ(0, capn_rpc_stream_send(&client, &s, 0, 0, 0, 8, 1, fill_u32, &b));
	EXPECT_EQ(2, s.nout);

	settle();
	/* The third send has to retire one before it can go. */
	ASSERT_EQ(0, capn_rpc_stream_send(&client, &s, 0, 0, 0, 8, 1, fill_u32, &c3));
	EXPECT_LE(s.nout, 2);

	settle();
	EXPECT_EQ(0, capn_rpc_stream_finish(&client, &s));
	EXPECT_EQ(0, s.nout);
	EXPECT_EQ(3, doubler_calls);
}

TEST_F(RpcClient, StreamFinishReportsFailureAfterDrainingTheWindow)
{
	doubler_fail_from = 2;
	capn_rpc_send_bootstrap(&client);
	settle();

	struct capn_rpc_stream s;
	capn_rpc_stream_init(&s, 4);
	uint32_t v = 1;
	capn_rpc_stream_send(&client, &s, 0, 0, 0, 8, 1, fill_u32, &v);
	capn_rpc_stream_send(&client, &s, 0, 0, 0, 8, 1, fill_u32, &v);
	capn_rpc_stream_send(&client, &s, 0, 0, 0, 8, 1, fill_u32, &v);
	settle();

	/* The window still drains fully; finish is where the failure lands. */
	EXPECT_NE(0, capn_rpc_stream_finish(&client, &s));
	EXPECT_EQ(0, s.nout);
	EXPECT_TRUE(s.failed);
}

TEST_F(RpcClient, OnceFailedALaterSendIsRefused)
{
	doubler_fail_from = 1;
	capn_rpc_send_bootstrap(&client);
	settle();

	struct capn_rpc_stream s;
	capn_rpc_stream_init(&s, 4);
	uint32_t v = 1;
	capn_rpc_stream_send(&client, &s, 0, 0, 0, 8, 1, fill_u32, &v);
	settle();
	EXPECT_NE(0, capn_rpc_stream_finish(&client, &s));
	EXPECT_EQ(0, s.nout);
	/* The window is empty, so this refusal can only come from the stream
	 * remembering it failed. */
	EXPECT_NE(0, capn_rpc_stream_send(&client, &s, 0, 0, 0, 8, 1, fill_u32, &v));
}

} // namespace
