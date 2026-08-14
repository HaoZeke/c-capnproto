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


/* A call whose target is `promisedAnswer`: the answer to
 * `answer_question_id`, optionally walked by getPointerField ops. */
void send_pipelined_call(struct capn_rpc_conn *c, uint32_t qid,
                         uint32_t answer_question_id,
                         const std::vector<uint16_t> &ops)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;

	struct PromisedAnswer pa;
	memset(&pa, 0, sizeof pa);
	pa.questionId = answer_question_id;
	pa.transform = new_PromisedAnswer_Op_list(cs, (int)ops.size());
	for (size_t i = 0; i < ops.size(); i++) {
		struct PromisedAnswer_Op op;
		memset(&op, 0, sizeof op);
		op.which = PromisedAnswer_Op_getPointerField;
		op.getPointerField = ops[i];
		set_PromisedAnswer_Op(&op, pa.transform, (int)i);
	}
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
	call.params = new_Payload(cs);
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

void send_finish(struct capn_rpc_conn *c, uint32_t qid)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct Finish f;
	memset(&f, 0, sizeof f);
	f.questionId = qid;
	Finish_ptr fp = new_Finish(cs);
	write_Finish(&f, fp);
	struct Message m;
	memset(&m, 0, sizeof m);
	m.which = Message_finish;
	m.finish = fp;
	Message_ptr mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	uint8_t buf[4096];
	ssize_t sz = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(sz, 0);
	ASSERT_EQ(0, capn_rpc_handle(c, buf, static_cast<size_t>(sz)));
}

/* Return union tag and, for results, the first u32 of the content. */
struct ReturnInfo {
	uint32_t answer_id;
	int which;
	uint32_t value;
};

ReturnInfo read_return(const std::vector<uint8_t> &frame)
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

	ReturnInfo out;
	out.answer_id = r.answerId;
	out.which = (int)r.which;
	out.value = 0;
	if (r.which == Return_results) {
		struct Payload pl;
		read_Payload(&pl, r.results);
		capn_ptr content = pl.content;
		capn_resolve(&content);
		if (content.type == CAPN_STRUCT)
			out.value = capn_read32(content, 0);
	}
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
};

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

} // namespace
