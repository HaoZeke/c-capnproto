/* rpc-join-test.cpp
 *
 * Level 4 `Join`: does a set of capabilities name one object?
 *
 * The vat is driven with raw messages rather than through a client
 * facade, so each assertion is about the wire behaviour the spec
 * prescribes and not about a convenience layer on top of it.
 */

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifndef CAPNP_SOURCE_ROOT
#define CAPNP_SOURCE_ROOT ""
#endif

#include "capn-rpc.h"
#include "capnp_c.h"
#include "rpc-threeparty.capnp.h"
#include "rpc.capnp.h"

namespace {

/* Collects the frames the vat sends, so a test can read replies in order. */
struct Outbox {
	std::vector<std::vector<uint8_t> > frames;
};

int collect(void *ctx, const uint8_t *data, size_t len)
{
	Outbox *out = static_cast<Outbox *>(ctx);
	out->frames.push_back(std::vector<uint8_t>(data, data + len));
	return 0;
}

int counting_dispatch(void *server, uint64_t, uint16_t, capn_ptr, capn_ptr results)
{
	int *calls = static_cast<int *>(server);
	(*calls)++;
	capn_write32(results, 0, static_cast<uint32_t>(*calls));
	return 0;
}

/* Send one Join part naming `export_id`. */
void send_join_part(struct capn_rpc_conn *c, uint32_t qid, uint32_t export_id,
                    uint32_t join_id, uint16_t part_count, uint16_t part_num)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	struct MessageTarget t;
	memset(&t, 0, sizeof t);
	t.which = MessageTarget_importedCap;
	t.importedCap = export_id;
	MessageTarget_ptr tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);

	struct JoinKeyPart kp;
	memset(&kp, 0, sizeof kp);
	kp.joinId = join_id;
	kp.partCount = part_count;
	kp.partNum = part_num;
	JoinKeyPart_ptr kpp = new_JoinKeyPart(cs);
	write_JoinKeyPart(&kp, kpp);

	struct Join j;
	memset(&j, 0, sizeof j);
	j.questionId = qid;
	j.target = tp;
	j.keyPart = kpp.p;
	Join_ptr jp = new_Join(cs);
	write_Join(&j, jp);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_join;
	m.join = jp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	ASSERT_EQ(0, capn_rpc_handle(c, buf, static_cast<size_t>(sz)));
}

struct JoinReply {
	uint32_t answer_id;
	uint32_t join_id;
	bool succeeded;
	bool has_cap;
};

JoinReply read_join_reply(const std::vector<uint8_t> &frame)
{
	struct capn msg;
	EXPECT_EQ(0, capn_init_mem(&msg, frame.data(), frame.size(), 0));

	Message_ptr mp;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	struct Message m;
	read_Message(&m, mp);
	EXPECT_EQ(Message__return, m.which);

	struct Return r;
	read_Return(&r, m._return);
	EXPECT_EQ(Return_results, r.which);

	struct Payload pl;
	read_Payload(&pl, r.results);

	JoinResult_ptr jrp;
	jrp.p = pl.content;
	struct JoinResult jr;
	read_JoinResult(&jr, jrp);

	JoinReply out;
	out.answer_id = r.answerId;
	out.join_id = jr.joinId;
	out.succeeded = jr.succeeded != 0;
	/* Generated getters hand back the unresolved pointer, which is a far
	 * pointer whenever the object landed in another segment, so the kind
	 * is only visible after resolving. */
	capn_resolve(&jr.cap);
	out.has_cap = jr.cap.type == CAPN_INTERFACE;
	capn_free(&msg);
	return out;
}


/* Bootstrap once so the vat holds a live export. */
uint32_t bootstrap_export(struct capn_rpc_conn *c, Outbox *out)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	struct Bootstrap b;
	memset(&b, 0, sizeof b);
	b.questionId = 1;
	Bootstrap_ptr bp = new_Bootstrap(cs);
	write_Bootstrap(&b, bp);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_bootstrap;
	m.bootstrap = bp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	EXPECT_GT(sz, 0);
	EXPECT_EQ(0, capn_rpc_handle(c, buf, static_cast<size_t>(sz)));
	out->frames.clear(); /* the Return itself is not what these tests read */
	return 0;             /* first export allocated is id 0 */
}

class RpcJoin : public ::testing::Test {
 protected:
	void SetUp() override
	{
		calls = 0;
		capn_rpc_init(&conn, collect, &out);
		capn_rpc_set_bootstrap(&conn, &calls, counting_dispatch);
		live = bootstrap_export(&conn, &out);
	}

	struct capn_rpc_conn conn;
	Outbox out;
	int calls;
	uint32_t live;

	void TearDown() override
	{
		capn_rpc_free(&conn);
	}
};

TEST_F(RpcJoin, SenderLoopbackRequiresALocalTarget)
{
	struct capn msg;
	struct MessageTarget target;
	struct Disembargo embargo;
	struct Message message;
	MessageTarget_ptr tp;
	Disembargo_ptr dp;
	Message_ptr mp;
	uint8_t buf[4096];
	ssize_t len;

	capn_init_malloc(&msg);
	memset(&target, 0, sizeof target);
	target.which = MessageTarget_importedCap;
	target.importedCap = live + 1;
	tp = new_MessageTarget(capn_root(&msg).seg);
	write_MessageTarget(&target, tp);
	memset(&embargo, 0, sizeof embargo);
	embargo.target = tp;
	embargo.context_which = Disembargo_context_senderLoopback;
	embargo.context.senderLoopback = 7;
	dp = new_Disembargo(capn_root(&msg).seg);
	write_Disembargo(&embargo, dp);
	memset(&message, 0, sizeof message);
	message.which = Message_disembargo;
	message.disembargo = dp;
	mp = new_Message(capn_root(&msg).seg);
	write_Message(&message, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	len = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(len, 0);
	ASSERT_EQ(0, capn_rpc_handle(&conn, buf, static_cast<size_t>(len)));
	EXPECT_TRUE(out.frames.empty());
}

TEST_F(RpcJoin, SameCapabilityJoinsAndOneResultCarriesTheCap)
{
	send_join_part(&conn, 700, live, 9, 2, 0);
	/* An incomplete set is not answerable, so nothing comes back yet. */
	ASSERT_EQ(0u, out.frames.size());

	send_join_part(&conn, 701, live, 9, 2, 1);
	ASSERT_EQ(2u, out.frames.size());

	JoinReply a = read_join_reply(out.frames[0]);
	JoinReply b = read_join_reply(out.frames[1]);
	EXPECT_EQ(9u, a.join_id);
	EXPECT_EQ(9u, b.join_id);
	EXPECT_TRUE(a.succeeded);
	EXPECT_TRUE(b.succeeded);
	EXPECT_TRUE((a.answer_id == 700 && b.answer_id == 701) ||
	            (a.answer_id == 701 && b.answer_id == 700));
	/* JoinResult: exactly one of the set carries the joined capability. */
	EXPECT_EQ(1, (a.has_cap ? 1 : 0) + (b.has_cap ? 1 : 0));
}

TEST_F(RpcJoin, UnresolvablePartFailsTheWholeSet)
{
	send_join_part(&conn, 710, live, 11, 2, 0);
	send_join_part(&conn, 711, live + 40, 11, 2, 1);
	ASSERT_EQ(2u, out.frames.size());

	JoinReply a = read_join_reply(out.frames[0]);
	JoinReply b = read_join_reply(out.frames[1]);
	EXPECT_FALSE(a.succeeded);
	EXPECT_FALSE(b.succeeded);
	/* A failed join carries no capability. */
	EXPECT_FALSE(a.has_cap);
	EXPECT_FALSE(b.has_cap);
}

TEST_F(RpcJoin, AllPartsUnresolvableStillFails)
{
	/* Every part agrees, but they agree on naming nothing: equality has
	 * to be proven against a capability we host, not against absence. */
	send_join_part(&conn, 740, live + 40, 17, 2, 0);
	send_join_part(&conn, 741, live + 40, 17, 2, 1);
	ASSERT_EQ(2u, out.frames.size());

	JoinReply a = read_join_reply(out.frames[0]);
	JoinReply b = read_join_reply(out.frames[1]);
	EXPECT_FALSE(a.succeeded);
	EXPECT_FALSE(b.succeeded);
	EXPECT_FALSE(a.has_cap);
	EXPECT_FALSE(b.has_cap);
}

TEST_F(RpcJoin, IncompleteSetIsNeverAnswered)
{
	send_join_part(&conn, 720, live, 13, 3, 0);
	send_join_part(&conn, 721, live, 13, 3, 1);
	/* Two of three parts: the receiver still cannot compare the set. */
	EXPECT_EQ(0u, out.frames.size());
}

TEST_F(RpcJoin, PartNumOutsideTheSetIsRejectedOnItsOwn)
{
	send_join_part(&conn, 730, live, 15, 2, 7);
	ASSERT_EQ(1u, out.frames.size());
	JoinReply a = read_join_reply(out.frames[0]);
	EXPECT_EQ(730u, a.answer_id);
	EXPECT_FALSE(a.succeeded);
}

/* Level 3: Alice tells us to hold a capability for Carol, and Carol
 * claims it with the nonce Alice chose. We match on the nonce alone, so
 * we never have to trust Carol's account of who sent her. */
static void send_provide(struct capn_rpc_conn *c, uint32_t qid,
                         uint32_t export_id, uint64_t nonce)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	struct MessageTarget t;
	memset(&t, 0, sizeof t);
	t.which = MessageTarget_importedCap;
	t.importedCap = export_id;
	MessageTarget_ptr tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);

	struct VatId vat;
	memset(&vat, 0, sizeof vat);
	vat.host.str = "127.0.0.1";
	vat.host.len = 9;
	/* NULL segment: capn_set_text copies into the message. Naming a
	 * segment would claim the literal already lives there. */
	vat.host.seg = NULL;
	vat.port = 4000;
	VatId_ptr vp = new_VatId(cs);
	write_VatId(&vat, vp);

	struct RecipientId rid;
	memset(&rid, 0, sizeof rid);
	rid.vat = vp;
	rid.nonce = nonce;
	RecipientId_ptr rp = new_RecipientId(cs);
	write_RecipientId(&rid, rp);

	struct Provide pv;
	memset(&pv, 0, sizeof pv);
	pv.questionId = qid;
	pv.target = tp;
	pv.recipient = rp.p;
	Provide_ptr pp = new_Provide(cs);
	write_Provide(&pv, pp);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_provide;
	m.provide = pp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	capn_rpc_handle(c, buf, static_cast<size_t>(sz));
}

static void send_accept(struct capn_rpc_conn *c, uint32_t qid, uint64_t nonce)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	struct ProvisionId pid;
	memset(&pid, 0, sizeof pid);
	pid.nonce = nonce;
	ProvisionId_ptr pip = new_ProvisionId(cs);
	write_ProvisionId(&pid, pip);

	struct Accept ac;
	memset(&ac, 0, sizeof ac);
	ac.questionId = qid;
	ac.provision = pip.p;
	Accept_ptr ap = new_Accept(cs);
	write_Accept(&ac, ap);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_accept;
	m.accept = ap;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	capn_rpc_handle(c, buf, static_cast<size_t>(sz));
}

/* Return tag, and whether the results content is a capability. */
struct L3Reply {
	uint32_t answer_id;
	bool is_exception;
	bool has_cap;
};

static L3Reply read_l3(const std::vector<uint8_t> &frame)
{
	struct capn msg;
	EXPECT_EQ(0, capn_init_mem(&msg, frame.data(), frame.size(), 0));
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	struct Message m;
	read_Message(&m, mp);
	EXPECT_EQ(Message__return, m.which);
	struct Return r;
	read_Return(&r, m._return);

	L3Reply out;
	out.answer_id = r.answerId;
	out.is_exception = (r.which != Return_results);
	out.has_cap = false;
	if (!out.is_exception) {
		struct Payload pl;
		read_Payload(&pl, r.results);
		capn_ptr c = pl.content;
		capn_resolve(&c);
		out.has_cap = (c.type == CAPN_INTERFACE);
	}
	capn_free(&msg);
	return out;
}

TEST_F(RpcJoin, ProvidedCapabilityIsClaimableExactlyOnce)
{
	const uint64_t nonce = 0xfeedfaceULL;
	send_provide(&conn, 10, live, nonce);
	ASSERT_EQ(1u, out.frames.size());
	EXPECT_FALSE(read_l3(out.frames[0]).is_exception);
	EXPECT_EQ(1, capn_rpc_pending_provisions(&conn, NULL, 0));
	out.frames.clear();

	send_accept(&conn, 11, nonce);
	ASSERT_EQ(1u, out.frames.size());
	{
		L3Reply r = read_l3(out.frames[0]);
		EXPECT_EQ(11u, r.answer_id);
		EXPECT_FALSE(r.is_exception);
		/* The capability comes back as a capability, not a struct. */
		EXPECT_TRUE(r.has_cap);
	}
	EXPECT_EQ(0, capn_rpc_pending_provisions(&conn, NULL, 0));
	out.frames.clear();

	/* A nonce is single-use: leaving it claimable would let anyone who
	 * learned it take the capability again. */
	send_accept(&conn, 12, nonce);
	ASSERT_EQ(1u, out.frames.size());
	EXPECT_TRUE(read_l3(out.frames[0]).is_exception);
	out.frames.clear();
}

/* Refused even while a different arrangement is standing: matching is on
 * the nonce, not on there being something to hand over. */
TEST_F(RpcJoin, AcceptWithUnknownNonceIsRefused)
{
	send_provide(&conn, 19, live, 0xc0ffeeULL);
	out.frames.clear();

	send_accept(&conn, 20, 0xdeadbeefULL);
	ASSERT_EQ(1u, out.frames.size());
	L3Reply r = read_l3(out.frames[0]);
	EXPECT_EQ(20u, r.answer_id);
	EXPECT_TRUE(r.is_exception);
	EXPECT_EQ(1, capn_rpc_pending_provisions(&conn, NULL, 0));
	out.frames.clear();

	send_accept(&conn, 21, 0xc0ffeeULL);
	ASSERT_EQ(1u, out.frames.size());
	EXPECT_FALSE(read_l3(out.frames[0]).is_exception);
}

/* Carol -> us: claim the capability under `nonce`, but hold the answer
 * until the introducer lifts the embargo. */
static void send_accept_embargoed(struct capn_rpc_conn *c, uint32_t qid, uint64_t nonce)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	struct ProvisionId pid;
	memset(&pid, 0, sizeof pid);
	pid.nonce = nonce;
	ProvisionId_ptr pip = new_ProvisionId(cs);
	write_ProvisionId(&pid, pip);

	struct Accept ac;
	memset(&ac, 0, sizeof ac);
	ac.questionId = qid;
	ac.provision = pip.p;
	ac.embargo = 1;
	Accept_ptr ap = new_Accept(cs);
	write_Accept(&ac, ap);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_accept;
	m.accept = ap;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	capn_rpc_handle(c, buf, static_cast<size_t>(sz));
}

/* Alice -> us: lift the embargo on the Accept she arranged, naming her
 * own Provide question. */
static void send_disembargo_provide(struct capn_rpc_conn *c, uint32_t provide_qid)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	struct Disembargo d;
	memset(&d, 0, sizeof d);
	d.context_which = Disembargo_context_provide;
	d.context.provide = provide_qid;
	Disembargo_ptr dp = new_Disembargo(cs);
	write_Disembargo(&d, dp);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_disembargo;
	m.disembargo = dp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	capn_rpc_handle(c, buf, static_cast<size_t>(sz));
}

/* Level 3 embargo: an embargoed Accept claims the capability but is not
 * answered until the introducer says the recipient's earlier calls have
 * all arrived. Answering sooner would let a call sent straight to us
 * overtake one still in flight through the introducer. */
TEST_F(RpcJoin, EmbargoedAcceptIsAnsweredOnlyAfterDisembargo)
{
	const uint64_t nonce = 0x5150ULL;
	send_provide(&conn, 70, live, nonce);
	ASSERT_EQ(1u, out.frames.size());
	EXPECT_FALSE(read_l3(out.frames[0]).is_exception);
	out.frames.clear();

	send_accept_embargoed(&conn, 71, nonce);
	EXPECT_EQ(0u, out.frames.size()) << "an embargoed Accept must not be answered yet";
	EXPECT_EQ(1, capn_rpc_embargoed_accepts(&conn));
	/* The arrangement is claimed, so a second Accept finds nothing. */
	send_accept(&conn, 72, nonce);
	ASSERT_EQ(1u, out.frames.size());
	EXPECT_TRUE(read_l3(out.frames[0]).is_exception);
	out.frames.clear();

	send_disembargo_provide(&conn, 70);
	ASSERT_EQ(1u, out.frames.size());
	L3Reply r = read_l3(out.frames[0]);
	EXPECT_EQ(71u, r.answer_id);
	EXPECT_FALSE(r.is_exception);
	EXPECT_TRUE(r.has_cap);
	EXPECT_EQ(0, capn_rpc_embargoed_accepts(&conn));
	EXPECT_EQ(0, capn_rpc_pending_provisions(&conn, NULL, 0));
}

/* A Disembargo naming a Provide we never received changes nothing. */
TEST_F(RpcJoin, DisembargoForAnUnknownProvideIsIgnored)
{
	const uint64_t nonce = 0x5151ULL;
	send_provide(&conn, 73, live, nonce);
	out.frames.clear();
	send_accept_embargoed(&conn, 74, nonce);
	out.frames.clear();

	send_disembargo_provide(&conn, 999);
	EXPECT_EQ(0u, out.frames.size());
	EXPECT_EQ(1, capn_rpc_embargoed_accepts(&conn));

	send_disembargo_provide(&conn, 73);
	ASSERT_EQ(1u, out.frames.size());
	EXPECT_EQ(74u, read_l3(out.frames[0]).answer_id);
}

/* Alice -> us: a Call whose params name a capability hosted by a third
 * vat, with a vine we can use in the meantime. This is the receiving
 * half of the introduction; the Provide/Accept tests above are the
 * hosting half. */
static void send_call_with_third_party_cap(struct capn_rpc_conn *c, uint32_t qid,
                                           uint32_t target_export,
                                           const char *host, uint16_t port,
                                           uint64_t nonce, uint32_t vine_id)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	CapDescriptor_ptr cdp = new_CapDescriptor(cs);
	ASSERT_EQ(0, capn_rpc_write_third_party_cap(c, cdp, host, port, nonce, vine_id));

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
	tgt.importedCap = target_export;
	MessageTarget_ptr tp = new_MessageTarget(cs);
	write_MessageTarget(&tgt, tp);

	struct Call call;
	memset(&call, 0, sizeof call);
	call.questionId = qid;
	call.target = tp;
	call.interfaceId = 0x1234;
	call.methodId = 0;
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
	capn_rpc_handle(c, buf, static_cast<size_t>(sz));
}

/* Whether a frame is a Release naming `id`. */
static bool is_release_of(const std::vector<uint8_t> &frame, uint32_t id)
{
	struct capn msg;
	if (capn_init_mem(&msg, frame.data(), frame.size(), 0) != 0)
		return false;
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	struct Message m;
	read_Message(&m, mp);
	bool match = false;
	if (m.which == Message_release) {
		struct Release rel;
		read_Release(&rel, m.release);
		match = rel.id == id && rel.referenceCount == 1;
	}
	capn_free(&msg);
	return match;
}

/* Level 3, receiving half: a payload that names a third party's
 * capability is recorded as an introduction, and the vine survives until
 * the pickup is finished. */
TEST_F(RpcJoin, ThirdPartyCapDescriptorIsRecordedAsAnIntroduction)
{
	EXPECT_EQ(0, capn_rpc_pending_introductions(&conn, NULL, 0));

	send_call_with_third_party_cap(&conn, 50, live, "10.0.0.7", 5000,
	                               0xabcdefULL, 77);

	struct capn_rpc_introduction got[4];
	ASSERT_EQ(1, capn_rpc_pending_introductions(&conn, got, 4));
	EXPECT_EQ(0xabcdefULL, got[0].nonce);
	EXPECT_EQ(77u, got[0].vine_id);
	EXPECT_EQ(5000, got[0].port);
	EXPECT_STREQ("10.0.0.7", got[0].host);

	/* Nothing is released while the pickup is outstanding: the vine is
	 * the only way to reach the capability until then. */
	for (const auto &f : out.frames)
		EXPECT_FALSE(is_release_of(f, 77));
	out.frames.clear();

	ASSERT_EQ(0, capn_rpc_introduction_done(&conn, 0xabcdefULL));
	bool released = false;
	for (const auto &f : out.frames)
		released = released || is_release_of(f, 77);
	EXPECT_TRUE(released);
	EXPECT_EQ(0, capn_rpc_pending_introductions(&conn, NULL, 0));

	/* Finishing an introduction nobody handed us is refused, even while
	 * another is outstanding: the nonce picks the arrangement, not the
	 * fact that there is one. */
	send_call_with_third_party_cap(&conn, 52, live, "10.0.0.8", 5001,
	                               0x99ULL, 79);
	ASSERT_EQ(1, capn_rpc_pending_introductions(&conn, NULL, 0));
	EXPECT_EQ(-1, capn_rpc_introduction_done(&conn, 0xabcdefULL));
	EXPECT_EQ(1, capn_rpc_pending_introductions(&conn, NULL, 0));
	EXPECT_EQ(0, capn_rpc_introduction_done(&conn, 0x99ULL));
	EXPECT_EQ(0, capn_rpc_pending_introductions(&conn, NULL, 0));
}

/* The introducer can also hand us the descriptor in an answer, not only
 * in a call's params. */
static void send_return_with_third_party_cap(struct capn_rpc_conn *c, uint32_t qid,
                                             const char *host, uint16_t port,
                                             uint64_t nonce, uint32_t vine_id)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	CapDescriptor_ptr cdp = new_CapDescriptor(cs);
	ASSERT_EQ(0, capn_rpc_write_third_party_cap(c, cdp, host, port, nonce, vine_id));
	CapDescriptor_list table = new_CapDescriptor_list(cs, 1);
	capn_setp(table.p, 0, cdp.p);

	struct Payload results;
	memset(&results, 0, sizeof results);
	results.content = capn_new_struct(cs, 8, 0);
	results.capTable = table;
	Payload_ptr plp = new_Payload(cs);
	write_Payload(&results, plp);

	struct Return r;
	memset(&r, 0, sizeof r);
	r.answerId = qid;
	r.which = Return_results;
	r.results = plp;
	Return_ptr rp = new_Return(cs);
	write_Return(&r, rp);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message__return;
	m._return = rp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	capn_rpc_handle(c, buf, static_cast<size_t>(sz));
}

TEST_F(RpcJoin, ThirdPartyCapDescriptorInAnAnswerIsRecorded)
{
	uint32_t qid = capn_rpc_send_bootstrap(&conn);
	out.frames.clear();
	send_return_with_third_party_cap(&conn, qid, "10.0.0.9", 5002, 0x77ULL, 80);

	struct capn_rpc_introduction got[4];
	ASSERT_EQ(1, capn_rpc_pending_introductions(&conn, got, 4));
	EXPECT_EQ(0x77ULL, got[0].nonce);
	EXPECT_EQ(80u, got[0].vine_id);
	EXPECT_STREQ("10.0.0.9", got[0].host);
}

/* A descriptor naming a host too long to store is refused rather than
 * truncated: a truncated address names a different vat. */
TEST_F(RpcJoin, OverlongHostIsRefusedNotTruncated)
{
	std::string host(CAPN_RPC_MAX_HOST + 8, 'h');
	send_call_with_third_party_cap(&conn, 51, live, host.c_str(), 5000,
	                               0xfeedULL, 78);
	EXPECT_EQ(0, capn_rpc_pending_introductions(&conn, NULL, 0));
}

/* Level 3 driven by frames the reference `capnp` CLI encoded.
 *
 * Every other level 3 test builds its own Provide and Accept, so it
 * shows the vat agrees with this library's builder and nothing more: a
 * layout both sides share but the wire format does not would pass all of
 * them. These bytes come from the reference implementation
 * (scripts/gen-rpc-frames.sh): hold export 0 for whoever presents
 * 0xfeedface (question 42), then claim it (question 43).
 */
static std::vector<uint8_t> golden_frame(const char *name)
{
	const char *env = getenv("CAPNP_SOURCE_ROOT");
	std::string root = (env && env[0]) ? env : CAPNP_SOURCE_ROOT;
	if (root.empty())
		root = ".";
	std::string path = root + "/tests/fixtures/" + name;
	std::vector<uint8_t> bytes;
	FILE *f = fopen(path.c_str(), "rb");
	/* A checked-in golden that will not open is a broken tree, not a
	 * reason to pass. */
	EXPECT_NE(nullptr, f) << path;
	if (!f)
		return bytes;
	uint8_t buf[4096];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
		bytes.insert(bytes.end(), buf, buf + n);
	fclose(f);
	return bytes;
}

/* The frame the reference encoder writes, rather than one these tests
 * built: a layout the writer and reader share but the wire format does
 * not would pass every case above. */
TEST_F(RpcJoin, ReferenceThirdPartyDescriptorIsRecorded)
{
	std::vector<uint8_t> frame = golden_frame("rpc-introduce.bin");
	ASSERT_FALSE(frame.empty());
	ASSERT_EQ(0, capn_rpc_handle(&conn, frame.data(), frame.size()));

	struct capn_rpc_introduction got[4];
	ASSERT_EQ(1, capn_rpc_pending_introductions(&conn, got, 4));
	EXPECT_EQ(0xabcdefULL, got[0].nonce);
	EXPECT_EQ(77u, got[0].vine_id);
	EXPECT_EQ(5000, got[0].port);
	EXPECT_STREQ("10.0.0.7", got[0].host);
}

TEST_F(RpcJoin, ReferenceEncoderFramesDriveTheHandoff)
{
	ASSERT_EQ(0u, live);

	std::vector<uint8_t> provide = golden_frame("rpc-provide.bin");
	ASSERT_FALSE(provide.empty());
	ASSERT_EQ(0, capn_rpc_handle(&conn, provide.data(), provide.size()));
	ASSERT_EQ(1u, out.frames.size());
	L3Reply p = read_l3(out.frames[0]);
	EXPECT_EQ(42u, p.answer_id);
	EXPECT_FALSE(p.is_exception);
	/* The nonce the vat recorded is the one the CLI wrote. */
	EXPECT_EQ(1, capn_rpc_pending_provisions(&conn, NULL, 0));
	out.frames.clear();

	std::vector<uint8_t> accept = golden_frame("rpc-accept.bin");
	ASSERT_FALSE(accept.empty());
	ASSERT_EQ(0, capn_rpc_handle(&conn, accept.data(), accept.size()));
	ASSERT_EQ(1u, out.frames.size());
	L3Reply a = read_l3(out.frames[0]);
	EXPECT_EQ(43u, a.answer_id);
	EXPECT_FALSE(a.is_exception);
	EXPECT_TRUE(a.has_cap);
	EXPECT_EQ(0, capn_rpc_pending_provisions(&conn, NULL, 0));
}

TEST_F(RpcJoin, ProvidingACapabilityWeDoNotHostIsRefused)
{
	send_provide(&conn, 30, live + 40, 0x1234ULL);
	ASSERT_EQ(1u, out.frames.size());
	EXPECT_TRUE(read_l3(out.frames[0]).is_exception);
	EXPECT_EQ(0, capn_rpc_pending_provisions(&conn, NULL, 0));
}

TEST_F(RpcJoin, TwoProvisionsOfTheSameCapabilityAreIndependent)
{
	send_provide(&conn, 40, live, 0xaaaULL);
	send_provide(&conn, 41, live, 0xbbbULL);
	EXPECT_EQ(2, capn_rpc_pending_provisions(&conn, NULL, 0));
	out.frames.clear();

	send_accept(&conn, 42, 0xaaaULL);
	ASSERT_EQ(1u, out.frames.size());
	EXPECT_FALSE(read_l3(out.frames[0]).is_exception);
	/* Claiming one leaves the other standing. */
	EXPECT_EQ(1, capn_rpc_pending_provisions(&conn, NULL, 0));
}

void send_release(struct capn_rpc_conn *c, uint32_t id, uint32_t count)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	struct Release rel;
	memset(&rel, 0, sizeof rel);
	rel.id = id;
	rel.referenceCount = count;
	Release_ptr rp = new_Release(cs);
	write_Release(&rel, rp);

	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_release;
	m.release = rp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	uint8_t buf[512];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	ASSERT_EQ(0, capn_rpc_handle(c, buf, static_cast<size_t>(sz)));
}

TEST_F(RpcJoin, ReleasingMoreReferencesThanHeldAborts)
{
	ASSERT_TRUE(conn.exports[live].used);
	int held = conn.exports[live].refcount;
	send_release(&conn, live, 0xffffffffu);
	EXPECT_TRUE(capn_rpc_is_aborted(&conn));
	EXPECT_TRUE(conn.exports[live].used);
	EXPECT_EQ(held, conn.exports[live].refcount);
}

TEST_F(RpcJoin, ReleasingFewerReferencesKeepsTheExport)
{
	ASSERT_GE(conn.exports[live].refcount, 1);
	conn.exports[live].refcount = 3;
	send_release(&conn, live, 2);
	EXPECT_TRUE(conn.exports[live].used);
	EXPECT_EQ(1, conn.exports[live].refcount);
	send_release(&conn, live, 1);
	EXPECT_FALSE(conn.exports[live].used);
}

/* Alice arranges the handoff. Carol accepts on her own connection.
 * The Return belongs on Carol's connection, and Alice's extra
 * reference is gone once Carol has claimed the capability. */
TEST_F(RpcJoin, AcceptAfterTheIntroducerIsFreed)
{
	struct capn_rpc_vat vat;
	struct capn_rpc_conn alice, carol;
	Outbox alice_out, carol_out;
	const uint64_t nonce = 0x111ULL;

	memset(&vat, 0, sizeof vat);
	capn_rpc_init(&alice, collect, &alice_out);
	capn_rpc_init(&carol, collect, &carol_out);
	capn_rpc_set_vat(&alice, &vat);
	capn_rpc_set_vat(&carol, &vat);
	capn_rpc_set_bootstrap(&alice, &calls, counting_dispatch);
	uint32_t exp = bootstrap_export(&alice, &alice_out);
	send_provide(&alice, 80, exp, nonce);
	capn_rpc_free(&alice);
	send_accept_embargoed(&carol, 81, nonce);
	EXPECT_EQ(0u, carol_out.frames.size());
	send_disembargo_provide(&carol, 80);
	capn_rpc_free(&carol);
}

TEST_F(RpcJoin, DisembargoAfterTheAcceptorIsFreed)
{
	struct capn_rpc_vat vat;
	struct capn_rpc_conn alice, carol;
	Outbox alice_out, carol_out;
	const uint64_t nonce = 0x222ULL;

	memset(&vat, 0, sizeof vat);
	capn_rpc_init(&alice, collect, &alice_out);
	capn_rpc_init(&carol, collect, &carol_out);
	capn_rpc_set_vat(&alice, &vat);
	capn_rpc_set_vat(&carol, &vat);
	capn_rpc_set_bootstrap(&alice, &calls, counting_dispatch);
	uint32_t exp = bootstrap_export(&alice, &alice_out);
	send_provide(&alice, 80, exp, nonce);
	send_accept_embargoed(&carol, 81, nonce);
	capn_rpc_free(&carol);
	alice_out.frames.clear();
	send_disembargo_provide(&alice, 80);
	EXPECT_EQ(0u, alice_out.frames.size());
	capn_rpc_free(&alice);
}

TEST_F(RpcJoin, EmbargoedAcceptIsAnsweredOnTheAcceptingConnection)
{
	struct capn_rpc_vat vat;
	struct capn_rpc_conn alice, carol;
	Outbox alice_out, carol_out;
	const uint64_t nonce = 0xca201ULL;
	uint32_t exp;

	memset(&vat, 0, sizeof vat);
	memset(&alice, 0, sizeof alice);
	memset(&carol, 0, sizeof carol);
	capn_rpc_init(&alice, collect, &alice_out);
	capn_rpc_init(&carol, collect, &carol_out);
	capn_rpc_set_vat(&alice, &vat);
	capn_rpc_set_vat(&carol, &vat);
	capn_rpc_set_bootstrap(&alice, &calls, counting_dispatch);
	exp = bootstrap_export(&alice, &alice_out);
	ASSERT_EQ(1, alice.exports[exp].refcount);

	send_provide(&alice, 80, exp, nonce);
	EXPECT_EQ(2, alice.exports[exp].refcount);
	alice_out.frames.clear();

	send_accept_embargoed(&carol, 81, nonce);
	EXPECT_EQ(0u, carol_out.frames.size());
	EXPECT_EQ(0u, alice_out.frames.size());
	EXPECT_EQ(1, alice.exports[exp].refcount);

	send_disembargo_provide(&alice, 80);
	EXPECT_EQ(0u, alice_out.frames.size());
	ASSERT_EQ(1u, carol_out.frames.size());
	{
		L3Reply r = read_l3(carol_out.frames[0]);
		EXPECT_EQ(81u, r.answer_id);
		EXPECT_FALSE(r.is_exception);
		EXPECT_TRUE(r.has_cap);
	}
	capn_rpc_free(&alice);
	capn_rpc_free(&carol);
}

struct PipePark {
	int armed;
	struct capn_rpc_conn *conn;
	uint32_t question;
	uint32_t generation;
	capn_ptr results;
};

PipePark g_pipe;
int g_later_calls;

int later_dispatch(void *, uint64_t, uint16_t, capn_ptr, capn_ptr results)
{
	g_later_calls++;
	capn_write32(results, 0, 5);
	return 0;
}

int pipe_dispatch(void *, uint64_t, uint16_t method, capn_ptr, capn_ptr results)
{
	if (method == 0 && g_pipe.armed) {
		g_pipe.question = capn_rpc_current_question(g_pipe.conn);
		g_pipe.generation = capn_rpc_current_generation(g_pipe.conn);
		g_pipe.results = results;
		g_pipe.armed = 0;
		return CAPN_RPC_PENDING;
	}
	capn_write32(results, 0, 1);
	return 0;
}

void send_raw(struct capn_rpc_conn *c, struct capn *msg)
{
	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(msg, buf, sizeof buf, 0);
	ASSERT_GT(sz, 0);
	ASSERT_EQ(0, capn_rpc_handle(c, buf, static_cast<size_t>(sz)));
}

void send_method(struct capn_rpc_conn *c, uint32_t qid, uint32_t export_id, uint16_t method)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct MessageTarget t;
	memset(&t, 0, sizeof t);
	t.which = MessageTarget_importedCap;
	t.importedCap = export_id;
	MessageTarget_ptr tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);
	struct Call call;
	memset(&call, 0, sizeof call);
	call.questionId = qid;
	call.target = tp;
	call.methodId = method;
	Call_ptr cp = new_Call(cs);
	write_Call(&call, cp);
	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_call;
	m.call = cp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	send_raw(c, &msg);
	capn_free(&msg);
}

void send_pipeline_call(struct capn_rpc_conn *c, uint32_t qid, uint32_t promise_qid)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct PromisedAnswer_Op op;
	memset(&op, 0, sizeof op);
	op.which = PromisedAnswer_Op_getPointerField;
	op.getPointerField = 0;
	PromisedAnswer_Op_list ol = new_PromisedAnswer_Op_list(cs, 1);
	set_PromisedAnswer_Op(&op, ol, 0);
	struct PromisedAnswer pa;
	memset(&pa, 0, sizeof pa);
	pa.questionId = promise_qid;
	pa.transform = ol;
	PromisedAnswer_ptr pap = new_PromisedAnswer(cs);
	write_PromisedAnswer(&pa, pap);
	struct MessageTarget t;
	memset(&t, 0, sizeof t);
	t.which = MessageTarget_promisedAnswer;
	t.promisedAnswer = pap;
	MessageTarget_ptr tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);
	struct Call call;
	memset(&call, 0, sizeof call);
	call.questionId = qid;
	call.target = tp;
	call.methodId = 1;
	Call_ptr cp = new_Call(cs);
	write_Call(&call, cp);
	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_call;
	m.call = cp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	send_raw(c, &msg);
	capn_free(&msg);
}

/* A call pipelined onto an answer that has not been returned yet must
 * wait. Answering it at once with "no such export" drops the call. */
TEST_F(RpcJoin, PipelineOntoAPendingAnswerWaits)
{
	struct capn_rpc_conn vat;
	Outbox box;
	uint32_t exp;

	memset(&vat, 0, sizeof vat);
	memset(&g_pipe, 0, sizeof g_pipe);
	g_later_calls = 0;
	g_pipe.armed = 1;
	g_pipe.conn = &vat;
	capn_rpc_init(&vat, collect, &box);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_set_bootstrap(&vat, &calls, pipe_dispatch);
	exp = bootstrap_export(&vat, &box);

	send_method(&vat, 10, exp, 0);
	EXPECT_EQ(0u, box.frames.size()) << "the parked call has no Return yet";

	send_pipeline_call(&vat, 11, 10);
	EXPECT_EQ(0u, box.frames.size()) << "a pipeline onto a pending answer waits";

	{
		capn_ptr cap = capn_rpc_new_export_cap(&vat, g_pipe.results.seg,
		                                       &g_later_calls, later_dispatch);
		ASSERT_EQ(CAPN_INTERFACE, cap.type);
		ASSERT_EQ(0, capn_setp(g_pipe.results, 0, cap));
	}
	ASSERT_EQ(0, capn_rpc_return(&vat, g_pipe.question, g_pipe.generation));
	ASSERT_EQ(2u, box.frames.size());
	EXPECT_EQ(1, g_later_calls);
	{
		L3Reply second = read_l3(box.frames[1]);
		EXPECT_EQ(11u, second.answer_id);
		EXPECT_FALSE(second.is_exception);
	}
	capn_rpc_free(&vat);
}

static void send_finish(struct capn_rpc_conn *c, uint32_t qid)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct Finish fin;
	memset(&fin, 0, sizeof fin);
	fin.questionId = qid;
	fin.releaseResultCaps = 1;
	Finish_ptr fp = new_Finish(cs);
	write_Finish(&fin, fp);
	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_finish;
	m.finish = fp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	send_raw(c, &msg);
	capn_free(&msg);
}

static int return_which(const std::vector<uint8_t> &frame, uint32_t *answer, std::string *reason)
{
	struct capn msg;
	if (capn_init_mem(&msg, frame.data(), frame.size(), 0) != 0)
		return -1;
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	struct Message m;
	read_Message(&m, mp);
	if (m.which == Message_abort) {
		capn_free(&msg);
		return 1;
	}
	if (m.which != Message__return) {
		capn_free(&msg);
		return -1;
	}
	struct Return r;
	read_Return(&r, m._return);
	if (answer)
		*answer = r.answerId;
	if (reason && r.which == Return_exception) {
		struct Exception ex;
		read_Exception(&ex, r.exception);
		if (ex.reason.str && ex.reason.len > 0)
			reason->assign(ex.reason.str, ex.reason.str + ex.reason.len);
	}
	int which = (int) r.which;
	capn_free(&msg);
	return which;
}

TEST_F(RpcJoin, FinishForAQueuedCallIsCanceled)
{
	struct capn_rpc_conn vat;
	Outbox box;
	uint32_t exp, answer = 0;

	memset(&vat, 0, sizeof vat);
	memset(&g_pipe, 0, sizeof g_pipe);
	g_later_calls = 0;
	g_pipe.armed = 1;
	g_pipe.conn = &vat;
	capn_rpc_init(&vat, collect, &box);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_set_bootstrap(&vat, &calls, pipe_dispatch);
	exp = bootstrap_export(&vat, &box);

	send_method(&vat, 10, exp, 0);
	send_pipeline_call(&vat, 11, 10);
	box.frames.clear();
	send_finish(&vat, 11);
	ASSERT_EQ(1u, box.frames.size());
	EXPECT_EQ(Return_canceled, return_which(box.frames[0], &answer, NULL));
	EXPECT_EQ(11u, answer);
	EXPECT_EQ(0, g_later_calls);

	{
		capn_ptr cap = capn_rpc_new_export_cap(&vat, g_pipe.results.seg,
		                                       &g_later_calls, later_dispatch);
		ASSERT_EQ(CAPN_INTERFACE, cap.type);
		ASSERT_EQ(0, capn_setp(g_pipe.results, 0, cap));
	}
	ASSERT_EQ(0, capn_rpc_return(&vat, g_pipe.question, g_pipe.generation));
	EXPECT_EQ(0, g_later_calls);
	send_method(&vat, 11, exp, 1);
	EXPECT_FALSE(capn_rpc_is_aborted(&vat));
	capn_rpc_free(&vat);
}

TEST_F(RpcJoin, NinePipelinedCallsAllRun)
{
	struct capn_rpc_conn vat;
	Outbox box;
	uint32_t exp;
	int i;

	memset(&vat, 0, sizeof vat);
	memset(&g_pipe, 0, sizeof g_pipe);
	g_later_calls = 0;
	g_pipe.armed = 1;
	g_pipe.conn = &vat;
	capn_rpc_init(&vat, collect, &box);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_set_bootstrap(&vat, &calls, pipe_dispatch);
	exp = bootstrap_export(&vat, &box);
	send_method(&vat, 10, exp, 0);
	for (i = 0; i < 9; i++)
		send_pipeline_call(&vat, (uint32_t) (11 + i), 10);
	EXPECT_EQ(0u, box.frames.size());
	{
		capn_ptr cap = capn_rpc_new_export_cap(&vat, g_pipe.results.seg,
		                                       &g_later_calls, later_dispatch);
		ASSERT_EQ(CAPN_INTERFACE, cap.type);
		ASSERT_EQ(0, capn_setp(g_pipe.results, 0, cap));
	}
	ASSERT_EQ(0, capn_rpc_return(&vat, g_pipe.question, g_pipe.generation));
	EXPECT_EQ(9, g_later_calls);
	EXPECT_EQ(10u, box.frames.size());
	capn_rpc_free(&vat);
}

static int g_order[4];
static int g_order_n;
static int order_dispatch(void *, uint64_t, uint16_t, capn_ptr, capn_ptr);

static int middle_dispatch(void *, uint64_t, uint16_t, capn_ptr, capn_ptr results)
{
	if (g_order_n < 4)
		g_order[g_order_n++] = (int) capn_rpc_current_question(g_pipe.conn);
	capn_ptr cap = capn_rpc_new_export_cap(g_pipe.conn, results.seg,
	                                       &g_later_calls, order_dispatch);
	if (cap.type != CAPN_INTERFACE)
		return -1;
	if (capn_setp(results, 0, cap) != 0)
		return -1;
	return 0;
}

static int order_dispatch(void *s, uint64_t a, uint16_t b, capn_ptr p, capn_ptr results)
{
	if (g_order_n < 4)
		g_order[g_order_n++] = (int) capn_rpc_current_question(g_pipe.conn);
	return later_dispatch(s, a, b, p, results);
}

TEST_F(RpcJoin, PipelinedCallsRunInArrivalOrder)
{
	struct capn_rpc_conn vat;
	Outbox box;
	uint32_t exp;

	memset(&vat, 0, sizeof vat);
	memset(&g_pipe, 0, sizeof g_pipe);
	memset(g_order, 0, sizeof g_order);
	g_order_n = 0;
	g_later_calls = 0;
	g_pipe.armed = 1;
	g_pipe.conn = &vat;
	capn_rpc_init(&vat, collect, &box);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_set_bootstrap(&vat, &calls, pipe_dispatch);
	exp = bootstrap_export(&vat, &box);
	send_method(&vat, 10, exp, 0);
	send_pipeline_call(&vat, 11, 10);
	send_pipeline_call(&vat, 12, 10);
	{
		capn_ptr cap = capn_rpc_new_export_cap(&vat, g_pipe.results.seg,
		                                       &g_later_calls, order_dispatch);
		ASSERT_EQ(0, capn_setp(g_pipe.results, 0, cap));
	}
	ASSERT_EQ(0, capn_rpc_return(&vat, g_pipe.question, g_pipe.generation));
	ASSERT_EQ(2, g_order_n);
	EXPECT_EQ(11, g_order[0]);
	EXPECT_EQ(12, g_order[1]);
	capn_rpc_free(&vat);
}

TEST_F(RpcJoin, PipelineOntoADeferredCallWaits)
{
	struct capn_rpc_conn vat;
	Outbox box;
	uint32_t exp, answer = 0;
	std::string reason;

	memset(&vat, 0, sizeof vat);
	memset(&g_pipe, 0, sizeof g_pipe);
	memset(g_order, 0, sizeof g_order);
	g_order_n = 0;
	g_later_calls = 0;
	g_pipe.armed = 1;
	g_pipe.conn = &vat;
	capn_rpc_init(&vat, collect, &box);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_set_bootstrap(&vat, &calls, pipe_dispatch);
	exp = bootstrap_export(&vat, &box);
	send_method(&vat, 10, exp, 0);
	send_pipeline_call(&vat, 11, 10);
	send_pipeline_call(&vat, 12, 11);
	EXPECT_EQ(0u, box.frames.size());
	{
		capn_ptr cap = capn_rpc_new_export_cap(&vat, g_pipe.results.seg,
		                                       &g_later_calls, middle_dispatch);
		ASSERT_EQ(0, capn_setp(g_pipe.results, 0, cap));
	}
	ASSERT_EQ(0, capn_rpc_return(&vat, g_pipe.question, g_pipe.generation));
	ASSERT_EQ(2, g_order_n);
	EXPECT_EQ(11, g_order[0]);
	EXPECT_EQ(12, g_order[1]);
	for (size_t i = 0; i < box.frames.size(); i++) {
		reason.clear();
		int which = return_which(box.frames[i], &answer, &reason);
		if (answer == 12) {
			EXPECT_NE(Return_exception, which) << reason;
		}
	}
	capn_rpc_free(&vat);
}

TEST_F(RpcJoin, PendingExceptionIsSharedWithThePipeline)
{
	struct capn_rpc_conn vat;
	Outbox box;
	uint32_t exp, answer = 0;
	int seen = 0;
	std::string reason;

	memset(&vat, 0, sizeof vat);
	memset(&g_pipe, 0, sizeof g_pipe);
	g_pipe.armed = 1;
	g_pipe.conn = &vat;
	capn_rpc_init(&vat, collect, &box);
	capn_rpc_set_result_size(&vat, 8, 1);
	capn_rpc_set_bootstrap(&vat, &calls, pipe_dispatch);
	exp = bootstrap_export(&vat, &box);
	send_method(&vat, 10, exp, 0);
	send_pipeline_call(&vat, 11, 10);
	ASSERT_EQ(0, capn_rpc_return_exception(&vat, g_pipe.question, g_pipe.generation,
	                                      "slow failed"));
	ASSERT_EQ(2u, box.frames.size());
	for (size_t i = 0; i < box.frames.size(); i++) {
		reason.clear();
		EXPECT_EQ(Return_exception, return_which(box.frames[i], &answer, &reason));
		EXPECT_EQ("slow failed", reason);
		seen |= answer == 10 ? 1 : answer == 11 ? 2 : 4;
	}
	EXPECT_EQ(3, seen);
	capn_rpc_free(&vat);
}

static void send_results_to(struct capn_rpc_conn *c, uint32_t qid, uint32_t export_id,
                            int which)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct MessageTarget t;
	memset(&t, 0, sizeof t);
	t.which = MessageTarget_importedCap;
	t.importedCap = export_id;
	MessageTarget_ptr tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);
	struct Call call;
	memset(&call, 0, sizeof call);
	call.questionId = qid;
	call.target = tp;
	call.sendResultsTo_which = (enum Call_sendResultsTo_which) which;
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
	capn_rpc_handle(c, buf, static_cast<size_t>(sz));
}

TEST_F(RpcJoin, ThirdPartyResultsAbort)
{
	out.frames.clear();
	send_results_to(&conn, 20, live, Call_sendResultsTo_thirdParty);
	EXPECT_TRUE(capn_rpc_is_aborted(&conn));
	ASSERT_FALSE(out.frames.empty());
	EXPECT_EQ(1, return_which(out.frames.back(), NULL, NULL));
}

} // namespace
