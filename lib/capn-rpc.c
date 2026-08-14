/* Two-party Cap'n Proto RPC vat. See capn-rpc.h. */

#include "capn-rpc.h"

#include <string.h>

#include "rpc.capnp.h"
#include "rpc-twoparty.capnp.h"

void capn_rpc_init(struct capn_rpc_conn *c, capn_rpc_send_fn send,
                   void *send_ctx)
{
	memset(c, 0, sizeof(*c));
	c->send = send;
	c->send_ctx = send_ctx;
}

void capn_rpc_set_bootstrap(struct capn_rpc_conn *c, void *server,
                            capn_rpc_dispatch_fn dispatch)
{
	c->bootstrap = server;
	c->bootstrap_dispatch = dispatch;
}

int capn_rpc_export(struct capn_rpc_conn *c, void *server,
                    capn_rpc_dispatch_fn dispatch)
{
	int i;
	/* Reuse the export already naming this server: the peer must see one
	 * id per object, or reference equality could not be answered. */
	for (i = 0; i < CAPN_RPC_MAX_EXPORTS; i++) {
		if (c->exports[i].used && c->exports[i].server == server) {
			c->exports[i].refcount++;
			return i;
		}
	}
	for (i = 0; i < CAPN_RPC_MAX_EXPORTS; i++) {
		if (!c->exports[i].used) {
			c->exports[i].used = 1;
			c->exports[i].refcount = 1;
			c->exports[i].server = server;
			c->exports[i].dispatch = dispatch;
			return i;
		}
	}
	return -1;
}

/* Resolve a MessageTarget to a local export index, or -1 when it names
 * nothing this vat hosts. Shared by Call and Join, which address
 * capabilities the same way. */
static int resolve_target(struct capn_rpc_conn *c, MessageTarget_ptr tp)
{
	struct MessageTarget t;
	if (tp.p.type == CAPN_NULL)
		return -1;
	read_MessageTarget(&t, tp);
	if (t.which != MessageTarget_importedCap)
		return -1;
	if (t.importedCap >= CAPN_RPC_MAX_EXPORTS)
		return -1;
	return c->exports[t.importedCap].used ? (int)t.importedCap : -1;
}

static int send_message(struct capn_rpc_conn *c, struct capn *msg)
{
	uint8_t buf[8192];
	ssize_t sz = capn_write_mem(msg, buf, sizeof buf, 0);
	if (sz < 0)
		return -1;
	return c->send(c->send_ctx, buf, (size_t)sz);
}

/* Write a one-entry capTable naming a senderHosted export, and point
 * `slot` at it. */
static void write_cap_payload(struct capn_segment *cs, struct Payload *pl,
                              int eid, capn_ptr *slot)
{
	struct CapDescriptor cd;
	pl->capTable = new_CapDescriptor_list(cs, 1);
	memset(&cd, 0, sizeof cd);
	cd.which = CapDescriptor_senderHosted;
	cd.senderHosted = (uint32_t)eid;
	set_CapDescriptor(&cd, pl->capTable, 0);
	/* A capability pointer carries the capTable index in `len`
	 * (encoding.html: A=3, B=0, C=index). */
	*slot = capn_new_interface(cs, 0, 0);
	slot->len = 0;
}

static int send_return_exception(struct capn_rpc_conn *c, uint32_t qid,
                                 const char *reason)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	struct Exception ex;
	Message_ptr mp;
	Return_ptr rp;
	Exception_ptr ep;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&ex, 0, sizeof ex);

	ep = new_Exception(cs);
	ex.reason.str = reason;
	ex.reason.len = (int)strlen(reason);
	ex.reason.seg = cs;
	ex.type = Exception_Type_failed;
	write_Exception(&ex, ep);

	rp = new_Return(cs);
	r.answerId = qid;
	r.releaseParamCaps = 0;
	r.which = Return_exception;
	r.exception = ep;
	write_Return(&r, rp);

	mp = new_Message(cs);
	m.which = Message__return;
	m._return = rp;
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

static int handle_bootstrap(struct capn_rpc_conn *c, Bootstrap_ptr bp)
{
	struct capn msg;
	struct Bootstrap b;
	struct Message m;
	struct Return r;
	struct Payload pl;
	Message_ptr mp;
	Return_ptr rp;
	Payload_ptr plp;
	struct capn_segment *cs;
	int eid, rc;

	read_Bootstrap(&b, bp);
	if (c->bootstrap == NULL)
		return send_return_exception(c, b.questionId,
		                             "no bootstrap capability");
	eid = capn_rpc_export(c, c->bootstrap, c->bootstrap_dispatch);
	if (eid < 0)
		return send_return_exception(c, b.questionId, "export table full");

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&pl, 0, sizeof pl);

	plp = new_Payload(cs);
	write_cap_payload(cs, &pl, eid, &pl.content);
	write_Payload(&pl, plp);

	rp = new_Return(cs);
	r.answerId = b.questionId;
	r.which = Return_results;
	r.results = plp;
	write_Return(&r, rp);

	mp = new_Message(cs);
	m.which = Message__return;
	m._return = rp;
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

static int handle_call(struct capn_rpc_conn *c, Call_ptr cp)
{
	struct capn msg;
	struct Call call;
	struct Message m;
	struct Return r;
	struct Payload params, results;
	Message_ptr mp;
	Return_ptr rp;
	Payload_ptr plp;
	struct capn_segment *cs;
	capn_ptr rstruct;
	int eid, rc;

	read_Call(&call, cp);
	eid = resolve_target(c, call.target);
	if (eid < 0)
		return send_return_exception(c, call.questionId, "no such export");

	memset(&params, 0, sizeof params);
	if (call.params.p.type != CAPN_NULL)
		read_Payload(&params, call.params);

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&results, 0, sizeof results);

	/* One data word and one pointer word is enough for the replies the
	 * bundled servers make; a richer server allocates inside dispatch. */
	rstruct = capn_new_struct(cs, 8, 1);
	rc = c->exports[eid].dispatch(c->exports[eid].server, call.interfaceId,
	                              call.methodId, params.content, rstruct);
	if (rc != 0) {
		capn_free(&msg);
		return send_return_exception(c, call.questionId, "call failed");
	}

	plp = new_Payload(cs);
	results.content = rstruct;
	write_Payload(&results, plp);

	rp = new_Return(cs);
	r.answerId = call.questionId;
	r.which = Return_results;
	r.results = plp;
	write_Return(&r, rp);

	mp = new_Message(cs);
	m.which = Message__return;
	m._return = rp;
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

static void handle_release(struct capn_rpc_conn *c, Release_ptr rp)
{
	struct Release rel;
	read_Release(&rel, rp);
	if (rel.id >= CAPN_RPC_MAX_EXPORTS || !c->exports[rel.id].used)
		return;
	c->exports[rel.id].refcount -= (int)rel.referenceCount;
	if (c->exports[rel.id].refcount <= 0)
		memset(&c->exports[rel.id], 0, sizeof c->exports[rel.id]);
}

/* Answer one Join question with a JoinResult payload. */
static int send_join_result(struct capn_rpc_conn *c, uint32_t qid,
                            uint32_t join_id, int succeeded, int with_cap,
                            int eid)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	struct Payload pl;
	struct JoinResult jr;
	Message_ptr mp;
	Return_ptr rp;
	Payload_ptr plp;
	JoinResult_ptr jrp;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&pl, 0, sizeof pl);
	memset(&jr, 0, sizeof jr);

	jrp = new_JoinResult(cs);
	jr.joinId = join_id;
	jr.succeeded = succeeded ? 1 : 0;
	if (with_cap && eid >= 0) {
		/* The receiver gains a reference, so the refcount rises with it. */
		c->exports[eid].refcount++;
		write_cap_payload(cs, &pl, eid, &jr.cap);
	}
	write_JoinResult(&jr, jrp);

	plp = new_Payload(cs);
	pl.content = jrp.p;
	write_Payload(&pl, plp);

	rp = new_Return(cs);
	r.answerId = qid;
	r.which = Return_results;
	r.results = plp;
	write_Return(&r, rp);

	mp = new_Message(cs);
	m.which = Message__return;
	m._return = rp;
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

/* Find the slot holding joinId, or claim a free one. Returns -1 when the
 * table is full or partCount disagrees with the parts already in. */
static int join_slot_find(struct capn_rpc_conn *c, uint32_t jid, int pcount)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_JOINS; i++) {
		if (c->joins[i].used && c->joins[i].join_id == jid)
			return c->joins[i].part_count == pcount ? i : -1;
	}
	for (i = 0; i < CAPN_RPC_MAX_JOINS; i++) {
		if (!c->joins[i].used) {
			memset(&c->joins[i], 0, sizeof c->joins[i]);
			c->joins[i].used = 1;
			c->joins[i].join_id = jid;
			c->joins[i].part_count = pcount;
			return i;
		}
	}
	return -1;
}

/* The set is complete: compare the targets and answer every part. */
static int join_complete(struct capn_rpc_conn *c, int slot)
{
	struct capn_rpc_join *j = &c->joins[slot];
	int i, first = j->eids[0], same, rc = 0;

	/* An unresolved part names nothing we host, so the set cannot be
	 * proven equal. */
	same = first >= 0;
	for (i = 1; same && i < j->part_count; i++)
		if (j->eids[i] != first)
			same = 0;

	/* Exactly one result carries the joined capability, per JoinResult. */
	for (i = 0; i < j->part_count; i++) {
		int r = send_join_result(c, j->qids[i], j->join_id, same,
		                         same && i == 0, first);
		if (r != 0)
			rc = r;
	}
	memset(j, 0, sizeof *j);
	return rc;
}

static int handle_join(struct capn_rpc_conn *c, Join_ptr jp)
{
	struct Join j;
	struct JoinKeyPart kp;
	JoinKeyPart_ptr kpp;
	int slot, eid;

	read_Join(&j, jp);
	if (j.keyPart.type == CAPN_NULL)
		/* Without a JoinKeyPart there is no way to tell which set this
		 * belongs to, so it can only fail on its own. */
		return send_join_result(c, j.questionId, 0, 0, 0, -1);

	kpp.p = j.keyPart;
	read_JoinKeyPart(&kp, kpp);
	if (kp.partCount == 0 || kp.partCount > CAPN_RPC_MAX_JOIN_PARTS ||
	    kp.partNum >= kp.partCount)
		return send_join_result(c, j.questionId, kp.joinId, 0, 0, -1);

	slot = join_slot_find(c, kp.joinId, (int)kp.partCount);
	if (slot < 0)
		return send_join_result(c, j.questionId, kp.joinId, 0, 0, -1);
	if (c->joins[slot].seen[kp.partNum])
		/* A partNum reused before the set completes leaves it
		 * unanswerable. */
		return send_join_result(c, j.questionId, kp.joinId, 0, 0, -1);

	eid = resolve_target(c, j.target);
	c->joins[slot].seen[kp.partNum] = 1;
	c->joins[slot].qids[kp.partNum] = j.questionId;
	c->joins[slot].eids[kp.partNum] = eid;
	c->joins[slot].nseen++;

	if (c->joins[slot].nseen == c->joins[slot].part_count)
		return join_complete(c, slot);
	return 0;
}

/* Echo a message we did not understand, per the spec. */
static int send_unimplemented(struct capn_rpc_conn *c, Message_ptr orig)
{
	struct capn msg;
	struct Message m;
	Message_ptr mp;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	mp = new_Message(cs);
	m.which = Message_unimplemented;
	/* capn_setp copies across messages, which is what this needs: the
	 * incoming message is freed as soon as this returns. */
	m.unimplemented.p = capn_new_struct(cs, 8, 1);
	capn_setp(m.unimplemented.p, 0, orig.p);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

int capn_rpc_handle(struct capn_rpc_conn *c, const uint8_t *data, size_t len)
{
	struct capn msg;
	struct Message m;
	Message_ptr mp;
	capn_ptr root;
	int rc = 0;

	if (capn_init_mem(&msg, data, len, 0) != 0)
		return -1;
	root = capn_getp(capn_root(&msg), 0, 1);
	if (root.type == CAPN_NULL) {
		capn_free(&msg);
		return -1;
	}
	mp.p = root;
	read_Message(&m, mp);

	switch (m.which) {
	case Message_bootstrap:
		rc = handle_bootstrap(c, m.bootstrap);
		break;
	case Message_call:
		rc = handle_call(c, m.call);
		break;
	case Message_finish:
		/* Nothing is retained per answer yet, so a Finish only needs to
		 * be accepted rather than acted on. */
		break;
	case Message_release:
		handle_release(c, m.release);
		break;
	case Message_join:
		rc = handle_join(c, m.join);
		break;
	default:
		/* Provide and Accept name a third vat, which a two-party
		 * connection cannot; the obsolete save/delete messages are gone
		 * from the protocol. */
		rc = send_unimplemented(c, mp);
		break;
	}

	capn_free(&msg);
	return rc;
}
