/* A whole level 3 handoff, with three vats.
 *
 * Alice holds a capability that Bob hosts and wants Carol to have it.
 * The cases in rpc-join-test drive one side at a time with hand-built
 * frames; this one runs all three vats and lets them speak to each
 * other, which is the only way to see that the halves agree:
 *
 *   Alice -> Bob    Provide{target, recipient = RecipientId{carol, nonce}}
 *   Alice -> Carol  a payload carrying ThirdPartyCapId{bob, nonce}
 *   Carol -> Bob    Accept{ProvisionId{nonce}}
 *   Bob   -> Carol  Return carrying the capability
 *
 * The nonce is the only thing the three messages share, which is what
 * lets Bob hand the capability over without taking Carol's word for who
 * sent her.
 */
#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "capn-rpc.h"
#include "capnp_c.h"
#include "rpc-threeparty.capnp.h"
#include "rpc.capnp.h"

namespace {

/* A pipe between two vats: whatever one writes, the other handles. */
struct Wire {
	struct capn_rpc_conn *peer;
	std::vector<std::vector<uint8_t> > queued;
};

int queue_frame(void *ctx, const uint8_t *data, size_t len)
{
	Wire *w = static_cast<Wire *>(ctx);
	w->queued.push_back(std::vector<uint8_t>(data, data + len));
	return 0;
}

/* Deliver everything one side has written to the other. */
void flush(Wire *w)
{
	std::vector<std::vector<uint8_t> > frames;
	frames.swap(w->queued);
	for (size_t i = 0; i < frames.size(); i++)
		capn_rpc_handle(w->peer, frames[i].data(), frames[i].size());
}

/* Answers with its own mark, so a call shows which object it reached. */
struct Marked {
	int calls;
	uint32_t mark;
};

int marked_dispatch(void *server, uint64_t, uint16_t, capn_ptr, capn_ptr results)
{
	Marked *m = static_cast<Marked *>(server);
	m->calls++;
	capn_write32(results, 0, m->mark);
	return 0;
}

/* The answer to `qid`, if one has arrived: whether it carries a
 * capability, and its export id. */
struct Answer {
	bool answered;
	bool failed;
	bool has_cap;
	uint32_t cap_id;
};

Answer read_answer(struct capn_rpc_conn *c, uint32_t qid)
{
	Answer a;
	memset(&a, 0, sizeof a);
	a.answered = capn_rpc_is_answered(c, qid) != 0;
	if (!a.answered)
		return a;
	a.failed = capn_rpc_is_failed(c, qid) != 0;
	if (a.failed)
		return a;

	struct capn held;
	capn_ptr content;
	if (capn_rpc_answer_content(c, qid, &held, &content) != 0)
		return a;
	a.has_cap = content.type == CAPN_INTERFACE;
	capn_free(&held);
	if (a.has_cap)
		a.cap_id = static_cast<uint32_t>(capn_rpc_answer_cap_id(c, qid));
	return a;
}

/* Alice -> Carol: a call whose params name the capability Bob hosts.
 * Alice is the introducer, so she writes the descriptor. */
void tell_carol_where_to_go(struct capn_rpc_conn *alice, uint32_t qid,
                            const char *host, uint16_t port, uint64_t nonce,
                            uint32_t vine_id)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	CapDescriptor_ptr cdp = new_CapDescriptor(cs);
	ASSERT_EQ(0, capn_rpc_write_third_party_cap(alice, cdp, host, port, nonce, vine_id));
	CapDescriptor_list table = new_CapDescriptor_list(cs, 1);
	capn_setp(table.p, 0, cdp.p);

	struct Payload params;
	memset(&params, 0, sizeof params);
	params.content = capn_new_struct(cs, 8, 0);
	params.capTable = table;
	Payload_ptr plp = new_Payload(cs);
	write_Payload(&params, plp);

	struct MessageTarget tgt;
	memset(&tgt, 0, sizeof tgt);
	tgt.which = MessageTarget_importedCap;
	tgt.importedCap = 0;
	MessageTarget_ptr tp = new_MessageTarget(cs);
	write_MessageTarget(&tgt, tp);

	struct Call call;
	memset(&call, 0, sizeof call);
	call.questionId = qid;
	call.target = tp;
	call.interfaceId = 0x1234;
	call.params = plp;
	Call_ptr cp = new_Call(cs);
	write_Call(&call, cp);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_call;
	m.call = cp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	/* Sent on Alice's connection to Carol; the send callback queues it. */
	capn_rpc_handle(alice, buf, static_cast<size_t>(sz));
}

TEST(RpcHandoff, CarolEndsUpHoldingTheCapabilityBobHosts)
{
	const uint64_t nonce = 0x5eedULL;
	Marked hosted = {0, 42};
	/* Bob answers Carol's connection with a different object, so the two
	 * connections do not agree on export ids by accident: the capability
	 * Alice hands over must arrive under an id of Carol's connection,
	 * not the one Alice used. */
	Marked sidecar = {0, 1000};

	/* Bob is one vat with two connections, so the arrangement Alice
	 * makes on hers is claimable on Carol's. */
	struct capn_rpc_vat bob_vat;
	memset(&bob_vat, 0, sizeof bob_vat);

	struct capn_rpc_conn alice_to_bob, bob_to_alice;
	struct capn_rpc_conn carol_to_bob, bob_to_carol;
	Wire a2b, b2a, c2b, b2c;

	capn_rpc_init(&alice_to_bob, queue_frame, &a2b);
	capn_rpc_init(&bob_to_alice, queue_frame, &b2a);
	capn_rpc_init(&carol_to_bob, queue_frame, &c2b);
	capn_rpc_init(&bob_to_carol, queue_frame, &b2c);
	capn_rpc_set_vat(&bob_to_alice, &bob_vat);
	capn_rpc_set_vat(&bob_to_carol, &bob_vat);
	capn_rpc_set_bootstrap(&bob_to_alice, &hosted, marked_dispatch);
	capn_rpc_set_bootstrap(&bob_to_carol, &sidecar, marked_dispatch);
	a2b.peer = &bob_to_alice;
	b2a.peer = &alice_to_bob;
	c2b.peer = &bob_to_carol;
	b2c.peer = &carol_to_bob;

	/* Alice bootstraps, so Bob exports the capability to her. */
	uint32_t boot = capn_rpc_send_bootstrap(&alice_to_bob);
	flush(&a2b);
	flush(&b2a);
	ASSERT_TRUE(read_answer(&alice_to_bob, boot).answered);

	/* 1. Alice tells Bob to expect Carol. */
	uint32_t provide = capn_rpc_send_provide(&alice_to_bob, 0, "10.0.0.2", 5001, nonce);
	ASSERT_NE((uint32_t)-1, provide);
	flush(&a2b);
	flush(&b2a);
	EXPECT_EQ(1, capn_rpc_pending_provisions(&bob_to_alice, NULL, 0));
	/* The same vat, seen through its other connection. */
	EXPECT_EQ(1, capn_rpc_pending_provisions(&bob_to_carol, NULL, 0));

	/* 2. Alice tells Carol where to go. Carol records the introduction
	 *    rather than dialling: reaching Bob is the network's job, and
	 *    here the connection already exists. */
	struct capn_rpc_conn alice_to_carol, carol_to_alice;
	Wire a2c, c2a;
	capn_rpc_init(&alice_to_carol, queue_frame, &a2c);
	capn_rpc_init(&carol_to_alice, queue_frame, &c2a);
	a2c.peer = &carol_to_alice;
	c2a.peer = &alice_to_carol;
	Marked carols = {0, 1};
	capn_rpc_set_bootstrap(&carol_to_alice, &carols, marked_dispatch);

	{
		/* Built on Alice's side, delivered to Carol. */
		struct capn msg;
		capn_init_malloc(&msg);
		struct capn_segment *cs = capn_root(&msg).seg;
		CapDescriptor_ptr cdp = new_CapDescriptor(cs);
		ASSERT_EQ(0, capn_rpc_write_third_party_cap(&alice_to_carol, cdp,
		                                            "10.0.0.1", 5000, nonce, 7));
		CapDescriptor_list table = new_CapDescriptor_list(cs, 1);
		capn_setp(table.p, 0, cdp.p);
		struct Payload params;
		memset(&params, 0, sizeof params);
		params.content = capn_new_struct(cs, 8, 0);
		params.capTable = table;
		Payload_ptr plp = new_Payload(cs);
		write_Payload(&params, plp);
		struct MessageTarget tgt;
		memset(&tgt, 0, sizeof tgt);
		tgt.which = MessageTarget_importedCap;
		tgt.importedCap = 0;
		MessageTarget_ptr tp = new_MessageTarget(cs);
		write_MessageTarget(&tgt, tp);
		struct Call call;
		memset(&call, 0, sizeof call);
		call.questionId = 90;
		call.target = tp;
		call.interfaceId = 0x1234;
		call.params = plp;
		Call_ptr cp = new_Call(cs);
		write_Call(&call, cp);
		struct Message m;
		memset(&m, 0, sizeof m);
		m.which = Message_call;
		m.call = cp;
		Message_ptr mp = new_Message(cs);
		write_Message(&m, mp);
		capn_setp(capn_root(&msg), 0, mp.p);
		uint8_t buf[4096];
		ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
		capn_free(&msg);
		ASSERT_GT(sz, 0);
		capn_rpc_handle(&carol_to_alice, buf, static_cast<size_t>(sz));
	}

	struct capn_rpc_introduction learned[4];
	ASSERT_EQ(1, capn_rpc_pending_introductions(&carol_to_alice, learned, 4));
	EXPECT_EQ(nonce, learned[0].nonce);
	EXPECT_STREQ("10.0.0.1", learned[0].host);
	EXPECT_EQ(5000, learned[0].port);

	/* Carol bootstraps Bob first, so her connection's export 0 is the
	 * sidecar and the handed-over capability cannot land on 0 too. */
	uint32_t side = capn_rpc_send_bootstrap(&carol_to_bob);
	flush(&c2b);
	flush(&b2c);
	ASSERT_TRUE(read_answer(&carol_to_bob, side).answered);

	/* 3. Carol presents the nonce to Bob, over her own connection. She
	 *    was never told which export id Alice used, and it would mean
	 *    nothing here: the arrangement is keyed by the nonce alone. */
	uint32_t claim = capn_rpc_send_accept(&carol_to_bob, nonce, 0);
	ASSERT_NE((uint32_t)-1, claim);
	flush(&c2b);
	flush(&b2c);
	Answer got = read_answer(&carol_to_bob, claim);
	EXPECT_TRUE(got.answered);
	EXPECT_FALSE(got.failed);
	EXPECT_EQ(0, capn_rpc_pending_provisions(&bob_to_alice, NULL, 0));

	/* Claimable exactly once, on any connection. */
	uint32_t replay = capn_rpc_send_accept(&carol_to_bob, nonce, 0);
	flush(&c2b);
	flush(&b2c);
	EXPECT_TRUE(read_answer(&carol_to_bob, replay).failed);

	/* 4. Carol drops the vine now that the pickup is done. */
	EXPECT_EQ(0, capn_rpc_introduction_done(&carol_to_alice, nonce));
	EXPECT_EQ(0, capn_rpc_pending_introductions(&carol_to_alice, NULL, 0));

	/* Carol now holds the capability Bob hosts. Calling it reaches the
	 * object Alice was talking to, not whatever else sits at that id on
	 * Carol's connection: this one answers 42, the sidecar 1000. */
	int before = hosted.calls;
	ASSERT_TRUE(got.has_cap);
	/* Not the id Alice used: her connection called it 0, and 0 here is
	 * the sidecar. */
	EXPECT_NE(0u, got.cap_id);
	uint32_t q = capn_rpc_send_call(&carol_to_bob, got.cap_id, 0x1234, 0, 1, 0,
	                                NULL, NULL);
	flush(&c2b);
	flush(&b2c);
	EXPECT_EQ(before + 1, hosted.calls);
	EXPECT_EQ(0, sidecar.calls);
	EXPECT_TRUE(read_answer(&carol_to_bob, q).answered);

	(void)tell_carol_where_to_go;
	(void)provide;
}

} // namespace
