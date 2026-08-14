/* rpc-join-test.cpp
 *
 * Level 4 `Join`: does a set of capabilities name one object?
 *
 * The vat is driven with raw messages rather than through a client
 * facade, so each assertion is about the wire behaviour the spec
 * prescribes and not about a convenience layer on top of it.
 */

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "capn-rpc.h"
#include "capnp_c.h"
#include "rpc-twoparty.capnp.h"
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

TEST_F(RpcJoin, ProvideDrawsUnimplemented)
{
	struct capn msg;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	struct Provide pv;
	memset(&pv, 0, sizeof pv);
	pv.questionId = 60;
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
	ASSERT_EQ(0, capn_rpc_handle(&conn, buf, static_cast<size_t>(sz)));

	ASSERT_EQ(1u, out.frames.size());
	struct capn reply;
	ASSERT_EQ(0, capn_init_mem(&reply, out.frames[0].data(),
	                           out.frames[0].size(), 0));
	Message_ptr rmp;
	rmp.p = capn_getp(capn_root(&reply), 0, 1);
	struct Message rm;
	read_Message(&rm, rmp);
	EXPECT_EQ(Message_unimplemented, rm.which);
	capn_free(&reply);
}

} // namespace
