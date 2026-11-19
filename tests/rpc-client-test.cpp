/* rpc-client-test.cpp
 *
 * The client half of level 1: asking questions rather than only
 * answering them, and the `-> stream` flow-control window.
 *
 * Two vats share a pair of outboxes, so every message is a real frame
 * and the two are pumped in lockstep.
 */

#include <gtest/gtest.h>

#include <climits>
#include <cstring>
#include <string>
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

/* rpc.c++ handleCall: KJ_REQUIRE(!answer.active, "questionId is already in use"). */
const char kLiveQuestionReason[] =
	"expected !answer.active; questionId is already in use";

std::string frame_abort_reason(const std::vector<uint8_t> &f)
{
	struct capn m;
	if (capn_init_mem(&m, f.data(), f.size(), 0) != 0)
		return "";
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&m), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	if (msg.which != Message_abort || msg.abort.p.type == CAPN_NULL) {
		capn_free(&m);
		return "";
	}
	struct Exception ex;
	memset(&ex, 0, sizeof ex);
	read_Exception(&ex, msg.abort);
	std::string reason;
	if (ex.reason.str != NULL && ex.reason.len > 0)
		reason.assign(ex.reason.str, static_cast<size_t>(ex.reason.len));
	capn_free(&m);
	return reason;
}

bool wire_has_abort_reason(const std::deque<std::vector<uint8_t> > &q, const char *reason)
{
	for (size_t i = 0; i < q.size(); i++) {
		if (frame_abort_reason(q[i]) == reason)
			return true;
	}
	return false;
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

int promise_hits;
int settled_hits;
int g_seen_import;
uint32_t g_remote_question;
int promise_obj;
struct capn_rpc_conn *g_vat;
struct capn_rpc_conn *g_client;
int g_param_is_export;
int g_param_id;
int g_returned_export;
int g_return_caps;
int g_other_obj;

int settled_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr results)
{
	settled_hits++;
	capn_resolve(&params);
	uint32_t in = params.type == CAPN_STRUCT ? capn_read32(params, 0) : 0;
	capn_write32(results, 0, in + 5);
	return 0;
}

int promise_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr results)
{
	promise_hits++;
	capn_resolve(&params);
	uint32_t in = params.type == CAPN_STRUCT ? capn_read32(params, 0) : 0;
	capn_write32(results, 0, in + 1);
	return 0;
}

/* method 0 records a callback and calls it. method 1 returns a promise.
 * method 2 returns that call as a receiverAnswer. */
int promise_host_dispatch(void *, uint64_t, uint16_t method, capn_ptr params,
                          capn_ptr results)
{
	if (method == 0) {
		capn_resolve(&params);
		capn_ptr cap = capn_getp(params, 0, 1);
		int is_export = -1;
		g_seen_import = capn_rpc_param_cap(g_vat, cap, &is_export);
		if (g_seen_import < 0 || is_export != 0)
			return -1;
		uint32_t arg = 4;
		g_remote_question = capn_rpc_send_call(g_vat, static_cast<uint32_t>(g_seen_import),
		                                       0, 0, 8, 0, fill_u32, &arg);
		return g_remote_question == static_cast<uint32_t>(-1) ? -1 : 0;
	}
	if (method == 1) {
		capn_ptr cap = capn_rpc_new_promise_cap(g_vat, results.seg, &promise_obj,
		                                        settled_dispatch);
		if (cap.type != CAPN_INTERFACE)
			return -1;
		return capn_setp(results, 0, cap);
	}
	if (method == 2) {
		uint16_t path = 0;
		capn_ptr cap = capn_rpc_new_answer_cap(g_vat, results.seg, g_remote_question,
		                                       &path, 1);
		if (cap.type != CAPN_INTERFACE)
			return -1;
		return capn_setp(results, 0, cap);
	}
	return -1;
}

int frame_which(const std::vector<uint8_t> &f)
{
	struct capn m;
	if (capn_init_mem(&m, f.data(), f.size(), 0) != 0)
		return -1;
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&m), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	int w = msg.which;
	capn_free(&m);
	return w;
}

uint32_t frame_call_target(const std::vector<uint8_t> &f)
{
	struct capn m;
	if (capn_init_mem(&m, f.data(), f.size(), 0) != 0)
		return static_cast<uint32_t>(-1);
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&m), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	struct Call call;
	read_Call(&call, msg.call);
	struct MessageTarget t;
	read_MessageTarget(&t, call.target);
	uint32_t id = t.importedCap;
	capn_free(&m);
	return id;
}

/* Capabilities in payloads. The server hands out `doubler` objects and
 * accepts a callback capability it then calls back. */
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

/* Records the first capability parameter. A reflected or embargoed call
 * must present it as an export of this vat, which is the swap of the
 * senderHosted descriptor the caller wrote. */
int record_cap_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr results)
{
	capn_resolve(&params);
	capn_ptr cap = capn_getp(params, 0, 1);
	capn_resolve(&cap);
	g_callback_hits++;
	if (g_return_caps && results.ptrs >= 1) {
		capn_ptr out = capn_rpc_new_export_cap(g_client, results.seg,
		                                      &g_callback_hits, counter_dispatch);
		struct capn_rpc_pending_cap *built = g_client->pending_caps;
		int nbuilt = g_client->npending;
		if (g_client->cap_dest != NULL && g_client->cap_dest_n != NULL) {
			built = g_client->cap_dest;
			nbuilt = *g_client->cap_dest_n;
		}
		if (out.type != CAPN_INTERFACE || nbuilt < 1)
			return -1;
		g_returned_export = static_cast<int>(built[nbuilt - 1].id);
		return capn_setp(results, 0, out);
	}
	if (cap.type != CAPN_INTERFACE) {
		g_param_id = -1;
		g_param_is_export = -1;
		return 0;
	}
	g_param_is_export = -1;
	g_param_id = capn_rpc_param_cap(g_client, cap, &g_param_is_export);
	return g_param_id < 0 ? -1 : 0;
}

void fill_record_cap(void *ctx, capn_ptr params)
{
	struct capn_rpc_conn *c = static_cast<struct capn_rpc_conn *>(ctx);
	capn_ptr cap = capn_rpc_new_export_cap(c, params.seg, &g_callback_hits, record_cap_dispatch);
	capn_setp(params, 0, cap);
}

/* A different object from the callback the call is reflected to, so the
 * cap-table index cannot be confused with that callback's own export. */
void fill_other_cap(void *ctx, capn_ptr params)
{
	struct capn_rpc_conn *c = static_cast<struct capn_rpc_conn *>(ctx);
	capn_ptr cap = capn_rpc_new_export_cap(c, params.seg, &g_other_obj, counter_dispatch);
	capn_setp(params, 0, cap);
}

int frame_call_param_cap(const std::vector<uint8_t> &f, int *which, uint32_t *id)
{
	struct capn m;
	if (capn_init_mem(&m, f.data(), f.size(), 0) != 0)
		return -1;
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&m), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	if (msg.which != Message_call) {
		capn_free(&m);
		return -1;
	}
	struct Call call;
	read_Call(&call, msg.call);
	struct Payload pl;
	memset(&pl, 0, sizeof pl);
	if (call.params.p.type != CAPN_NULL)
		read_Payload(&pl, call.params);
	capn_ptr cap = capn_getp(pl.content, 0, 1);
	capn_resolve(&cap);
	if (cap.type != CAPN_INTERFACE || capn_len(pl.capTable) < 0 ||
	    cap.cap_idx > (uint32_t) INT_MAX ||
	    (int) cap.cap_idx >= capn_len(pl.capTable)) {
		capn_free(&m);
		return -1;
	}
	struct CapDescriptor cd;
	get_CapDescriptor(&cd, pl.capTable, (int) cap.cap_idx);
	*which = cd.which;
	if (cd.which == CapDescriptor_receiverHosted)
		*id = cd.receiverHosted;
	else if (cd.which == CapDescriptor_senderHosted)
		*id = cd.senderHosted;
	else if (cd.which == CapDescriptor_senderPromise)
		*id = cd.senderPromise;
	else
		*id = 0;
	capn_free(&m);
	return 0;
}

std::vector<uint8_t> disembargo_frame(uint32_t export_id, uint32_t embargo)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct Message m;
	struct Disembargo d;
	struct MessageTarget t;
	memset(&m, 0, sizeof m);
	memset(&d, 0, sizeof d);
	memset(&t, 0, sizeof t);
	t.which = MessageTarget_importedCap;
	t.importedCap = export_id;
	MessageTarget_ptr tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);
	d.target = tp;
	d.context_which = Disembargo_context_senderLoopback;
	d.context.senderLoopback = embargo;
	Disembargo_ptr dp = new_Disembargo(cs);
	write_Disembargo(&d, dp);
	m.which = Message_disembargo;
	m.disembargo = dp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	int64_t need = capn_size(&msg);
	std::vector<uint8_t> buf(static_cast<size_t>(need > 0 ? need : 0));
	if (need > 0)
		capn_write_mem(&msg, buf.data(), buf.size(), 0);
	capn_free(&msg);
	return buf;
}

/* A server that answers later: it parks the call and returns PENDING. */
struct Parked {
	uint32_t question;
	uint32_t generation;
	capn_ptr results;
	uint32_t arg;
	int calls;
};
Parked g_parked;

/* Distinct object so its export refcount is not the bootstrap's. */
int g_held_obj;

int parking_with_cap(void *, uint64_t, uint16_t, capn_ptr, capn_ptr results)
{
	capn_ptr cap = capn_rpc_new_export_cap(g_vat, results.seg, &g_held_obj,
	                                       doubler_dispatch);
	if (cap.type != CAPN_INTERFACE)
		return -1;
	if (capn_setp(results, 0, cap) != 0)
		return -1;
	g_parked.question = capn_rpc_current_question(g_vat);
	g_parked.generation = capn_rpc_current_generation(g_vat);
	g_parked.results = results;
	g_parked.calls++;
	return CAPN_RPC_PENDING;
}

int export_refcount(struct capn_rpc_conn *c, void *server)
{
	int i;
	for (i = 0; i < c->exports_cap; i++) {
		if (c->exports[i].used && c->exports[i].server == server)
			return c->exports[i].refcount;
	}
	return 0;
}

int parking_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr results)
{
	capn_resolve(&params);
	g_parked.question = capn_rpc_current_question(g_vat);
	g_parked.generation = capn_rpc_current_generation(g_vat);
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
		promise_hits = 0;
		settled_hits = 0;
		g_seen_import = -1;
		g_remote_question = static_cast<uint32_t>(-1);
		g_client = NULL;
		g_param_is_export = -1;
		g_param_id = -1;
		g_returned_export = -1;
		g_return_caps = 0;
		g_other_obj = 0;
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
	/* obsoleteSave is not implemented. The reply's payload is that
	 * message, not an Unimplemented wrapped around another. */
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_obsoleteSave;
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
	EXPECT_EQ(Message_obsoleteSave, echoed.which);
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

TEST_F(RpcClient, RpcTablesStopAtTheDocumentedLimit)
{
	std::vector<int> objects(CAPN_RPC_MAX_TABLE_CAPACITY + 1);
	int i;

	for (i = 0; i < CAPN_RPC_MAX_TABLE_CAPACITY; i++)
		ASSERT_EQ(i, capn_rpc_export(&vat, &objects[i], doubler_dispatch)) << i;
	EXPECT_LT(capn_rpc_export(&vat, &objects[CAPN_RPC_MAX_TABLE_CAPACITY],
	                          doubler_dispatch),
	          0);
	EXPECT_EQ(CAPN_RPC_MAX_TABLE_CAPACITY, vat.exports_cap);
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
	ASSERT_EQ(0u, cap.cap_idx);
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
	ASSERT_EQ(0, capn_rpc_return(&vat, g_parked.question, g_parked.generation));
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	EXPECT_EQ(27u, capn_read32(content, 0));
	capn_free(&m);

	/* Answering twice is refused. */
	EXPECT_EQ(-1, capn_rpc_return(&vat, g_parked.question, g_parked.generation));
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
	ASSERT_EQ(0, capn_rpc_return_exception(&vat, g_parked.question, g_parked.generation, "gave up"));
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
	EXPECT_EQ(-1, capn_rpc_return(&vat, g_parked.question, g_parked.generation));
	EXPECT_EQ(before, wire.to_client.size());
}

TEST_F(RpcClient, FillAfterFinishDoesNotUseFreedResults)
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

	/* The results struct is still live, and the write must not be a
	 * use after free. The return sends nothing. */
	capn_write32(g_parked.results, 0, 123);
	size_t before = wire.to_client.size();
	EXPECT_EQ(-1, capn_rpc_return(&vat, g_parked.question, g_parked.generation));
	EXPECT_EQ(before, wire.to_client.size());
	EXPECT_FALSE(capn_rpc_is_answered(&client, q));
}

TEST_F(RpcClient, ReusedQuestionIdDoesNotCompleteTheWrongCall)
{
	g_vat = &vat;
	g_parked = Parked();
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t arg = 1;
	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, fill_u32, &arg);
	settle();
	uint32_t gen1 = g_parked.generation;
	/* The id stays live until the cancelled call is returned. Finish
	 * alone does not free it. */
	capn_rpc_send_finish(&client, q);
	settle();
	EXPECT_EQ(-1, capn_rpc_return(&vat, q, gen1));
	EXPECT_FALSE(capn_rpc_is_aborted(&vat));

	client.next_question_id = q;
	arg = 4;
	uint32_t q2 = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, fill_u32, &arg);
	settle();
	ASSERT_EQ(q, q2);
	uint32_t gen2 = g_parked.generation;
	capn_ptr new_results = g_parked.results;
	EXPECT_NE(gen1, gen2);
	EXPECT_FALSE(capn_rpc_is_aborted(&vat));

	/* The finished call's generation must not send the new call's results. */
	EXPECT_EQ(-1, capn_rpc_return(&vat, q, gen1));
	EXPECT_FALSE(capn_rpc_is_answered(&client, q2));

	capn_write32(new_results, 0, 222);
	ASSERT_EQ(0, capn_rpc_return(&vat, q, gen2));
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q2));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q2));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q2, &m, &content));
	EXPECT_EQ(222u, capn_read32(content, 0));
	capn_free(&m);
}

TEST_F(RpcClient, ReusedLiveQuestionIdAbortsTheConnection)
{
	g_vat = &vat;
	g_parked = Parked();
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, NULL, NULL);
	settle();
	ASSERT_EQ(1, g_parked.calls);

	/* The answer is still running. Reusing its questionId is a protocol
	 * error even after Finish, until the server has returned. */
	capn_rpc_send_finish(&client, q);
	settle();
	client.next_question_id = q;
	uint32_t q2 = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, NULL, NULL);
	ASSERT_EQ(q, q2);
	pump(&vat, wire.to_vat);
	EXPECT_TRUE(wire_has_abort_reason(wire.to_client, kLiveQuestionReason));
	pump(&client, wire.to_client);
	EXPECT_TRUE(capn_rpc_is_aborted(&vat));

	/* An answer that has been sent and not finished is live too. */
	struct capn_rpc_conn client2, vat2;
	capn_rpc_init(&client2, send_to_vat, NULL);
	capn_rpc_init(&vat2, send_to_client, NULL);
	capn_rpc_set_bootstrap(&vat2, &doubler_calls, doubler_dispatch);
	wire = Wire();
	g_wire = &wire;
	uint32_t b = capn_rpc_send_bootstrap(&client2);
	pump(&vat2, wire.to_vat);
	pump(&client2, wire.to_client);
	ASSERT_TRUE(capn_rpc_is_answered(&client2, b));
	client2.next_question_id = b;
	capn_rpc_send_bootstrap(&client2);
	pump(&vat2, wire.to_vat);
	EXPECT_TRUE(wire_has_abort_reason(wire.to_client, kLiveQuestionReason));
	EXPECT_TRUE(capn_rpc_is_aborted(&vat2));
	capn_rpc_free(&client2);
	capn_rpc_free(&vat2);
}

TEST_F(RpcClient, QuestionIdMayBeReusedAfterTheCallIsFullyDone)
{
	g_vat = &vat;
	g_parked = Parked();
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t arg = 1;
	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, fill_u32, &arg);
	settle();
	uint32_t gen1 = g_parked.generation;
	capn_rpc_send_finish(&client, q);
	settle();
	EXPECT_EQ(-1, capn_rpc_return(&vat, q, gen1));
	EXPECT_FALSE(capn_rpc_is_aborted(&vat));

	client.next_question_id = q;
	arg = 4;
	uint32_t q2 = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, fill_u32, &arg);
	settle();
	ASSERT_EQ(q, q2);
	EXPECT_FALSE(capn_rpc_is_aborted(&vat));
	EXPECT_NE(gen1, g_parked.generation);
	capn_write32(g_parked.results, 0, 222);
	ASSERT_EQ(0, capn_rpc_return(&vat, q, g_parked.generation));
	settle();
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q2, &m, &content));
	EXPECT_EQ(222u, capn_read32(content, 0));
	capn_free(&m);
}

TEST_F(RpcClient, DeferredReturnKeepsCapsAcrossAnotherCall)
{
	g_vat = &vat;
	g_parked = Parked();
	g_held_obj = 0;
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_with_cap);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q1 = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, NULL, NULL);
	settle();
	ASSERT_EQ(1, g_parked.calls);
	uint32_t gen1 = g_parked.generation;
	ASSERT_GT(export_refcount(&vat, &g_held_obj), 0);

	/* The second call must not wipe the cap the first one already added. */
	uint32_t q2 = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, NULL, NULL);
	settle();
	ASSERT_EQ(2, g_parked.calls);
	uint32_t q2_id = g_parked.question;
	uint32_t gen2 = g_parked.generation;
	EXPECT_NE(q1, q2);

	ASSERT_EQ(0, capn_rpc_return(&vat, q1, gen1));
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q1));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q1));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q1, &m, &content));
	capn_ptr cap = capn_getp(content, 0, 1);
	ASSERT_EQ(CAPN_INTERFACE, cap.type);
	int is_export = 1;
	int id = capn_rpc_answer_cap(&client, q1, cap, &is_export);
	EXPECT_GE(id, 0);
	EXPECT_EQ(0, is_export);
	capn_free(&m);

	ASSERT_EQ(0, capn_rpc_return(&vat, q2_id, gen2));
	settle();
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q2, &m, &content));
	cap = capn_getp(content, 0, 1);
	ASSERT_EQ(CAPN_INTERFACE, cap.type);
	capn_free(&m);
}

TEST_F(RpcClient, CancelledDeferredCallDropsTheUnsentExport)
{
	g_vat = &vat;
	g_parked = Parked();
	g_held_obj = 0;
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_with_cap);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, NULL, NULL);
	settle();
	ASSERT_EQ(1, g_parked.calls);
	EXPECT_EQ(1, export_refcount(&vat, &g_held_obj));

	capn_rpc_send_finish(&client, q);
	settle();
	EXPECT_EQ(-1, capn_rpc_return(&vat, g_parked.question, g_parked.generation));
	EXPECT_EQ(0, export_refcount(&vat, &g_held_obj));
}

/* A cap created after dispatch returns PENDING has to travel in the
 * later Return. Creating it only while dispatch runs cannot see a list
 * that was left on the connection and then abandoned. */
TEST_F(RpcClient, DeferredCapCreatedAfterDispatchIsReturned)
{
	g_vat = &vat;
	g_parked = Parked();
	g_held_obj = 0;
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, NULL, NULL);
	settle();
	ASSERT_EQ(1, g_parked.calls);
	EXPECT_EQ(0, export_refcount(&vat, &g_held_obj));

	capn_ptr cap = capn_rpc_new_export_cap(&vat, g_parked.results.seg, &g_held_obj,
	                                      doubler_dispatch);
	ASSERT_EQ(CAPN_INTERFACE, cap.type);
	ASSERT_EQ(0, capn_setp(g_parked.results, 0, cap));
	EXPECT_EQ(1, export_refcount(&vat, &g_held_obj));
	EXPECT_EQ(0, vat.npending);

	ASSERT_EQ(0, capn_rpc_return(&vat, g_parked.question, g_parked.generation));
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	capn_ptr got = capn_getp(content, 0, 1);
	ASSERT_EQ(CAPN_INTERFACE, got.type);
	int is_export = 1;
	int id = capn_rpc_answer_cap(&client, q, got, &is_export);
	EXPECT_GE(id, 0);
	EXPECT_EQ(0, is_export);
	capn_free(&m);
	EXPECT_EQ(0, vat.npending);
	EXPECT_EQ(1, export_refcount(&vat, &g_held_obj));

	/* The peer never saw the cap, so Finish of a second parked call that
	 * added one and then cancelled must drop the export. */
	g_parked.calls = 0;
	uint32_t q2 = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, NULL, NULL);
	settle();
	ASSERT_EQ(1, g_parked.calls);
	cap = capn_rpc_new_export_cap(&vat, g_parked.results.seg, &g_held_obj,
	                             doubler_dispatch);
	ASSERT_EQ(CAPN_INTERFACE, cap.type);
	ASSERT_EQ(0, capn_setp(g_parked.results, 0, cap));
	EXPECT_EQ(2, export_refcount(&vat, &g_held_obj));
	capn_rpc_send_finish(&client, q2);
	settle();
	EXPECT_EQ(-1, capn_rpc_return(&vat, g_parked.question, g_parked.generation));
	EXPECT_EQ(1, export_refcount(&vat, &g_held_obj));
	EXPECT_EQ(0, vat.npending);
}

TEST_F(RpcClient, ManyDeferredCapsAfterDispatchDoNotExhaustThePool)
{
	g_vat = &vat;
	g_parked = Parked();
	g_held_obj = 0;
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_send_bootstrap(&client);
	settle();

	/* More than CAPN_RPC_MAX_PENDING_CAPS (16). A connection-global
	 * counter that is never cleared returns a null cap after that. */
	for (int i = 0; i < 24; i++) {
		uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 0, 1, NULL, NULL);
		settle();
		ASSERT_EQ(i + 1, g_parked.calls) << i;
		capn_ptr cap = capn_rpc_new_export_cap(&vat, g_parked.results.seg,
		                                      &g_held_obj, doubler_dispatch);
		ASSERT_EQ(CAPN_INTERFACE, cap.type) << i;
		ASSERT_EQ(0, capn_setp(g_parked.results, 0, cap)) << i;
		ASSERT_EQ(0, capn_rpc_return(&vat, g_parked.question, g_parked.generation)) << i;
		settle();
		ASSERT_TRUE(capn_rpc_is_answered(&client, q)) << i;
		ASSERT_FALSE(capn_rpc_is_failed(&client, q)) << i;
		struct capn m;
		capn_ptr content;
		ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content)) << i;
		capn_ptr got = capn_getp(content, 0, 1);
		ASSERT_EQ(CAPN_INTERFACE, got.type) << i;
		int is_export = 1;
		int id = capn_rpc_answer_cap(&client, q, got, &is_export);
		EXPECT_GE(id, 0) << i;
		EXPECT_EQ(0, is_export) << i;
		capn_free(&m);
		EXPECT_EQ(0, vat.npending) << i;
		capn_rpc_send_finish(&client, q);
		settle();
	}
	EXPECT_EQ(24, export_refcount(&vat, &g_held_obj));
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

TEST_F(RpcClient, QueuedPromiseCallIsDeliveredBeforeResolve)
{
	int obj = 0;
	int settled = 0;
	ASSERT_EQ(0, capn_rpc_export_promise(&vat, &obj, promise_dispatch));
	uint32_t arg = 3;
	uint32_t q = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, fill_u32, &arg);
	pump(&vat, wire.to_vat);
	EXPECT_TRUE(wire.to_client.empty());
	EXPECT_EQ(0, promise_hits);

	ASSERT_EQ(0, capn_rpc_resolve(&vat, 0, &settled, settled_dispatch));
	ASSERT_EQ(2u, wire.to_client.size());
	EXPECT_EQ(Message__return, frame_which(wire.to_client[0]));
	EXPECT_EQ(Message_resolve, frame_which(wire.to_client[1]));
	EXPECT_EQ(0, promise_hits);
	EXPECT_EQ(1, settled_hits);

	pump(&client, wire.to_client);
	EXPECT_TRUE(capn_rpc_is_answered(&client, q));
	EXPECT_FALSE(capn_rpc_is_failed(&client, q));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	EXPECT_EQ(8u, capn_read32(content, 0));
	capn_free(&m);

	int broken = 1, to_export = -1;
	uint32_t target = 0;
	EXPECT_EQ(1, capn_rpc_promise_state(&client, 0, &broken, &to_export, &target));
	EXPECT_EQ(0, broken);
	EXPECT_EQ(0, to_export);
	EXPECT_EQ(1u, target);

	arg = 4;
	uint32_t q2 = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, fill_u32, &arg);
	ASSERT_EQ(1u, wire.to_vat.size());
	EXPECT_EQ(1u, frame_call_target(wire.to_vat[0]));
	pump(&vat, wire.to_vat);
	pump(&client, wire.to_client);
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q2, &m, &content));
	EXPECT_EQ(9u, capn_read32(content, 0));
	capn_free(&m);
	EXPECT_EQ(2, settled_hits);
}

TEST_F(RpcClient, ResolvingAPromiseInPlaceDispatchesTheSameExport)
{
	int obj = 0;
	ASSERT_EQ(0, capn_rpc_export_promise(&vat, &obj, promise_dispatch));
	uint32_t arg = 10;
	uint32_t q = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, fill_u32, &arg);
	pump(&vat, wire.to_vat);
	ASSERT_EQ(0, capn_rpc_resolve(&vat, 0, &obj, promise_dispatch));
	EXPECT_EQ(1, promise_hits);
	ASSERT_EQ(Message__return, frame_which(wire.to_client[0]));
	EXPECT_EQ(Message_resolve, frame_which(wire.to_client[1]));
	pump(&client, wire.to_client);
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	EXPECT_EQ(11u, capn_read32(content, 0));
	capn_free(&m);
	int to_export = -1;
	uint32_t target = 9;
	EXPECT_EQ(1, capn_rpc_promise_state(&client, 0, NULL, &to_export, &target));
	EXPECT_EQ(0u, target);
}

TEST_F(RpcClient, RejectedPromiseFailsQueuedAndLaterCalls)
{
	int obj = 0;
	ASSERT_EQ(0, capn_rpc_export_promise(&vat, &obj, promise_dispatch));
	uint32_t arg = 1;
	uint32_t q = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, fill_u32, &arg);
	pump(&vat, wire.to_vat);
	ASSERT_EQ(0, capn_rpc_resolve_exception(&vat, 0, "no such service"));
	ASSERT_EQ(2u, wire.to_client.size());
	EXPECT_EQ(Message__return, frame_which(wire.to_client[0]));
	EXPECT_EQ(Message_resolve, frame_which(wire.to_client[1]));
	pump(&client, wire.to_client);
	EXPECT_TRUE(capn_rpc_is_failed(&client, q));
	int broken = 0;
	EXPECT_EQ(1, capn_rpc_promise_state(&client, 0, &broken, NULL, NULL));
	EXPECT_EQ(1, broken);

	size_t sent = wire.to_vat.size();
	uint32_t q2 = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, fill_u32, &arg);
	EXPECT_TRUE(capn_rpc_is_failed(&client, q2));
	EXPECT_EQ(sent, wire.to_vat.size());
}

TEST_F(RpcClient, ReflectedPromiseWaitsForTheDisembargoEcho)
{
	g_vat = &vat;
	g_callback_hits = 0;
	capn_rpc_set_bootstrap(&vat, &promise_obj, promise_host_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t qreg = capn_rpc_send_call(&client, 0, 0, 0, 0, 1, fill_with_callback, &client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, qreg));
	ASSERT_GE(g_seen_import, 0);

	capn_rpc_send_call(&client, 0, 0, 1, 0, 1, NULL, NULL);
	pump(&vat, wire.to_vat);
	ASSERT_FALSE(wire.to_client.empty());
	struct capn desc;
	ASSERT_EQ(0, capn_init_mem(&desc, wire.to_client[0].data(), wire.to_client[0].size(), 0));
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&desc), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	struct Return ret;
	read_Return(&ret, msg._return);
	struct Payload pl;
	read_Payload(&pl, ret.results);
	capn_ptr cap = capn_getp(pl.content, 0, 1);
	struct CapDescriptor cd;
	get_CapDescriptor(&cd, pl.capTable, (int) cap.cap_idx);
	EXPECT_EQ(CapDescriptor_senderPromise, cd.which);
	uint32_t promise_id = cd.senderPromise;
	capn_free(&desc);
	pump(&client, wire.to_client);

	uint32_t arg = 9;
	uint32_t qcall = capn_rpc_send_call(&client, promise_id, 0, 0, 8, 0, fill_u32, &arg);
	pump(&vat, wire.to_vat);
	EXPECT_TRUE(wire.to_client.empty());

	ASSERT_EQ(0, capn_rpc_resolve_import(&vat, static_cast<int>(promise_id),
	                                    static_cast<uint32_t>(g_seen_import)));
	ASSERT_EQ(2u, wire.to_client.size());
	EXPECT_EQ(Message_call, frame_which(wire.to_client[0]));
	EXPECT_EQ(Message_resolve, frame_which(wire.to_client[1]));
	EXPECT_EQ(static_cast<uint32_t>(g_seen_import), frame_call_target(wire.to_client[0]));

	pump(&client, wire.to_client);
	EXPECT_FALSE(capn_rpc_is_answered(&client, qcall));
	int disembargoes = 0;
	int calls = 0;
	for (size_t i = 0; i < wire.to_vat.size(); i++) {
		int w = frame_which(wire.to_vat[i]);
		if (w == Message_disembargo)
			disembargoes++;
		if (w == Message_call)
			calls++;
	}
	EXPECT_EQ(1, disembargoes);
	EXPECT_EQ(0, calls);

	arg = 1;
	uint32_t qheld = capn_rpc_send_call(&client, promise_id, 0, 0, 8, 0, fill_u32, &arg);
	EXPECT_FALSE(capn_rpc_is_answered(&client, qheld));
	disembargoes = 0;
	calls = 0;
	for (size_t i = 0; i < wire.to_vat.size(); i++) {
		int w = frame_which(wire.to_vat[i]);
		if (w == Message_disembargo)
			disembargoes++;
		if (w == Message_call)
			calls++;
	}
	EXPECT_EQ(1, disembargoes);
	EXPECT_EQ(0, calls);

	pump(&vat, wire.to_vat);
	pump(&client, wire.to_client);
	EXPECT_TRUE(capn_rpc_is_answered(&client, qcall));
	EXPECT_FALSE(capn_rpc_is_failed(&client, qcall));
	EXPECT_TRUE(capn_rpc_is_answered(&client, qheld));
	EXPECT_FALSE(capn_rpc_is_failed(&client, qheld));
	int broken = 1, to_export = 0;
	uint32_t target = 0;
	EXPECT_EQ(1, capn_rpc_promise_state(&client, promise_id, &broken, &to_export, &target));
	EXPECT_EQ(0, broken);
	EXPECT_EQ(1, to_export);
	EXPECT_EQ(static_cast<uint32_t>(g_seen_import), target);
	/* Register called the callback once, the forwarded call once, and the
	 * call that waited on the echo once. */
	EXPECT_EQ(3, g_callback_hits);
}

TEST_F(RpcClient, ReturnedAnswerCapNamesTheOutstandingQuestion)
{
	g_vat = &vat;
	g_callback_hits = 0;
	capn_rpc_set_bootstrap(&vat, &promise_obj, promise_host_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();
	uint32_t qreg = capn_rpc_send_call(&client, 0, 0, 0, 0, 1, fill_with_callback, &client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, qreg));
	ASSERT_NE(static_cast<uint32_t>(-1), g_remote_question);

	uint32_t q = capn_rpc_send_call(&client, 0, 0, 2, 0, 1, NULL, NULL);
	pump(&vat, wire.to_vat);
	ASSERT_FALSE(wire.to_client.empty());
	struct capn desc;
	ASSERT_EQ(0, capn_init_mem(&desc, wire.to_client[0].data(), wire.to_client[0].size(), 0));
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&desc), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	struct Return ret;
	read_Return(&ret, msg._return);
	struct Payload pl;
	read_Payload(&pl, ret.results);
	capn_ptr cap = capn_getp(pl.content, 0, 1);
	ASSERT_EQ(CAPN_INTERFACE, cap.type);
	struct CapDescriptor cd;
	get_CapDescriptor(&cd, pl.capTable, (int) cap.cap_idx);
	EXPECT_EQ(CapDescriptor_receiverAnswer, cd.which);
	struct PromisedAnswer pa;
	read_PromisedAnswer(&pa, cd.receiverAnswer);
	EXPECT_EQ(g_remote_question, pa.questionId);
	EXPECT_EQ(1, capn_len(pa.transform));
	capn_free(&desc);

	pump(&client, wire.to_client);
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q, &m, &content));
	cap = capn_getp(content, 0, 1);
	int is_export = -1;
	int id = capn_rpc_answer_cap(&client, q, cap, &is_export);
	EXPECT_EQ(2, is_export);
	EXPECT_EQ(static_cast<int>(g_remote_question), id);
	capn_free(&m);
}

TEST_F(RpcClient, ReflectedCallKeepsTheCapTable)
{
	g_vat = &vat;
	g_client = &client;
	g_callback_hits = 0;
	capn_rpc_set_bootstrap(&vat, &promise_obj, promise_host_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t qreg = capn_rpc_send_call(&client, 0, 0, 0, 0, 1, fill_record_cap, &client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, qreg));
	ASSERT_GE(g_seen_import, 0);
	g_callback_hits = 0;
	g_param_id = -1;

	capn_rpc_send_call(&client, 0, 0, 1, 0, 1, NULL, NULL);
	pump(&vat, wire.to_vat);
	ASSERT_FALSE(wire.to_client.empty());
	struct capn desc;
	ASSERT_EQ(0, capn_init_mem(&desc, wire.to_client[0].data(), wire.to_client[0].size(), 0));
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&desc), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	struct Return ret;
	read_Return(&ret, msg._return);
	struct Payload pl;
	read_Payload(&pl, ret.results);
	capn_ptr cap = capn_getp(pl.content, 0, 1);
	struct CapDescriptor cd;
	get_CapDescriptor(&cd, pl.capTable, (int) cap.cap_idx);
	ASSERT_EQ(CapDescriptor_senderPromise, cd.which);
	uint32_t promise_id = cd.senderPromise;
	capn_free(&desc);
	pump(&client, wire.to_client);

	/* A second export rides in the params. Reflecting the call has to
	 * turn that senderHosted entry into receiverHosted of the same id. */
	uint32_t qcall = capn_rpc_send_call(&client, promise_id, 0, 0, 0, 1,
	                                   fill_other_cap, &client);
	ASSERT_NE((uint32_t)-1, qcall);
	pump(&vat, wire.to_vat);
	EXPECT_TRUE(wire.to_client.empty());

	ASSERT_EQ(0, capn_rpc_resolve_import(&vat, static_cast<int>(promise_id),
	                                    static_cast<uint32_t>(g_seen_import)));
	ASSERT_FALSE(wire.to_client.empty());
	EXPECT_EQ(Message_call, frame_which(wire.to_client[0]));
	int which = -1;
	uint32_t id = 0;
	ASSERT_EQ(0, frame_call_param_cap(wire.to_client[0], &which, &id));
	EXPECT_EQ(CapDescriptor_receiverHosted, which);
	EXPECT_EQ(1u, id);

	pump(&client, wire.to_client);
	EXPECT_EQ(1, g_param_is_export);
	EXPECT_EQ(1, g_param_id);
	EXPECT_FALSE(capn_rpc_is_failed(&client, qcall));
}

TEST_F(RpcClient, RelayedReturnSwapsHostedFields)
{
	g_vat = &vat;
	g_client = &client;
	g_callback_hits = 0;
	capn_rpc_set_bootstrap(&vat, &promise_obj, promise_host_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t qreg = capn_rpc_send_call(&client, 0, 0, 0, 0, 1, fill_record_cap, &client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, qreg));
	ASSERT_GE(g_seen_import, 0);

	capn_rpc_send_call(&client, 0, 0, 1, 0, 1, NULL, NULL);
	pump(&vat, wire.to_vat);
	struct capn desc;
	ASSERT_EQ(0, capn_init_mem(&desc, wire.to_client[0].data(), wire.to_client[0].size(), 0));
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&desc), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	struct Return ret;
	read_Return(&ret, msg._return);
	struct Payload pl;
	read_Payload(&pl, ret.results);
	capn_ptr cap = capn_getp(pl.content, 0, 1);
	struct CapDescriptor cd;
	get_CapDescriptor(&cd, pl.capTable, (int) cap.cap_idx);
	uint32_t promise_id = cd.senderPromise;
	capn_free(&desc);
	pump(&client, wire.to_client);

	g_return_caps = 1;
	g_returned_export = -1;
	uint32_t qcall = capn_rpc_send_call(&client, promise_id, 0, 0, 0, 0, NULL, NULL);
	pump(&vat, wire.to_vat);
	ASSERT_EQ(0, capn_rpc_resolve_import(&vat, static_cast<int>(promise_id),
	                                    static_cast<uint32_t>(g_seen_import)));
	/* Call and Resolve out, the reflected Return back, then the relay. */
	pump(&client, wire.to_client);
	pump(&vat, wire.to_vat);
	pump(&client, wire.to_client);

	ASSERT_TRUE(capn_rpc_is_answered(&client, qcall));
	ASSERT_FALSE(capn_rpc_is_failed(&client, qcall));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, qcall, &m, &content));
	cap = capn_getp(content, 0, 1);
	int is_export = -1;
	int id = capn_rpc_answer_cap(&client, qcall, cap, &is_export);
	EXPECT_EQ(1, is_export);
	EXPECT_EQ(g_returned_export, id);
	EXPECT_GE(g_returned_export, 0);
	capn_free(&m);
}

TEST_F(RpcClient, HeldCallKeepsCapsUntilTheEmbargoLifts)
{
	g_vat = &vat;
	g_client = &client;
	g_callback_hits = 0;
	capn_rpc_set_bootstrap(&vat, &promise_obj, promise_host_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	ASSERT_NE((uint32_t)-1,
	          capn_rpc_send_call(&client, 0, 0, 0, 0, 1, fill_record_cap, &client));
	settle();
	ASSERT_GE(g_seen_import, 0);

	capn_rpc_send_call(&client, 0, 0, 1, 0, 1, NULL, NULL);
	pump(&vat, wire.to_vat);
	struct capn desc;
	ASSERT_EQ(0, capn_init_mem(&desc, wire.to_client[0].data(), wire.to_client[0].size(), 0));
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&desc), 0, 1);
	struct Message msg;
	read_Message(&msg, mp);
	struct Return ret;
	read_Return(&ret, msg._return);
	struct Payload pl;
	read_Payload(&pl, ret.results);
	capn_ptr cap = capn_getp(pl.content, 0, 1);
	struct CapDescriptor cd;
	get_CapDescriptor(&cd, pl.capTable, (int) cap.cap_idx);
	uint32_t promise_id = cd.senderPromise;
	capn_free(&desc);
	pump(&client, wire.to_client);

	uint32_t arg = 9;
	uint32_t qcall = capn_rpc_send_call(&client, promise_id, 0, 0, 8, 0, fill_u32, &arg);
	pump(&vat, wire.to_vat);
	ASSERT_EQ(0, capn_rpc_resolve_import(&vat, static_cast<int>(promise_id),
	                                    static_cast<uint32_t>(g_seen_import)));
	pump(&client, wire.to_client);
	EXPECT_FALSE(capn_rpc_is_answered(&client, qcall));

	g_param_id = -1;
	g_param_is_export = -1;
	int hits_before = g_callback_hits;
	uint32_t qheld = capn_rpc_send_call(&client, promise_id, 0, 0, 0, 1,
	                                   fill_other_cap, &client);
	ASSERT_NE((uint32_t)-1, qheld);
	EXPECT_FALSE(capn_rpc_is_answered(&client, qheld));
	int calls = 0;
	for (size_t i = 0; i < wire.to_vat.size(); i++) {
		if (frame_which(wire.to_vat[i]) == Message_call)
			calls++;
	}
	EXPECT_EQ(0, calls);

	pump(&vat, wire.to_vat);
	pump(&client, wire.to_client);
	EXPECT_TRUE(capn_rpc_is_answered(&client, qheld));
	EXPECT_FALSE(capn_rpc_is_failed(&client, qheld));
	EXPECT_EQ(hits_before + 1, g_callback_hits);
	EXPECT_EQ(1, g_param_is_export);
	EXPECT_EQ(1, g_param_id);
}

TEST_F(RpcClient, PipelinedCallBeforeReturnIsQueued)
{
	g_vat = &vat;
	g_parked = Parked();
	capn_rpc_set_bootstrap(&vat, &doubler_calls, parking_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t arg = 6;
	uint32_t q = capn_rpc_send_call(&client, 0, 0x1234, 0, 8, 1, fill_u32, &arg);
	settle();
	ASSERT_EQ(1, g_parked.calls);
	EXPECT_FALSE(capn_rpc_is_answered(&client, q));

	const uint16_t path[] = {0};
	uint32_t arg2 = 9;
	uint32_t q2 = capn_rpc_send_pipelined_call(&client, q, path, 1, 0, 0, 8, 0,
	                                          fill_u32, &arg2);
	ASSERT_NE((uint32_t)-1, q2);
	pump(&vat, wire.to_vat);
	EXPECT_TRUE(wire.to_client.empty());
	EXPECT_FALSE(capn_rpc_is_answered(&client, q2));

	capn_ptr cap = capn_rpc_new_export_cap(&vat, g_parked.results.seg, &doubler_calls,
	                                      doubler_dispatch);
	ASSERT_EQ(CAPN_INTERFACE, cap.type);
	ASSERT_EQ(0, capn_setp(g_parked.results, 0, cap));
	ASSERT_EQ(0, capn_rpc_return(&vat, g_parked.question, g_parked.generation));
	settle();

	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));
	ASSERT_TRUE(capn_rpc_is_answered(&client, q2));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q2));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, q2, &m, &content));
	EXPECT_EQ(18u, capn_read32(content, 0));
	capn_free(&m);
}

TEST_F(RpcClient, PipelinedCallOnPromiseResolvesInOrder)
{
	g_vat = &vat;
	g_callback_hits = 0;
	capn_rpc_set_bootstrap(&vat, &promise_obj, promise_host_dispatch);
	capn_rpc_send_bootstrap(&client);
	settle();

	uint32_t qreg = capn_rpc_send_call(&client, 0, 0, 0, 0, 1, fill_with_callback, &client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, qreg));
	ASSERT_GE(g_seen_import, 0);

	uint32_t qprom = capn_rpc_send_call(&client, 0, 0, 1, 0, 1, NULL, NULL);
	const uint16_t path[] = {0};
	uint32_t arg = 9;
	uint32_t qpipe = capn_rpc_send_pipelined_call(&client, qprom, path, 1, 0, 0, 8, 0,
	                                             fill_u32, &arg);
	ASSERT_NE((uint32_t)-1, qpipe);
	pump(&vat, wire.to_vat);
	int returns = 0;
	int calls = 0;
	for (size_t i = 0; i < wire.to_client.size(); i++) {
		int w = frame_which(wire.to_client[i]);
		if (w == Message__return)
			returns++;
		if (w == Message_call)
			calls++;
	}
	EXPECT_EQ(1, returns);
	EXPECT_EQ(0, calls);

	pump(&client, wire.to_client);
	ASSERT_TRUE(capn_rpc_is_answered(&client, qprom));
	ASSERT_FALSE(capn_rpc_is_failed(&client, qprom));
	struct capn m;
	capn_ptr content;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, qprom, &m, &content));
	capn_ptr cap = capn_getp(content, 0, 1);
	int is_export = -1;
	int promise_id = capn_rpc_answer_cap(&client, qprom, cap, &is_export);
	capn_free(&m);
	ASSERT_GE(promise_id, 0);
	EXPECT_EQ(0, is_export);

	ASSERT_EQ(0, capn_rpc_resolve_import(&vat, promise_id,
	                                    static_cast<uint32_t>(g_seen_import)));
	ASSERT_GE(wire.to_client.size(), 2u);
	EXPECT_EQ(Message_call, frame_which(wire.to_client[0]));
	EXPECT_EQ(Message_resolve, frame_which(wire.to_client[1]));
	EXPECT_EQ(static_cast<uint32_t>(g_seen_import), frame_call_target(wire.to_client[0]));

	pump(&client, wire.to_client);
	int disembargoes = 0;
	calls = 0;
	for (size_t i = 0; i < wire.to_vat.size(); i++) {
		int w = frame_which(wire.to_vat[i]);
		if (w == Message_disembargo)
			disembargoes++;
		if (w == Message_call)
			calls++;
	}
	EXPECT_EQ(1, disembargoes);
	EXPECT_EQ(0, calls);

	arg = 1;
	uint32_t qheld = capn_rpc_send_call(&client, static_cast<uint32_t>(promise_id),
	                                   0, 0, 8, 0, fill_u32, &arg);
	EXPECT_FALSE(capn_rpc_is_answered(&client, qheld));
	calls = 0;
	for (size_t i = 0; i < wire.to_vat.size(); i++) {
		if (frame_which(wire.to_vat[i]) == Message_call)
			calls++;
	}
	EXPECT_EQ(0, calls);

	pump(&vat, wire.to_vat);
	pump(&client, wire.to_client);
	EXPECT_TRUE(capn_rpc_is_answered(&client, qpipe));
	EXPECT_FALSE(capn_rpc_is_failed(&client, qpipe));
	EXPECT_TRUE(capn_rpc_is_answered(&client, qheld));
	EXPECT_FALSE(capn_rpc_is_failed(&client, qheld));
	struct capn answered;
	ASSERT_EQ(0, capn_rpc_answer_content(&client, qpipe, &answered, &content));
	EXPECT_EQ(1000u + 2u, capn_read32(content, 0));
	capn_free(&answered);
	ASSERT_EQ(0, capn_rpc_answer_content(&client, qheld, &answered, &content));
	EXPECT_EQ(1000u + 3u, capn_read32(content, 0));
	capn_free(&answered);
	EXPECT_EQ(3, g_callback_hits);
}

TEST_F(RpcClient, DisembargoWaitsUntilQueuedCallsAreSent)
{
	int obj = 0;
	ASSERT_EQ(0, capn_rpc_export_promise(&vat, &obj, promise_dispatch));
	uint32_t arg = 3;
	uint32_t q = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, fill_u32, &arg);
	pump(&vat, wire.to_vat);
	EXPECT_TRUE(wire.to_client.empty());

	std::vector<uint8_t> frame = disembargo_frame(0, 7);
	ASSERT_EQ(0, capn_rpc_handle(&vat, frame.data(), frame.size()));
	EXPECT_TRUE(wire.to_client.empty());

	ASSERT_EQ(0, capn_rpc_resolve_import(&vat, 0, 4));
	ASSERT_GE(wire.to_client.size(), 3u);
	EXPECT_EQ(Message_call, frame_which(wire.to_client[0]));
	EXPECT_EQ(4u, frame_call_target(wire.to_client[0]));
	EXPECT_EQ(Message_disembargo, frame_which(wire.to_client[1]));
	EXPECT_EQ(Message_resolve, frame_which(wire.to_client[2]));
	(void)q;
}

/* A1, B1, A2, resolve B, A3, resolve A. B's resolve frees the slot B1
 * occupied, and A3 must not be replayed ahead of A2. */
uint32_t g_arrival[8];
int g_arrival_n;

int arrival_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr results)
{
	capn_resolve(&params);
	uint32_t in = params.type == CAPN_STRUCT ? capn_read32(params, 0) : 0;
	if (g_arrival_n < 8)
		g_arrival[g_arrival_n++] = in;
	capn_write32(results, 0, in);
	return 0;
}

TEST_F(RpcClient, QueuedCallsReplayInArrivalOrder)
{
	int obj_a = 1;
	int obj_b = 2;
	int settled = 3;
	uint32_t a1 = 1, b1 = 10, a2 = 2, a3 = 3;

	g_arrival_n = 0;
	ASSERT_EQ(0, capn_rpc_export_promise(&vat, &obj_a, arrival_dispatch));
	ASSERT_EQ(1, capn_rpc_export_promise(&vat, &obj_b, arrival_dispatch));
	ASSERT_NE(static_cast<uint32_t>(-1),
	          capn_rpc_send_call(&client, 0, 0, 0, 8, 0, fill_u32, &a1));
	ASSERT_NE(static_cast<uint32_t>(-1),
	          capn_rpc_send_call(&client, 1, 0, 0, 8, 0, fill_u32, &b1));
	ASSERT_NE(static_cast<uint32_t>(-1),
	          capn_rpc_send_call(&client, 0, 0, 0, 8, 0, fill_u32, &a2));
	pump(&vat, wire.to_vat);
	EXPECT_EQ(0, g_arrival_n);

	ASSERT_EQ(0, capn_rpc_resolve(&vat, 1, &settled, arrival_dispatch));
	ASSERT_EQ(1, g_arrival_n);
	EXPECT_EQ(10u, g_arrival[0]);

	ASSERT_NE(static_cast<uint32_t>(-1),
	          capn_rpc_send_call(&client, 0, 0, 0, 8, 0, fill_u32, &a3));
	pump(&vat, wire.to_vat);
	EXPECT_EQ(1, g_arrival_n);

	ASSERT_EQ(0, capn_rpc_resolve(&vat, 0, &settled, arrival_dispatch));
	ASSERT_EQ(4, g_arrival_n);
	EXPECT_EQ(1u, g_arrival[1]);
	EXPECT_EQ(2u, g_arrival[2]);
	EXPECT_EQ(3u, g_arrival[3]);
}

/* Once a promise is resolved to import R, a call that was waiting on it
 * is forwarded to R even when R has already resolved to something else.
 * rpc.capnp Disembargo, "The Tribble 4-way Race Condition". */
int g_saved_import;
int g_tribble_hits;

int save_import_dispatch(void *, uint64_t, uint16_t, capn_ptr params, capn_ptr)
{
	capn_resolve(&params);
	capn_ptr cap = capn_getp(params, 0, 1);
	int is_export = -1;
	g_saved_import = capn_rpc_param_cap(g_vat, cap, &is_export);
	return g_saved_import < 0 ? -1 : 0;
}

int tribble_dispatch(void *, uint64_t, uint16_t, capn_ptr, capn_ptr)
{
	g_tribble_hits++;
	return 0;
}

int g_client_promise = -1;
int g_fill_obj;

void fill_client_promise(void *ctx, capn_ptr params)
{
	struct capn_rpc_conn *c = static_cast<struct capn_rpc_conn *>(ctx);
	capn_ptr cap = capn_rpc_new_promise_cap(c, params.seg, &g_fill_obj, tribble_dispatch);
	struct capn_rpc_pending_cap *built = c->pending_caps;
	int nbuilt = c->npending;
	if (c->cap_dest != NULL && c->cap_dest_n != NULL) {
		built = c->cap_dest;
		nbuilt = *c->cap_dest_n;
	}
	if (nbuilt > 0)
		g_client_promise = static_cast<int>(built[nbuilt - 1].id);
	capn_setp(params, 0, cap);
}

TEST_F(RpcClient, ForwardStopsAtTheImportThePromiseResolvedTo)
{
	int obj = 0;
	int settled = 0;
	g_vat = &vat;
	g_saved_import = -1;
	g_tribble_hits = 0;
	g_client_promise = -1;

	ASSERT_EQ(0, capn_rpc_export_promise(&vat, &obj, promise_dispatch));
	ASSERT_EQ(1, capn_rpc_export(&vat, &obj, save_import_dispatch));

	uint32_t q = capn_rpc_send_call(&client, 1, 1, 0, 0, 1, fill_client_promise, &client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));
	ASSERT_EQ(0, g_saved_import);
	ASSERT_EQ(0, g_client_promise);

	/* The import the vat will forward to has itself resolved further.
	 * A later call must still name the first import, not that later one.
	 * rpc.c++ PromiseClient::resolve keeps the merged target, and
	 * TribbleRaceBlocker stops the walk. */
	ASSERT_EQ(0, capn_rpc_resolve(&client, g_client_promise, &settled, tribble_dispatch));
	settle();

	uint32_t arg = 1;
	uint32_t q2 = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, fill_u32, &arg);
	pump(&vat, wire.to_vat);
	EXPECT_TRUE(wire.to_client.empty());
	ASSERT_EQ(0, capn_rpc_resolve_import(&vat, 0, static_cast<uint32_t>(g_saved_import)));
	ASSERT_FALSE(wire.to_client.empty());
	EXPECT_EQ(Message_call, frame_which(wire.to_client[0]));
	EXPECT_EQ(static_cast<uint32_t>(g_saved_import), frame_call_target(wire.to_client[0]));
	EXPECT_NE(1u, frame_call_target(wire.to_client[0]));
	EXPECT_EQ(0, g_tribble_hits);
	(void)q2;
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

	/* 0x20000000 words is 4 GiB. On a 32-bit size_t the byte count wraps
	 * past the message limit unless the multiply is checked first. */
	ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sv));
	const uint8_t wrap[16] = {0, 0, 0, 0, 0, 0, 0, 0x20, 0, 0, 0, 0, 0, 0, 0, 0};
	ASSERT_EQ(16, write(sv[0], wrap, 16));
	EXPECT_EQ(-1, capn_rpc_fd_read_frame(sv[1], &f, &n));
	EXPECT_EQ(nullptr, f);
	close(sv[0]);
	close(sv[1]);
}
