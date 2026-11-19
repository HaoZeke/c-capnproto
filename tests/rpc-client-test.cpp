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
#include <sys/socket.h>
#include <unistd.h>
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


/* Echo the Data in parameter pointer 0 into result pointer 0, and report
 * its length in the first result word. */
int echo_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr results)
{
	capn_resolve(&params);
	if (params.type != CAPN_STRUCT)
		return -1;
	capn_data in = capn_get_data(params, 0);
	int n = capn_len(in);
	capn_list8 out = capn_new_list8(results.seg, n);
	if (n > 0 && out.p.type == CAPN_NULL)
		return -1;
	std::vector<uint8_t> tmp(static_cast<size_t>(n));
	if (n > 0) {
		capn_getv8(in.p.type == CAPN_NULL ? capn_list8{} : capn_list8{in.p}, 0, tmp.data(), n);
		capn_setv8(out, 0, tmp.data(), n);
	}
	capn_setp(results, 0, out.p);
	capn_write32(results, 0, static_cast<uint32_t>(n));
	capn_write32(results, 8, 0xfeedbeefu);
	return 0;
}

struct Blob {
	size_t n;
};

void fill_blob(void *ctx, capn_ptr params)
{
	size_t n = static_cast<Blob *>(ctx)->n;
	capn_list8 l = capn_new_list8(params.seg, static_cast<int>(n));
	std::vector<uint8_t> bytes(n);
	for (size_t i = 0; i < n; i++)
		bytes[i] = static_cast<uint8_t>(i * 31 + 7);
	capn_setv8(l, 0, bytes.data(), static_cast<int>(n));
	capn_setp(params, 0, l.p);
}

/* persistent.capnp's Persistent.save (interface 0xc8cb212fcd9f5691, method
 * 0): the application answers with a SturdyRef of its own choosing. */
const uint64_t kPersistentId = 0xc8cb212fcd9f5691ULL;

int persistent_dispatch(void *, uint64_t iface, uint16_t method, capn_ptr,
                        capn_ptr results)
{
	if (iface != kPersistentId || method != 0)
		return -1;
	capn_text ref = {static_cast<int>(strlen("sturdy:vault/42")), "sturdy:vault/42", NULL};
	return capn_set_text(results, 0, ref);
}

int counter_like_dispatch(void *, uint64_t, uint16_t, capn_ptr, capn_ptr)
{
	return 0;
}

/* Capabilities in payloads. The server hands out `doubler` objects and
 * accepts a callback capability it then calls back. */
struct capn_rpc_conn *g_vat;
int g_callback_hits;
int g_callback_arg;
int g_callback_failed;

int counter_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr results)
{
	capn_resolve(&params);
	g_callback_arg = params.type == CAPN_STRUCT ? static_cast<int>(capn_read32(params, 0)) : -1;
	g_callback_hits++;
	capn_write32(results, 0, 1000u + static_cast<uint32_t>(g_callback_hits));
	return 0;
}

/* method 0: return a new doubler capability in result pointer 0.
 * method 1: take a callback capability in parameter pointer 0 and call it. */
int cap_server_dispatch(void *, uint64_t, uint16_t method, capn_ptr params, capn_ptr results)
{
	if (method == 0) {
		capn_ptr cap = capn_rpc_new_export_cap(g_vat, results.seg, &doubler_calls, doubler_dispatch);
		if (cap.type != CAPN_INTERFACE)
			return -1;
		return capn_setp(results, 0, cap);
	}
	capn_resolve(&params);
	capn_ptr cap = capn_getp(params, 0, 1);
	int is_export = -1;
	int id = capn_rpc_param_cap(g_vat, cap, &is_export);
	if (id < 0 || is_export != 0) {
		g_callback_failed = 1;
		return -1;
	}
	uint32_t arg = 77;
	return capn_rpc_send_call(g_vat, static_cast<uint32_t>(id), 0, 0, 8, 1, fill_u32, &arg) == (uint32_t)-1;
}

void fill_with_callback(void *ctx, capn_ptr params)
{
	struct capn_rpc_conn *c = static_cast<struct capn_rpc_conn *>(ctx);
	capn_ptr cap = capn_rpc_new_export_cap(c, params.seg, &g_callback_hits, counter_dispatch);
	capn_setp(params, 0, cap);
}

/* A server that answers later: it parks the call and returns PENDING. */
struct Parked {
	uint32_t question;
	capn_ptr results;
	uint32_t arg;
	int calls;
};
Parked g_parked;

int parking_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr results)
{
	capn_resolve(&params);
	g_parked.question = capn_rpc_current_question(g_vat);
	g_parked.results = results;
	g_parked.arg = params.type == CAPN_STRUCT ? capn_read32(params, 0) : 0;
	g_parked.calls++;
	return CAPN_RPC_PENDING;
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

	void TearDown() override
	{
		capn_rpc_free(&client);
		capn_rpc_free(&vat);
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

TEST_F(RpcClient, PayloadsLargerThanEightKilobytesRoundTrip)
{
	capn_rpc_set_bootstrap(&vat, &doubler_calls, echo_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	Blob blob = {40000};
	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, fill_blob, &blob);
	ASSERT_NE((uint32_t)-1, q);
	settle();

	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	EXPECT_EQ(40000u, capn_read32(content, 0));
	capn_list8 echoed;
	echoed.p = capn_getp(content, 0, 1);
	ASSERT_EQ(40000, capn_len(echoed));
	EXPECT_EQ(static_cast<uint8_t>(39999 * 31 + 7), capn_get8(echoed, 39999));
	capn_free(&m);
}

TEST_F(RpcClient, ResultStructSizeFollowsTheConnectionSetting)
{
	capn_rpc_set_bootstrap(&vat, &doubler_calls, echo_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	/* The default result struct has one data word, so the second word the
	 * echo server writes is dropped. */
	Blob blob = {4};
	uint32_t q1 = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, fill_blob, &blob);
	settle();
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q1, &m, &content));
	EXPECT_EQ(0u, capn_read32(content, 8));
	capn_free(&m);

	capn_rpc_set_result_size(&vat, 16, 1);
	uint32_t q2 = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, fill_blob, &blob);
	settle();
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q2, &m, &content));
	EXPECT_EQ(4u, capn_read32(content, 0));
	EXPECT_EQ(0xfeedbeefu, capn_read32(content, 8));
	capn_free(&m);
}

TEST_F(RpcClient, PeerAbortFailsOutstandingQuestionsAndStopsTheConnection)
{
	uint32_t q = capn_rpc_send_bootstrap(&client);
	ASSERT_NE((uint32_t)-1, q);
	wire.to_vat.clear();

	ASSERT_EQ(0, capn_rpc_send_abort(&vat, "going away"));
	EXPECT_TRUE(capn_rpc_is_aborted(&vat));
	ASSERT_EQ(1u, wire.to_client.size());
	EXPECT_EQ(0, capn_rpc_handle(&client, wire.to_client[0].data(), wire.to_client[0].size()));
	wire.to_client.clear();

	EXPECT_TRUE(capn_rpc_is_aborted(&client));
	EXPECT_TRUE(capn_rpc_is_answered(&client, q));
	EXPECT_TRUE(capn_rpc_is_failed(&client, q));

	/* Nothing more is accepted, and nothing is sent back. */
	capn_rpc_send_bootstrap(&vat);
	size_t before = wire.to_client.size();
	pump(&client, wire.to_vat);
	pump(&vat, wire.to_vat);
	EXPECT_EQ(before, wire.to_client.size());
}

TEST_F(RpcClient, UnimplementedForOurCallFailsItAndIsNeverEchoed)
{
	capn_rpc_send_bootstrap(&client);
	settle();
	uint32_t arg = 1;
	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, fill_u32, &arg);
	ASSERT_NE((uint32_t)-1, q);
	ASSERT_EQ(1u, wire.to_vat.size());

	/* The peer replies Unimplemented, echoing our Call, instead of a Return. */
	std::vector<uint8_t> call = wire.to_vat.front();
	wire.to_vat.clear();
	struct capn in;
	ASSERT_EQ(0, capn_init_mem(&in, call.data(), call.size(), 0));
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_unimplemented;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(mp.p, 0, capn_getp(capn_root(&in), 0, 1));
	capn_setp(capn_root(&msg), 0, mp.p);
	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	capn_free(&in);
	ASSERT_GT(sz, 0);

	wire.to_client.clear();
	EXPECT_EQ(0, capn_rpc_handle(&client, buf, static_cast<size_t>(sz)));
	EXPECT_TRUE(capn_rpc_is_answered(&client, q));
	EXPECT_TRUE(capn_rpc_is_failed(&client, q));
	EXPECT_TRUE(wire.to_vat.empty()) << "answered an Unimplemented with another";
	EXPECT_TRUE(wire.to_client.empty());
}

TEST_F(RpcClient, UnimplementedEchoesTheOriginalMessageNotAWrapper)
{
	/* A Resolve this vat does not adopt is answered with Unimplemented
	 * whose payload is that Resolve itself. */
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct Resolve rs;
	memset(&rs, 0, sizeof rs);
	rs.promiseId = 5;
	Resolve_ptr rp = new_Resolve(cs);
	write_Resolve(&rs, rp);
	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_resolve;
	m.resolve = rp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	uint8_t buf[512];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);

	ASSERT_EQ(0, capn_rpc_handle(&vat, buf, static_cast<size_t>(sz)));
	ASSERT_EQ(1u, wire.to_client.size());

	struct capn reply;
	ASSERT_EQ(0, capn_init_mem(&reply, wire.to_client[0].data(), wire.to_client[0].size(), 0));
	Message_ptr rmp;
	rmp.p = capn_getp(capn_root(&reply), 0, 1);
	struct Message out;
	read_Message(&out, rmp);
	ASSERT_EQ(Message_unimplemented, out.which);
	struct Message echoed;
	read_Message(&echoed, out.unimplemented);
	EXPECT_EQ(Message_resolve, echoed.which);
	struct Resolve back;
	read_Resolve(&back, echoed.resolve);
	EXPECT_EQ(5u, back.promiseId);
	capn_free(&reply);
}

TEST_F(RpcClient, PersistenceIsOrdinaryDispatchOfPersistentSave)
{
	capn_rpc_set_bootstrap(&vat, &doubler_calls, persistent_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, kPersistentId, 0, 0, 1, NULL, NULL);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	capn_text def = {0, "", NULL};
	capn_text ref = capn_get_text(content, 0, def);
	EXPECT_STREQ("sturdy:vault/42", ref.str);
	capn_free(&m);

	/* A method the application does not implement fails, not hangs. */
	uint32_t q2 = capn_rpc_send_call(&client, 0, kPersistentId, 1, 0, 1, NULL, NULL);
	settle();
	EXPECT_TRUE(capn_rpc_is_failed(&client, q2));
}

TEST_F(RpcClient, OneServerExportedWithTwoDispatchersGetsTwoIds)
{
	int obj = 0;
	int a = capn_rpc_export(&vat, &obj, doubler_dispatch);
	int b = capn_rpc_export(&vat, &obj, doubler_dispatch);
	int c2 = capn_rpc_export(&vat, &obj, counter_like_dispatch);
	EXPECT_GE(a, 0);
	EXPECT_EQ(a, b) << "the same object must keep one id";
	EXPECT_NE(a, c2) << "a different dispatcher is a different capability";
	EXPECT_EQ(2, vat.exports[a].refcount);
}

TEST_F(RpcClient, RpcTablesGrowPastTheirInitialCapacity)
{
	std::vector<int> objects(129);
	int i;

	for (i = 0; i < static_cast<int>(objects.size()); i++)
		ASSERT_EQ(i, capn_rpc_export(&vat, &objects[i], doubler_dispatch));
	EXPECT_GT(vat.exports_cap, 64);

	for (i = 0; i < 129; i++)
		ASSERT_NE((uint32_t)-1, capn_rpc_send_bootstrap(&client));
	pump(&vat, wire.to_vat);
	EXPECT_GT(client.questions_cap, 64);
	EXPECT_GT(vat.answers_cap, 64);
}

TEST_F(RpcClient, ServerReturnsACapabilityTheClientThenCalls)
{
	g_vat = &vat;
	capn_rpc_set_bootstrap(&vat, &doubler_calls, cap_server_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, NULL, NULL);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));

	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	capn_ptr cap = capn_getp(content, 0, 1);
	ASSERT_EQ(CAPN_INTERFACE, cap.type);
	int is_export = -1;
	int id = capn_rpc_answer_cap(&client, q, cap, &is_export);
	ASSERT_GE(id, 0);
	EXPECT_EQ(0, is_export);
	capn_free(&m);

	uint32_t arg = 21;
	uint32_t q2 = capn_rpc_send_call(&client, static_cast<uint32_t>(id), 0, 0, 8, 1, fill_u32, &arg);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q2));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q2));
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q2, &m, &content));
	EXPECT_EQ(42u, capn_read32(content, 0));
	capn_free(&m);
}

TEST_F(RpcClient, PipelinedCallUsesTheCapabilityBeforeTheAnswerIsRead)
{
	g_vat = &vat;
	capn_rpc_set_bootstrap(&vat, &doubler_calls, cap_server_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, NULL, NULL);
	/* Neither message has been delivered: the answer does not exist yet. */
	const uint16_t path[] = {0};
	uint32_t arg = 8;
	uint32_t q2 = capn_rpc_send_pipelined_call(&client, q, path, 1, 0, 0, 8, 1, fill_u32, &arg);
	ASSERT_NE((uint32_t)-1, q2);
	settle();

	ASSERT_TRUE(capn_rpc_is_answered(&client, q2));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q2));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q2, &m, &content));
	EXPECT_EQ(16u, capn_read32(content, 0));
	capn_free(&m);
}

TEST_F(RpcClient, CapabilityInParamsLetsTheServerCallBack)
{
	g_vat = &vat;
	g_callback_hits = 0;
	g_callback_arg = 0;
	g_callback_failed = 0;
	capn_rpc_set_bootstrap(&vat, &doubler_calls, cap_server_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 1, 0, 1, fill_with_callback, &client);
	ASSERT_NE((uint32_t)-1, q);
	/* The call, the server's call back into the client, and both Returns. */
	for (int i = 0; i < 4; i++)
		settle();

	EXPECT_FALSE(g_callback_failed);
	EXPECT_TRUE(capn_rpc_is_answered(&client, q));
	EXPECT_FALSE(capn_rpc_is_failed(&client, q));
	EXPECT_EQ(1, g_callback_hits);
	EXPECT_EQ(77, g_callback_arg);
}

TEST_F(RpcClient, PassingBackACapabilityNamesTheReceiversExport)
{
	g_vat = &vat;
	capn_rpc_set_bootstrap(&vat, &doubler_calls, doubler_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	/* A receiverHosted entry (what a peer sends when it passes back one of
	 * ours) resolves as an export, not an import. */
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	capn_ptr cap = capn_rpc_new_import_cap(&client, cs, 5);
	ASSERT_EQ(CAPN_INTERFACE, cap.type);
	ASSERT_EQ(0, cap.len);
	EXPECT_EQ(1, client.npending);
	client.npending = 0;
	capn_free(&msg);
}

TEST_F(RpcClient, DeferredReplyIsSentWhenTheServerReturns)
{
	g_vat = &vat;
	g_parked = Parked();
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t arg = 9;
	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, fill_u32, &arg);
	settle();
	EXPECT_EQ(1, g_parked.calls);
	EXPECT_EQ(9u, g_parked.arg);
	EXPECT_FALSE(capn_rpc_is_answered(&client, q)) << "answered before the server returned";

	capn_write32(g_parked.results, 0, g_parked.arg * 3);
	ASSERT_EQ(0, capn_rpc_return(&vat, g_parked.question));
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	EXPECT_EQ(27u, capn_read32(content, 0));
	capn_free(&m);

	/* Answering twice is refused. */
	EXPECT_EQ(-1, capn_rpc_return(&vat, g_parked.question));
}

TEST_F(RpcClient, DeferredReplyCanFail)
{
	g_vat = &vat;
	g_parked = Parked();
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, NULL, NULL);
	settle();
	ASSERT_EQ(0, capn_rpc_return_exception(&vat, g_parked.question, "gave up"));
	settle();
	EXPECT_TRUE(capn_rpc_is_answered(&client, q));
	EXPECT_TRUE(capn_rpc_is_failed(&client, q));
}

TEST_F(RpcClient, FinishCancelsAParkedCall)
{
	g_vat = &vat;
	g_parked = Parked();
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, NULL, NULL);
	settle();
	ASSERT_EQ(1, g_parked.calls);
	capn_rpc_send_finish(&client, q);
	settle();

	/* The call is gone; returning for it reports that and sends nothing. */
	size_t before = wire.to_client.size();
	EXPECT_EQ(-1, capn_rpc_return(&vat, g_parked.question));
	EXPECT_EQ(before, wire.to_client.size());
}

TEST_F(RpcClient, ParkedCallsAreReleasedWithTheConnection)
{
	g_vat = &vat;
	g_parked = Parked();
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();
	capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, NULL, NULL);
	settle();
	/* TearDown's capn_rpc_free must release the parked message (leak check). */
}

} // namespace

TEST(RpcFd, TwoVatsTalkOverASocketPair)
{
	int sv[2];
	ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
	int calls = 0;
	struct capn_rpc_conn client, server;
	capn_rpc_init(&client, capn_rpc_fd_send, &sv[0]);
	capn_rpc_init(&server, capn_rpc_fd_send, &sv[1]);
	capn_rpc_set_bootstrap(&server, &calls, doubler_dispatch);
	doubler_calls = 0;
	doubler_fail_from = 1 << 30;

	uint32_t qb = capn_rpc_send_bootstrap(&client);
	ASSERT_EQ(0, capn_rpc_fd_pump(&server, sv[1]));
	ASSERT_EQ(0, capn_rpc_fd_pump(&client, sv[0]));
	EXPECT_TRUE(capn_rpc_is_answered(&client, qb));

	uint32_t arg = 20;
	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, fill_u32, &arg);
	ASSERT_NE((uint32_t)-1, q);
	ASSERT_EQ(0, capn_rpc_fd_pump(&server, sv[1]));
	ASSERT_EQ(0, capn_rpc_fd_pump(&client, sv[0]));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	EXPECT_EQ(40u, capn_read32(content, 0));
	capn_free(&m);

	close(sv[0]);
	EXPECT_EQ(1, capn_rpc_fd_pump(&server, sv[1]));
	close(sv[1]);
	capn_rpc_free(&client);
	capn_rpc_free(&server);
}

TEST(RpcFd, ARefusedFrameIsAnErrorNotAHang)
{
	int sv[2];
	ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
	/* A table claiming 2000 segments, and then a truncated stream. */
	const uint8_t bad[8] = {0xcf, 0x07, 0, 0, 0, 0, 0, 0};
	ASSERT_EQ(8, write(sv[0], bad, 8));
	uint8_t *f = nullptr;
	size_t n = 0;
	EXPECT_EQ(-1, capn_rpc_fd_read_frame(sv[1], &f, &n));
	EXPECT_EQ(nullptr, f);
	close(sv[0]);
	close(sv[1]);

	/* A segment claiming more than the limit allows. */
	ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
	const uint8_t big[16] = {0, 0, 0, 0, 0xff, 0xff, 0xff, 0x1f, 0, 0, 0, 0, 0, 0, 0, 0};
	ASSERT_EQ(16, write(sv[0], big, 16));
	EXPECT_EQ(-1, capn_rpc_fd_read_frame(sv[1], &f, &n));
	close(sv[0]);
	close(sv[1]);
}
