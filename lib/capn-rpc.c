/* Two-party Cap'n Proto RPC vat. See capn-rpc.h. */

#include "capn-rpc.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "rpc.capnp.h"
/* One network layer, not two. rpc-threeparty.capnp defines the join keys
 * as well as the third-party ids, and both files declare the same C
 * names, so including either alongside the other would not compile. */
#include "rpc-threeparty.capnp.h"

/* Defined below, beside the rest of the answer table. */
static void attach_cap_table(struct capn_rpc_conn *c, struct capn_segment *cs,
                             struct Payload *pl);
static struct capn_rpc_answer *answer_find(struct capn_rpc_conn *c,
                                           uint32_t qid);

/* Grow `table` and return the new pointer. The caller stores it back
 * into the typed slot. Writing through a void ** would alias a
 * struct pointer, which compilers may treat as a different object. */
static void *table_grow(void *table, int *cap, size_t item_size)
{
	void *next;
	int old = *cap;
	int next_cap;

	if (old <= 0)
		next_cap = CAPN_RPC_INITIAL_TABLE_CAPACITY;
	else if (old >= CAPN_RPC_MAX_TABLE_CAPACITY ||
		 old > CAPN_RPC_MAX_TABLE_CAPACITY / 2)
		return NULL;
	else
		next_cap = old * 2;
	if (next_cap > CAPN_RPC_MAX_TABLE_CAPACITY)
		return NULL;
	if (item_size == 0 || (size_t) next_cap > SIZE_MAX / item_size)
		return NULL;
	next = realloc(table, (size_t) next_cap * item_size);
	if (next == NULL)
		return NULL;
	memset((char *) next + (size_t) old * item_size, 0,
	       (size_t) (next_cap - old) * item_size);
	*cap = next_cap;
	return next;
}

static int queue_frame(struct capn_rpc_conn *c, int export_id,
                       const uint8_t *data, size_t len);
static void flush_reject(struct capn_rpc_conn *c, int export_id,
                         const char *reason);
static void lift_embargo(struct capn_rpc_conn *c, uint32_t embargo_id);
static int forward_one(struct capn_rpc_conn *c, uint32_t import_id,
                       const uint8_t *frame, size_t len);
static void relay_on_return(struct capn_rpc_conn *c, uint32_t local_qid,
                            const uint8_t *frame, size_t len);
static int handle_resolve(struct capn_rpc_conn *c, Resolve_ptr rp);

void capn_rpc_init(struct capn_rpc_conn *c, capn_rpc_send_fn send,
                   void *send_ctx)
{
	memset(c, 0, sizeof(*c));
	c->send = send;
	c->send_ctx = send_ctx;
	c->vat = &c->own_vat;
	c->results_datasz = 8;
	c->results_ptrs = 1;
}

void capn_rpc_set_result_size(struct capn_rpc_conn *c, int datasz, int ptrs)
{
	if (datasz < 0 || ptrs < 0)
		return;
	c->results_datasz = datasz;
	c->results_ptrs = ptrs;
}

void capn_rpc_free(struct capn_rpc_conn *c)
{
	int i;

	for (i = 0; i < c->answers_cap; i++)
		free(c->answers[i].frame);
	for (i = 0; i < c->questions_cap; i++)
		free(c->questions[i].reply);
	for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
		if (c->pending_calls[i].used) {
			capn_free(c->pending_calls[i].msg);
			free(c->pending_calls[i].msg);
		}
	}
	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++)
		free(c->queued[i].frame);
	for (i = 0; i < CAPN_RPC_MAX_HELD; i++) {
		if (c->held[i].used && c->held[i].msg != NULL) {
			capn_free(c->held[i].msg);
			free(c->held[i].msg);
		}
	}
	memset(c->pending_calls, 0, sizeof c->pending_calls);
	free(c->exports);
	free(c->answers);
	free(c->questions);
	free(c->imports);
	c->exports = NULL;
	c->answers = NULL;
	c->questions = NULL;
	c->imports = NULL;
	c->exports_cap = 0;
	c->answers_cap = 0;
	c->questions_cap = 0;
	c->imports_cap = 0;
}

void capn_rpc_set_vat(struct capn_rpc_conn *c, struct capn_rpc_vat *vat)
{
	c->vat = vat != NULL ? vat : &c->own_vat;
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
	for (i = 0; i < c->exports_cap; i++) {
		/* A promise keeps its own id until Resolve, even for the same
		 * object: handing it out as senderHosted would claim it is ready. */
		if (c->exports[i].used && !c->exports[i].promise &&
		    c->exports[i].server == server &&
		    c->exports[i].dispatch == dispatch) {
			c->exports[i].refcount++;
			return i;
		}
	}
	for (i = 0; i < c->exports_cap; i++) {
		if (!c->exports[i].used) {
			c->exports[i].used = 1;
			c->exports[i].refcount = 1;
			c->exports[i].server = server;
			c->exports[i].dispatch = dispatch;
			c->exports[i].promise = 0;
			c->exports[i].broken = 0;
			c->exports[i].has_forward = 0;
			c->exports[i].has_forward_import = 0;
			return i;
		}
	}
	i = c->exports_cap;
	{
		void *grown = table_grow(c->exports, &c->exports_cap,
		                         sizeof *c->exports);
		if (grown == NULL)
			return -1;
		c->exports = grown;
	}
	c->exports[i].used = 1;
	c->exports[i].refcount = 1;
	c->exports[i].server = server;
	c->exports[i].dispatch = dispatch;
	c->exports[i].promise = 0;
	c->exports[i].broken = 0;
	c->exports[i].has_forward = 0;
	c->exports[i].has_forward_import = 0;
	return i;
}

int capn_rpc_export_promise(struct capn_rpc_conn *c, void *server,
                            capn_rpc_dispatch_fn dispatch)
{
	int i;

	if (dispatch == NULL)
		return -1;
	for (i = 0; i < c->exports_cap; i++) {
		if (c->exports[i].used && c->exports[i].promise &&
		    !c->exports[i].broken && c->exports[i].server == server &&
		    c->exports[i].dispatch == dispatch) {
			c->exports[i].refcount++;
			return i;
		}
	}
	for (i = 0; i < c->exports_cap; i++) {
		if (!c->exports[i].used) {
			memset(&c->exports[i], 0, sizeof c->exports[i]);
			c->exports[i].used = 1;
			c->exports[i].refcount = 1;
			c->exports[i].server = server;
			c->exports[i].dispatch = dispatch;
			c->exports[i].promise = 1;
			return i;
		}
	}
	i = c->exports_cap;
	{
		void *grown = table_grow(c->exports, &c->exports_cap,
		                         sizeof *c->exports);
		if (grown == NULL)
			return -1;
		c->exports = grown;
	}
	memset(&c->exports[i], 0, sizeof c->exports[i]);
	c->exports[i].used = 1;
	c->exports[i].refcount = 1;
	c->exports[i].server = server;
	c->exports[i].dispatch = dispatch;
	c->exports[i].promise = 1;
	return i;
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
	if (t.importedCap >= (uint32_t) c->exports_cap)
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
	if (capn_len(pl.capTable) < 0 ||
	    cursor.cap_idx > (uint32_t) INT_MAX ||
	    (int) cursor.cap_idx >= capn_len(pl.capTable))
		goto out;
	get_CapDescriptor(&cd, pl.capTable, (int) cursor.cap_idx);
	{
		uint32_t hosted;
		if (cd.which == CapDescriptor_senderHosted)
			hosted = cd.senderHosted;
		else if (cd.which == CapDescriptor_senderPromise)
			hosted = cd.senderPromise;
		else
			goto out;
		if (hosted < (uint32_t) c->exports_cap && c->exports[hosted].used)
			eid = (int) hosted;
	}

out:
	capn_free(&stored);
	return eid;
}

/* Serialise `msg` into a buffer sized to it. The size is the message's own,
 * so a large payload goes out whole instead of failing against a fixed
 * ceiling the peer would never learn about. */
static uint8_t *frame_alloc(struct capn *msg, size_t *len_out)
{
	int64_t need = capn_size(msg);
	uint8_t *buf;
	int64_t sz;

	if (need <= 0 || (uint64_t) need > SIZE_MAX)
		return NULL;
	buf = (uint8_t *) malloc((size_t) need);
	if (buf == NULL)
		return NULL;
	sz = capn_write_mem(msg, buf, (size_t) need, 0);
	if (sz < 0) {
		free(buf);
		return NULL;
	}
	*len_out = (size_t) sz;
	return buf;
}

static int send_message(struct capn_rpc_conn *c, struct capn *msg)
{
	size_t len;
	uint8_t *buf = frame_alloc(msg, &len);
	int rc;

	if (buf == NULL)
		return -1;
	rc = c->send(c->send_ctx, buf, len);
	free(buf);
	return rc;
}

/* Send a Return and keep it until `Finish`, so a call pipelined against
 * this answer can still find the capability it names. */
static int send_answer(struct capn_rpc_conn *c, uint32_t qid, struct capn *msg)
{
	size_t len;
	uint8_t *buf = frame_alloc(msg, &len);
	int i, rc;

	if (buf == NULL)
		return -1;
	for (i = 0; i < c->answers_cap; i++) {
		if (!c->answers[i].used) {
			c->answers[i].used = 1;
			c->answers[i].question_id = qid;
			c->answers[i].frame = buf;
			c->answers[i].len = len;
			return c->send(c->send_ctx, buf, len);
		}
	}
	i = c->answers_cap;
	{
		void *grown = table_grow(c->answers, &c->answers_cap,
		                         sizeof *c->answers);
		if (grown == NULL) {
			free(buf);
			return -1;
		}
		c->answers = grown;
	}
	c->answers[i].used = 1;
	c->answers[i].question_id = qid;
	c->answers[i].frame = buf;
	c->answers[i].len = len;
	rc = c->send(c->send_ctx, buf, len);
	return rc;
}

static struct capn_rpc_answer *answer_find(struct capn_rpc_conn *c, uint32_t qid)
{
	int i;
	for (i = 0; i < c->answers_cap; i++)
		if (c->answers[i].used && c->answers[i].question_id == qid)
			return &c->answers[i];
	return NULL;
}

static void answer_drop(struct capn_rpc_conn *c, uint32_t qid)
{
	struct capn_rpc_answer *a = answer_find(c, qid);
	if (a) {
		free(a->frame);
		memset(a, 0, sizeof *a);
	}
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

/* Send the Return for a call whose results struct is filled, and release
 * the message it was built in. */
static int finish_call(struct capn_rpc_conn *c, uint32_t qid, struct capn *msg,
                       capn_ptr rstruct)
{
	struct Message m;
	struct Return r;
	struct Payload results;
	Message_ptr mp;
	Return_ptr rp;
	Payload_ptr plp;
	struct capn_segment *cs = rstruct.seg;
	int rc;

	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&results, 0, sizeof results);

	plp = new_Payload(cs);
	results.content = rstruct;
	attach_cap_table(c, cs, &results);
	write_Payload(&results, plp);

	rp = new_Return(cs);
	r.answerId = qid;
	r.which = Return_results;
	r.results = plp;
	write_Return(&r, rp);

	mp = new_Message(cs);
	m.which = Message__return;
	m._return = rp;
	write_Message(&m, mp);
	capn_setp(capn_root(msg), 0, mp.p);

	rc = send_answer(c, qid, msg);
	capn_free(msg);
	free(msg);
	return rc;
}

static struct capn_rpc_pending_call *pending_find_gen(struct capn_rpc_conn *c,
                                                      uint32_t qid,
                                                      uint32_t generation)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
		struct capn_rpc_pending_call *pc = &c->pending_calls[i];
		if (pc->used && pc->question_id == qid && pc->generation == generation)
			return pc;
	}
	return NULL;
}

static void pending_release(struct capn_rpc_pending_call *pc)
{
	capn_free(pc->msg);
	free(pc->msg);
	memset(pc, 0, sizeof *pc);
}

/* Finish cancels the live answer but keeps its message until return. */
static void pending_cancel(struct capn_rpc_conn *c, uint32_t qid)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
		struct capn_rpc_pending_call *pc = &c->pending_calls[i];
		if (pc->used && !pc->cancelled && pc->question_id == qid)
			pc->cancelled = 1;
	}
}

static uint32_t take_generation(struct capn_rpc_conn *c)
{
	c->answer_clock++;
	if (c->answer_clock == 0)
		c->answer_clock = 1;
	return c->answer_clock;
}

uint32_t capn_rpc_current_question(const struct capn_rpc_conn *c)
{
	return c->current_question;
}

uint32_t capn_rpc_current_generation(const struct capn_rpc_conn *c)
{
	return c->current_generation;
}

int capn_rpc_return(struct capn_rpc_conn *c, uint32_t question_id,
                    uint32_t generation)
{
	struct capn_rpc_pending_call *pc = pending_find_gen(c, question_id, generation);
	struct capn *msg;
	capn_ptr rstruct;

	if (pc == NULL)
		return -1;
	if (pc->cancelled) {
		pending_release(pc);
		return -1;
	}
	msg = pc->msg;
	rstruct = pc->results;
	memset(pc, 0, sizeof *pc);
	return finish_call(c, question_id, msg, rstruct);
}

int capn_rpc_return_exception(struct capn_rpc_conn *c, uint32_t question_id,
                              uint32_t generation, const char *reason)
{
	struct capn_rpc_pending_call *pc = pending_find_gen(c, question_id, generation);
	int cancelled;

	if (pc == NULL)
		return -1;
	cancelled = pc->cancelled;
	pending_release(pc);
	if (cancelled)
		return -1;
	return send_return_exception(c, question_id, reason ? reason : "call failed");
}

static int handle_call(struct capn_rpc_conn *c, Call_ptr cp,
                       const uint8_t *frame, size_t frame_len)
{
	struct capn *msg;
	struct Call call;
	struct Payload params;
	struct capn_segment *cs;
	capn_ptr rstruct;
	int eid, rc;

	read_Call(&call, cp);
	memset(&params, 0, sizeof params);
	/* The cap table describes the message, not the dispatch: a call this
	 * vat cannot route still told us where a third party's capability
	 * lives. */
	if (call.params.p.type != CAPN_NULL) {
		read_Payload(&params, call.params);
		note_introductions(c, &params);
	}

	eid = resolve_target(c, call.target);
	if (eid < 0)
		return send_return_exception(c, call.questionId, "no such export");
	if (c->exports[eid].broken)
		return send_return_exception(c, call.questionId, "broken promise");
	/* An unresolved promise cannot be answered yet. The frame is copied
	 * because the caller frees its buffer when handle returns. */
	if (c->exports[eid].promise) {
		if (queue_frame(c, eid, frame, frame_len) != 0)
			return send_return_exception(c, call.questionId,
			                             "promise queue full");
		return 0;
	}
	if (c->exports[eid].has_forward_import)
		return forward_one(c, c->exports[eid].forward_import, frame, frame_len);
	if (c->exports[eid].has_forward) {
		int dest = c->exports[eid].forward_export;
		if (dest < 0 || dest >= c->exports_cap || !c->exports[dest].used ||
		    c->exports[dest].dispatch == NULL)
			return send_return_exception(c, call.questionId,
			                             "resolved promise has no target");
		eid = dest;
	}

	msg = (struct capn *) malloc(sizeof *msg);
	if (msg == NULL)
		return send_return_exception(c, call.questionId, "out of memory");
	capn_init_malloc(msg);
	cs = capn_root(msg).seg;
	c->npending = 0;
	c->in_caps = params.capTable;
	c->current_question = call.questionId;
	c->current_generation = take_generation(c);

	/* Sized by capn_rpc_set_result_size: one data word and one pointer is
	 * enough for the bundled servers, and a richer one asks for more. */
	rstruct = capn_new_struct(cs, c->results_datasz, c->results_ptrs);
	if (rstruct.type == CAPN_NULL) {
		c->current_question = 0;
		c->current_generation = 0;
		capn_free(msg);
		free(msg);
		return send_return_exception(c, call.questionId, "results too large");
	}
	rc = c->exports[eid].dispatch(c->exports[eid].server, call.interfaceId,
	                              call.methodId, params.content, rstruct);
	memset(&c->in_caps, 0, sizeof c->in_caps);
	{
		uint32_t gen = c->current_generation;
		c->current_question = 0;
		c->current_generation = 0;

		if (rc == CAPN_RPC_PENDING) {
			int i;
			for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
				if (!c->pending_calls[i].used) {
					c->pending_calls[i].used = 1;
					c->pending_calls[i].cancelled = 0;
					c->pending_calls[i].question_id = call.questionId;
					c->pending_calls[i].generation = gen;
					c->pending_calls[i].msg = msg;
					c->pending_calls[i].results = rstruct;
					c->npending = 0;
					return 0;
				}
			}
			c->npending = 0;
			capn_free(msg);
			free(msg);
			return send_return_exception(c, call.questionId, "too many pending calls");
		}
	}
	if (rc != 0) {
		c->npending = 0;
		capn_free(msg);
		free(msg);
		return send_return_exception(c, call.questionId, "call failed");
	}
	return finish_call(c, call.questionId, msg, rstruct);
}

static void handle_release(struct capn_rpc_conn *c, Release_ptr rp)
{
	struct Release rel;
	read_Release(&rel, rp);
	if (rel.id >= (uint32_t) c->exports_cap || !c->exports[rel.id].used)
		return;
	/* Releasing more references than were handed out drops the export;
	 * the count is peer-controlled, so compare before subtracting. */
	if (c->exports[rel.id].refcount <= 0 ||
	    rel.referenceCount >= (uint32_t) c->exports[rel.id].refcount) {
		if (c->exports[rel.id].promise)
			flush_reject(c, (int) rel.id, "promise released");
		memset(&c->exports[rel.id], 0, sizeof c->exports[rel.id]);
	} else {
		c->exports[rel.id].refcount -= (int) rel.referenceCount;
	}
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

static int release_provide_embargo(struct capn_rpc_conn *c, uint32_t provide_qid);

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
	if (in.context_which == Disembargo_context_provide)
		return release_provide_embargo(c, in.context.provide);
	/* The echo of an embargo we raised. Deliver the calls that were
	 * waiting on it, and do not echo again. */
	if (in.context_which == Disembargo_context_receiverLoopback) {
		lift_embargo(c, in.context.receiverLoopback);
		return 0;
	}
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
	write_Message(&m, mp);
	/* The echoed message is the union's pointer, not a Message wrapped
	 * in another. capn_setp copies across messages, which is what this
	 * needs: the incoming message is freed as soon as this returns. */
	capn_setp(mp.p, 0, orig.p);
	capn_setp(capn_root(&msg), 0, mp.p);

	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}


/* --- client side -------------------------------------------------- */

static void question_clear(struct capn_rpc_question *q)
{
	free(q->reply);
	memset(q, 0, sizeof *q);
}

static struct capn_rpc_question *question_claim(struct capn_rpc_conn *c,
                                                uint32_t *qid_out)
{
	int i;
	for (i = 0; i < c->questions_cap; i++) {
		if (!c->questions[i].used) {
			question_clear(&c->questions[i]);
			c->questions[i].used = 1;
			c->questions[i].question_id = c->next_question_id++;
			*qid_out = c->questions[i].question_id;
			return &c->questions[i];
		}
	}
	i = c->questions_cap;
	{
		void *grown = table_grow(c->questions, &c->questions_cap,
		                         sizeof *c->questions);
		if (grown == NULL)
			return NULL;
		c->questions = grown;
	}
	c->questions[i].used = 1;
	c->questions[i].question_id = c->next_question_id++;
	*qid_out = c->questions[i].question_id;
	return &c->questions[i];
}

static struct capn_rpc_question *question_find(struct capn_rpc_conn *c,
                                               uint32_t qid)
{
	int i;
	for (i = 0; i < c->questions_cap; i++)
		if (c->questions[i].used && c->questions[i].question_id == qid)
			return &c->questions[i];
	return NULL;
}

uint32_t capn_rpc_send_provide(struct capn_rpc_conn *c, uint32_t imported_cap,
                               const char *recipient_host, uint16_t recipient_port,
                               uint64_t nonce)
{
	struct capn msg;
	struct Message m;
	struct Provide pv;
	struct MessageTarget tgt;
	struct RecipientId rid;
	struct VatId vat;
	Message_ptr mp;
	Provide_ptr pp;
	MessageTarget_ptr tp;
	RecipientId_ptr rp;
	VatId_ptr vp;
	struct capn_segment *cs;
	struct capn_rpc_question *q;
	uint32_t qid = 0;

	if (recipient_host == NULL)
		return (uint32_t)-1;
	q = question_claim(c, &qid);
	if (q == NULL)
		return (uint32_t)-1;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;

	memset(&vat, 0, sizeof vat);
	vat.host.str = recipient_host;
	vat.host.len = (int)strlen(recipient_host);
	/* seg stays NULL so capn_set_text copies into the message. */
	vat.host.seg = NULL;
	vat.port = recipient_port;
	vp = new_VatId(cs);
	write_VatId(&vat, vp);

	memset(&rid, 0, sizeof rid);
	rid.vat = vp;
	rid.nonce = nonce;
	rp = new_RecipientId(cs);
	write_RecipientId(&rid, rp);

	memset(&tgt, 0, sizeof tgt);
	tgt.which = MessageTarget_importedCap;
	tgt.importedCap = imported_cap;
	tp = new_MessageTarget(cs);
	write_MessageTarget(&tgt, tp);

	memset(&pv, 0, sizeof pv);
	pv.questionId = qid;
	pv.target = tp;
	pv.recipient = rp.p;
	pp = new_Provide(cs);
	write_Provide(&pv, pp);

	memset(&m, 0, sizeof m);
	m.which = Message_provide;
	m.provide = pp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	if (send_message(c, &msg) != 0) {
		question_clear(q);
		qid = (uint32_t)-1;
	}
	capn_free(&msg);
	return qid;
}

uint32_t capn_rpc_send_accept(struct capn_rpc_conn *c, uint64_t nonce, int embargo)
{
	struct capn msg;
	struct Message m;
	struct Accept ac;
	struct ProvisionId pid;
	Message_ptr mp;
	Accept_ptr ap;
	ProvisionId_ptr pip;
	struct capn_segment *cs;
	struct capn_rpc_question *q;
	uint32_t qid = 0;

	q = question_claim(c, &qid);
	if (q == NULL)
		return (uint32_t)-1;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;

	memset(&pid, 0, sizeof pid);
	pid.nonce = nonce;
	pip = new_ProvisionId(cs);
	write_ProvisionId(&pid, pip);

	memset(&ac, 0, sizeof ac);
	ac.questionId = qid;
	ac.provision = pip.p;
	ac.embargo = embargo ? 1 : 0;
	ap = new_Accept(cs);
	write_Accept(&ac, ap);

	memset(&m, 0, sizeof m);
	m.which = Message_accept;
	m.accept = ap;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	if (send_message(c, &msg) != 0) {
		question_clear(q);
		qid = (uint32_t)-1;
	}
	capn_free(&msg);
	return qid;
}

int capn_rpc_send_disembargo_provide(struct capn_rpc_conn *c, uint32_t provide_qid)
{
	struct capn msg;
	struct Message m;
	struct Disembargo d;
	Message_ptr mp;
	Disembargo_ptr dp;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&d, 0, sizeof d);
	d.context_which = Disembargo_context_provide;
	d.context.provide = provide_qid;
	dp = new_Disembargo(cs);
	write_Disembargo(&d, dp);
	memset(&m, 0, sizeof m);
	m.which = Message_disembargo;
	m.disembargo = dp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
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
		question_clear(q);
		qid = (uint32_t)-1;
	}
	capn_free(&msg);
	return qid;
}

/* Write the Payload for params or results: the content struct is already
 * filled, and any capability made while filling it is in the pending list. */
static void attach_cap_table(struct capn_rpc_conn *c, struct capn_segment *cs,
                             struct Payload *pl)
{
	int i;

	if (c->npending == 0)
		return;
	pl->capTable = new_CapDescriptor_list(cs, c->npending);
	for (i = 0; i < c->npending; i++) {
		struct CapDescriptor cd;
		memset(&cd, 0, sizeof cd);
		if (c->pending_caps[i].kind == CAPN_RPC_CAP_PROMISE) {
			cd.which = CapDescriptor_senderPromise;
			cd.senderPromise = c->pending_caps[i].id;
		} else if (c->pending_caps[i].kind == CAPN_RPC_CAP_ANSWER) {
			struct PromisedAnswer ans;
			PromisedAnswer_ptr pap;
			int n = c->pending_caps[i].npath;
			int k;

			memset(&ans, 0, sizeof ans);
			ans.questionId = c->pending_caps[i].question_id;
			if (n > 0) {
				ans.transform = new_PromisedAnswer_Op_list(cs, n);
				for (k = 0; k < n; k++) {
					struct PromisedAnswer_Op op;
					memset(&op, 0, sizeof op);
					op.which = PromisedAnswer_Op_getPointerField;
					op.getPointerField = c->pending_caps[i].path[k];
					set_PromisedAnswer_Op(&op, ans.transform, k);
				}
			}
			pap = new_PromisedAnswer(cs);
			write_PromisedAnswer(&ans, pap);
			cd.which = CapDescriptor_receiverAnswer;
			cd.receiverAnswer = pap;
		} else if (c->pending_caps[i].is_import) {
			cd.which = CapDescriptor_receiverHosted;
			cd.receiverHosted = c->pending_caps[i].id;
		} else {
			cd.which = CapDescriptor_senderHosted;
			cd.senderHosted = c->pending_caps[i].id;
		}
		set_CapDescriptor(&cd, pl->capTable, i);
	}
	c->npending = 0;
}

capn_ptr capn_rpc_new_export_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 void *server, capn_rpc_dispatch_fn dispatch)
{
	capn_ptr p;
	int eid;

	memset(&p, 0, sizeof p);
	if (c->npending >= CAPN_RPC_MAX_PENDING_CAPS)
		return p;
	eid = capn_rpc_export(c, server, dispatch);
	if (eid < 0)
		return p;
	p = capn_new_interface(seg, 0, 0);
	if (p.type == CAPN_NULL)
		return p;
	p.cap_idx = (uint32_t) c->npending;
	memset(&c->pending_caps[c->npending], 0, sizeof c->pending_caps[0]);
	c->pending_caps[c->npending].is_import = 0;
	c->pending_caps[c->npending].kind = CAPN_RPC_CAP_HOSTED;
	c->pending_caps[c->npending].id = (uint32_t) eid;
	c->npending++;
	return p;
}

capn_ptr capn_rpc_new_import_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 uint32_t import_id)
{
	capn_ptr p;

	memset(&p, 0, sizeof p);
	if (c->npending >= CAPN_RPC_MAX_PENDING_CAPS)
		return p;
	p = capn_new_interface(seg, 0, 0);
	if (p.type == CAPN_NULL)
		return p;
	p.cap_idx = (uint32_t) c->npending;
	memset(&c->pending_caps[c->npending], 0, sizeof c->pending_caps[0]);
	c->pending_caps[c->npending].is_import = 1;
	c->pending_caps[c->npending].kind = CAPN_RPC_CAP_HOSTED;
	c->pending_caps[c->npending].id = import_id;
	c->npending++;
	return p;
}

capn_ptr capn_rpc_new_promise_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                  void *server, capn_rpc_dispatch_fn dispatch)
{
	capn_ptr p;
	int eid;

	memset(&p, 0, sizeof p);
	if (c->npending >= CAPN_RPC_MAX_PENDING_CAPS)
		return p;
	eid = capn_rpc_export_promise(c, server, dispatch);
	if (eid < 0)
		return p;
	p = capn_new_interface(seg, 0, 0);
	if (p.type == CAPN_NULL)
		return p;
	p.cap_idx = (uint32_t) c->npending;
	memset(&c->pending_caps[c->npending], 0, sizeof c->pending_caps[0]);
	c->pending_caps[c->npending].kind = CAPN_RPC_CAP_PROMISE;
	c->pending_caps[c->npending].id = (uint32_t) eid;
	c->npending++;
	return p;
}

capn_ptr capn_rpc_new_answer_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 uint32_t question_id, const uint16_t *path,
                                 int npath)
{
	capn_ptr p;

	memset(&p, 0, sizeof p);
	if (c->npending >= CAPN_RPC_MAX_PENDING_CAPS)
		return p;
	if (npath < 0 || npath > CAPN_RPC_PATH_MAX || (npath > 0 && path == NULL))
		return p;
	p = capn_new_interface(seg, 0, 0);
	if (p.type == CAPN_NULL)
		return p;
	p.cap_idx = (uint32_t) c->npending;
	memset(&c->pending_caps[c->npending], 0, sizeof c->pending_caps[0]);
	c->pending_caps[c->npending].kind = CAPN_RPC_CAP_ANSWER;
	c->pending_caps[c->npending].question_id = question_id;
	c->pending_caps[c->npending].npath = npath;
	if (npath > 0)
		memcpy(c->pending_caps[c->npending].path, path,
		       (size_t) npath * sizeof path[0]);
	c->npending++;
	return p;
}

/* What the capTable entry a capability pointer indexes names. */
static int cap_lookup(CapDescriptor_list table, capn_ptr cap, int *is_export)
{
	struct CapDescriptor cd;

	capn_resolve(&cap);
	capn_resolve(&table.p);
	if (cap.type != CAPN_INTERFACE || table.p.type == CAPN_NULL)
		return -1;
	if (capn_ptr_len(table.p) < 0 ||
	    cap.cap_idx > (uint32_t) INT_MAX ||
	    (int) cap.cap_idx >= capn_ptr_len(table.p))
		return -1;
	get_CapDescriptor(&cd, table, (int) cap.cap_idx);
	if (cd.which == CapDescriptor_senderHosted ||
	    cd.which == CapDescriptor_senderPromise) {
		if (is_export)
			*is_export = 0;
		return (int) (cd.which == CapDescriptor_senderHosted ?
		              cd.senderHosted : cd.senderPromise);
	}
	if (cd.which == CapDescriptor_receiverHosted) {
		if (is_export)
			*is_export = 1;
		return (int) cd.receiverHosted;
	}
	if (cd.which == CapDescriptor_receiverAnswer &&
	    cd.receiverAnswer.p.type != CAPN_NULL) {
		struct PromisedAnswer pa;
		read_PromisedAnswer(&pa, cd.receiverAnswer);
		/* 2: the id is a question this vat asked, not an export. */
		if (is_export)
			*is_export = 2;
		return (int) pa.questionId;
	}
	return -1;
}

int capn_rpc_param_cap(struct capn_rpc_conn *c, capn_ptr cap, int *is_export)
{
	return cap_lookup(c->in_caps, cap, is_export);
}

static uint32_t send_call_impl(struct capn_rpc_conn *c, int pipelined,
                               uint32_t target_id, const uint16_t *path, int npath,
                               uint64_t interface_id, uint16_t method_id,
                               int params_datasz, int params_ptrs,
                               capn_rpc_fill_fn fill, void *fill_ctx,
                               struct capn_rpc_question *reuse, uint32_t reuse_qid)
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

	if (reuse != NULL) {
		q = reuse;
		qid = reuse_qid;
	} else {
		q = question_claim(c, &qid);
		if (q == NULL)
			return (uint32_t)-1;
	}

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&call, 0, sizeof call);
	memset(&t, 0, sizeof t);
	memset(&params, 0, sizeof params);
	c->npending = 0;

	if (pipelined) {
		struct PromisedAnswer pa;
		PromisedAnswer_ptr pap;
		int i;

		memset(&pa, 0, sizeof pa);
		pa.questionId = target_id;
		if (npath > 0) {
			pa.transform = new_PromisedAnswer_Op_list(cs, npath);
			for (i = 0; i < npath; i++) {
				struct PromisedAnswer_Op op;
				memset(&op, 0, sizeof op);
				op.which = PromisedAnswer_Op_getPointerField;
				op.getPointerField = path[i];
				set_PromisedAnswer_Op(&op, pa.transform, i);
			}
		}
		pap = new_PromisedAnswer(cs);
		write_PromisedAnswer(&pa, pap);
		t.which = MessageTarget_promisedAnswer;
		t.promisedAnswer = pap;
	} else {
		t.which = MessageTarget_importedCap;
		t.importedCap = target_id;
	}
	tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);

	params.content = capn_new_struct(cs, params_datasz, params_ptrs);
	if (fill)
		fill(fill_ctx, params.content);
	attach_cap_table(c, cs, &params);
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
		if (reuse != NULL) {
			q->answered = 1;
			q->failed = 1;
		} else {
			question_clear(q);
			qid = (uint32_t)-1;
		}
	}
	capn_free(&msg);
	return qid;
}

enum {
	CAPN_RPC_HELD_LOCAL = 1,
	CAPN_RPC_HELD_REMOTE = 2,
	CAPN_RPC_HELD_PIPE = 3
};

struct capn_rpc_copy {
	capn_ptr ptr;
};

static void fill_from_ptr(void *ctx, capn_ptr dest)
{
	struct capn_rpc_copy *src = (struct capn_rpc_copy *) ctx;
	capn_ptr from = src->ptr;
	int i, n;

	capn_resolve(&from);
	capn_resolve(&dest);
	if (from.type != CAPN_STRUCT || dest.type != CAPN_STRUCT)
		return;
	n = from.datasz < dest.datasz ? from.datasz : dest.datasz;
	if (n > 0 && from.data != NULL && dest.data != NULL)
		memcpy(dest.data, from.data, (size_t) n);
	n = from.ptrs < dest.ptrs ? from.ptrs : dest.ptrs;
	for (i = 0; i < n; i++)
		capn_setp(dest, i, capn_getp(from, i, 0));
}

static struct capn_rpc_import *import_slot(struct capn_rpc_conn *c, uint32_t id)
{
	if (id >= CAPN_RPC_MAX_IMPORTS)
		return NULL;
	while ((uint32_t) c->imports_cap <= id) {
		void *grown = table_grow(c->imports, &c->imports_cap, sizeof *c->imports);
		if (grown == NULL)
			return NULL;
		c->imports = grown;
	}
	if (!c->imports[id].used) {
		memset(&c->imports[id], 0, sizeof c->imports[id]);
		c->imports[id].used = 1;
	}
	return &c->imports[id];
}

static void note_import_call(struct capn_rpc_conn *c, uint32_t id)
{
	struct capn_rpc_import *im = import_slot(c, id);
	if (im != NULL && !im->resolved)
		im->calls++;
}

static void fail_existing(struct capn_rpc_question *q)
{
	if (q == NULL)
		return;
	q->answered = 1;
	q->failed = 1;
}

static uint32_t fail_question(struct capn_rpc_conn *c)
{
	uint32_t qid;
	struct capn_rpc_question *q = question_claim(c, &qid);
	if (q == NULL)
		return (uint32_t) -1;
	fail_existing(q);
	return qid;
}

static int queue_frame(struct capn_rpc_conn *c, int export_id,
                       const uint8_t *data, size_t len)
{
	int i;
	uint8_t *copy;

	if (data == NULL || len == 0)
		return -1;
	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++) {
		if (!c->queued[i].used)
			break;
	}
	if (i == CAPN_RPC_MAX_QUEUED)
		return -1;
	copy = (uint8_t *) malloc(len);
	if (copy == NULL)
		return -1;
	memcpy(copy, data, len);
	c->queued[i].used = 1;
	c->queued[i].export_id = export_id;
	c->queued[i].frame = copy;
	c->queued[i].len = len;
	return 0;
}

static int frame_question_id(const uint8_t *data, size_t len, uint32_t *qid)
{
	struct capn msg;
	struct Message m;
	struct Call call;
	Message_ptr mp;

	if (capn_init_mem(&msg, data, len, 0) != 0)
		return -1;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message_call) {
		capn_free(&msg);
		return -1;
	}
	read_Call(&call, m.call);
	*qid = call.questionId;
	capn_free(&msg);
	return 0;
}

static void flush_reject(struct capn_rpc_conn *c, int export_id, const char *reason)
{
	int i;

	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++) {
		uint32_t qid;
		if (!c->queued[i].used || c->queued[i].export_id != export_id)
			continue;
		if (frame_question_id(c->queued[i].frame, c->queued[i].len, &qid) == 0)
			send_return_exception(c, qid, reason ? reason : "promise released");
		free(c->queued[i].frame);
		memset(&c->queued[i], 0, sizeof c->queued[i]);
	}
}

static void flush_replay(struct capn_rpc_conn *c, int export_id)
{
	int i;

	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++) {
		uint8_t *frame;
		size_t len;
		if (!c->queued[i].used || c->queued[i].export_id != export_id)
			continue;
		frame = c->queued[i].frame;
		len = c->queued[i].len;
		memset(&c->queued[i], 0, sizeof c->queued[i]);
		capn_rpc_handle(c, frame, len);
		free(frame);
	}
}

static int relay_add(struct capn_rpc_conn *c, uint32_t local_qid, uint32_t remote_qid)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_RELAYS; i++) {
		if (!c->relays[i].used) {
			c->relays[i].used = 1;
			c->relays[i].local_qid = local_qid;
			c->relays[i].remote_qid = remote_qid;
			return 0;
		}
	}
	return -1;
}

static int send_copied_return(struct capn_rpc_conn *c, uint32_t remote_qid,
                              const uint8_t *frame, size_t len)
{
	struct capn in, out;
	struct Message m;
	struct Return r;
	struct Payload pl;
	struct Message om;
	struct Return orr;
	struct Payload opl;
	Message_ptr mp, omp;
	Return_ptr rp;
	Payload_ptr plp;
	struct capn_segment *cs;
	int rc, i, n;

	if (capn_init_mem(&in, frame, len, 0) != 0)
		return -1;
	mp.p = capn_getp(capn_root(&in), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message__return) {
		capn_free(&in);
		return -1;
	}
	read_Return(&r, m._return);
	if (r.which == Return_exception) {
		struct Exception ex;
		char *reason = NULL;
		int ret;
		memset(&ex, 0, sizeof ex);
		if (r.exception.p.type != CAPN_NULL)
			read_Exception(&ex, r.exception);
		if (ex.reason.str != NULL && ex.reason.len > 0) {
			reason = (char *) malloc((size_t) ex.reason.len + 1);
			if (reason != NULL) {
				memcpy(reason, ex.reason.str, (size_t) ex.reason.len);
				reason[ex.reason.len] = '\0';
			}
		}
		capn_free(&in);
		ret = send_return_exception(c, remote_qid, reason ? reason : "failed");
		free(reason);
		return ret;
	}
	if (r.which != Return_results) {
		capn_free(&in);
		return -1;
	}
	read_Payload(&pl, r.results);
	capn_resolve(&pl.content);

	capn_init_malloc(&out);
	cs = capn_root(&out).seg;
	memset(&om, 0, sizeof om);
	memset(&orr, 0, sizeof orr);
	memset(&opl, 0, sizeof opl);
	if (pl.content.type == CAPN_STRUCT) {
		opl.content = capn_new_struct(cs, pl.content.datasz, pl.content.ptrs);
		{
			struct capn_rpc_copy src;
			src.ptr = pl.content;
			fill_from_ptr(&src, opl.content);
		}
	}
	n = capn_len(pl.capTable);
	if (n > 0) {
		opl.capTable = new_CapDescriptor_list(cs, n);
		for (i = 0; i < n; i++) {
			struct CapDescriptor cd;
			get_CapDescriptor(&cd, pl.capTable, i);
			if (cd.which == CapDescriptor_receiverAnswer ||
			    cd.which == CapDescriptor_thirdPartyHosted) {
				capn_free(&in);
				capn_free(&out);
				return send_return_exception(c, remote_qid,
				                             "cannot relay capability");
			}
			set_CapDescriptor(&cd, opl.capTable, i);
		}
	}
	plp = new_Payload(cs);
	write_Payload(&opl, plp);
	orr.answerId = remote_qid;
	orr.which = Return_results;
	orr.results = plp;
	rp = new_Return(cs);
	write_Return(&orr, rp);
	om.which = Message__return;
	om._return = rp;
	omp = new_Message(cs);
	write_Message(&om, omp);
	capn_setp(capn_root(&out), 0, omp.p);
	rc = send_message(c, &out);
	capn_free(&in);
	capn_free(&out);
	return rc;
}

static void relay_on_return(struct capn_rpc_conn *c, uint32_t local_qid,
                            const uint8_t *frame, size_t len)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_RELAYS; i++) {
		uint32_t remote;
		if (!c->relays[i].used || c->relays[i].local_qid != local_qid)
			continue;
		remote = c->relays[i].remote_qid;
		c->relays[i].used = 0;
		send_copied_return(c, remote, frame, len);
		return;
	}
}

static int forward_one(struct capn_rpc_conn *c, uint32_t import_id,
                       const uint8_t *frame, size_t len)
{
	struct capn msg;
	struct Message m;
	struct Call call;
	struct Payload params;
	struct capn_rpc_copy src;
	Message_ptr mp;
	uint32_t local, remote;
	int datasz = 0, ptrs = 0;

	if (capn_init_mem(&msg, frame, len, 0) != 0)
		return -1;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message_call) {
		capn_free(&msg);
		return -1;
	}
	read_Call(&call, m.call);
	remote = call.questionId;
	memset(&params, 0, sizeof params);
	if (call.params.p.type != CAPN_NULL)
		read_Payload(&params, call.params);
	src.ptr = params.content;
	capn_resolve(&src.ptr);
	if (src.ptr.type == CAPN_STRUCT) {
		datasz = src.ptr.datasz;
		ptrs = src.ptr.ptrs;
	}
	local = capn_rpc_send_call(c, import_id, call.interfaceId, call.methodId,
	                           datasz, ptrs, fill_from_ptr, &src);
	capn_free(&msg);
	if (local == (uint32_t) -1) {
		send_return_exception(c, remote, "forward failed");
		return -1;
	}
	if (relay_add(c, local, remote) != 0) {
		send_return_exception(c, remote, "forward table full");
		return -1;
	}
	return 0;
}

static void flush_forward(struct capn_rpc_conn *c, int export_id, uint32_t import_id)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++) {
		if (!c->queued[i].used || c->queued[i].export_id != export_id)
			continue;
		forward_one(c, import_id, c->queued[i].frame, c->queued[i].len);
		free(c->queued[i].frame);
		memset(&c->queued[i], 0, sizeof c->queued[i]);
	}
}

static uint32_t dispatch_local(struct capn_rpc_conn *c, int export_id,
                               uint64_t interface_id, uint16_t method_id,
                               int params_datasz, int params_ptrs,
                               capn_rpc_fill_fn fill, void *fill_ctx,
                               struct capn_rpc_question *reuse, uint32_t reuse_qid)
{
	struct capn_rpc_question *q;
	uint32_t qid;
	struct capn pmsg, rmsg;
	capn_ptr params, results;
	struct capn_segment *cs;
	int rc, eid = export_id;

	if (eid >= 0 && eid < c->exports_cap && c->exports[eid].used &&
	    c->exports[eid].has_forward)
		eid = c->exports[eid].forward_export;
	if (eid < 0 || eid >= c->exports_cap || !c->exports[eid].used ||
	    c->exports[eid].broken || c->exports[eid].dispatch == NULL) {
		if (reuse != NULL) {
			fail_existing(reuse);
			return reuse_qid;
		}
		return fail_question(c);
	}
	if (reuse != NULL) {
		q = reuse;
		qid = reuse_qid;
	} else {
		q = question_claim(c, &qid);
		if (q == NULL)
			return (uint32_t) -1;
	}

	capn_init_malloc(&pmsg);
	params = capn_new_struct(capn_root(&pmsg).seg, params_datasz, params_ptrs);
	if (fill != NULL)
		fill(fill_ctx, params);

	capn_init_malloc(&rmsg);
	cs = capn_root(&rmsg).seg;
	results = capn_new_struct(cs, c->results_datasz, c->results_ptrs);
	c->npending = 0;
	rc = c->exports[eid].dispatch(c->exports[eid].server, interface_id,
	                              method_id, params, results);
	if (rc != 0) {
		fail_existing(q);
		capn_free(&pmsg);
		capn_free(&rmsg);
		return qid;
	}
	{
		struct Message m;
		struct Return r;
		struct Payload pl;
		Message_ptr mp;
		Return_ptr rp;
		Payload_ptr plp;
		uint8_t *buf;
		size_t len;

		memset(&m, 0, sizeof m);
		memset(&r, 0, sizeof r);
		memset(&pl, 0, sizeof pl);
		pl.content = results;
		attach_cap_table(c, cs, &pl);
		plp = new_Payload(cs);
		write_Payload(&pl, plp);
		r.answerId = qid;
		r.which = Return_results;
		r.results = plp;
		rp = new_Return(cs);
		write_Return(&r, rp);
		m.which = Message__return;
		m._return = rp;
		mp = new_Message(cs);
		write_Message(&m, mp);
		capn_setp(capn_root(&rmsg), 0, mp.p);
		buf = frame_alloc(&rmsg, &len);
		if (buf == NULL) {
			fail_existing(q);
		} else {
			free(q->reply);
			q->reply = buf;
			q->reply_len = len;
			q->answered = 1;
			q->failed = 0;
		}
	}
	capn_free(&pmsg);
	capn_free(&rmsg);
	return qid;
}

static uint32_t hold_call(struct capn_rpc_conn *c, int mode, uint32_t import_id,
                          uint32_t target, const uint16_t *path, int npath,
                          uint64_t interface_id, uint16_t method_id,
                          int datasz, int ptrs, capn_rpc_fill_fn fill, void *ctx)
{
	int i;
	struct capn *msg;
	struct capn_rpc_question *q;
	uint32_t qid;
	capn_ptr params;

	for (i = 0; i < CAPN_RPC_MAX_HELD; i++) {
		if (!c->held[i].used)
			break;
	}
	if (i == CAPN_RPC_MAX_HELD)
		return (uint32_t) -1;
	q = question_claim(c, &qid);
	if (q == NULL)
		return (uint32_t) -1;
	msg = (struct capn *) malloc(sizeof *msg);
	if (msg == NULL) {
		question_clear(q);
		return (uint32_t) -1;
	}
	capn_init_malloc(msg);
	params = capn_new_struct(capn_root(msg).seg, datasz, ptrs);
	if (fill != NULL)
		fill(ctx, params);
	memset(&c->held[i], 0, sizeof c->held[i]);
	c->held[i].used = 1;
	c->held[i].mode = mode;
	c->held[i].import_id = import_id;
	c->held[i].target = target;
	c->held[i].question_id = qid;
	c->held[i].interface_id = interface_id;
	c->held[i].method_id = method_id;
	c->held[i].datasz = datasz;
	c->held[i].ptrs = ptrs;
	c->held[i].msg = msg;
	c->held[i].params = params;
	if (path != NULL && npath > 0) {
		if (npath > CAPN_RPC_PATH_MAX)
			npath = CAPN_RPC_PATH_MAX;
		memcpy(c->held[i].path, path, (size_t) npath * sizeof path[0]);
		c->held[i].npath = npath;
	}
	return qid;
}

static void fire_held(struct capn_rpc_conn *c, uint32_t import_id)
{
	int i;
	for (i = 0; i < CAPN_RPC_MAX_HELD; i++) {
		struct capn_rpc_held h;
		struct capn_rpc_copy src;
		struct capn_rpc_question *q;
		if (!c->held[i].used || c->held[i].import_id != import_id)
			continue;
		h = c->held[i];
		c->held[i].used = 0;
		c->held[i].msg = NULL;
		q = question_find(c, h.question_id);
		src.ptr = h.params;
		if (q == NULL) {
			if (h.msg != NULL) {
				capn_free(h.msg);
				free(h.msg);
			}
			continue;
		}
		if (h.mode == CAPN_RPC_HELD_LOCAL) {
			dispatch_local(c, (int) h.target, h.interface_id, h.method_id,
			               h.datasz, h.ptrs, fill_from_ptr, &src, q,
			               h.question_id);
		} else if (h.mode == CAPN_RPC_HELD_PIPE) {
			send_call_impl(c, 1, h.target, h.path, h.npath, h.interface_id,
			               h.method_id, h.datasz, h.ptrs, fill_from_ptr, &src,
			               q, h.question_id);
		} else {
			note_import_call(c, h.target);
			send_call_impl(c, 0, h.target, NULL, 0, h.interface_id, h.method_id,
			               h.datasz, h.ptrs, fill_from_ptr, &src, q,
			               h.question_id);
		}
		if (h.msg != NULL) {
			capn_free(h.msg);
			free(h.msg);
		}
	}
}

static void lift_embargo(struct capn_rpc_conn *c, uint32_t embargo_id)
{
	int i;
	for (i = 0; i < c->imports_cap; i++) {
		if (c->imports[i].used && c->imports[i].embargoed &&
		    c->imports[i].embargo_id == embargo_id) {
			c->imports[i].embargoed = 0;
			fire_held(c, (uint32_t) i);
		}
	}
}

static int send_disembargo(struct capn_rpc_conn *c, uint32_t promise_id,
                           uint32_t embargo_id)
{
	struct capn msg;
	struct Message m;
	struct Disembargo d;
	struct MessageTarget t;
	Message_ptr mp;
	Disembargo_ptr dp;
	MessageTarget_ptr tp;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&d, 0, sizeof d);
	memset(&t, 0, sizeof t);
	t.which = MessageTarget_importedCap;
	t.importedCap = promise_id;
	tp = new_MessageTarget(cs);
	write_MessageTarget(&t, tp);
	d.target = tp;
	d.context_which = Disembargo_context_senderLoopback;
	d.context.senderLoopback = embargo_id;
	dp = new_Disembargo(cs);
	write_Disembargo(&d, dp);
	m.which = Message_disembargo;
	m.disembargo = dp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

static int send_resolve_id(struct capn_rpc_conn *c, uint32_t promise_id,
                           enum CapDescriptor_which which, uint32_t id)
{
	struct capn msg;
	struct Message m;
	struct Resolve r;
	struct CapDescriptor cd;
	Message_ptr mp;
	Resolve_ptr rp;
	CapDescriptor_ptr cdp;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&cd, 0, sizeof cd);
	cd.which = which;
	if (which == CapDescriptor_senderPromise)
		cd.senderPromise = id;
	else if (which == CapDescriptor_receiverHosted)
		cd.receiverHosted = id;
	else
		cd.senderHosted = id;
	cdp = new_CapDescriptor(cs);
	write_CapDescriptor(&cd, cdp);
	r.promiseId = promise_id;
	r.which = Resolve_cap;
	r.cap = cdp;
	rp = new_Resolve(cs);
	write_Resolve(&r, rp);
	m.which = Message_resolve;
	m.resolve = rp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

static int send_resolve_exception(struct capn_rpc_conn *c, uint32_t promise_id,
                                  const char *reason)
{
	struct capn msg;
	struct Message m;
	struct Resolve r;
	struct Exception ex;
	Message_ptr mp;
	Resolve_ptr rp;
	Exception_ptr ep;
	struct capn_segment *cs;
	int rc;

	if (reason == NULL)
		reason = "failed";
	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&ex, 0, sizeof ex);
	ex.reason.str = reason;
	ex.reason.len = (int) strlen(reason);
	ex.reason.seg = cs;
	ex.type = Exception_Type_failed;
	ep = new_Exception(cs);
	write_Exception(&ex, ep);
	r.promiseId = promise_id;
	r.which = Resolve_exception;
	r.exception = ep;
	rp = new_Resolve(cs);
	write_Resolve(&r, rp);
	m.which = Message_resolve;
	m.resolve = rp;
	mp = new_Message(cs);
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);
	rc = send_message(c, &msg);
	capn_free(&msg);
	return rc;
}

static int promise_index(struct capn_rpc_conn *c, int promise_id,
                         struct capn_rpc_export **out)
{
	if (promise_id < 0 || promise_id >= c->exports_cap)
		return -1;
	if (!c->exports[promise_id].used || !c->exports[promise_id].promise)
		return -1;
	*out = &c->exports[promise_id];
	return 0;
}

int capn_rpc_resolve(struct capn_rpc_conn *c, int promise_id, void *server,
                     capn_rpc_dispatch_fn dispatch)
{
	struct capn_rpc_export *ex;
	int target;

	if (promise_index(c, promise_id, &ex) != 0 || dispatch == NULL)
		return -1;
	if (server == ex->server && dispatch == ex->dispatch) {
		target = promise_id;
	} else {
		target = capn_rpc_export(c, server, dispatch);
		if (target < 0)
			return -1;
	}
	/* Re-read: export() may have grown the table. */
	ex = &c->exports[promise_id];
	ex->promise = 0;
	if (target != promise_id) {
		ex->has_forward = 1;
		ex->forward_export = target;
	}
	flush_replay(c, promise_id);
	return send_resolve_id(c, (uint32_t) promise_id, CapDescriptor_senderHosted,
	                       (uint32_t) target);
}

int capn_rpc_resolve_import(struct capn_rpc_conn *c, int promise_id,
                            uint32_t import_id)
{
	struct capn_rpc_export *ex;

	if (promise_index(c, promise_id, &ex) != 0)
		return -1;
	ex->promise = 0;
	ex->has_forward_import = 1;
	ex->forward_import = import_id;
	flush_forward(c, promise_id, import_id);
	return send_resolve_id(c, (uint32_t) promise_id, CapDescriptor_receiverHosted,
	                       import_id);
}

int capn_rpc_resolve_exception(struct capn_rpc_conn *c, int promise_id,
                               const char *reason)
{
	struct capn_rpc_export *ex;

	if (promise_index(c, promise_id, &ex) != 0)
		return -1;
	if (reason == NULL)
		reason = "failed";
	ex->promise = 0;
	ex->broken = 1;
	flush_reject(c, promise_id, reason);
	return send_resolve_exception(c, (uint32_t) promise_id, reason);
}

int capn_rpc_promise_state(const struct capn_rpc_conn *c, uint32_t import_id,
                           int *broken, int *to_export, uint32_t *target)
{
	const struct capn_rpc_import *im;

	if (broken)
		*broken = 0;
	if (to_export)
		*to_export = -1;
	if (target)
		*target = 0;
	if (import_id >= (uint32_t) c->imports_cap)
		return 0;
	im = &c->imports[import_id];
	if (!im->used || !im->resolved)
		return 0;
	if (im->broken) {
		if (broken)
			*broken = 1;
		return 1;
	}
	if (im->has_export) {
		if (to_export)
			*to_export = 1;
		if (target)
			*target = im->to_export;
	} else if (im->has_import) {
		if (to_export)
			*to_export = 0;
		if (target)
			*target = im->to_import;
	} else if (im->has_question) {
		if (to_export)
			*to_export = -1;
		if (target)
			*target = im->to_question;
	}
	return 1;
}

static int handle_resolve(struct capn_rpc_conn *c, Resolve_ptr rp)
{
	struct Resolve r;
	struct capn_rpc_import *im;
	struct CapDescriptor cd;

	if (rp.p.type == CAPN_NULL)
		return -1;
	read_Resolve(&r, rp);
	im = import_slot(c, r.promiseId);
	if (im == NULL)
		return -1;
	if (im->resolved)
		return 0;
	im->resolved = 1;
	if (r.which == Resolve_exception) {
		im->broken = 1;
		return 0;
	}
	if (r.which != Resolve_cap || r.cap.p.type == CAPN_NULL) {
		im->broken = 1;
		return 0;
	}
	read_CapDescriptor(&cd, r.cap);
	if (cd.which == CapDescriptor_senderHosted ||
	    cd.which == CapDescriptor_senderPromise) {
		im->has_import = 1;
		im->to_import = cd.which == CapDescriptor_senderHosted ?
		                cd.senderHosted : cd.senderPromise;
		import_slot(c, im->to_import);
		return 0;
	}
	if (cd.which == CapDescriptor_receiverHosted) {
		im->has_export = 1;
		im->to_export = cd.receiverHosted;
		if (im->calls > 0) {
			im->embargoed = 1;
			im->embargo_id = ++c->next_embargo;
			return send_disembargo(c, r.promiseId, im->embargo_id);
		}
		return 0;
	}
	if (cd.which == CapDescriptor_receiverAnswer &&
	    cd.receiverAnswer.p.type != CAPN_NULL) {
		struct PromisedAnswer pa;
		int n, i;
		read_PromisedAnswer(&pa, cd.receiverAnswer);
		im->has_question = 1;
		im->to_question = pa.questionId;
		n = capn_len(pa.transform);
		for (i = 0; i < n && im->to_npath < CAPN_RPC_PATH_MAX; i++) {
			struct PromisedAnswer_Op op;
			get_PromisedAnswer_Op(&op, pa.transform, i);
			if (op.which == PromisedAnswer_Op_getPointerField)
				im->to_path[im->to_npath++] = op.getPointerField;
		}
		if (im->calls > 0) {
			im->embargoed = 1;
			im->embargo_id = ++c->next_embargo;
			return send_disembargo(c, r.promiseId, im->embargo_id);
		}
		return 0;
	}
	im->broken = 1;
	return 0;
}

uint32_t capn_rpc_send_call(struct capn_rpc_conn *c, uint32_t imported_cap,
                            uint64_t interface_id, uint16_t method_id,
                            int params_datasz, int params_ptrs,
                            capn_rpc_fill_fn fill, void *fill_ctx)
{
	uint32_t id = imported_cap;
	int depth = 0;

	while (id < (uint32_t) c->imports_cap && c->imports[id].used &&
	       c->imports[id].resolved && depth < 8) {
		struct capn_rpc_import *im = &c->imports[id];
		int mode;
		uint32_t target;
		if (im->broken)
			return fail_question(c);
		if (im->has_export) {
			mode = CAPN_RPC_HELD_LOCAL;
			target = im->to_export;
		} else if (im->has_question) {
			mode = CAPN_RPC_HELD_PIPE;
			target = im->to_question;
		} else if (im->has_import) {
			mode = CAPN_RPC_HELD_REMOTE;
			target = im->to_import;
		} else {
			return fail_question(c);
		}
		if (im->embargoed) {
			return hold_call(c, mode, id, target, im->to_path, im->to_npath,
			                 interface_id, method_id, params_datasz, params_ptrs,
			                 fill, fill_ctx);
		}
		if (mode == CAPN_RPC_HELD_LOCAL) {
			return dispatch_local(c, (int) target, interface_id, method_id,
			                      params_datasz, params_ptrs, fill, fill_ctx,
			                      NULL, 0);
		}
		if (mode == CAPN_RPC_HELD_PIPE) {
			return capn_rpc_send_pipelined_call(c, im->to_question, im->to_path,
			                                    im->to_npath, interface_id, method_id,
			                                    params_datasz, params_ptrs, fill,
			                                    fill_ctx);
		}
		/* senderHosted of this same id means the promise settled in place. */
		if (target == id)
			break;
		id = target;
		depth++;
	}
	note_import_call(c, id);
	return send_call_impl(c, 0, id, NULL, 0, interface_id, method_id,
	                      params_datasz, params_ptrs, fill, fill_ctx, NULL, 0);
}

uint32_t capn_rpc_send_pipelined_call(struct capn_rpc_conn *c, uint32_t question_id,
                                      const uint16_t *path, int npath,
                                      uint64_t interface_id, uint16_t method_id,
                                      int params_datasz, int params_ptrs,
                                      capn_rpc_fill_fn fill, void *fill_ctx)
{
	if (npath < 0 || (npath > 0 && path == NULL))
		return (uint32_t)-1;
	return send_call_impl(c, 1, question_id, path, npath, interface_id, method_id,
	                      params_datasz, params_ptrs, fill, fill_ctx, NULL, 0);
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
		question_clear(q);

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

int capn_rpc_answer_cap_id(struct capn_rpc_conn *c, uint32_t question_id)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	struct Payload pl;
	struct CapDescriptor cd;
	Message_ptr mp;
	struct capn_rpc_question *q = question_find(c, question_id);
	int id = -1;

	if (q == NULL || !q->answered || q->failed)
		return -1;
	if (capn_init_mem(&msg, q->reply, q->reply_len, 0) != 0)
		return -1;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message__return)
		goto out;
	read_Return(&r, m._return);
	if (r.which != Return_results)
		goto out;
	read_Payload(&pl, r.results);
	/* Both may be far pointers into another segment. */
	capn_resolve(&pl.content);
	capn_resolve(&pl.capTable.p);
	if (pl.content.type != CAPN_INTERFACE)
		goto out;
	/* The content pointer holds a capTable index; the descriptor beside
	 * it names the export. */
	if (pl.capTable.p.type == CAPN_NULL)
		goto out;
	if (capn_ptr_len(pl.capTable.p) < 0 ||
	    pl.content.cap_idx > (uint32_t) INT_MAX ||
	    (int) pl.content.cap_idx >= capn_ptr_len(pl.capTable.p))
		goto out;
	/* A struct list: read the element through the generated accessor,
	 * not as a pointer list. */
	get_CapDescriptor(&cd, pl.capTable, (int) pl.content.cap_idx);
	if (cd.which == CapDescriptor_senderHosted)
		id = (int)cd.senderHosted;
	else if (cd.which == CapDescriptor_senderPromise)
		id = (int)cd.senderPromise;
	else if (cd.which == CapDescriptor_receiverHosted)
		id = (int)cd.receiverHosted;

out:
	capn_free(&msg);
	return id;
}

int capn_rpc_answer_cap(struct capn_rpc_conn *c, uint32_t question_id, capn_ptr cap,
                        int *is_export)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	struct Payload pl;
	Message_ptr mp;
	struct capn_rpc_question *q = question_find(c, question_id);
	int id = -1;

	if (q == NULL || !q->answered || q->failed)
		return -1;
	if (capn_init_mem(&msg, q->reply, q->reply_len, 0) != 0)
		return -1;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	read_Message(&m, mp);
	if (m.which == Message__return) {
		read_Return(&r, m._return);
		if (r.which == Return_results) {
			read_Payload(&pl, r.results);
			id = cap_lookup(pl.capTable, cap, is_export);
		}
	}
	capn_free(&msg);
	return id;
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
	if (q == NULL || q->answered)
		return;
	{
		uint8_t *copy = (uint8_t *) malloc(len ? len : 1);
		if (copy == NULL)
			return;
		memcpy(copy, frame, len);
		q->reply = copy;
		q->reply_len = len;
	}
	q->answered = 1;
	q->failed = (r.which != Return_results);
	if (r.which == Return_results && r.results.p.type != CAPN_NULL) {
		struct Payload pl;
		read_Payload(&pl, r.results);
		note_introductions(c, &pl);
	}
	relay_on_return(c, r.answerId, frame, len);
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

/* Answer an embargoed Accept with the capability it claimed. */
static int send_accepted_cap(struct capn_rpc_conn *c, uint32_t qid, int eid)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	struct Payload pl;
	Message_ptr mp;
	Return_ptr rp;
	Payload_ptr plp;
	struct capn_segment *cs;
	int rc;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&pl, 0, sizeof pl);
	plp = new_Payload(cs);
	write_cap_payload(cs, &pl, eid, &pl.content);
	write_Payload(&pl, plp);
	r.answerId = qid;
	r.which = Return_results;
	r.results = plp;
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

/* Disembargo.provide: the introducer lifts the embargo on the Accept it
 * arranged, naming its own Provide question. */
static int release_provide_embargo(struct capn_rpc_conn *c, uint32_t provide_qid)
{
	int i;

	for (i = 0; i < CAPN_RPC_MAX_PROVISIONS; i++) {
		if (!c->vat->provisions[i].used || !c->vat->provisions[i].embargoed)
			continue;
		if (c->vat->provisions[i].question_id != provide_qid)
			continue;
		{
			uint32_t qid = c->vat->provisions[i].accept_question_id;
			int eid = c->vat->provisions[i].export_id;
			memset(&c->vat->provisions[i], 0, sizeof c->vat->provisions[i]);
			return send_accepted_cap(c, qid, eid);
		}
	}
	/* A Disembargo naming nothing we hold is the sender's problem, not
	 * a reason to disturb this connection. */
	return 0;
}

int capn_rpc_embargoed_accepts(struct capn_rpc_conn *c)
{
	int i, n = 0;
	for (i = 0; i < CAPN_RPC_MAX_PROVISIONS; i++)
		if (c->vat->provisions[i].used && c->vat->provisions[i].embargoed)
			n++;
	return n;
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
		if (!c->vat->provisions[i].used) {
			c->vat->provisions[i].used = 1;
			c->vat->provisions[i].nonce = rid.nonce;
			c->vat->provisions[i].server = c->exports[eid].server;
			c->vat->provisions[i].dispatch = c->exports[eid].dispatch;
			c->vat->provisions[i].export_id = eid;
			c->vat->provisions[i].question_id = pv.questionId;
			c->vat->provisions[i].embargoed = 0;
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
		if (c->vat->provisions[i].used && !c->vat->provisions[i].embargoed &&
		    c->vat->provisions[i].nonce == pid.nonce) {
			eid = capn_rpc_export(c, c->vat->provisions[i].server,
			                      c->vat->provisions[i].dispatch);
			if (eid < 0)
				return send_return_exception(c, ac.questionId,
				                             "accept: export table full");
			c->vat->provisions[i].export_id = eid;
			if (ac.embargo) {
				/* Claimed, but the Return waits: the recipient
				 * has pipelined calls in flight through the
				 * introducer, and answering now would let a
				 * later call overtake them. The introducer
				 * lifts it with Disembargo.provide. */
				c->vat->provisions[i].embargoed = 1;
				c->vat->provisions[i].accept_question_id = ac.questionId;
			} else {
				c->vat->provisions[i].used = 0;
			}
			break;
		}
	}
	if (eid < 0)
		return send_return_exception(c, ac.questionId,
		                             "accept: no such provision");
	if (ac.embargo)
		return 0;

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
		if (c->vat->provisions[i].used) {
			if (out && n < cap)
				out[n] = c->vat->provisions[i].nonce;
			n++;
		}
	}
	return n;
}

/* Fail every question still waiting for a Return. */
static void connection_abort(struct capn_rpc_conn *c)
{
	int i;

	c->aborted = 1;
	for (i = 0; i < c->questions_cap; i++) {
		struct capn_rpc_question *q = &c->questions[i];
		if (q->used && !q->answered) {
			q->answered = 1;
			q->failed = 1;
		}
	}
}

/* An Unimplemented that echoes one of our Calls or Bootstraps answers that
 * question: it will never get a Return, so it fails now rather than
 * waiting for ever. Echoes of other messages need no action. */
static void handle_unimplemented(struct capn_rpc_conn *c, Message_ptr orig)
{
	struct Message m;
	struct capn_rpc_question *q = NULL;

	if (orig.p.type == CAPN_NULL)
		return;
	read_Message(&m, orig);
	if (m.which == Message_call) {
		struct Call call;
		read_Call(&call, m.call);
		q = question_find(c, call.questionId);
	} else if (m.which == Message_bootstrap) {
		struct Bootstrap b;
		read_Bootstrap(&b, m.bootstrap);
		q = question_find(c, b.questionId);
	}
	if (q != NULL && !q->answered) {
		q->answered = 1;
		q->failed = 1;
	}
}

int capn_rpc_send_abort(struct capn_rpc_conn *c, const char *reason)
{
	struct capn msg;
	struct Message m;
	struct Exception ex;
	Message_ptr mp;
	Exception_ptr ep;
	struct capn_segment *cs;
	int rc;

	if (reason == NULL)
		reason = "aborted";
	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&ex, 0, sizeof ex);

	ep = new_Exception(cs);
	ex.reason.str = reason;
	ex.reason.len = (int)strlen(reason);
	ex.type = Exception_Type_failed;
	write_Exception(&ex, ep);

	mp = new_Message(cs);
	m.which = Message_abort;
	m.abort = ep;
	write_Message(&m, mp);
	capn_setp(capn_root(&msg), 0, mp.p);

	rc = send_message(c, &msg);
	capn_free(&msg);
	connection_abort(c);
	return rc;
}

int capn_rpc_is_aborted(const struct capn_rpc_conn *c)
{
	return c->aborted;
}

int capn_rpc_handle(struct capn_rpc_conn *c, const uint8_t *data, size_t len)
{
	struct capn msg;
	struct Message m;
	Message_ptr mp;
	capn_ptr root;
	int rc = 0;

	if (c->aborted)
		return -1;
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
		rc = handle_call(c, m.call, data, len);
		break;
	case Message_finish: {
		/* The caller is done with the answer, so the results it might
		 * have pipelined against can go. */
		struct Finish fin;
		read_Finish(&fin, m.finish);
		answer_drop(c, fin.questionId);
		/* Finish before the Return cancels it. The message stays until
		 * the matching return, which then sends nothing. */
		pending_cancel(c, fin.questionId);
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
		rc = handle_resolve(c, m.resolve);
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
	case Message_unimplemented:
		/* The peer did not understand something we sent. Never answer
		 * with Unimplemented ourselves: two vats doing that to each
		 * other would loop for ever. */
		handle_unimplemented(c, m.unimplemented);
		break;
	case Message_abort:
		/* The peer is giving up on the connection: nothing it asked is
		 * coming back, and nothing more will be accepted from it. */
		connection_abort(c);
		break;
	default:
		/* The obsolete save/delete messages. */
		rc = send_unimplemented(c, mp);
		break;
	}

	capn_free(&msg);
	return rc;
}

/* --- blocking file descriptor transport ------------------------------------ */

#if defined(__unix__) || defined(__unix) || defined(__APPLE__)
#include <errno.h>
#include <unistd.h>

static int fd_write_all(int fd, const uint8_t *p, size_t n)
{
	while (n > 0) {
		ssize_t w = write(fd, p, n);
		if (w < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		p += w;
		n -= (size_t) w;
	}
	return 0;
}

/* Read exactly n bytes; 1 on end of stream before the first byte. */
static int fd_read_all(int fd, uint8_t *p, size_t n, int allow_eof)
{
	size_t got = 0;
	while (got < n) {
		ssize_t r = read(fd, p + got, n - got);
		if (r < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (r == 0)
			return (got == 0 && allow_eof) ? 1 : -1;
		got += (size_t) r;
	}
	return 0;
}

int capn_rpc_fd_send(void *ctx, const uint8_t *data, size_t len)
{
	return fd_write_all(*(const int *) ctx, data, len);
}

int capn_rpc_fd_read_frame(int fd, uint8_t **frame, size_t *len)
{
	uint8_t head[4];
	uint8_t *buf;
	uint32_t nsegs, i;
	size_t table, total = 0, off;
	int rc;

	*frame = NULL;
	*len = 0;
	rc = fd_read_all(fd, head, 4, 1);
	if (rc != 0)
		return rc;
	/* Adding one to a count of 2^32-1 would wrap to zero segments. */
	nsegs = (uint32_t) head[0] | (uint32_t) head[1] << 8 | (uint32_t) head[2] << 16 |
	        (uint32_t) head[3] << 24;
	if (nsegs >= 1024u)
		return -1;
	nsegs += 1u;
	/* [count-1][size x count][padding to a word] */
	table = 4 + 4 * (size_t) nsegs;
	table = (table + 7) & ~(size_t) 7;
	buf = (uint8_t *) malloc(table);
	if (buf == NULL)
		return -1;
	memcpy(buf, head, 4);
	if (fd_read_all(fd, buf + 4, table - 4, 0) != 0)
		goto fail;
	for (i = 0; i < nsegs; i++) {
		const uint8_t *e = buf + 4 + 4 * (size_t) i;
		uint32_t words = (uint32_t) e[0] | (uint32_t) e[1] << 8 | (uint32_t) e[2] << 16 |
		                 (uint32_t) e[3] << 24;
		if ((size_t) words > SIZE_MAX / 8)
			goto fail;
		if ((size_t) words * 8 > SIZE_MAX - total)
			goto fail;
		total += (size_t) words * 8;
		if (total > 64u * 1024u * 1024u)
			goto fail;
	}
	if (total > SIZE_MAX - table)
		goto fail;
	{
		uint8_t *grown = (uint8_t *) realloc(buf, table + total);
		if (grown == NULL)
			goto fail;
		buf = grown;
	}
	off = table;
	if (fd_read_all(fd, buf + off, total, 0) != 0)
		goto fail;
	*frame = buf;
	*len = table + total;
	return 0;
fail:
	free(buf);
	return -1;
}

int capn_rpc_fd_pump(struct capn_rpc_conn *c, int fd)
{
	uint8_t *frame;
	size_t len;
	int rc = capn_rpc_fd_read_frame(fd, &frame, &len);

	if (rc != 0)
		return rc;
	rc = capn_rpc_handle(c, frame, len);
	free(frame);
	return rc == 0 ? 0 : -1;
}
#endif /* unix */
