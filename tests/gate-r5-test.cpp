/* Regression tests for the wire, reflection and RPC fixes. */

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "capn-dyn.h"
#include "capn-rpc.h"
#include "capnp_c.h"
#include "rpc.capnp.h"

namespace {

struct Session {
	Session() { capn_init_malloc(&capn); }
	~Session() { capn_free(&capn); }
	struct capn capn;
};

static struct capn_segment *create_small(void *, uint32_t, int sz)
{
	sz += (int)sizeof(struct capn_segment);
	struct capn_segment *s = (struct capn_segment *)calloc(1, (size_t)sz);
	if (s == NULL)
		return NULL;
	s->data = (char *)(s + 1);
	s->cap = (size_t)sz - sizeof(*s);
	s->user = s;
	return s;
}

static int g_created;

static struct capn_segment *create_flag(void *, uint32_t, int)
{
	g_created = 1;
	return NULL;
}

TEST(WireFormat, SetpDeepCopiesAFarPointerInsteadOfAliasingIt)
{
	Session ctx;
	ctx.capn.create = create_small;

	capn_ptr root = capn_new_struct(capn_root(&ctx.capn).seg, 8, 2);
	ASSERT_EQ(CAPN_STRUCT, root.type);
	ASSERT_EQ(0, capn_set_root(&ctx.capn, root));
	capn_text word = {3, "far", NULL};
	ASSERT_EQ(0, capn_set_text(root, 0, word));

	capn_ptr slot = capn_getp(root, 0, 0);
	ASSERT_EQ(CAPN_FAR_POINTER, slot.type);
	ASSERT_EQ(0, capn_setp(root, 1, slot));

	capn_text def = {0, NULL, NULL};
	capn_text orig = capn_get_text(root, 0, def);
	capn_text copy = capn_get_text(root, 1, def);
	ASSERT_EQ(3, orig.len);
	ASSERT_EQ(3, copy.len);
	EXPECT_NE(orig.str, copy.str);
	const_cast<char *>(orig.str)[0] = 'X';
	EXPECT_EQ('f', copy.str[0]);
}

TEST(WireFormat, SetpDropsAFarPointerThatDoesNotResolve)
{
	Session ctx;
	capn_ptr root = capn_new_struct(capn_root(&ctx.capn).seg, 0, 2);
	ASSERT_EQ(CAPN_STRUCT, root.type);
	uint64_t bad = capn_flip64(6ull | (0xffffull << 32));
	memcpy(root.data + root.datasz, &bad, sizeof bad);

	capn_ptr slot = capn_getp(root, 0, 0);
	ASSERT_EQ(CAPN_FAR_POINTER, slot.type);
	ASSERT_EQ(0, capn_setp(root, 1, slot));

	uint64_t dst = 1;
	memcpy(&dst, root.data + root.datasz + 8, sizeof dst);
	EXPECT_EQ(0ull, dst);
}

TEST(WireFormat, NewStringRejectsALengthPastThe29BitLimit)
{
	Session ctx;
	struct capn_segment *seg = capn_root(&ctx.capn).seg;
	ASSERT_TRUE(seg != NULL);
	ctx.capn.create = create_flag;
	g_created = 0;
	/* One past the longest list the wire can name. The bytes are not
	 * read: the length is rejected before a segment is asked for. */
	char dummy = 'a';
	capn_ptr p = capn_new_string(seg, &dummy, (ssize_t)0x1fffffff);
	EXPECT_EQ(CAPN_NULL, p.type);
	EXPECT_EQ(0, g_created);
}

static struct capn_dyn_struct text_struct()
{
	static struct capn_dyn_field field;
	static struct capn_dyn_struct desc;
	static int ready;
	if (!ready) {
		memset(&field, 0, sizeof field);
		field.name = "name";
		field.type.kind = CAPN_DYN_TEXT;
		field.discriminant = CAPN_DYN_NO_DISCRIMINANT;
		memset(&desc, 0, sizeof desc);
		desc.name = "P";
		desc.pointers = 1;
		desc.nfields = 1;
		desc.fields = &field;
		ready = 1;
	}
	return desc;
}

TEST(Dyn, DecodeRejectsANullBufferWithALength)
{
	Session ctx;
	struct capn_dyn_struct desc = text_struct();
	struct capn_dyn_error err;
	capn_ptr out;
	memset(&err, 0, sizeof err);
	memset(&out, 0, sizeof out);
	EXPECT_EQ(-1, capn_dyn_from_text(&desc, NULL, 4, capn_root(&ctx.capn).seg,
	                                 &out, &err));
	EXPECT_NE(nullptr, strstr(err.message, "bad arguments"));
}

TEST(Dyn, DecodeAcceptsAnEmptyObject)
{
	Session ctx;
	struct capn_dyn_struct desc = text_struct();
	struct capn_dyn_error err;
	capn_ptr out;
	const char *text = "()";
	memset(&err, 0, sizeof err);
	ASSERT_EQ(0, capn_dyn_from_text(&desc, text, 2, capn_root(&ctx.capn).seg,
	                                &out, &err))
		<< err.message;
	EXPECT_EQ(CAPN_STRUCT, out.type);
}

struct Wire {
	std::deque<std::vector<uint8_t> > to_client;
	std::deque<std::vector<uint8_t> > to_vat;
};

Wire *g_wire;
int g_fail_send_at;
int g_sends;

int send_to_vat(void *, const uint8_t *d, size_t n)
{
	g_sends++;
	if (g_fail_send_at > 0 && g_sends == g_fail_send_at)
		return -1;
	g_wire->to_vat.push_back(std::vector<uint8_t>(d, d + n));
	return 0;
}

int send_to_client(void *, const uint8_t *d, size_t n)
{
	g_wire->to_client.push_back(std::vector<uint8_t>(d, d + n));
	return 0;
}

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

int noop_dispatch(void *, uint64_t, uint16_t, capn_ptr, capn_ptr)
{
	return 0;
}

struct capn_rpc_conn *g_vat;
int g_server;
uint32_t g_vat_question;
uint32_t g_vat_gen;
capn_ptr g_park_results;

int export_pending_dispatch(void *, uint64_t, uint16_t, capn_ptr, capn_ptr results)
{
	capn_ptr cap = capn_rpc_new_export_cap(g_vat, results.seg, &g_server, noop_dispatch);
	if (cap.type != CAPN_INTERFACE)
		return -1;
	if (capn_setp(results, 0, cap) != 0)
		return -1;
	g_vat_question = capn_rpc_current_question(g_vat);
	g_vat_gen = capn_rpc_current_generation(g_vat);
	return CAPN_RPC_PENDING;
}

int park_dispatch(void *, uint64_t, uint16_t, capn_ptr, capn_ptr results)
{
	g_park_results = results;
	g_vat_question = capn_rpc_current_question(g_vat);
	g_vat_gen = capn_rpc_current_generation(g_vat);
	return CAPN_RPC_PENDING;
}

void fill_grow(void *ctx, capn_ptr)
{
	struct capn_rpc_conn *c = static_cast<struct capn_rpc_conn *>(ctx);
	capn_rpc_send_bootstrap(c);
}

bool message_has_text(const std::vector<uint8_t> &frame, const char *text)
{
	struct capn msg;
	if (capn_init_mem(&msg, frame.data(), frame.size(), 0) != 0)
		return false;
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	struct Message m;
	read_Message(&m, mp);
	bool found = false;
	if (m.which == Message__return) {
		struct Return r;
		read_Return(&r, m._return);
		if (r.which == Return_exception) {
			struct Exception ex;
			read_Exception(&ex, r.exception);
			if (ex.reason.str != NULL &&
			    strstr(ex.reason.str, text) != NULL)
				found = true;
		}
	} else if (m.which == Message_abort) {
		struct Exception ex;
		read_Exception(&ex, m.abort);
		if (ex.reason.str != NULL && strstr(ex.reason.str, text) != NULL)
			found = true;
	}
	capn_free(&msg);
	return found;
}

int return_sender_hosted(const std::vector<uint8_t> &frame, uint32_t *id)
{
	struct capn msg;
	if (capn_init_mem(&msg, frame.data(), frame.size(), 0) != 0)
		return -1;
	Message_ptr mp;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	struct Message m;
	struct Return r;
	struct Payload pl;
	struct CapDescriptor cd;
	int rc = -1;
	read_Message(&m, mp);
	if (m.which != Message__return)
		goto out;
	read_Return(&r, m._return);
	if (r.which != Return_results)
		goto out;
	read_Payload(&pl, r.results);
	if (capn_len(pl.capTable) < 1)
		goto out;
	get_CapDescriptor(&cd, pl.capTable, 0);
	if (cd.which != CapDescriptor_senderHosted)
		goto out;
	*id = cd.senderHosted;
	rc = 0;
out:
	capn_free(&msg);
	return rc;
}

class GateRpc : public ::testing::Test {
 protected:
	void SetUp() override
	{
		wire = Wire();
		g_wire = &wire;
		g_sends = 0;
		g_fail_send_at = 0;
		g_server = 0;
		capn_rpc_init(&client, send_to_vat, NULL);
		capn_rpc_init(&vat, send_to_client, NULL);
		g_vat = &vat;
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

TEST_F(GateRpc, ExceptionReasonIsCopiedIntoTheMessage)
{
	capn_rpc_set_bootstrap(&vat, &g_server, noop_dispatch);
	uint32_t q = capn_rpc_send_call(&client, 99, 1, 0, 0, 0, NULL, NULL);
	ASSERT_NE((uint32_t)-1, q);
	ASSERT_GT(pump(&vat, wire.to_vat), 0);
	ASSERT_FALSE(wire.to_client.empty());
	EXPECT_TRUE(message_has_text(wire.to_client.back(), "no such export"));
}

TEST_F(GateRpc, PendingReturnKeepsTheCapabilityFromDispatch)
{
	capn_rpc_set_bootstrap(&vat, &g_server, export_pending_dispatch);
	capn_rpc_set_result_size(&vat, 0, 1);
	uint32_t boot = capn_rpc_send_bootstrap(&client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, boot));
	uint32_t q = capn_rpc_send_call(&client, 0, 1, 0, 0, 0, NULL, NULL);
	pump(&vat, wire.to_vat);
	ASSERT_EQ(1u, g_vat_question);
	ASSERT_EQ(0, capn_rpc_return(&vat, g_vat_question, g_vat_gen));
	ASSERT_FALSE(wire.to_client.empty());
	std::vector<uint8_t> returned = wire.to_client.back();
	pump(&client, wire.to_client);
	ASSERT_TRUE(capn_rpc_is_answered(&client, q));
	ASSERT_FALSE(capn_rpc_is_failed(&client, q));
	uint32_t id = 0;
	ASSERT_EQ(0, return_sender_hosted(returned, &id));
	EXPECT_EQ(1u, id);
}

TEST_F(GateRpc, LiveQuestionIdAborts)
{
	capn_rpc_set_bootstrap(&vat, &g_server, park_dispatch);
	capn_rpc_set_result_size(&vat, 8, 1);
	uint32_t boot = capn_rpc_send_bootstrap(&client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, boot));
	uint32_t q = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, NULL, NULL);
	pump(&vat, wire.to_vat);
	ASSERT_FALSE(capn_rpc_is_aborted(&vat));
	client.next_question_id = q;
	uint32_t q2 = capn_rpc_send_call(&client, 0, 1, 0, 8, 0, NULL, NULL);
	ASSERT_EQ(q, q2);
	pump(&vat, wire.to_vat);
	ASSERT_FALSE(wire.to_client.empty());
	EXPECT_TRUE(message_has_text(wire.to_client.back(),
	                             "questionId is already in use"));
	EXPECT_TRUE(capn_rpc_is_aborted(&vat));
}

TEST_F(GateRpc, FinishReleaseResultCapsDropsTheExport)
{
	capn_rpc_set_bootstrap(&vat, &g_server, export_pending_dispatch);
	capn_rpc_set_result_size(&vat, 0, 1);
	uint32_t boot = capn_rpc_send_bootstrap(&client);
	settle();
	ASSERT_TRUE(capn_rpc_is_answered(&client, boot));
	uint32_t q = capn_rpc_send_call(&client, 0, 1, 0, 0, 0, NULL, NULL);
	pump(&vat, wire.to_vat);
	ASSERT_EQ(0, capn_rpc_return(&vat, g_vat_question, g_vat_gen));
	ASSERT_FALSE(wire.to_client.empty());
	std::vector<uint8_t> returned = wire.to_client.back();
	pump(&client, wire.to_client);
	uint32_t id = 0;
	ASSERT_EQ(0, return_sender_hosted(returned, &id));

	struct capn msg;
	struct Message m;
	struct Finish fin;
	Message_ptr mp;
	Finish_ptr fp;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&fin, 0, sizeof fin);
	fin.questionId = q;
	fin.releaseResultCaps = 1;
	fp = new_Finish(cs);
	write_Finish(&fin, fp);
	mp = new_Message(cs);
	m.which = Message_finish;
	m.finish = fp;
	write_Message(&m, mp);
	ASSERT_EQ(0, capn_setp(capn_root(&msg), 0, mp.p));
	uint8_t buf[512];
	int64_t n = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(n, 0);
	ASSERT_EQ(0, capn_rpc_handle(&vat, buf, (size_t)n));

	/* The export was the call's only reference. A later call naming it
	 * has nothing to dispatch. */
	wire.to_client.clear();
	uint32_t again = capn_rpc_send_call(&client, id, 1, 0, 0, 0, NULL, NULL);
	pump(&vat, wire.to_vat);
	ASSERT_FALSE(wire.to_client.empty());
	EXPECT_TRUE(message_has_text(wire.to_client.back(), "no such export"));
	(void)again;
}

TEST_F(GateRpc, AnswerCapIdKeepsTheHighBit)
{
	uint32_t q = capn_rpc_send_bootstrap(&client);
	ASSERT_NE((uint32_t)-1, q);

	struct capn msg;
	struct Message m;
	struct Return r;
	struct Payload pl;
	struct CapDescriptor cd;
	Message_ptr mp;
	Return_ptr rp;
	Payload_ptr plp;
	capn_init_malloc(&msg);
	struct capn_segment *cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&pl, 0, sizeof pl);
	memset(&cd, 0, sizeof cd);
	plp = new_Payload(cs);
	pl.capTable = new_CapDescriptor_list(cs, 1);
	cd.which = CapDescriptor_senderHosted;
	cd.senderHosted = 0x80000000u;
	set_CapDescriptor(&cd, pl.capTable, 0);
	pl.content = capn_new_interface(cs, 0, 0);
	pl.content.len = 0;
	pl.content.cap_idx = 0;
	write_Payload(&pl, plp);
	rp = new_Return(cs);
	r.answerId = q;
	r.which = Return_results;
	r.results = plp;
	write_Return(&r, rp);
	mp = new_Message(cs);
	m.which = Message__return;
	m._return = rp;
	write_Message(&m, mp);
	ASSERT_EQ(0, capn_setp(capn_root(&msg), 0, mp.p));
	uint8_t buf[1024];
	int64_t n = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(n, 0);
	ASSERT_EQ(0, capn_rpc_handle(&client, buf, (size_t)n));
	EXPECT_EQ((int64_t)0x80000000u, capn_rpc_answer_cap_id(&client, q));
}

TEST_F(GateRpc, FillThatGrowsTheQuestionTableDoesNotUseTheOldPointer)
{
	/* Eight slots, then a fill that claims one more. The send of the
	 * outer call fails, which is the path that used to clear the
	 * question through the pointer taken before the grow. */
	g_fail_send_at = 9;
	for (int i = 0; i < 7; i++) {
		uint32_t q = capn_rpc_send_bootstrap(&client);
		ASSERT_NE((uint32_t)-1, q);
	}
	uint32_t q = capn_rpc_send_call(&client, 0, 1, 0, 0, 0, fill_grow, &client);
	EXPECT_EQ((uint32_t)-1, q);
	EXPECT_FALSE(capn_rpc_is_aborted(&client));
}

static void put_u32(uint8_t *p, uint32_t v)
{
	p[0] = (uint8_t)v;
	p[1] = (uint8_t)(v >> 8);
	p[2] = (uint8_t)(v >> 16);
	p[3] = (uint8_t)(v >> 24);
}

static void put_u64(uint8_t *p, uint64_t v)
{
	for (int i = 0; i < 8; i++)
		p[i] = (uint8_t)(v >> (8 * i));
}

static std::vector<uint8_t> one_segment(const std::vector<uint64_t> &words)
{
	std::vector<uint8_t> f(8 + words.size() * 8);
	put_u32(f.data(), 0);
	put_u32(f.data() + 4, (uint32_t)words.size());
	for (size_t i = 0; i < words.size(); i++)
		put_u64(f.data() + 8 + i * 8, words[i]);
	return f;
}

/* Zero-length list of 2, 4 or 8 byte elements. datasz is the element
 * width and the body is the next word, which is past a one-word segment. */
static uint64_t empty_list(unsigned elem_size)
{
	return 1ull | ((uint64_t)elem_size << 32);
}

static int drop_send(void *, const uint8_t *, size_t)
{
	return 0;
}

TEST(WireFormat, NonStructPointerReadsAsTheDefault)
{
	struct capn_rpc_conn conn;
	capn_rpc_init(&conn, drop_send, NULL);
	const unsigned sizes[] = {3, 4, 5};
	for (unsigned sz : sizes) {
		std::vector<uint8_t> built = one_segment({empty_list(sz)});
		uint8_t *root = (uint8_t *)malloc(built.size());
		ASSERT_NE(nullptr, root);
		memcpy(root, built.data(), built.size());
		EXPECT_EQ(0, capn_rpc_handle(&conn, root, built.size()));
		free(root);
		/* Unimplemented whose payload is that list, not a struct. */
		uint64_t msg = (1ull << 32) | (1ull << 48);
		built = one_segment({msg, 0, empty_list(sz)});
		uint8_t *unimpl = (uint8_t *)malloc(built.size());
		ASSERT_NE(nullptr, unimpl);
		memcpy(unimpl, built.data(), built.size());
		EXPECT_EQ(0, capn_rpc_handle(&conn, unimpl, built.size()));
		free(unimpl);
	}
	EXPECT_FALSE(capn_rpc_is_aborted(&conn));
	capn_rpc_free(&conn);

	uint8_t *tiny = (uint8_t *)malloc(1);
	ASSERT_NE(nullptr, tiny);
	tiny[0] = 0xab;
	capn_ptr p;
	memset(&p, 0, sizeof p);
	p.type = CAPN_LIST;
	p.data = (char *)tiny;
	p.datasz = 2;
	EXPECT_EQ(0, capn_read16(p, 0));
	p.datasz = 4;
	EXPECT_EQ(0, capn_read32(p, 0));
	p.datasz = 8;
	EXPECT_EQ(0, capn_read8(p, 0));
	EXPECT_EQ(0, capn_read64(p, 0));
	free(tiny);
}

TEST(WireFormat, CompositeListMayHaveTrailingPadding)
{
	/* Body is two words; the tag describes one 8-byte element. */
	uint64_t struct_ptr = 1ull << 48;
	uint64_t list_ptr = 1ull | (7ull << 32) | (2ull << 35);
	uint64_t tag = (1ull << 2) | (1ull << 32);
	uint64_t data = 0x1122334455667788ull;
	std::vector<uint8_t> frame = one_segment({struct_ptr, list_ptr, tag, data, 0});
	struct capn c;
	ASSERT_EQ(0, capn_init_mem(&c, frame.data(), frame.size(), 0));
	capn_ptr st = capn_getp(capn_root(&c), 0, 1);
	ASSERT_EQ(CAPN_STRUCT, st.type);
	capn_ptr list = capn_getp(st, 0, 1);
	EXPECT_EQ(1, capn_ok(&c));
	EXPECT_EQ(CAPN_LIST, list.type);
	EXPECT_EQ(1, list.len);
	capn_ptr el = capn_getp(list, 0, 1);
	EXPECT_EQ(CAPN_STRUCT, el.type);
	EXPECT_EQ(data, capn_read64(el, 0));
	capn_free(&c);

	uint64_t short_list = 1ull | (7ull << 32);
	std::vector<uint8_t> over = one_segment({struct_ptr, short_list, tag, data});
	ASSERT_EQ(0, capn_init_mem(&c, over.data(), over.size(), 0));
	st = capn_getp(capn_root(&c), 0, 1);
	(void)capn_getp(st, 0, 1);
	EXPECT_EQ(0, capn_ok(&c));
	capn_free(&c);
}

TEST(WireFormat, FarLandingPadIsChargedToTheTraversalLimit)
{
	/* seg0: far pointer to seg1. seg1: one-word pad then an 8-byte struct.
	 * A limit of one word fits the struct alone and not the pad as well. */
	uint8_t frame[16 + 8 + 16];
	memset(frame, 0, sizeof frame);
	put_u32(frame, 1);
	put_u32(frame + 4, 1);
	put_u32(frame + 8, 2);
	put_u64(frame + 16, 2ull | (1ull << 32));
	put_u64(frame + 24, 1ull << 32);
	put_u64(frame + 32, 0x99);
	struct capn c;
	ASSERT_EQ(0, capn_init_mem(&c, frame, sizeof frame, 0));
	c.traversal_limit = 8;
	capn_ptr root = capn_getp(capn_root(&c), 0, 1);
	EXPECT_EQ(0, capn_ok(&c));
	EXPECT_NE(CAPN_STRUCT, root.type);
	capn_free(&c);
}

static void send_release(struct capn_rpc_conn *conn, uint32_t id, uint32_t count)
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
	EXPECT_EQ(0, capn_setp(capn_root(&msg), 0, mp.p));
	uint8_t buf[512];
	int64_t n = capn_write_mem(&msg, buf, sizeof buf, 0);
	capn_free(&msg);
	ASSERT_GT(n, 0);
	ASSERT_EQ(0, capn_rpc_handle(conn, buf, (size_t)n));
}

TEST_F(GateRpc, OverReleaseAbortsAndLeavesTheExport)
{
	int id = capn_rpc_export(&vat, &g_server, noop_dispatch);
	ASSERT_GE(id, 0);
	ASSERT_EQ(1, vat.exports[id].refcount);
	send_release(&vat, (uint32_t)id, 2);
	EXPECT_TRUE(capn_rpc_is_aborted(&vat));
	EXPECT_EQ(1, vat.exports[id].used);
	EXPECT_EQ(1, vat.exports[id].refcount);
	EXPECT_FALSE(wire.to_client.empty());
}

TEST_F(GateRpc, ExactReleaseDropsTheExport)
{
	int id = capn_rpc_export(&vat, &g_server, noop_dispatch);
	ASSERT_GE(id, 0);
	send_release(&vat, (uint32_t)id, 1);
	EXPECT_FALSE(capn_rpc_is_aborted(&vat));
	EXPECT_EQ(0, vat.exports[id].used);
}

TEST_F(GateRpc, UnknownExportIdAborts)
{
	send_release(&vat, 50, 1);
	EXPECT_TRUE(capn_rpc_is_aborted(&vat));
}

}  /* namespace */
