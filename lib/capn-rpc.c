/* Two-party Cap'n Proto RPC vat. See capn-rpc.h. */

#include "capn-rpc.h"

#include <string.h>

#include "rpc.capnp.h"
/* One network layer, not two. rpc-threeparty.capnp defines the join keys
 * as well as the third-party ids, and both files declare the same C
 * names, so including either alongside the other would not compile. */
#include "rpc-threeparty.capnp.h"

/* Defined below, beside the rest of the answer table. */
static struct capn_rpc_answer *answer_find(struct capn_rpc_conn *c,
                                           uint32_t qid);

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
static int resolve_promised_answer(struct capn_rpc_conn *c,
                                   PromisedAnswer_ptr pap);

static int resolve_target(struct capn_rpc_conn *c, MessageTarget_ptr tp)
{
	struct MessageTarget t;
	if (tp.p.type == CAPN_NULL)
		return -1;
	read_MessageTarget(&t, tp);
	if (t.which == MessageTarget_promisedAnswer)
		return resolve_promised_answer(c, t.promisedAnswer);
	if (t.which != MessageTarget_importedCap)
		return -1;
	if (t.importedCap >= CAPN_RPC_MAX_EXPORTS)
		return -1;
	return c->exports[t.importedCap].used ? (int)t.importedCap : -1;
}

/* Promise pipelining: the caller addressed a capability inside an answer,
 * identified by walking the transform ops into that answer's results and
 * reading the capTable entry the resulting pointer names. */
static int resolve_promised_answer(struct capn_rpc_conn *c,
                                   PromisedAnswer_ptr pap)
{
	struct capn_rpc_answer *a;
	struct capn stored;
	struct PromisedAnswer pa;
	struct Message m;
	struct Return r;
	struct Payload pl;
	struct CapDescriptor cd;
	Message_ptr mp;
	capn_ptr cursor;
	int i, n, eid = -1;

	if (pap.p.type == CAPN_NULL)
		return -1;
	read_PromisedAnswer(&pa, pap);
	a = answer_find(c, pa.questionId);
	if (a == NULL)
		return -1;
	if (capn_init_mem(&stored, a->frame, a->len, 0) != 0)
		return -1;

	mp.p = capn_getp(capn_root(&stored), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message__return)
		goto out;
	read_Return(&r, m._return);
	if (r.which != Return_results)
		goto out;
	read_Payload(&pl, r.results);

	cursor = pl.content;
	n = capn_len(pa.transform);
	for (i = 0; i < n; i++) {
		struct PromisedAnswer_Op op;
		get_PromisedAnswer_Op(&op, pa.transform, i);
		if (op.which != PromisedAnswer_Op_getPointerField)
			continue;
		/* The peer chooses the transform, so a step into something with
		 * no pointer section is an unresolvable target, not a fault. */
		capn_resolve(&cursor);
		if (cursor.type != CAPN_STRUCT)
			goto out;
		cursor = capn_getp(cursor, op.getPointerField, 0);
	}

	capn_resolve(&cursor);
	if (cursor.type != CAPN_INTERFACE)
		goto out;
	/* The pointer holds a capTable index; the descriptor beside it says
	 * which export the caller is naming. */
	if (cursor.len < 0 || cursor.len >= capn_len(pl.capTable))
		goto out;
	get_CapDescriptor(&cd, pl.capTable, cursor.len);
	if (cd.which != CapDescriptor_senderHosted)
		goto out;
	if (cd.senderHosted < CAPN_RPC_MAX_EXPORTS &&
	    c->exports[cd.senderHosted].used)
		eid = (int)cd.senderHosted;

out:
	capn_free(&stored);
	return eid;
}

static int send_message(struct capn_rpc_conn *c, struct capn *msg)
{
	uint8_t buf[CAPN_RPC_MAX_ANSWER_BYTES];
	ssize_t sz = capn_write_mem(msg, buf, sizeof buf, 0);
	if (sz < 0)
		return -1;
	return c->send(c->send_ctx, buf, (size_t)sz);
}

/* Send a Return and keep it until `Finish`, so a call pipelined against
 * this answer can still find the capability it names. */
static int send_answer(struct capn_rpc_conn *c, uint32_t qid, struct capn *msg)
{
	uint8_t buf[CAPN_RPC_MAX_ANSWER_BYTES];
	ssize_t sz = capn_write_mem(msg, buf, sizeof buf, 0);
	int i;
	if (sz < 0)
		return -1;
	for (i = 0; i < CAPN_RPC_MAX_ANSWERS; i++) {
		if (!c->answers[i].used) {
			c->answers[i].used = 1;
			c->answers[i].question_id = qid;
			memcpy(c->answers[i].frame, buf, (size_t)sz);
			c->answers[i].len = (size_t)sz;
			break;
		}
	}
	return c->send(c->send_ctx, buf, (size_t)sz);
}

static struct capn_rpc_answer *answer_find(struct capn_rpc_conn *c, uint32_t qid)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_ANSWERS; i++)
		if (c->answers[i].used && c->answers[i].question_id == qid)
			return &c->answers[i];
	return NULL;
}

static void answer_drop(struct capn_rpc_conn *c, uint32_t qid)
{
	struct capn_rpc_answer *a = answer_find(c, qid);
	if (a)
		a->used = 0;
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

	rc = send_answer(c, b.questionId, &msg);
	capn_free(&msg);
	return rc;
}

/* --- level 3: introductions handed to us ---------------------------- */

/* Record every `thirdPartyHosted` entry in an incoming cap table.
 *
 * The descriptor says where the capability really lives and hands us a
 * vine, an ordinary import through the introducer. Calls on the vine
 * work right away, which is the fallback the spec gives receivers that
 * cannot reach a third party; the vine must therefore outlive the
 * pickup. Dialling the third vat belongs to the network layer, so the
 * arrangement is recorded and handed over rather than acted on here.
 */
static void note_introductions(struct capn_rpc_conn *c, struct Payload *pl)
{
	int n, i, j;

	if (pl->capTable.p.type == CAPN_NULL)
		return;
	n = capn_ptr_len(pl->capTable.p);
	for (i = 0; i < n; i++) {
		struct CapDescriptor cd;
		struct ThirdPartyCapDescriptor tp;
		struct ThirdPartyCapId id;
		struct VatId vat;
		CapDescriptor_ptr cdp;

		cdp.p = capn_getp(pl->capTable.p, i, 0);
		if (cdp.p.type == CAPN_NULL)
			continue;
		read_CapDescriptor(&cd, cdp);
		if (cd.which != CapDescriptor_thirdPartyHosted)
			continue;
		if (cd.thirdPartyHosted.p.type == CAPN_NULL)
			continue;
		read_ThirdPartyCapDescriptor(&tp, cd.thirdPartyHosted);
		if (tp.id.type == CAPN_NULL)
			continue;
		{
			ThirdPartyCapId_ptr idp;
			idp.p = tp.id;
			read_ThirdPartyCapId(&id, idp);
		}
		if (id.vat.p.type == CAPN_NULL)
			continue;
		read_VatId(&vat, id.vat);
		/* A host that will not fit is refused rather than truncated:
		 * a truncated address names a different vat. */
		if (vat.host.len >= CAPN_RPC_MAX_HOST)
			continue;

		for (j = 0; j < CAPN_RPC_MAX_INTRODUCTIONS; j++) {
			if (c->introductions[j].used)
				continue;
			c->introductions[j].used = 1;
			c->introductions[j].nonce = id.nonce;
			c->introductions[j].vine_id = tp.vineId;
			c->introductions[j].port = vat.port;
			memcpy(c->introductions[j].host, vat.host.str,
			       (size_t)vat.host.len);
			c->introductions[j].host[vat.host.len] = '\0';
			break;
		}
	}
}

int capn_rpc_pending_introductions(struct capn_rpc_conn *c,
                                   struct capn_rpc_introduction *out, int max)
{
	int i, n = 0;

	for (i = 0; i < CAPN_RPC_MAX_INTRODUCTIONS; i++) {
		if (!c->introductions[i].used)
			continue;
		if (out != NULL && n < max)
			out[n] = c->introductions[i];
		n++;
	}
	return n;
}

int capn_rpc_introduction_done(struct capn_rpc_conn *c, uint64_t nonce)
{
	int i;

	for (i = 0; i < CAPN_RPC_MAX_INTRODUCTIONS; i++) {
		if (!c->introductions[i].used || c->introductions[i].nonce != nonce)
			continue;
		/* Releasing the vine is what tells the introducer it may close
		 * the Provide it opened on our behalf. */
		capn_rpc_send_release(c, c->introductions[i].vine_id, 1);
		memset(&c->introductions[i], 0, sizeof c->introductions[i]);
		return 0;
	}
	return -1;
}

int capn_rpc_write_third_party_cap(struct capn_rpc_conn *c, CapDescriptor_ptr cd,
                                   const char *host, uint16_t port,
                                   uint64_t nonce, uint32_t vine_id)
{
	struct CapDescriptor desc;
	struct ThirdPartyCapDescriptor tp;
	struct ThirdPartyCapId id;
	struct VatId vat;
	ThirdPartyCapDescriptor_ptr tpp;
	ThirdPartyCapId_ptr idp;
	VatId_ptr vp;
	struct capn_segment *cs;

	(void)c;
	if (cd.p.type == CAPN_NULL || host == NULL)
		return -1;
	cs = cd.p.seg;

	vp = new_VatId(cs);
	memset(&vat, 0, sizeof vat);
	vat.host.str = host;
	vat.host.len = (int)strlen(host);
	/* seg stays NULL: a non-NULL segment tells capn_set_text the bytes
	 * already live in the message, and `host` is the caller's. */
	vat.host.seg = NULL;
	vat.port = port;
	write_VatId(&vat, vp);

	idp = new_ThirdPartyCapId(cs);
	memset(&id, 0, sizeof id);
	id.vat = vp;
	id.nonce = nonce;
	write_ThirdPartyCapId(&id, idp);

	tpp = new_ThirdPartyCapDescriptor(cs);
	memset(&tp, 0, sizeof tp);
	tp.id = idp.p;
	tp.vineId = vine_id;
	write_ThirdPartyCapDescriptor(&tp, tpp);

	memset(&desc, 0, sizeof desc);
	desc.which = CapDescriptor_thirdPartyHosted;
	desc.thirdPartyHosted = tpp;
	desc.attachedFd = 0xff;
	write_CapDescriptor(&desc, cd);
	return 0;
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
	if (call.params.p.type != CAPN_NULL) {
		read_Payload(&params, call.params);
		note_introductions(c, &params);
	}

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

	rc = send_answer(c, call.questionId, &msg);
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

/* A Disembargo with `senderLoopback` is echoed back as `receiverLoopback`
 * carrying the same id. That reflection is what lets the sender know
 * every call it had already sent through a promise has arrived, so it can
 * stop routing new ones the long way round. */
static int handle_disembargo(struct capn_rpc_conn *c, Disembargo_ptr dp)
{
	struct capn msg;
	struct Disembargo in, out;
	struct Message m;
	Message_ptr mp;
	Disembargo_ptr op;
	struct capn_segment *cs;
	int rc;

	read_Disembargo(&in, dp);
	/* receiverLoopback is the reply to an embargo we raised, and this vat
	 * raises none; accept it without echoing to avoid a loop. */
	if (in.context_which != Disembargo_context_senderLoopback)
		return 0;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&out, 0, sizeof out);

	op = new_Disembargo(cs);
	/* Echo the target back untouched: the sender matches on it. */
	if (in.target.p.type != CAPN_NULL) {
		MessageTarget_ptr tp = new_MessageTarget(cs);
		struct MessageTarget t;
		read_MessageTarget(&t, in.target);
		write_MessageTarget(&t, tp);
		out.target = tp;
	}
	out.context_which = Disembargo_context_receiverLoopback;
	out.context.receiverLoopback = in.context.senderLoopback;
	write_Disembargo(&out, op);

	mp = new_Message(cs);
	m.which = Message_disembargo;
	m.disembargo = op;
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
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


/* --- client side -------------------------------------------------- */

static struct capn_rpc_question *question_claim(struct capn_rpc_conn *c,
                                                uint32_t *qid_out)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_QUESTIONS; i++) {
		if (!c->questions[i].used) {
			memset(&c->questions[i], 0, sizeof c->questions[i]);
			c->questions[i].used = 1;
			c->questions[i].question_id = c->next_question_id++;
			*qid_out = c->questions[i].question_id;
			return &c->questions[i];
		}
	}
	return NULL;
}

static struct capn_rpc_question *question_find(struct capn_rpc_conn *c,
                                               uint32_t qid)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_QUESTIONS; i++)
		if (c->questions[i].used && c->questions[i].question_id == qid)
			return &c->questions[i];
	return NULL;
}

uint32_t capn_rpc_send_bootstrap(struct capn_rpc_conn *c)
{
	struct capn msg;
	struct Message m;
	struct Bootstrap bs;
	Message_ptr mp;
	Bootstrap_ptr bp;
	struct capn_segment *cs;
	struct capn_rpc_question *q;
	uint32_t qid = 0;

	q = question_claim(c, &qid);
	if (q == NULL)
		return (uint32_t)-1;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&bs, 0, sizeof bs);
	bs.questionId = qid;
	bp = new_Bootstrap(cs);
	write_Bootstrap(&bs, bp);
	m.which = Message_bootstrap;
	m.bootstrap = bp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	if (send_message(c, &msg) != 0) {
		q->used = 0;
		qid = (uint32_t)-1;
	}
	capn_free(&msg);
	return qid;
}

uint32_t capn_rpc_send_call(struct capn_rpc_conn *c, uint32_t imported_cap,
                            uint64_t interface_id, uint16_t method_id,
                            int params_datasz, int params_ptrs,
                            capn_rpc_fill_fn fill, void *fill_ctx)
{
	struct capn msg;
	struct Message m;
	struct Call call;
	struct MessageTarget t;
	struct Payload params;
	Message_ptr mp;
	Call_ptr cp;
	MessageTarget_ptr tp;
	Payload_ptr plp;
	struct capn_segment *cs;
	struct capn_rpc_question *q;
	uint32_t qid = 0;

	q = question_claim(c, &qid);
	if (q == NULL)
		return (uint32_t)-1;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&call, 0, sizeof call);
	memset(&t, 0, sizeof t);
	memset(&params, 0, sizeof params);

	t.which = MessageTarget_importedCap;
	t.importedCap = imported_cap;
	tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);

	params.content = capn_new_struct(cs, params_datasz, params_ptrs);
	if (fill)
		fill(fill_ctx, params.content);
	plp = new_Payload(cs);
	write_Payload(&params, plp);

	call.questionId = qid;
	call.target = tp;
	call.interfaceId = interface_id;
	call.methodId = method_id;
	call.params = plp;
	cp = new_Call(cs);
	write_Call(&call, cp);

	m.which = Message_call;
	m.call = cp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	if (send_message(c, &msg) != 0) {
		q->used = 0;
		qid = (uint32_t)-1;
	}
	capn_free(&msg);
	return qid;
}

int capn_rpc_send_finish(struct capn_rpc_conn *c, uint32_t question_id)
{
	struct capn msg;
	struct Message m;
	struct Finish f;
	Message_ptr mp;
	Finish_ptr fp;
	struct capn_segment *cs;
	struct capn_rpc_question *q;
	int rc;

	q = question_find(c, question_id);
	if (q)
		q->used = 0;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&f, 0, sizeof f);
	f.questionId = question_id;
	fp = new_Finish(cs);
	write_Finish(&f, fp);
	m.which = Message_finish;
	m.finish = fp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

int capn_rpc_send_release(struct capn_rpc_conn *c, uint32_t import_id,
                          uint32_t count)
{
	struct capn msg;
	struct Message m;
	struct Release rel;
	Message_ptr mp;
	Release_ptr rp;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&rel, 0, sizeof rel);
	rel.id = import_id;
	rel.referenceCount = count;
	rp = new_Release(cs);
	write_Release(&rel, rp);
	m.which = Message_release;
	m.release = rp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

int capn_rpc_is_answered(struct capn_rpc_conn *c, uint32_t question_id)
{
	struct capn_rpc_question *q = question_find(c, question_id);
	return q && q->answered;
}

int capn_rpc_is_failed(struct capn_rpc_conn *c, uint32_t question_id)
{
	struct capn_rpc_question *q = question_find(c, question_id);
	return q && q->answered && q->failed;
}

int capn_rpc_answer_content(struct capn_rpc_conn *c, uint32_t question_id,
                            struct capn *msg_out, capn_ptr *out)
{
	struct capn_rpc_question *q = question_find(c, question_id);
	struct Message m;
	struct Return r;
	struct Payload pl;
	Message_ptr mp;

	if (q == NULL || !q->answered || q->failed)
		return -1;
	if (capn_init_mem(msg_out, q->reply, q->reply_len, 0) != 0)
		return -1;
	mp.p = capn_getp(capn_root(msg_out), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message__return) {
		capn_free(msg_out);
		return -1;
	}
	read_Return(&r, m._return);
	if (r.which != Return_results) {
		capn_free(msg_out);
		return -1;
	}
	read_Payload(&pl, r.results);
	*out = pl.content;
	capn_resolve(out);
	return 0;
}

/* Record a Return against the question that asked it. A Return naming a
 * question this vat never asked is dropped: recording it would let a
 * peer plant answers that later pipelining would trust. */
static void handle_return(struct capn_rpc_conn *c, Return_ptr rp,
                          const uint8_t *frame, size_t len)
{
	struct Return r;
	struct capn_rpc_question *q;

	read_Return(&r, rp);
	q = question_find(c, r.answerId);
	if (q == NULL || len > CAPN_RPC_MAX_ANSWER_BYTES)
		return;
	q->answered = 1;
	q->failed = (r.which != Return_results);
	if (r.which == Return_results && r.results.p.type != CAPN_NULL) {
		struct Payload pl;
		read_Payload(&pl, r.results);
		note_introductions(c, &pl);
	}
	memcpy(q->reply, frame, len);
	q->reply_len = len;
}

/* --- stream flow control ------------------------------------------- */

void capn_rpc_stream_init(struct capn_rpc_stream *s, int window)
{
	memset(s, 0, sizeof *s);
	if (window < 1)
		window = 1;
	if (window > CAPN_RPC_STREAM_MAX_WINDOW)
		window = CAPN_RPC_STREAM_MAX_WINDOW;
	s->window = window;
}

/* Retire the oldest outstanding call. A call that never answered, or
 * answered with an exception, marks the stream failed. */
static void stream_retire_oldest(struct capn_rpc_conn *c,
                                 struct capn_rpc_stream *s)
{
	uint32_t qid;
	int i;

	if (s->nout == 0)
		return;
	qid = s->qids[0];
	for (i = 1; i < s->nout; i++)
		s->qids[i - 1] = s->qids[i];
	s->nout--;

	if (!capn_rpc_is_answered(c, qid) || capn_rpc_is_failed(c, qid)) {
		s->failed = 1;
		if (!s->have_failure) {
			s->have_failure = 1;
			s->first_failure = qid;
		}
	}
	capn_rpc_send_finish(c, qid);
}

int capn_rpc_stream_send(struct capn_rpc_conn *c, struct capn_rpc_stream *s,
                         uint32_t imported_cap, uint64_t interface_id,
                         uint16_t method_id, int params_datasz,
                         int params_ptrs, capn_rpc_fill_fn fill,
                         void *fill_ctx)
{
	uint32_t qid;

	if (s->failed)
		return -1;
	if (s->nout >= s->window) {
		stream_retire_oldest(c, s);
		if (s->failed)
			return -1;
	}
	qid = capn_rpc_send_call(c, imported_cap, interface_id, method_id,
	                         params_datasz, params_ptrs, fill, fill_ctx);
	if (qid == (uint32_t)-1)
		return -1;
	s->qids[s->nout++] = qid;
	return 0;
}

int capn_rpc_stream_finish(struct capn_rpc_conn *c, struct capn_rpc_stream *s)
{
	while (s->nout > 0)
		stream_retire_oldest(c, s);
	return s->have_failure ? -1 : 0;
}


/* --- level 3: three-party handoff ---------------------------------- */

/* Answer a question with empty results: the introducer is not waiting
 * for a value, only for confirmation that the arrangement is recorded. */
static int send_empty_return(struct capn_rpc_conn *c, uint32_t qid)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	Message_ptr mp;
	Return_ptr rp;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	r.answerId = qid;
	r.which = Return_results;
	r.results = new_Payload(cs);
	rp = new_Return(cs);
	write_Return(&r, rp);
	m.which = Message__return;
	m._return = rp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	rc = send_answer(c, qid, &msg);
	capn_free(&msg);
	return rc;
}

/* `Provide`: hold the target for whoever presents this nonce. */
static int handle_provide(struct capn_rpc_conn *c, Provide_ptr pp)
{
	struct Provide pv;
	struct RecipientId rid;
	RecipientId_ptr rp;
	int eid, i;

	read_Provide(&pv, pp);
	eid = resolve_target(c, pv.target);
	if (eid < 0)
		return send_return_exception(c, pv.questionId,
		                             "provide: no such capability");
	if (pv.recipient.type == CAPN_NULL)
		return send_return_exception(c, pv.questionId,
		                             "provide: no recipient");
	rp.p = pv.recipient;
	read_RecipientId(&rid, rp);

	for (i = 0; i < CAPN_RPC_MAX_PROVISIONS; i++) {
		if (!c->provisions[i].used) {
			c->provisions[i].used = 1;
			c->provisions[i].nonce = rid.nonce;
			c->provisions[i].export_id = eid;
			/* The recipient holds a reference once it accepts. */
			c->exports[eid].refcount++;
			return send_empty_return(c, pv.questionId);
		}
	}
	return send_return_exception(c, pv.questionId, "provide: table full");
}

/* `Accept`: claim a capability a third vat provided for us.
 *
 * A nonce is single-use. Leaving it claimable would let anyone who
 * learns it take the capability again later. */
static int handle_accept(struct capn_rpc_conn *c, Accept_ptr ap)
{
	struct capn msg;
	struct Accept ac;
	struct ProvisionId pid;
	struct Message m;
	struct Return r;
	struct Payload pl;
	ProvisionId_ptr pp;
	Message_ptr mp;
	Return_ptr rp;
	Payload_ptr plp;
	struct capn_segment *cs;
	int i, eid = -1, rc;

	read_Accept(&ac, ap);
	if (ac.provision.type == CAPN_NULL)
		return send_return_exception(c, ac.questionId,
		                             "accept: no provision");
	pp.p = ac.provision;
	read_ProvisionId(&pid, pp);

	for (i = 0; i < CAPN_RPC_MAX_PROVISIONS; i++) {
		if (c->provisions[i].used && c->provisions[i].nonce == pid.nonce) {
			eid = c->provisions[i].export_id;
			c->provisions[i].used = 0;
			break;
		}
	}
	if (eid < 0)
		return send_return_exception(c, ac.questionId,
		                             "accept: no such provision");

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&pl, 0, sizeof pl);
	plp = new_Payload(cs);
	write_cap_payload(cs, &pl, eid, &pl.content);
	write_Payload(&pl, plp);
	r.answerId = ac.questionId;
	r.which = Return_results;
	r.results = plp;
	rp = new_Return(cs);
	write_Return(&r, rp);
	m.which = Message__return;
	m._return = rp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	rc = send_answer(c, ac.questionId, &msg);
	capn_free(&msg);
	return rc;
}

int capn_rpc_pending_provisions(struct capn_rpc_conn *c, uint64_t *out, int cap)
{
	int i, n = 0;
	for (i = 0; i < CAPN_RPC_MAX_PROVISIONS; i++) {
		if (c->provisions[i].used) {
			if (out && n < cap)
				out[n] = c->provisions[i].nonce;
			n++;
		}
	}
	return n;
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
	case Message_finish: {
		/* The caller is done with the answer, so the results it might
		 * have pipelined against can go. */
		struct Finish fin;
		read_Finish(&fin, m.finish);
		answer_drop(c, fin.questionId);
		break;
	}
	case Message_release:
		handle_release(c, m.release);
		break;
	case Message_join:
		rc = handle_join(c, m.join);
		break;
	case Message__return:
		handle_return(c, m._return, data, len);
		break;
	case Message_resolve:
		/* Promise resolution. Replying unimplemented is the spec-defined
		 * signal that this vat does not adopt resolutions: the sender
		 * keeps forwarding calls addressed to the promise, which it does
		 * until Release. */
		rc = send_unimplemented(c, mp);
		break;
	case Message_disembargo:
		rc = handle_disembargo(c, m.disembargo);
		break;
	case Message_provide:
		rc = handle_provide(c, m.provide);
		break;
	case Message_accept:
		rc = handle_accept(c, m.accept);
		break;
	default:
		/* The obsolete save/delete messages. */
		rc = send_unimplemented(c, mp);
		break;
	}

	capn_free(&msg);
	return rc;
}
