/* Two-party Cap'n Proto RPC vat. See capn-rpc.h. */

#include "capn-rpc.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpc.capnp.h"
/* One network layer, not two. rpc-threeparty.capnp defines the join keys
 * as well as the third-party ids, and both files declare the same C
 * names, so including either alongside the other would not compile. */
#include "rpc-threeparty.capnp.h"

/* Defined below, beside the rest of the answer table. */
static void attach_cap_table(struct capn_segment *cs, struct Payload *pl,
                             struct capn_rpc_pending_cap *caps, int n);
static void release_unsent_caps(struct capn_rpc_conn *c,
                                struct capn_rpc_pending_cap *caps, int n);
static int frame_question_id(const uint8_t *data, size_t len, uint32_t *qid);
static int question_id_in_use(struct capn_rpc_conn *c, uint32_t qid);
static int abort_live_question(struct capn_rpc_conn *c, const uint32_t *answer_id);
static struct capn_rpc_answer *answer_find(struct capn_rpc_conn *c,
                                           uint32_t qid);

/* Grow `table` and return the new pointer. The caller stores it back
 * into the typed slot. Writing through a void ** would alias a
 * struct pointer, which compilers may treat as a different object. */
static void *table_grow(void *table, int *cap, size_t item_size, int max_cap)
{
	void *next;
	int old = *cap;
	int next_cap;

	if (max_cap < CAPN_RPC_INITIAL_TABLE_CAPACITY)
		max_cap = CAPN_RPC_INITIAL_TABLE_CAPACITY;
	if (old <= 0)
		next_cap = CAPN_RPC_INITIAL_TABLE_CAPACITY;
	else if (old >= max_cap || old > max_cap / 2)
		return NULL;
	else
		next_cap = old * 2;
	if (next_cap > max_cap)
		return NULL;
	if (item_size == 0 || (size_t) next_cap > SIZE_MAX / item_size)
		return NULL;
	/* A fresh block, not realloc. realloc may extend in place, and a
	 * caller that kept a pointer into the old block would not fault
	 * until the next move. Freeing the old block makes that a
	 * use-after-free on every grow. */
	next = malloc((size_t) next_cap * item_size);
	if (next == NULL)
		return NULL;
	if (table != NULL && old > 0)
		memcpy(next, table, (size_t) old * item_size);
	free(table);
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
static void relay_on_return(struct capn_rpc_conn *c, uint32_t local_qid,
                            const uint8_t *frame, size_t len);
static int handle_resolve(struct capn_rpc_conn *c, Resolve_ptr rp);
static void question_clear(struct capn_rpc_question *q);
static void free_cap_paths(struct capn_rpc_pending_cap *caps, int n);
static void release_export_ref(struct capn_rpc_conn *c, uint32_t id);
static void stash_drop(struct capn_rpc_conn *c, uint32_t qid, int release);

/* What a call or disembargo target names, after walking a promised
 * answer into the cap table of the Return that carried it. */
enum {
	CAPN_RES_NONE = 0,
	CAPN_RES_EXPORT = 1,
	CAPN_RES_REFLECT = 2,
	CAPN_RES_PIPE = 3
};

struct capn_rpc_resolved {
	int kind;
	int export_id;
	uint32_t import_id;
	uint32_t question_id;
	int npath;
	uint16_t *path;
};

struct capn_rpc_copy {
	capn_ptr ptr;
};

static void fill_from_ptr(void *ctx, capn_ptr dest);
static int reflect_send(struct capn_rpc_conn *c, int pipelined,
                        uint32_t target_id, const uint16_t *path, int npath,
                        const uint8_t *frame, size_t len);
static void resolve_message_target(struct capn_rpc_conn *c, MessageTarget_ptr tp,
                                   struct capn_rpc_resolved *out);
static int question_is_open(struct capn_rpc_conn *c, uint32_t qid);
static int queue_on_question(struct capn_rpc_conn *c, uint32_t question_id,
                             const uint8_t *data, size_t len);
static void flush_questions(struct capn_rpc_conn *c, uint32_t question_id);
static void reject_questions(struct capn_rpc_conn *c, uint32_t question_id,
                             const char *reason);
static void flush_echoes(struct capn_rpc_conn *c, int export_id);
static void deliver_tail(struct capn_rpc_conn *c, uint32_t peer_qid);
static void note_pipe(struct capn_rpc_conn *c, uint32_t question_id,
                      const uint16_t *path, int npath);
static void apply_pipeline_notes(struct capn_rpc_conn *c,
                                 struct capn_rpc_question *q,
                                 struct Payload *pl);
static int relay_add(struct capn_rpc_conn *c, uint32_t local_qid, uint32_t remote_qid);
static struct capn_rpc_import *import_slot(struct capn_rpc_conn *c, uint32_t id);
static int send_disembargo(struct capn_rpc_conn *c, uint32_t promise_id,
                           uint32_t embargo_id);

/* One finished question's param exports, until its Return arrives. */
struct capn_rpc_param_stash {
	int used;
	uint32_t question_id;
	uint32_t *ids;
	int n;
};

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

static void drop_intro_hold(struct capn_rpc_provision *pr);

/* A freed connection must not stay named by a provision. The next
 * Accept or Disembargo would otherwise follow the dead pointer. */
static void detach_provisions(struct capn_rpc_conn *c)
{
	int i;

	if (!c->vat)
		return;
	for (i = 0; i < CAPN_RPC_MAX_PROVISIONS; i++) {
		struct capn_rpc_provision *pr = &c->vat->provisions[i];
		if (!pr->used)
			continue;
		if (pr->accept_conn == c) {
			if (pr->intro_conn && pr->intro_conn != c)
				drop_intro_hold(pr);
			memset(pr, 0, sizeof *pr);
		} else if (pr->intro_conn == c) {
			pr->intro_conn = NULL;
			pr->intro_export = -1;
		}
	}
}

void capn_rpc_free(struct capn_rpc_conn *c)
{
	int i;

	detach_provisions(c);
	for (i = 0; i < c->answers_cap; i++) {
		free(c->answers[i].held_imports);
		free(c->answers[i].frame);
	}
	for (i = 0; i < c->questions_cap; i++)
		question_clear(&c->questions[i]);
	if (c->param_stash != NULL) {
		for (i = 0; i < c->param_stash_cap; i++)
			free(c->param_stash[i].ids);
		free(c->param_stash);
		c->param_stash = NULL;
		c->param_stash_cap = 0;
	}
	release_unsent_caps(c, c->pending_caps, c->npending);
	for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
		free_cap_paths(c->pending_calls[i].caps, c->pending_calls[i].ncaps);
		if (c->pending_calls[i].used) {
			release_unsent_caps(c, c->pending_calls[i].caps,
			                    c->pending_calls[i].ncaps);
			capn_free(c->pending_calls[i].msg);
			free(c->pending_calls[i].msg);
		}
		free(c->pending_calls[i].param_imports);
	}
	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++)
		free(c->queued[i].frame);
	for (i = 0; i < CAPN_RPC_MAX_HELD; i++) {
		int k;
		if (c->held[i].used && c->held[i].msg != NULL) {
			capn_free(c->held[i].msg);
			free(c->held[i].msg);
		}
		free(c->held[i].path);
		for (k = 0; k < c->held[i].ncaps; k++)
			free(c->held[i].caps[k].path);
	}
	for (i = 0; i < c->imports_cap; i++)
		free(c->imports[i].to_path);
	for (i = 0; i < c->npending; i++)
		free(c->pending_caps[i].path);
	for (i = 0; i < CAPN_RPC_MAX_ECHOES; i++)
		free(c->echoes[i].frame);
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
		                         sizeof *c->exports,
		                         CAPN_RPC_MAX_TABLE_CAPACITY);
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
		                         sizeof *c->exports,
		                         CAPN_RPC_MAX_TABLE_CAPACITY);
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
static int send_answer(struct capn_rpc_conn *c, uint32_t qid, struct capn *msg,
                       int transmit)
{
	size_t len;
	uint8_t *buf = frame_alloc(msg, &len);
	int i, rc;

	if (buf == NULL)
		return -1;
	/* No producer reaches this with the id already answered: each one
	 * checks question_id_in_use first, and a cancelled call sends
	 * nothing. Kept so a second Return cannot overwrite the first. */
	if (answer_find(c, qid) != NULL) {
		free(buf);
		if (!c->aborted)
			capn_rpc_send_abort(c,
			    "expected !answer.active; questionId is already in use");
		return -1;
	}
	for (i = 0; i < c->answers_cap; i++) {
		if (!c->answers[i].used) {
			c->answers[i].used = 1;
			c->answers[i].question_id = qid;
			c->answers[i].frame = buf;
			c->answers[i].len = len;
			c->answers[i].redirected = !transmit;
			c->answers[i].taken = 0;
			c->answers[i].nheld = 0;
			if (!transmit)
				return 0;
			return c->send(c->send_ctx, buf, len);
		}
	}
	i = c->answers_cap;
	{
		void *grown = table_grow(c->answers, &c->answers_cap,
		                         sizeof *c->answers,
		                         CAPN_RPC_MAX_TABLE_CAPACITY);
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
	c->answers[i].redirected = !transmit;
	c->answers[i].taken = 0;
	c->answers[i].nheld = 0;
	if (!transmit)
		return 0;
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
		int i;
		for (i = 0; i < a->nheld; i++)
			capn_rpc_send_release(c, a->held_imports[i], 1);
		free(a->held_imports);
		free(a->frame);
		memset(a, 0, sizeof *a);
	}
}

/* Drop one reference on an export that will not stay with the peer. */
static void release_export_ref(struct capn_rpc_conn *c, uint32_t id)
{
	if (id >= (uint32_t) c->exports_cap || !c->exports[id].used)
		return;
	if (c->exports[id].refcount <= 1)
		memset(&c->exports[id], 0, sizeof c->exports[id]);
	else
		c->exports[id].refcount--;
}

/* rpc.c++ keeps Question::paramExports until Return, including when
 * Finish was already sent. `release` is Return.releaseParamCaps. */
static void stash_drop(struct capn_rpc_conn *c, uint32_t qid, int release)
{
	int i;

	if (c->param_stash == NULL)
		return;
	for (i = 0; i < c->param_stash_cap; i++) {
		struct capn_rpc_param_stash *s = &c->param_stash[i];
		int k;

		if (!s->used || s->question_id != qid)
			continue;
		if (release) {
			for (k = 0; k < s->n; k++)
				release_export_ref(c, s->ids[k]);
		}
		free(s->ids);
		memset(s, 0, sizeof *s);
	}
}

static void stash_save(struct capn_rpc_conn *c, struct capn_rpc_question *q)
{
	struct capn_rpc_param_stash *s;
	int i, slot;

	if (q->nparam_exports <= 0 || q->param_exports == NULL)
		return;
	s = NULL;
	slot = -1;
	for (i = 0; i < c->param_stash_cap; i++) {
		if (c->param_stash[i].used &&
		    c->param_stash[i].question_id == q->question_id) {
			s = &c->param_stash[i];
			break;
		}
		if (slot < 0 && !c->param_stash[i].used)
			slot = i;
	}
	if (s == NULL) {
		if (slot < 0) {
			int old = c->param_stash_cap;
			void *grown = table_grow(c->param_stash, &c->param_stash_cap,
			                         sizeof *c->param_stash,
			                         CAPN_RPC_MAX_TABLE_CAPACITY);
			if (grown == NULL)
				return;
			c->param_stash = grown;
			slot = old;
		}
		s = &c->param_stash[slot];
	} else {
		free(s->ids);
	}
	s->used = 1;
	s->question_id = q->question_id;
	s->ids = q->param_exports;
	s->n = q->nparam_exports;
	q->param_exports = NULL;
	q->nparam_exports = 0;
}

static void release_unsent_caps(struct capn_rpc_conn *c,
                                struct capn_rpc_pending_cap *caps, int n)
{
	int i;

	if (caps == NULL || n <= 0)
		return;
	for (i = 0; i < n; i++) {
		if (caps[i].kind == CAPN_RPC_CAP_ANSWER || caps[i].is_import)
			continue;
		release_export_ref(c, caps[i].id);
	}
}

/* Release the senderHosted and senderPromise exports carried by an
 * answer's Return, the resultExports that rpc.c++ handleFinish drops
 * when Finish sets releaseResultCaps (the default). */
static void release_answer_result_caps(struct capn_rpc_conn *c,
                                       struct capn_rpc_answer *a)
{
	struct capn stored;
	struct Message m;
	struct Return r;
	struct Payload pl;
	Message_ptr mp;
	int n, i;

	if (a == NULL || a->frame == NULL)
		return;
	if (capn_init_mem(&stored, a->frame, a->len, 0) != 0)
		return;
	mp.p = capn_getp(capn_root(&stored), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message__return)
		goto out;
	read_Return(&r, m._return);
	if (r.which != Return_results)
		goto out;
	read_Payload(&pl, r.results);
	if (pl.capTable.p.type == CAPN_NULL)
		goto out;
	n = capn_len(pl.capTable);
	for (i = 0; i < n; i++) {
		struct CapDescriptor cd;
		get_CapDescriptor(&cd, pl.capTable, i);
		if (cd.which == CapDescriptor_senderHosted)
			release_export_ref(c, cd.senderHosted);
		else if (cd.which == CapDescriptor_senderPromise)
			release_export_ref(c, cd.senderPromise);
	}
out:
	capn_free(&stored);
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

static int return_exception(struct capn_rpc_conn *c, uint32_t qid,
                            const char *reason, int transmit)
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
	/* NULL segment: the bytes are the caller's, so the writer copies
	 * them into the message. A segment here would treat the literal
	 * as already resident and the peer would read an empty reason. */
	ex.reason.seg = NULL;
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

	/* Record the answer so the question id stays live until Finish, as
	 * C++ does for an exception Return. A tail call (sendResultsTo
	 * yourself) must not put the exception on the wire: the peer is
	 * waiting for resultsSentElsewhere. */
	rc = send_answer(c, qid, &msg, transmit);
	capn_free(&msg);
	if (rc == 0 && !transmit)
		deliver_tail(c, qid);
	return rc;
}

static int send_return_exception(struct capn_rpc_conn *c, uint32_t qid,
                                 const char *reason)
{
	return return_exception(c, qid, reason, 1);
}

static int call_exception(struct capn_rpc_conn *c, const struct Call *call,
                          const char *reason)
{
	if (call != NULL &&
	    call->sendResultsTo_which == Call_sendResultsTo_yourself)
		return return_exception(c, call->questionId, reason, 0);
	return send_return_exception(c, call->questionId, reason);
}

/* 1 when this question still occupies an answer, a call that has not
 * returned (including one Finish already cancelled), a queued frame, or
 * an unfinished Join part. Finish does not free a running call; return
 * does. C++ 1.4.0 does not implement Join; the join ids are included so
 * a reused question cannot collide with one. */
static int question_id_in_use(struct capn_rpc_conn *c, uint32_t qid)
{
	int i, k;

	if (answer_find(c, qid) != NULL)
		return 1;
	for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
		struct capn_rpc_pending_call *pc = &c->pending_calls[i];
		if (pc->used && pc->question_id == qid)
			return 1;
	}
	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++) {
		uint32_t fq;

		if (!c->queued[i].used || c->queued[i].frame == NULL)
			continue;
		if (frame_question_id(c->queued[i].frame, c->queued[i].len, &fq) == 0 &&
		    fq == qid)
			return 1;
	}
	for (i = 0; i < CAPN_RPC_MAX_JOINS; i++) {
		if (!c->joins[i].used)
			continue;
		for (k = 0; k < c->joins[i].part_count && k < CAPN_RPC_MAX_JOIN_PARTS; k++) {
			if (c->joins[i].seen[k] && c->joins[i].qids[k] == qid)
				return 1;
		}
	}
	return 0;
}

static int abort_live_question(struct capn_rpc_conn *c, const uint32_t *answer_id)
{
	char buf[96];
	const char *reason =
		"expected !answer.active; questionId is already in use";

	if (c->aborted)
		return -1;
	/* rpc.c++ handleCall omits the extra argument. handleBootstrap
	 * passes answerId, which KJ prints as "; answerId = N". */
	if (answer_id != NULL) {
		snprintf(buf, sizeof buf, "%s; answerId = %u", reason, *answer_id);
		reason = buf;
	}
	capn_rpc_send_abort(c, reason);
	return -1;
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
	if (question_id_in_use(c, b.questionId))
		return abort_live_question(c, &b.questionId);
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

	rc = send_answer(c, b.questionId, &msg, 1);
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

/* Every senderHosted id in the table. The caller frees the list. A
 * table with none of those ids returns NULL and sets *n_out to 0. */
static uint32_t *collect_param_imports(CapDescriptor_list table, int *n_out)
{
	int n = 0, i, len;
	uint32_t *ids;

	*n_out = 0;
	if (table.p.type == CAPN_NULL)
		return NULL;
	len = capn_len(table);
	if (len <= 0)
		return NULL;
	ids = (uint32_t *) calloc((size_t) len, sizeof *ids);
	if (ids == NULL)
		return NULL;
	for (i = 0; i < len; i++) {
		struct CapDescriptor cd;

		get_CapDescriptor(&cd, table, i);
		if (cd.which != CapDescriptor_senderHosted)
			continue;
		ids[n++] = cd.senderHosted;
	}
	*n_out = n;
	return ids;
}

static int id_list_add(uint32_t **ids, int *n, uint32_t id)
{
	uint32_t *grown = (uint32_t *) realloc(*ids, ((size_t) *n + 1) * sizeof *grown);

	if (grown == NULL)
		return -1;
	grown[(*n)++] = id;
	*ids = grown;
	return 0;
}

/* Drop the ids the dispatch took. */
static int drop_taken(uint32_t *ids, int n, const uint32_t *taken, int ntaken)
{
	int i, j, out = 0;

	if (ids == NULL)
		return 0;
	for (i = 0; i < n; i++) {
		for (j = 0; j < ntaken; j++) {
			if (taken[j] == ids[i])
				break;
		}
		if (j == ntaken)
			ids[out++] = ids[i];
	}
	return out;
}

static void release_import_list(struct capn_rpc_conn *c, const uint32_t *ids, int n)
{
	int i;

	for (i = 0; ids != NULL && i < n; i++)
		capn_rpc_send_release(c, ids[i], 1);
}

static int import_kept(const struct capn_rpc_pending_cap *caps, int ncaps,
                       uint32_t id)
{
	int i;

	for (i = 0; caps != NULL && i < ncaps; i++) {
		if (caps[i].is_import && caps[i].id == id)
			return 1;
	}
	return 0;
}

/* Send the Return for a call whose results struct is filled, and release
 * the message it was built in. */
static void deliver_tail(struct capn_rpc_conn *c, uint32_t peer_qid);

static int finish_call(struct capn_rpc_conn *c, uint32_t qid, struct capn *msg,
                       capn_ptr rstruct, struct capn_rpc_pending_cap *caps,
                       int ncaps, int redirect,
                       const uint32_t *imports, int nimports)
{
	uint32_t *kept = NULL;
	int nkept = 0, k;
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
	attach_cap_table(cs, &results, caps, ncaps);
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

	/* rpc.c++ drops the params on the pipeline branch, which runs
	 * before the branch that sends the Return. Imports the results
	 * hand back stay on the answer until Finish. */
	for (k = 0; imports != NULL && k < nimports; k++) {
		if (!import_kept(caps, ncaps, imports[k]) ||
		    id_list_add(&kept, &nkept, imports[k]) != 0)
			capn_rpc_send_release(c, imports[k], 1);
	}
	rc = send_answer(c, qid, msg, redirect ? 0 : 1);
	{
		struct capn_rpc_answer *a = answer_find(c, qid);
		if (rc == 0 && a != NULL) {
			a->held_imports = kept;
			a->nheld = nkept;
		} else {
			release_import_list(c, kept, nkept);
			free(kept);
		}
	}
	/* A failed send that did not keep the answer never handed the
	 * capabilities to the peer. */
	if (rc != 0 && answer_find(c, qid) == NULL)
		release_unsent_caps(c, caps, ncaps);
	/* sendResultsTo.yourself: keep the results. The peer's Return names
	 * this question with takeFromOtherQuestion, and only then do we send
	 * resultsSentElsewhere. */
	if (rc == 0 && redirect)
		deliver_tail(c, qid);
	else if (rc == 0)
		flush_questions(c, qid);
	else
		reject_questions(c, qid, "promise released");
	capn_free(msg);
	free(msg);
	/* The Return already copied each step. The buffer was only the
	 * source of that copy. */
	free_cap_paths(caps, ncaps);
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

static void free_cap_paths(struct capn_rpc_pending_cap *caps, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		free(caps[i].path);
		caps[i].path = NULL;
		caps[i].npath = 0;
	}
}

static void pending_release(struct capn_rpc_conn *c,
                            struct capn_rpc_pending_call *pc)
{
	release_unsent_caps(c, pc->caps, pc->ncaps);
	free_cap_paths(pc->caps, pc->ncaps);
	capn_free(pc->msg);
	free(pc->msg);
	free(pc->param_imports);
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

capn_ptr capn_rpc_pending_results(struct capn_rpc_conn *c, uint32_t question_id,
                                  uint32_t generation)
{
	struct capn_rpc_pending_call *pc = pending_find_gen(c, question_id, generation);
	capn_ptr none;

	memset(&none, 0, sizeof none);
	if (pc == NULL)
		return none;
	return pc->results;
}

static struct capn_rpc_question *question_find(struct capn_rpc_conn *c,
                                               uint32_t qid);
static void fail_existing(struct capn_rpc_question *q);

/* A local caller is waiting on `qid`. Put the results on that question
 * instead of transmitting them to the peer. */
static int answer_local_question(struct capn_rpc_conn *c, uint32_t qid,
                                 struct capn *msg, capn_ptr rstruct,
                                 struct capn_rpc_pending_cap *caps, int ncaps)
{
	struct capn_rpc_question *q = question_find(c, qid);
	struct Message m;
	struct Return r;
	struct Payload pl;
	Message_ptr mp;
	Return_ptr rp;
	Payload_ptr plp;
	struct capn_segment *cs;
	uint8_t *buf;
	size_t len;

	if (q == NULL || msg == NULL) {
		if (msg != NULL) {
			capn_free(msg);
			free(msg);
		}
		release_unsent_caps(c, caps, ncaps);
		free_cap_paths(caps, ncaps);
		return -1;
	}
	cs = rstruct.seg;
	memset(&m, 0, sizeof m);
	memset(&r, 0, sizeof r);
	memset(&pl, 0, sizeof pl);
	pl.content = rstruct;
	attach_cap_table(cs, &pl, caps, ncaps);
	free_cap_paths(caps, ncaps);
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
	capn_setp(capn_root(msg), 0, mp.p);
	buf = frame_alloc(msg, &len);
	capn_free(msg);
	free(msg);
	if (buf == NULL) {
		fail_existing(q);
		return -1;
	}
	free(q->reply);
	q->reply = buf;
	q->reply_len = len;
	q->answered = 1;
	q->failed = 0;
	return 0;
}

int capn_rpc_return(struct capn_rpc_conn *c, uint32_t question_id,
                    uint32_t generation)
{
	struct capn_rpc_pending_call *pc = pending_find_gen(c, question_id, generation);
	struct capn *msg;
	capn_ptr rstruct;
	int redirect, ncaps, rc;

	if (pc == NULL)
		return -1;
	if (pc->cancelled) {
		uint32_t *imports = pc->param_imports;
		int nimports = pc->nparam_imports;

		pc->param_imports = NULL;
		pc->nparam_imports = 0;
		pending_release(c, pc);
		release_import_list(c, imports, nimports);
		free(imports);
		return -1;
	}
	if (pc->local_question) {
		msg = pc->msg;
		rstruct = pc->results;
		ncaps = pc->ncaps;
		rc = answer_local_question(c, question_id, msg, rstruct, pc->caps, ncaps);
		free(pc->param_imports);
		memset(pc, 0, sizeof *pc);
		return rc;
	}
	msg = pc->msg;
	rstruct = pc->results;
	redirect = pc->redirect;
	ncaps = pc->ncaps;
	/* The slot stays until the answer is recorded, so the question id
	 * is still in use for the duration of the send. */
	rc = finish_call(c, question_id, msg, rstruct, pc->caps, ncaps, redirect,
	                 pc->param_imports, pc->nparam_imports);
	free(pc->param_imports);
	memset(pc, 0, sizeof *pc);
	return rc;
}

int capn_rpc_return_exception(struct capn_rpc_conn *c, uint32_t question_id,
                              uint32_t generation, const char *reason)
{
	struct capn_rpc_pending_call *pc = pending_find_gen(c, question_id, generation);
	uint32_t *imports;
	int nimports;
	int cancelled;
	int rc;
	int local;

	if (pc == NULL)
		return -1;
	local = pc->local_question;
	cancelled = pc->cancelled;
	imports = pc->param_imports;
	nimports = pc->nparam_imports;
	pc->param_imports = NULL;
	pc->nparam_imports = 0;
	release_unsent_caps(c, pc->caps, pc->ncaps);
	free_cap_paths(pc->caps, pc->ncaps);
	pc->ncaps = 0;
	capn_free(pc->msg);
	free(pc->msg);
	pc->msg = NULL;
	if (cancelled) {
		memset(pc, 0, sizeof *pc);
		release_import_list(c, imports, nimports);
		free(imports);
		return -1;
	}
	if (local) {
		struct capn_rpc_question *q = question_find(c, question_id);
		if (q != NULL)
			fail_existing(q);
		memset(pc, 0, sizeof *pc);
		release_import_list(c, imports, nimports);
		free(imports);
		return 0;
	}
	reject_questions(c, question_id, reason ? reason : "call failed");
	if (pc->redirect)
		rc = return_exception(c, question_id, reason ? reason : "call failed", 0);
	else
		rc = send_return_exception(c, question_id, reason ? reason : "call failed");
	memset(pc, 0, sizeof *pc);
	/* The call context drops the params after the Return is sent. */
	release_import_list(c, imports, nimports);
	free(imports);
	return rc;
}

/* An exception Return for a dispatched call, then a Release for each
 * capability the params carried, as the call context drops them. */
static int param_exception_taken(struct capn_rpc_conn *c, const struct Call *call,
                                 const struct Payload *params,
                                 const uint32_t *taken, int ntaken,
                                 const char *reason)
{
	int n = 0;
	uint32_t *ids = collect_param_imports(params->capTable, &n);
	int rc = call_exception(c, call, reason);

	n = drop_taken(ids, n, taken, ntaken);
	release_import_list(c, ids, n);
	free(ids);
	return rc;
}

static int param_exception(struct capn_rpc_conn *c, const struct Call *call,
                           const struct Payload *params, const char *reason)
{
	return param_exception_taken(c, call, params, NULL, 0, reason);
}

static int handle_call(struct capn_rpc_conn *c, Call_ptr cp,
                       const uint8_t *frame, size_t frame_len)
{
	struct capn *msg;
	struct Call call;
	struct Payload params;
	struct capn_segment *cs;
	capn_ptr rstruct;
	int eid = -1, rc;

	read_Call(&call, cp);
	if (call.sendResultsTo_which != Call_sendResultsTo_caller &&
	    call.sendResultsTo_which != Call_sendResultsTo_yourself) {
		capn_rpc_send_abort(c, "Unsupported `Call.sendResultsTo`.");
		return -1;
	}
	if (question_id_in_use(c, call.questionId))
		return abort_live_question(c, NULL);
	memset(&params, 0, sizeof params);
	/* The cap table describes the message, not the dispatch: a call this
	 * vat cannot route still told us where a third party's capability
	 * lives. */
	if (call.params.p.type != CAPN_NULL) {
		read_Payload(&params, call.params);
		note_introductions(c, &params);
	}

	{
		struct capn_rpc_resolved tgt;
		struct MessageTarget mt;
		resolve_message_target(c, call.target, &tgt);
		/* A promised answer whose Return has not been sent yet is
		 * queued, not failed. The peer is allowed to pipeline before
		 * it has seen the results. rpc.c++ getMessageTarget on
		 * PROMISED_ANSWER delivers into the still-open pipeline. */
		if (tgt.kind == CAPN_RES_NONE) {
			uint32_t pq = 0;
			int is_promise = 0;
			if (call.target.p.type != CAPN_NULL) {
				read_MessageTarget(&mt, call.target);
				if (mt.which == MessageTarget_promisedAnswer &&
				    mt.promisedAnswer.p.type != CAPN_NULL) {
					struct PromisedAnswer pa;
					read_PromisedAnswer(&pa, mt.promisedAnswer);
					pq = pa.questionId;
					is_promise = 1;
				}
			}
			if (is_promise && question_is_open(c, pq)) {
				if (queue_on_question(c, pq, frame, frame_len) != 0)
					return param_exception(c, &call, &params,
					                       "promise queue full");
				return 0;
			}
			return param_exception(c, &call, &params, "no such export");
		}
		/* The cap in the answer is one the caller hosts, or a question
		 * this vat asked them. Reflect the call, swapping the cap table. */
		if (tgt.kind == CAPN_RES_REFLECT)
			return reflect_send(c, 0, tgt.import_id, NULL, 0, frame, frame_len);
		if (tgt.kind == CAPN_RES_PIPE) {
			int rrc = reflect_send(c, 1, tgt.question_id, tgt.path, tgt.npath,
			                       frame, frame_len);
			free(tgt.path);
			return rrc;
		}
		free(tgt.path);
		eid = tgt.export_id;
	}
	if (eid < 0 || eid >= c->exports_cap || !c->exports[eid].used)
		return param_exception(c, &call, &params, "no such export");
	if (c->exports[eid].broken)
		return param_exception(c, &call, &params, "broken promise");
	/* An unresolved promise cannot be answered yet. The frame is copied
	 * because the caller frees its buffer when handle returns. */
	if (c->exports[eid].promise) {
		if (queue_frame(c, eid, frame, frame_len) != 0)
			return param_exception(c, &call, &params,
			                       "promise queue full");
		return 0;
	}
	/* Tribble 4-way: once this promise resolved to import R, every
	 * later call keeps going to R even if R has resolved further.
	 * rpc.capnp Disembargo, "The Tribble 4-way Race Condition";
	 * rpc.c++ PromiseClient::resolve and TribbleRaceBlocker. */
	if (c->exports[eid].has_forward_import)
		return reflect_send(c, 0, c->exports[eid].forward_import, NULL, 0,
		                    frame, frame_len);
	if (c->exports[eid].has_forward) {
		int dest = c->exports[eid].forward_export;
		if (dest < 0 || dest >= c->exports_cap || !c->exports[dest].used ||
		    c->exports[dest].dispatch == NULL)
			return param_exception(c, &call, &params,
			                       "resolved promise has no target");
		eid = dest;
	}

	msg = (struct capn *) malloc(sizeof *msg);
	if (msg == NULL)
		return param_exception(c, &call, &params, "out of memory");
	capn_init_malloc(msg);
	cs = capn_root(msg).seg;

	{
		struct capn_rpc_pending_cap local_caps[CAPN_RPC_MAX_PENDING_CAPS];
		int nlocal = 0;
		CapDescriptor_list saved_in = c->in_caps;
		uint32_t saved_q = c->current_question;
		uint32_t saved_g = c->current_generation;
		struct capn_rpc_pending_cap *saved_dest = c->cap_dest;
		int *saved_dest_n = c->cap_dest_n;
		uint32_t **saved_taken = c->param_taken;
		int *saved_taken_n = c->param_taken_n;
		uint32_t *taken = NULL;
		int ntaken = 0;
		uint32_t gen;
		int redirect;

		/* Caps created while dispatch runs go to local_caps, not the
		 * connection list. The park slot is taken only if dispatch
		 * returns PENDING, so a synchronous handler still runs when
		 * every slot is already parked. */
		c->npending = 0;
		c->in_caps = params.capTable;
		c->current_question = call.questionId;
		gen = take_generation(c);
		c->current_generation = gen;
		c->cap_dest = local_caps;
		c->cap_dest_n = &nlocal;
		c->param_taken = &taken;
		c->param_taken_n = &ntaken;

		/* Sized by capn_rpc_set_result_size: one data word and one pointer is
		 * enough for the bundled servers, and a richer one asks for more. */
		rstruct = capn_new_struct(cs, c->results_datasz, c->results_ptrs);
		if (rstruct.type == CAPN_NULL) {
			c->in_caps = saved_in;
			c->current_question = saved_q;
			c->current_generation = saved_g;
			c->cap_dest = saved_dest;
			c->cap_dest_n = saved_dest_n;
			c->param_taken = saved_taken;
			c->param_taken_n = saved_taken_n;
			free_cap_paths(local_caps, nlocal);
			capn_free(msg);
			free(msg);
			free(taken);
			return param_exception(c, &call, &params, "results too large");
		}
		rc = c->exports[eid].dispatch(c->exports[eid].server, call.interfaceId,
		                              call.methodId, params.content, rstruct);
		c->in_caps = saved_in;
		c->current_question = saved_q;
		c->current_generation = saved_g;
		c->cap_dest = saved_dest;
		c->cap_dest_n = saved_dest_n;
		c->param_taken = saved_taken;
		c->param_taken_n = saved_taken_n;

		redirect = call.sendResultsTo_which == Call_sendResultsTo_yourself;
		if (rc == CAPN_RPC_PENDING) {
			int i;

			for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
				struct capn_rpc_pending_call *pc = &c->pending_calls[i];

				if (pc->used)
					continue;
				memset(pc, 0, sizeof *pc);
				pc->used = 1;
				pc->cancelled = 0;
				pc->question_id = call.questionId;
				pc->generation = gen;
				pc->msg = msg;
				pc->results = rstruct;
				pc->redirect = redirect;
				pc->ncaps = nlocal;
				if (nlocal > 0)
					memcpy(pc->caps, local_caps,
					       (size_t) nlocal * sizeof local_caps[0]);
				pc->param_imports = collect_param_imports(
					params.capTable, &pc->nparam_imports);
				pc->nparam_imports = drop_taken(pc->param_imports,
					pc->nparam_imports, taken, ntaken);
				free(taken);
				return 0;
			}
			release_unsent_caps(c, local_caps, nlocal);
			free_cap_paths(local_caps, nlocal);
			capn_free(msg);
			free(msg);
			rc = param_exception_taken(c, &call, &params, taken, ntaken,
			                           "too many pending calls");
			free(taken);
			return rc;
		}
		if (rc != 0) {
			release_unsent_caps(c, local_caps, nlocal);
			free_cap_paths(local_caps, nlocal);
			capn_free(msg);
			free(msg);
			rc = param_exception_taken(c, &call, &params, taken, ntaken,
			                           "call failed");
			free(taken);
			return rc;
		}
		{
			int nimports = 0;
			uint32_t *imports = collect_param_imports(params.capTable,
			                                          &nimports);

			nimports = drop_taken(imports, nimports, taken, ntaken);
			free(taken);
			rc = finish_call(c, call.questionId, msg, rstruct, local_caps,
			                 nlocal, redirect, imports, nimports);
			free(imports);
			return rc;
		}
	}
}

static void handle_release(struct capn_rpc_conn *c, Release_ptr rp)
{
	struct Release rel;
	read_Release(&rel, rp);
	/* C++ releaseExport throws, and taskFailed disconnects, when the id
	 * is not an export or the count would pass below zero. The export
	 * is left in place; the abort drops the connection. */
	if (rel.id >= (uint32_t) c->exports_cap || !c->exports[rel.id].used) {
		capn_rpc_send_abort(c, "Tried to release invalid export ID.");
		return;
	}
	if (rel.referenceCount > (uint32_t) c->exports[rel.id].refcount) {
		capn_rpc_send_abort(c, "Tried to drop export's refcount below zero.");
		return;
	}
	if (rel.referenceCount == (uint32_t) c->exports[rel.id].refcount) {
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
	if (question_id_in_use(c, j.questionId))
		return abort_live_question(c, NULL);
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
/* rpc.c++ handleDisembargo, senderLoopback. The KJ description also
 * appends the two pointer values in brackets; those addresses are not
 * available here, so the reason is the condition and the message. */
static const char kDisembargoForeign[] =
	"expected target->getBrand() == this; "
	"'Disembargo' of type 'senderLoopback' sent to an object that "
	"does not point back to the sender.";

/*  0  still a promise: park on *wait_id
 *  1  the resolution points back at the sender's import
 * -1  a local object, a cycle, or a target that never resolved that way
 */
static int disembargo_follow(struct capn_rpc_conn *c, int export_id,
                             int *wait_id, uint32_t *import_id)
{
	int seen[CAPN_RPC_MAX_TABLE_CAPACITY];
	int nseen = 0;
	int id = export_id;

	for (;;) {
		struct capn_rpc_export *ex;
		int i;

		if (id < 0 || id >= c->exports_cap || !c->exports[id].used)
			return -1;
		for (i = 0; i < nseen; i++) {
			if (seen[i] == id)
				return -1;
		}
		if (nseen >= CAPN_RPC_MAX_TABLE_CAPACITY)
			return -1;
		seen[nseen++] = id;
		ex = &c->exports[id];
		if (ex->promise) {
			*wait_id = id;
			return 0;
		}
		if (ex->has_forward_import) {
			*import_id = ex->forward_import;
			return 1;
		}
		if (ex->has_forward) {
			id = ex->forward_export;
			continue;
		}
		return -1;
	}
}

static int park_echo(struct capn_rpc_conn *c, int export_id,
                     const uint8_t *frame, size_t frame_len)
{
	int i;
	uint8_t *copy;

	if (frame == NULL || frame_len == 0)
		return 0;
	for (i = 0; i < CAPN_RPC_MAX_ECHOES; i++) {
		if (!c->echoes[i].used)
			break;
	}
	if (i == CAPN_RPC_MAX_ECHOES)
		return -1;
	copy = (uint8_t *) malloc(frame_len);
	if (copy == NULL)
		return -1;
	memcpy(copy, frame, frame_len);
	c->echoes[i].used = 1;
	c->echoes[i].export_id = (uint32_t) export_id;
	c->echoes[i].frame = copy;
	c->echoes[i].len = frame_len;
	c->echoes[i].seq = ++c->queue_clock;
	return 0;
}

static int abort_foreign_disembargo(struct capn_rpc_conn *c)
{
	if (c->aborted)
		return -1;
	capn_rpc_send_abort(c, kDisembargoForeign);
	return -1;
}

static int handle_disembargo(struct capn_rpc_conn *c, Disembargo_ptr dp,
                             const uint8_t *frame, size_t frame_len)
{
	struct capn msg;
	struct Disembargo in, out;
	struct Message m;
	Message_ptr mp;
	Disembargo_ptr op;
	struct capn_segment *cs;
	struct capn_rpc_resolved tgt;
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
	resolve_message_target(c, in.target, &tgt);
	/* A loopback aimed at something this vat does not know is a
	 * protocol error to ignore, not to reflect. */
	if (tgt.kind == CAPN_RES_NONE) {
		free(tgt.path);
		return 0;
	}
	/* Wait until the target settles, then follow the resolution.
	 * Echo only when that resolution points back at the sender.
	 * An ordinary local export is the C++ failure. */
	if (tgt.kind == CAPN_RES_EXPORT) {
		int wait_id = tgt.export_id;
		uint32_t import_id = 0;
		int followed = disembargo_follow(c, tgt.export_id, &wait_id,
		                                 &import_id);
		if (followed < 0) {
			free(tgt.path);
			return abort_foreign_disembargo(c);
		}
		if (followed == 0) {
			free(tgt.path);
			return park_echo(c, wait_id, frame, frame_len);
		}
		in.target.p.type = CAPN_NULL;
		tgt.kind = CAPN_RES_REFLECT;
		tgt.import_id = import_id;
	} else if (tgt.kind == CAPN_RES_REFLECT) {
		in.target.p.type = CAPN_NULL;
	} else {
		free(tgt.path);
		return abort_foreign_disembargo(c);
	}
	free(tgt.path);
	tgt.path = NULL;

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&out, 0, sizeof out);

	op = new_Disembargo(cs);
	/* The echo names the sender's import, which is where rpc.c++
	 * RpcClient::writeTarget points after the promise settles. */
	if (tgt.kind == CAPN_RES_REFLECT) {
		struct MessageTarget t;
		MessageTarget_ptr tp = new_MessageTarget(cs);
		memset(&t, 0, sizeof t);
		t.which = MessageTarget_importedCap;
		t.importedCap = tgt.import_id;
		write_MessageTarget(&t, tp);
		out.target = tp;
	} else if (in.target.p.type != CAPN_NULL) {
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
	int i;

	free(q->reply);
	for (i = 0; i < q->npipes; i++)
		free(q->pipes[i].path);
	free(q->pipes);
	free(q->param_exports);
	free(q->taken);
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
			/* A reused id can no longer name the finished call. */
			stash_drop(c, c->questions[i].question_id, 0);
			*qid_out = c->questions[i].question_id;
			return &c->questions[i];
		}
	}
	i = c->questions_cap;
	{
		void *grown = table_grow(c->questions, &c->questions_cap,
		                         sizeof *c->questions,
		                         CAPN_RPC_MAX_TABLE_CAPACITY);
		if (grown == NULL)
			return NULL;
		c->questions = grown;
	}
	c->questions[i].used = 1;
	c->questions[i].question_id = c->next_question_id++;
	stash_drop(c, c->questions[i].question_id, 0);
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

/* rpc.c++ Question::paramExports: the exports a sent Call named. */
static void note_param_exports(struct capn_rpc_question *q,
                               const struct capn_rpc_pending_cap *caps, int n)
{
	int i, k = 0;

	free(q->param_exports);
	q->param_exports = NULL;
	q->nparam_exports = 0;
	for (i = 0; i < n; i++)
		if (!caps[i].is_import && caps[i].kind != CAPN_RPC_CAP_ANSWER)
			k++;
	if (k == 0)
		return;
	q->param_exports = (uint32_t *) malloc((size_t) k * sizeof *q->param_exports);
	if (q->param_exports == NULL)
		return;
	for (i = 0; i < n; i++)
		if (!caps[i].is_import && caps[i].kind != CAPN_RPC_CAP_ANSWER)
			q->param_exports[q->nparam_exports++] = caps[i].id;
}

static void note_taken(struct capn_rpc_question *q, uint32_t idx)
{
	uint32_t *grown;
	int i;

	for (i = 0; i < q->ntaken; i++)
		if (q->taken[i] == idx)
			return;
	grown = (uint32_t *) realloc(q->taken, ((size_t) q->ntaken + 1) * sizeof *grown);
	if (grown == NULL)
		return;
	grown[q->ntaken++] = idx;
	q->taken = grown;
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
		q = question_find(c, qid);
		if (q)
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
	q->bootstrap = 1;

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
		q = question_find(c, qid);
		if (q)
			question_clear(q);
		qid = (uint32_t)-1;
	}
	capn_free(&msg);
	return qid;
}

/* Write the Payload for params or results: the content struct is already
 * filled, and any capability made while filling it is in `caps`. */
static void attach_cap_table(struct capn_segment *cs, struct Payload *pl,
                             struct capn_rpc_pending_cap *caps, int n)
{
	int i;

	if (n <= 0 || caps == NULL)
		return;
	pl->capTable = new_CapDescriptor_list(cs, n);
	for (i = 0; i < n; i++) {
		struct CapDescriptor cd;
		memset(&cd, 0, sizeof cd);
		if (caps[i].kind == CAPN_RPC_CAP_PROMISE) {
			cd.which = CapDescriptor_senderPromise;
			cd.senderPromise = caps[i].id;
		} else if (caps[i].kind == CAPN_RPC_CAP_ANSWER) {
			struct PromisedAnswer answer;
			PromisedAnswer_ptr pap;
			int npath = caps[i].npath;
			int k;

			memset(&answer, 0, sizeof answer);
			answer.questionId = caps[i].question_id;
			if (npath > 0) {
				answer.transform = new_PromisedAnswer_Op_list(cs, npath);
				for (k = 0; k < npath; k++) {
					struct PromisedAnswer_Op op;
					memset(&op, 0, sizeof op);
					op.which = PromisedAnswer_Op_getPointerField;
					op.getPointerField = caps[i].path[k];
					set_PromisedAnswer_Op(&op, answer.transform, k);
				}
			}
			pap = new_PromisedAnswer(cs);
			write_PromisedAnswer(&answer, pap);
			cd.which = CapDescriptor_receiverAnswer;
			cd.receiverAnswer = pap;
		} else if (caps[i].is_import) {
			cd.which = CapDescriptor_receiverHosted;
			cd.receiverHosted = caps[i].id;
		} else {
			cd.which = CapDescriptor_senderHosted;
			cd.senderHosted = caps[i].id;
		}
		set_CapDescriptor(&cd, pl->capTable, i);
	}
}

/* The table a new capability is recorded in. cap_dest is set for the
 * duration of a fill or a dispatch. Otherwise a segment that belongs to
 * a parked call's message records the cap on that call, so one created
 * after dispatch returned PENDING is still in the later Return. Anything
 * else uses the connection list, which an outgoing or held call consumes. */
static struct capn_rpc_pending_cap *cap_build(struct capn_rpc_conn *c,
                                              struct capn_segment *seg, int **np)
{
	if (c->cap_dest != NULL && c->cap_dest_n != NULL) {
		*np = c->cap_dest_n;
		return c->cap_dest;
	}
	if (seg != NULL && seg->capn != NULL) {
		int i;

		for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
			struct capn_rpc_pending_call *pc = &c->pending_calls[i];

			if (pc->used && pc->msg == seg->capn) {
				*np = &pc->ncaps;
				return pc->caps;
			}
		}
	}
	*np = &c->npending;
	return c->pending_caps;
}

capn_ptr capn_rpc_new_export_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 void *server, capn_rpc_dispatch_fn dispatch)
{
	capn_ptr p;
	int eid;
	int *np;
	struct capn_rpc_pending_cap *caps;

	memset(&p, 0, sizeof p);
	caps = cap_build(c, seg, &np);
	if (*np >= CAPN_RPC_MAX_PENDING_CAPS)
		return p;
	eid = capn_rpc_export(c, server, dispatch);
	if (eid < 0)
		return p;
	p = capn_new_interface(seg, 0, 0);
	if (p.type == CAPN_NULL) {
		release_export_ref(c, (uint32_t) eid);
		return p;
	}
	p.cap_idx = (uint32_t) *np;
	memset(&caps[*np], 0, sizeof caps[*np]);
	caps[*np].is_import = 0;
	caps[*np].kind = CAPN_RPC_CAP_HOSTED;
	caps[*np].id = (uint32_t) eid;
	(*np)++;
	return p;
}

capn_ptr capn_rpc_new_import_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 uint32_t import_id)
{
	capn_ptr p;
	int *np;
	struct capn_rpc_pending_cap *caps;

	memset(&p, 0, sizeof p);
	caps = cap_build(c, seg, &np);
	if (*np >= CAPN_RPC_MAX_PENDING_CAPS)
		return p;
	p = capn_new_interface(seg, 0, 0);
	if (p.type == CAPN_NULL)
		return p;
	p.cap_idx = (uint32_t) *np;
	memset(&caps[*np], 0, sizeof caps[*np]);
	caps[*np].is_import = 1;
	caps[*np].kind = CAPN_RPC_CAP_HOSTED;
	caps[*np].id = import_id;
	(*np)++;
	return p;
}

capn_ptr capn_rpc_new_promise_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                  void *server, capn_rpc_dispatch_fn dispatch)
{
	capn_ptr p;
	int eid;
	int *np;
	struct capn_rpc_pending_cap *caps;

	memset(&p, 0, sizeof p);
	caps = cap_build(c, seg, &np);
	if (*np >= CAPN_RPC_MAX_PENDING_CAPS)
		return p;
	eid = capn_rpc_export_promise(c, server, dispatch);
	if (eid < 0)
		return p;
	p = capn_new_interface(seg, 0, 0);
	if (p.type == CAPN_NULL) {
		release_export_ref(c, (uint32_t) eid);
		return p;
	}
	p.cap_idx = (uint32_t) *np;
	memset(&caps[*np], 0, sizeof caps[*np]);
	caps[*np].kind = CAPN_RPC_CAP_PROMISE;
	caps[*np].id = (uint32_t) eid;
	(*np)++;
	return p;
}

capn_ptr capn_rpc_new_answer_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 uint32_t question_id, const uint16_t *path,
                                 int npath)
{
	capn_ptr p;
	int *np;
	struct capn_rpc_pending_cap *caps;

	memset(&p, 0, sizeof p);
	caps = cap_build(c, seg, &np);
	if (*np >= CAPN_RPC_MAX_PENDING_CAPS)
		return p;
	if (npath < 0 || (npath > 0 && path == NULL))
		return p;
	p = capn_new_interface(seg, 0, 0);
	if (p.type == CAPN_NULL)
		return p;
	p.cap_idx = (uint32_t) *np;
	memset(&caps[*np], 0, sizeof caps[*np]);
	caps[*np].kind = CAPN_RPC_CAP_ANSWER;
	caps[*np].question_id = question_id;
	if (npath > 0) {
		caps[*np].path = (uint16_t *) malloc((size_t) npath * sizeof path[0]);
		if (caps[*np].path == NULL) {
			memset(&p, 0, sizeof p);
			return p;
		}
		memcpy(caps[*np].path, path, (size_t) npath * sizeof path[0]);
		caps[*np].npath = npath;
	}
	(*np)++;
	return p;
}

/* What the capTable entry a capability pointer indexes names. The wire
 * id is a uint32; returning it as int makes values at or above 2^31
 * negative, which collides with the -1 error. */
static int64_t cap_lookup(CapDescriptor_list table, capn_ptr cap, int *is_export)
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
		return (int64_t) (cd.which == CapDescriptor_senderHosted ?
		                  cd.senderHosted : cd.senderPromise);
	}
	if (cd.which == CapDescriptor_receiverHosted) {
		if (is_export)
			*is_export = 1;
		return (int64_t) cd.receiverHosted;
	}
	if (cd.which == CapDescriptor_receiverAnswer &&
	    cd.receiverAnswer.p.type != CAPN_NULL) {
		struct PromisedAnswer pa;
		read_PromisedAnswer(&pa, cd.receiverAnswer);
		/* 2: the id is a question this vat asked, not an export. */
		if (is_export)
			*is_export = 2;
		return (int64_t) pa.questionId;
	}
	return -1;
}

int64_t capn_rpc_param_cap(struct capn_rpc_conn *c, capn_ptr cap, int *is_export)
{
	int exp = 0;
	int64_t id = cap_lookup(c->in_caps, cap, &exp);

	if (is_export)
		*is_export = exp;
	if (id >= 0 && !exp && c->param_taken && c->param_taken_n) {
		int i, n = *c->param_taken_n;
		for (i = 0; i < n; i++) {
			if ((*c->param_taken)[i] == (uint32_t) id)
				return id;
		}
		id_list_add(c->param_taken, c->param_taken_n, (uint32_t) id);
	}
	return id;
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
	struct capn_rpc_pending_cap local_caps[CAPN_RPC_MAX_PENDING_CAPS];
	struct capn_rpc_pending_cap *saved_dest;
	int *saved_n;
	int nlocal = 0;
	int own_local = 1;
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
	/* A held call already registered the caps its params point at.
	 * A new call lets fill() build that list from scratch. Either way
	 * the caps go to local_caps, so a parked answer's table is left
	 * alone and a nested dispatch does not see this call's caps. */
	if (reuse != NULL && c->npending > 0) {
		nlocal = c->npending;
		if (nlocal > CAPN_RPC_MAX_PENDING_CAPS)
			nlocal = CAPN_RPC_MAX_PENDING_CAPS;
		memcpy(local_caps, c->pending_caps,
		       (size_t) nlocal * sizeof local_caps[0]);
		/* The caller still owns these paths. */
		own_local = 0;
	}
	c->npending = 0;
	saved_dest = c->cap_dest;
	saved_n = c->cap_dest_n;
	c->cap_dest = local_caps;
	c->cap_dest_n = &nlocal;

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
	c->cap_dest = saved_dest;
	c->cap_dest_n = saved_n;
	attach_cap_table(cs, &params, local_caps, nlocal);
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
		/* The send callback may have grown the question table. */
		q = question_find(c, qid);
		release_unsent_caps(c, local_caps, nlocal);
		if (own_local)
			free_cap_paths(local_caps, nlocal);
		if (reuse != NULL) {
			if (q) {
				q->answered = 1;
				q->failed = 1;
			}
		} else {
			if (q)
				question_clear(q);
			release_unsent_caps(c, c->pending_caps, c->npending);
			qid = (uint32_t)-1;
		}
	} else {
		q = question_find(c, qid);
		if (q)
			note_param_exports(q, local_caps, nlocal);
	}
	c->npending = 0;
	capn_free(&msg);
	if (own_local)
		free_cap_paths(local_caps, nlocal);
	return qid;
}

enum {
	CAPN_RPC_HELD_LOCAL = 1,
	CAPN_RPC_HELD_REMOTE = 2,
	CAPN_RPC_HELD_PIPE = 3
};

/* Copy a payload pointer of any kind into `cs`. A struct-only copy
 * drops a list or a capability used as Payload.content. */
static capn_ptr copy_ptr(struct capn_segment *cs, capn_ptr src)
{
	capn_ptr box, out;

	memset(&out, 0, sizeof out);
	capn_resolve(&src);
	if (src.type == CAPN_NULL || cs == NULL)
		return src.type == CAPN_NULL ? src : out;
	box = capn_new_struct(cs, 0, 1);
	if (box.type == CAPN_NULL)
		return out;
	if (capn_setp(box, 0, src) != 0)
		return out;
	return capn_getp(box, 0, 1);
}

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
		void *grown = table_grow(c->imports, &c->imports_cap,
		                         sizeof *c->imports, CAPN_RPC_MAX_IMPORTS);
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
	c->queued[i].on_question = 0;
	c->queued[i].export_id = export_id;
	c->queued[i].frame = copy;
	c->queued[i].len = len;
	c->queued[i].seq = ++c->queue_clock;
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

/* senderHosted ids in the params of a queued Call frame. */
static uint32_t *frame_param_imports(const uint8_t *data, size_t len, int *n)
{
	struct capn msg;
	struct Message m;
	struct Call call;
	struct Payload params;
	Message_ptr mp;
	uint32_t *ids = NULL;

	*n = 0;
	if (capn_init_mem(&msg, data, len, 0) != 0)
		return NULL;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	read_Message(&m, mp);
	if (m.which == Message_call) {
		read_Call(&call, m.call);
		if (call.params.p.type != CAPN_NULL) {
			read_Payload(&params, call.params);
			ids = collect_param_imports(params.capTable, n);
		}
	}
	capn_free(&msg);
	return ids;
}

/* Scalar view of one CapDescriptor, so the message it was read from
 * can be freed before the caller writes a new one. */
struct capn_rpc_capid {
	int which;
	uint32_t id;
	uint32_t question_id;
	int npath;
	uint16_t *path;
};

/* 0 ok, -1 out of memory, -2 an op this vat does not implement.
 * NOOP is kept out of the walk: it does not move the pointer.
 * Every getPointerField is kept. The old 8-slot cap dropped the tail
 * and a later call followed the wrong field. */
static int ops_to_path(PromisedAnswer_Op_list transform, uint16_t **path, int *npath)
{
	int n = capn_len(transform);
	int i, k = 0;
	uint16_t *buf = NULL;

	*path = NULL;
	*npath = 0;
	if (n < 0)
		return -1;
	if (n == 0)
		return 0;
	buf = (uint16_t *) malloc((size_t) n * sizeof *buf);
	if (buf == NULL)
		return -1;
	for (i = 0; i < n; i++) {
		struct PromisedAnswer_Op op;
		get_PromisedAnswer_Op(&op, transform, i);
		if (op.which == PromisedAnswer_Op_noop)
			continue;
		if (op.which != PromisedAnswer_Op_getPointerField) {
			free(buf);
			return -2;
		}
		buf[k++] = op.getPointerField;
	}
	*path = buf;
	*npath = k;
	return 0;
}

static const char kBadPipelineOp[] = "Unsupported pipeline op.";

/* The capability a stored Return names after walking `path`. */
static int cap_in_answer(struct capn_rpc_conn *c, uint32_t question_id,
                         const uint16_t *path, int npath,
                         struct capn_rpc_capid *out)
{
	struct capn_rpc_answer *a;
	struct capn stored;
	struct Message m;
	struct Return r;
	struct Payload pl;
	struct CapDescriptor cd;
	Message_ptr mp;
	capn_ptr cursor;
	int i, ok = 0;

	memset(out, 0, sizeof *out);
	out->which = -1;
	a = answer_find(c, question_id);
	if (a == NULL)
		return 0;
	if (capn_init_mem(&stored, a->frame, a->len, 0) != 0)
		return 0;
	mp.p = capn_getp(capn_root(&stored), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message__return)
		goto done;
	read_Return(&r, m._return);
	if (r.which != Return_results)
		goto done;
	read_Payload(&pl, r.results);
	cursor = pl.content;
	for (i = 0; i < npath; i++) {
		capn_resolve(&cursor);
		if (cursor.type != CAPN_STRUCT)
			goto done;
		cursor = capn_getp(cursor, path[i], 0);
	}
	capn_resolve(&cursor);
	if (cursor.type != CAPN_INTERFACE)
		goto done;
	if (capn_len(pl.capTable) < 0 ||
	    cursor.cap_idx > (uint32_t) INT_MAX ||
	    (int) cursor.cap_idx >= capn_len(pl.capTable))
		goto done;
	get_CapDescriptor(&cd, pl.capTable, (int) cursor.cap_idx);
	out->which = (int) cd.which;
	if (cd.which == CapDescriptor_senderHosted)
		out->id = cd.senderHosted;
	else if (cd.which == CapDescriptor_senderPromise)
		out->id = cd.senderPromise;
	else if (cd.which == CapDescriptor_receiverHosted)
		out->id = cd.receiverHosted;
	else if (cd.which == CapDescriptor_receiverAnswer &&
	         cd.receiverAnswer.p.type != CAPN_NULL) {
		struct PromisedAnswer pa;
		read_PromisedAnswer(&pa, cd.receiverAnswer);
		out->question_id = pa.questionId;
		{
			int pr = ops_to_path(pa.transform, &out->path, &out->npath);
			if (pr == -2)
				capn_rpc_send_abort(c, kBadPipelineOp);
			if (pr != 0)
				goto done;
		}
	} else if (cd.which == CapDescriptor_thirdPartyHosted &&
	           cd.thirdPartyHosted.p.type != CAPN_NULL) {
		struct ThirdPartyCapDescriptor tp;
		read_ThirdPartyCapDescriptor(&tp, cd.thirdPartyHosted);
		out->id = tp.vineId;
	} else {
		goto done;
	}
	ok = 1;
done:
	capn_free(&stored);
	return ok;
}

static int write_capid(struct capn_segment *cs, const struct capn_rpc_capid *cap,
                       struct CapDescriptor *out)
{
	memset(out, 0, sizeof *out);
	out->attachedFd = 0xff;
	switch (cap->which) {
	case CapDescriptor_senderHosted:
		out->which = CapDescriptor_senderHosted;
		out->senderHosted = cap->id;
		return 1;
	case CapDescriptor_senderPromise:
		out->which = CapDescriptor_senderPromise;
		out->senderPromise = cap->id;
		return 1;
	case CapDescriptor_receiverHosted:
	case CapDescriptor_thirdPartyHosted:
		/* A third-party cap we ourselves returned is reached through
		 * its vine, which is an export of ours. */
		out->which = cap->which == CapDescriptor_thirdPartyHosted ?
		             CapDescriptor_senderHosted : CapDescriptor_receiverHosted;
		if (cap->which == CapDescriptor_thirdPartyHosted)
			out->senderHosted = cap->id;
		else
			out->receiverHosted = cap->id;
		return 1;
	case CapDescriptor_receiverAnswer: {
		struct PromisedAnswer pa;
		PromisedAnswer_ptr pap;
		int i;

		memset(&pa, 0, sizeof pa);
		pa.questionId = cap->question_id;
		if (cap->npath > 0) {
			pa.transform = new_PromisedAnswer_Op_list(cs, cap->npath);
			for (i = 0; i < cap->npath; i++) {
				struct PromisedAnswer_Op op;
				memset(&op, 0, sizeof op);
				op.which = PromisedAnswer_Op_getPointerField;
				op.getPointerField = cap->path[i];
				set_PromisedAnswer_Op(&op, pa.transform, i);
			}
		}
		pap = new_PromisedAnswer(cs);
		write_PromisedAnswer(&pa, pap);
		out->which = CapDescriptor_receiverAnswer;
		out->receiverAnswer = pap;
		return 1;
	}
	default:
		return 0;
	}
}

/* Copy `src` into `cs` with sender and receiver swapped. A call or
 * return reflected to the party that sent it has to rename every
 * hosted field: their export is now an import, and ours is now an
 * export. rpc.c++ writeDescriptor / receiveCap do the same rename by
 * rebuilding the table from capability objects. Level 1 turns
 * thirdPartyHosted into the vine (rpc.capnp, CapDescriptor). */
static CapDescriptor_list rewrite_cap_table(struct capn_rpc_conn *c,
                                            struct capn_segment *cs,
                                            CapDescriptor_list src)
{
	CapDescriptor_list dst;
	int n = capn_len(src);
	int i;

	memset(&dst, 0, sizeof dst);
	if (n <= 0)
		return dst;
	dst = new_CapDescriptor_list(cs, n);
	for (i = 0; i < n; i++) {
		struct CapDescriptor in, out;
		get_CapDescriptor(&in, src, i);
		memset(&out, 0, sizeof out);
		out.attachedFd = in.attachedFd;
		switch (in.which) {
		case CapDescriptor_none:
			out.which = CapDescriptor_none;
			break;
		case CapDescriptor_senderHosted:
			out.which = CapDescriptor_receiverHosted;
			out.receiverHosted = in.senderHosted;
			break;
		case CapDescriptor_senderPromise:
			/* The peer hosts this export and already tracks whether
			 * it is still a promise. */
			out.which = CapDescriptor_receiverHosted;
			out.receiverHosted = in.senderPromise;
			break;
		case CapDescriptor_receiverHosted:
			if (in.receiverHosted < (uint32_t) c->exports_cap &&
			    c->exports[in.receiverHosted].used &&
			    c->exports[in.receiverHosted].promise) {
				out.which = CapDescriptor_senderPromise;
				out.senderPromise = in.receiverHosted;
			} else {
				out.which = CapDescriptor_senderHosted;
				out.senderHosted = in.receiverHosted;
			}
			break;
		case CapDescriptor_receiverAnswer: {
			struct PromisedAnswer pa;
			struct capn_rpc_capid cap;
			uint16_t *path = NULL;
			int npath = 0;
			int pr;

			memset(&cap, 0, sizeof cap);
			if (in.receiverAnswer.p.type == CAPN_NULL) {
				out.which = CapDescriptor_none;
				break;
			}
			read_PromisedAnswer(&pa, in.receiverAnswer);
			pr = ops_to_path(pa.transform, &path, &npath);
			if (pr == -2)
				capn_rpc_send_abort(c, kBadPipelineOp);
			if (pr != 0 ||
			    !cap_in_answer(c, pa.questionId, path, npath, &cap) ||
			    !write_capid(cs, &cap, &out))
				out.which = CapDescriptor_none;
			free(path);
			free(cap.path);
			break;
		}
		case CapDescriptor_thirdPartyHosted: {
			struct ThirdPartyCapDescriptor tp;
			memset(&tp, 0, sizeof tp);
			if (in.thirdPartyHosted.p.type != CAPN_NULL)
				read_ThirdPartyCapDescriptor(&tp, in.thirdPartyHosted);
			out.which = CapDescriptor_receiverHosted;
			out.receiverHosted = tp.vineId;
			break;
		}
		default:
			out.which = CapDescriptor_none;
			break;
		}
		set_CapDescriptor(&out, dst, i);
	}
	return dst;
}

static void resolve_message_target(struct capn_rpc_conn *c, MessageTarget_ptr tp,
                                   struct capn_rpc_resolved *out)
{
	struct MessageTarget t;

	memset(out, 0, sizeof *out);
	out->kind = CAPN_RES_NONE;
	out->export_id = -1;
	if (tp.p.type == CAPN_NULL)
		return;
	read_MessageTarget(&t, tp);
	if (t.which == MessageTarget_importedCap) {
		if (t.importedCap < (uint32_t) c->exports_cap &&
		    c->exports[t.importedCap].used) {
			out->kind = CAPN_RES_EXPORT;
			out->export_id = (int) t.importedCap;
		}
		return;
	}
	if (t.which != MessageTarget_promisedAnswer ||
	    t.promisedAnswer.p.type == CAPN_NULL)
		return;
	{
		struct PromisedAnswer pa;
		struct capn_rpc_capid cap;
		uint16_t *path = NULL;
		int npath = 0;
		int pr;

		memset(&cap, 0, sizeof cap);
		read_PromisedAnswer(&pa, t.promisedAnswer);
		pr = ops_to_path(pa.transform, &path, &npath);
		if (pr == -2)
			capn_rpc_send_abort(c, kBadPipelineOp);
		if (pr != 0 || !cap_in_answer(c, pa.questionId, path, npath, &cap)) {
			free(path);
			free(cap.path);
			return;
		}
		free(path);
		if (cap.which == CapDescriptor_senderHosted ||
		    cap.which == CapDescriptor_senderPromise ||
		    cap.which == CapDescriptor_thirdPartyHosted) {
			if (cap.id < (uint32_t) c->exports_cap &&
			    c->exports[cap.id].used) {
				out->kind = CAPN_RES_EXPORT;
				out->export_id = (int) cap.id;
			}
			free(cap.path);
			return;
		}
		if (cap.which == CapDescriptor_receiverHosted) {
			out->kind = CAPN_RES_REFLECT;
			out->import_id = cap.id;
			free(cap.path);
			return;
		}
		if (cap.which == CapDescriptor_receiverAnswer) {
			out->kind = CAPN_RES_PIPE;
			out->question_id = cap.question_id;
			out->path = cap.path;
			out->npath = cap.npath;
			cap.path = NULL;
		}
		free(cap.path);
	}
}

static int question_is_open(struct capn_rpc_conn *c, uint32_t qid)
{
	int i;

	if (answer_find(c, qid) != NULL)
		return 0;
	if (c->current_question == qid && c->current_generation != 0)
		return 1;
	for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
		if (c->pending_calls[i].used && !c->pending_calls[i].cancelled &&
		    c->pending_calls[i].question_id == qid)
			return 1;
	}
	/* A pipeline already waiting on this question, a call still queued
	 * on a promise, or a call we forwarded and have not copied back
	 * yet, is still open. Answering "no such export" drops work the
	 * peer is allowed to send early. */
	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++) {
		uint32_t fid = 0;
		if (!c->queued[i].used)
			continue;
		if (c->queued[i].on_question && c->queued[i].question_id == qid)
			return 1;
		if (frame_question_id(c->queued[i].frame, c->queued[i].len, &fid) == 0 &&
		    fid == qid)
			return 1;
	}
	for (i = 0; i < CAPN_RPC_MAX_RELAYS; i++) {
		if (c->relays[i].used && c->relays[i].remote_qid == qid)
			return 1;
	}
	return 0;
}

static int queue_on_question(struct capn_rpc_conn *c, uint32_t question_id,
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
	memset(&c->queued[i], 0, sizeof c->queued[i]);
	c->queued[i].used = 1;
	c->queued[i].on_question = 1;
	c->queued[i].question_id = question_id;
	c->queued[i].frame = copy;
	c->queued[i].len = len;
	c->queued[i].seq = ++c->queue_clock;
	return 0;
}

/* Oldest matching slot. A resolve frees a slot, and the next arrival
 * may take that low index, so the index is not the arrival order. */
struct order_ctx {
	struct capn_rpc_conn *c;
	int on_question;
	int export_id;
	uint32_t question_id;
	uint32_t import_id;
};

static int oldest_slot(int n,
                       int (*match)(const struct order_ctx *, int),
                       uint64_t (*seq_at)(const struct order_ctx *, int),
                       const struct order_ctx *ctx)
{
	int i, best = -1;
	uint64_t best_seq = 0;

	for (i = 0; i < n; i++) {
		uint64_t seq;
		if (!match(ctx, i))
			continue;
		seq = seq_at(ctx, i);
		if (best < 0 || seq < best_seq) {
			best = i;
			best_seq = seq;
		}
	}
	return best;
}

static int queued_match(const struct order_ctx *o, int i)
{
	struct capn_rpc_queued *q = &o->c->queued[i];
	if (!q->used || q->on_question != o->on_question)
		return 0;
	if (o->on_question)
		return q->question_id == o->question_id;
	return q->export_id == o->export_id;
}

static uint64_t queued_seq(const struct order_ctx *o, int i)
{
	return o->c->queued[i].seq;
}

static int oldest_queued(struct capn_rpc_conn *c, int on_question,
                         int export_id, uint32_t question_id)
{
	struct order_ctx o;
	o.c = c;
	o.on_question = on_question;
	o.export_id = export_id;
	o.question_id = question_id;
	o.import_id = 0;
	return oldest_slot(CAPN_RPC_MAX_QUEUED, queued_match, queued_seq, &o);
}

static int held_match(const struct order_ctx *o, int i)
{
	return o->c->held[i].used && o->c->held[i].import_id == o->import_id;
}

static uint64_t held_seq(const struct order_ctx *o, int i)
{
	return o->c->held[i].seq;
}

static int oldest_held(struct capn_rpc_conn *c, uint32_t import_id)
{
	struct order_ctx o;
	o.c = c;
	o.on_question = 0;
	o.export_id = 0;
	o.question_id = 0;
	o.import_id = import_id;
	return oldest_slot(CAPN_RPC_MAX_HELD, held_match, held_seq, &o);
}

static int echo_match(const struct order_ctx *o, int i)
{
	return o->c->echoes[i].used &&
	       o->c->echoes[i].export_id == (uint32_t) o->export_id;
}

static uint64_t echo_seq(const struct order_ctx *o, int i)
{
	return o->c->echoes[i].seq;
}

static int oldest_echo(struct capn_rpc_conn *c, int export_id)
{
	struct order_ctx o;
	o.c = c;
	o.on_question = 0;
	o.export_id = export_id;
	o.question_id = 0;
	o.import_id = 0;
	return oldest_slot(CAPN_RPC_MAX_ECHOES, echo_match, echo_seq, &o);
}

static int send_return_canceled(struct capn_rpc_conn *c, uint32_t qid)
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
	rp = new_Return(cs);
	r.answerId = qid;
	r.releaseParamCaps = 0;
	r.which = Return_canceled;
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

/* Finish of a call that is still queued. Answer canceled now, and do not
 * run the call when its target settles. */
static int cancel_queued(struct capn_rpc_conn *c, uint32_t qid)
{
	int i;

	for (i = 0; i < CAPN_RPC_MAX_QUEUED; i++) {
		uint32_t fq;

		if (!c->queued[i].used || c->queued[i].frame == NULL)
			continue;
		if (frame_question_id(c->queued[i].frame, c->queued[i].len, &fq) != 0 ||
		    fq != qid)
			continue;
		{
			int n = 0;
			uint32_t *ids = frame_param_imports(c->queued[i].frame,
			                                    c->queued[i].len, &n);

			free(c->queued[i].frame);
			memset(&c->queued[i], 0, sizeof c->queued[i]);
			send_return_canceled(c, qid);
			release_import_list(c, ids, n);
			free(ids);
		}
		return 1;
	}
	return 0;
}

static void flush_questions(struct capn_rpc_conn *c, uint32_t question_id)
{
	int i;

	while ((i = oldest_queued(c, 1, 0, question_id)) >= 0) {
		uint8_t *frame = c->queued[i].frame;
		size_t len = c->queued[i].len;
		memset(&c->queued[i], 0, sizeof c->queued[i]);
		capn_rpc_handle(c, frame, len);
		free(frame);
	}
}

static void reject_questions(struct capn_rpc_conn *c, uint32_t question_id,
                             const char *reason)
{
	int i;

	while ((i = oldest_queued(c, 1, 0, question_id)) >= 0) {
		uint32_t qid;
		if (frame_question_id(c->queued[i].frame, c->queued[i].len, &qid) == 0)
			send_return_exception(c, qid, reason ? reason : "promise released");
		free(c->queued[i].frame);
		memset(&c->queued[i], 0, sizeof c->queued[i]);
	}
}

static void flush_echoes(struct capn_rpc_conn *c, int export_id)
{
	int i;

	while ((i = oldest_echo(c, export_id)) >= 0) {
		uint8_t *frame = c->echoes[i].frame;
		size_t len = c->echoes[i].len;
		memset(&c->echoes[i], 0, sizeof c->echoes[i]);
		capn_rpc_handle(c, frame, len);
		free(frame);
	}
}

/* Send `frame` back to the peer, either at `target_id` as an import or,
 * when `pipelined` is set, as a promised answer. The cap table is kept
 * and its hosted fields are swapped. The Return is relayed onto the
 * question id in the original call. */
static int reflect_send(struct capn_rpc_conn *c, int pipelined,
                        uint32_t target_id, const uint16_t *path, int npath,
                        const uint8_t *frame, size_t len)
{
	struct capn in, out;
	struct Message m, om;
	struct Call call, ocall;
	struct Payload params, oparams;
	struct MessageTarget t;
	Message_ptr mp, omp;
	Call_ptr cp;
	Payload_ptr plp;
	MessageTarget_ptr tp;
	struct capn_segment *cs;
	struct capn_rpc_question *q;
	uint32_t local = 0, remote;
	int rc, i;

	if (capn_init_mem(&in, frame, len, 0) != 0)
		return -1;
	mp.p = capn_getp(capn_root(&in), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message_call) {
		capn_free(&in);
		return -1;
	}
	read_Call(&call, m.call);
	remote = call.questionId;
	memset(&params, 0, sizeof params);
	if (call.params.p.type != CAPN_NULL)
		read_Payload(&params, call.params);
	capn_resolve(&params.content);

	q = question_claim(c, &local);
	if (q == NULL) {
		capn_free(&in);
		send_return_exception(c, remote, "forward failed");
		return -1;
	}

	capn_init_malloc(&out);
	cs = capn_root(&out).seg;
	memset(&om, 0, sizeof om);
	memset(&ocall, 0, sizeof ocall);
	memset(&t, 0, sizeof t);
	memset(&oparams, 0, sizeof oparams);
	oparams.content = copy_ptr(cs, params.content);
	if (capn_len(params.capTable) > 0)
		oparams.capTable = rewrite_cap_table(c, cs, params.capTable);

	if (pipelined) {
		struct PromisedAnswer pa;
		PromisedAnswer_ptr pap;
		memset(&pa, 0, sizeof pa);
		pa.questionId = target_id;
		if (npath > 0 && path != NULL) {
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
	plp = new_Payload(cs);
	write_Payload(&oparams, plp);
	ocall.questionId = local;
	ocall.target = tp;
	ocall.interfaceId = call.interfaceId;
	ocall.methodId = call.methodId;
	ocall.params = plp;
	cp = new_Call(cs);
	write_Call(&ocall, cp);
	om.which = Message_call;
	om.call = cp;
	omp = new_Message(cs);
	write_Message(&om, omp);
	capn_setp(capn_root(&out), 0, omp.p);
	rc = send_message(c, &out);
	capn_free(&in);
	capn_free(&out);
	if (rc != 0) {
		question_clear(q);
		send_return_exception(c, remote, "forward failed");
		return -1;
	}
	if (relay_add(c, local, remote) != 0) {
		send_return_exception(c, remote, "forward table full");
		return -1;
	}
	return 0;
}

static void flush_reject(struct capn_rpc_conn *c, int export_id, const char *reason)
{
	int i;

	while ((i = oldest_queued(c, 0, export_id, 0)) >= 0) {
		uint32_t qid;
		if (frame_question_id(c->queued[i].frame, c->queued[i].len, &qid) == 0)
			send_return_exception(c, qid, reason ? reason : "promise released");
		free(c->queued[i].frame);
		memset(&c->queued[i], 0, sizeof c->queued[i]);
	}
	flush_echoes(c, export_id);
}

static void flush_replay(struct capn_rpc_conn *c, int export_id)
{
	int i;

	while ((i = oldest_queued(c, 0, export_id, 0)) >= 0) {
		uint8_t *frame = c->queued[i].frame;
		size_t len = c->queued[i].len;
		memset(&c->queued[i], 0, sizeof c->queued[i]);
		capn_rpc_handle(c, frame, len);
		free(frame);
	}
	flush_echoes(c, export_id);
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
	int rc;

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
		if (ret == 0)
			flush_questions(c, remote_qid);
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
	opl.content = copy_ptr(cs, pl.content);
	/* The peer wrote this cap table. We are now the sender, so hosted
	 * fields swap. Dropping the table, or copying it unchanged, would
	 * hand the caller indexes that name the wrong vat. */
	if (capn_len(pl.capTable) > 0)
		opl.capTable = rewrite_cap_table(c, cs, pl.capTable);
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
	/* Keep the Return, the same way a direct answer is kept, so a
	 * pipeline that arrived while the forward was open can resolve. */
	rc = send_answer(c, remote_qid, &out, 1);
	capn_free(&in);
	capn_free(&out);
	if (rc == 0)
		flush_questions(c, remote_qid);
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
		/* The question we asked the forward target has its Return.
		 * Finish releases that id. Leaving it claimed leaks the slot. */
		capn_rpc_send_finish(c, local_qid);
		return;
	}
}

static void flush_forward(struct capn_rpc_conn *c, int export_id, uint32_t import_id)
{
	int i;
	while ((i = oldest_queued(c, 0, export_id, 0)) >= 0) {
		/* Straight to this import. Do not follow a later resolution
		 * of it: that is the Tribble rule, and reflect_send is what
		 * implements it. */
		reflect_send(c, 0, import_id, NULL, 0, c->queued[i].frame,
		             c->queued[i].len);
		free(c->queued[i].frame);
		memset(&c->queued[i], 0, sizeof c->queued[i]);
	}
	flush_echoes(c, export_id);
}

static void install_param_caps(struct capn_rpc_conn *c, struct capn_segment *cs,
                              struct capn_rpc_pending_cap *caps, int n)
{
	int i;

	memset(&c->in_caps, 0, sizeof c->in_caps);
	if (n <= 0 || cs == NULL)
		return;
	c->in_caps = new_CapDescriptor_list(cs, n);
	for (i = 0; i < n; i++) {
		struct CapDescriptor cd;
		struct capn_rpc_pending_cap *p = &caps[i];
		int k;

		memset(&cd, 0, sizeof cd);
		cd.attachedFd = 0xff;
		if (p->kind == CAPN_RPC_CAP_ANSWER) {
			struct PromisedAnswer pa;
			PromisedAnswer_ptr pap;

			memset(&pa, 0, sizeof pa);
			pa.questionId = p->question_id;
			if (p->npath > 0 && p->path != NULL) {
				pa.transform = new_PromisedAnswer_Op_list(cs, p->npath);
				for (k = 0; k < p->npath; k++) {
					struct PromisedAnswer_Op op;
					memset(&op, 0, sizeof op);
					op.which = PromisedAnswer_Op_getPointerField;
					op.getPointerField = p->path[k];
					set_PromisedAnswer_Op(&op, pa.transform, k);
				}
			}
			pap = new_PromisedAnswer(cs);
			write_PromisedAnswer(&pa, pap);
			cd.which = CapDescriptor_receiverAnswer;
			cd.receiverAnswer = pap;
		} else if (p->is_import && p->kind != CAPN_RPC_CAP_PROMISE) {
			cd.which = CapDescriptor_senderHosted;
			cd.senderHosted = p->id;
		} else {
			cd.which = CapDescriptor_receiverHosted;
			cd.receiverHosted = p->id;
		}
		set_CapDescriptor(&cd, c->in_caps, i);
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
	struct capn pmsg;
	struct capn *rmsg;
	capn_ptr params, results;
	struct capn_segment *cs;
	int rc, eid = export_id;
	CapDescriptor_list outer_in;

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

	outer_in = c->in_caps;
	capn_init_malloc(&pmsg);
	params = capn_new_struct(capn_root(&pmsg).seg, params_datasz, params_ptrs);
	{
		struct capn_rpc_pending_cap param_caps[CAPN_RPC_MAX_PENDING_CAPS];
		struct capn_rpc_pending_cap *saved_dest = c->cap_dest;
		int *saved_n = c->cap_dest_n;
		int nparam = 0;

		/* Caps the caller puts in the parameters have to be visible
		 * to the local target. Recording them after fill() drops them. */
		c->cap_dest = param_caps;
		c->cap_dest_n = &nparam;
		if (fill != NULL)
			fill(fill_ctx, params);
		c->cap_dest = saved_dest;
		c->cap_dest_n = saved_n;
		if (nparam > 0) {
			install_param_caps(c, capn_root(&pmsg).seg, param_caps, nparam);
			free_cap_paths(param_caps, nparam);
		}
	}

	/* The message has to be on the heap before init. A stack `struct capn`
	 * would leave every segment's `capn` pointer aimed at that frame, and
	 * a shallow copy into `pc->msg` would not retarget it. */
	rmsg = (struct capn *) malloc(sizeof *rmsg);
	if (rmsg == NULL) {
		q = question_find(c, qid);
		if (q != NULL)
			fail_existing(q);
		capn_free(&pmsg);
		return qid;
	}
	capn_init_malloc(rmsg);
	cs = capn_root(rmsg).seg;
	results = capn_new_struct(cs, c->results_datasz, c->results_ptrs);
	{
		struct capn_rpc_pending_cap local_caps[CAPN_RPC_MAX_PENDING_CAPS];
		struct capn_rpc_pending_cap *saved_dest = c->cap_dest;
		int *saved_n = c->cap_dest_n;
		int nlocal = 0;
		uint32_t saved_q = c->current_question;
		uint32_t saved_g = c->current_generation;
		uint32_t gen = take_generation(c);

		c->npending = 0;
		c->current_question = qid;
		c->current_generation = gen;
		c->cap_dest = local_caps;
		c->cap_dest_n = &nlocal;
		rc = c->exports[eid].dispatch(c->exports[eid].server, interface_id,
		                              method_id, params, results);
		c->cap_dest = saved_dest;
		c->cap_dest_n = saved_n;
		c->current_question = saved_q;
		c->current_generation = saved_g;
		if (rc == CAPN_RPC_PENDING) {
			int i;
			for (i = 0; i < CAPN_RPC_MAX_PENDING_CALLS; i++) {
				struct capn_rpc_pending_call *pc = &c->pending_calls[i];
				if (pc->used)
					continue;
				memset(pc, 0, sizeof *pc);
				pc->used = 1;
				pc->local_question = 1;
				pc->question_id = qid;
				pc->generation = gen;
				pc->msg = rmsg;
				pc->results = results;
				pc->ncaps = nlocal;
				if (nlocal > 0)
					memcpy(pc->caps, local_caps,
					       (size_t) nlocal * sizeof local_caps[0]);
				c->in_caps = outer_in;
				capn_free(&pmsg);
				return qid;
			}
			release_unsent_caps(c, local_caps, nlocal);
			free_cap_paths(local_caps, nlocal);
			q = question_find(c, qid);
			if (q != NULL)
				fail_existing(q);
			c->in_caps = outer_in;
			capn_free(&pmsg);
			capn_free(rmsg);
			free(rmsg);
			return qid;
		}
		c->in_caps = outer_in;
		if (rc != 0) {
			release_unsent_caps(c, local_caps, nlocal);
			free_cap_paths(local_caps, nlocal);
			q = question_find(c, qid);
			if (q != NULL)
				fail_existing(q);
			capn_free(&pmsg);
			capn_free(rmsg);
			free(rmsg);
			return qid;
		}
		/* The success path below reads these. */
		c->npending = nlocal;
		if (nlocal > 0)
			memcpy(c->pending_caps, local_caps,
			       (size_t) nlocal * sizeof local_caps[0]);
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
		attach_cap_table(cs, &pl, c->pending_caps, c->npending);
		free_cap_paths(c->pending_caps, c->npending);
		c->npending = 0;
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
		capn_setp(capn_root(rmsg), 0, mp.p);
		buf = frame_alloc(rmsg, &len);
		q = question_find(c, qid);
		if (buf == NULL || q == NULL) {
			free(buf);
			if (q)
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
	capn_free(rmsg);
	free(rmsg);
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
	{
		struct capn_rpc_pending_cap local_caps[CAPN_RPC_MAX_PENDING_CAPS];
		struct capn_rpc_pending_cap *saved_dest = c->cap_dest;
		int *saved_n = c->cap_dest_n;
		int nlocal = 0;

		c->cap_dest = local_caps;
		c->cap_dest_n = &nlocal;
		if (fill != NULL)
			fill(ctx, params);
		c->cap_dest = saved_dest;
		c->cap_dest_n = saved_n;
		c->npending = nlocal;
		if (nlocal > 0)
			memcpy(c->pending_caps, local_caps,
			       (size_t) nlocal * sizeof local_caps[0]);
	}
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
		uint16_t *copy = (uint16_t *) malloc((size_t) npath * sizeof path[0]);
		if (copy == NULL) {
			/* The slot was already marked used. Leaving `msg` set and
			 * then freeing it would free the same message again when
			 * the connection is destroyed. */
			release_unsent_caps(c, c->pending_caps, c->npending);
			free_cap_paths(c->pending_caps, c->npending);
			c->npending = 0;
			memset(&c->held[i], 0, sizeof c->held[i]);
			q = question_find(c, qid);
			if (q != NULL)
				question_clear(q);
			capn_free(msg);
			free(msg);
			return (uint32_t) -1;
		}
		memcpy(copy, path, (size_t) npath * sizeof path[0]);
		c->held[i].path = copy;
		c->held[i].npath = npath;
	}
	c->held[i].seq = ++c->queue_clock;
	if (c->npending > 0) {
		int n = c->npending;
		if (n > CAPN_RPC_MAX_PENDING_CAPS)
			n = CAPN_RPC_MAX_PENDING_CAPS;
		c->held[i].ncaps = n;
		memcpy(c->held[i].caps, c->pending_caps,
		       (size_t) n * sizeof c->pending_caps[0]);
		/* held owns the paths now. */
		{
			int k;
			for (k = 0; k < n; k++)
				c->pending_caps[k].path = NULL;
		}
		c->npending = 0;
	}
	return qid;
}

/* Present a held call's caps the way an incoming cap table would, so
 * capn_rpc_param_cap agrees with a call the peer had sent us. */
static void install_held_caps(struct capn_rpc_conn *c, struct capn_rpc_held *h)
{
	if (h->ncaps <= 0 || h->msg == NULL) {
		memset(&c->in_caps, 0, sizeof c->in_caps);
		return;
	}
	install_param_caps(c, capn_root(h->msg).seg, h->caps, h->ncaps);
}

static void fire_held(struct capn_rpc_conn *c, uint32_t import_id)
{
	int i;
	while ((i = oldest_held(c, import_id)) >= 0) {
		struct capn_rpc_held h;
		struct capn_rpc_copy src;
		struct capn_rpc_question *q;
		h = c->held[i];
		c->held[i].used = 0;
		c->held[i].msg = NULL;
		c->held[i].path = NULL;
		c->held[i].npath = 0;
		c->held[i].ncaps = 0;
		{
			int k;
			for (k = 0; k < CAPN_RPC_MAX_PENDING_CAPS; k++)
				c->held[i].caps[k].path = NULL;
		}
		q = question_find(c, h.question_id);
		src.ptr = h.params;
		if (q == NULL) {
			if (h.msg != NULL) {
				capn_free(h.msg);
				free(h.msg);
			}
			free(h.path);
			free_cap_paths(h.caps, h.ncaps);
			continue;
		}
		if (h.mode == CAPN_RPC_HELD_LOCAL) {
			install_held_caps(c, &h);
			dispatch_local(c, (int) h.target, h.interface_id, h.method_id,
			               h.datasz, h.ptrs, fill_from_ptr, &src, q,
			               h.question_id);
			memset(&c->in_caps, 0, sizeof c->in_caps);
		} else if (h.mode == CAPN_RPC_HELD_PIPE) {
			c->npending = h.ncaps;
			if (h.ncaps > 0)
				memcpy(c->pending_caps, h.caps,
				       (size_t) h.ncaps * sizeof h.caps[0]);
			send_call_impl(c, 1, h.target, h.path, h.npath, h.interface_id,
			               h.method_id, h.datasz, h.ptrs, fill_from_ptr, &src,
			               q, h.question_id);
		} else {
			note_import_call(c, h.target);
			c->npending = h.ncaps;
			if (h.ncaps > 0)
				memcpy(c->pending_caps, h.caps,
				       (size_t) h.ncaps * sizeof h.caps[0]);
			send_call_impl(c, 0, h.target, NULL, 0, h.interface_id, h.method_id,
			               h.datasz, h.ptrs, fill_from_ptr, &src, q,
			               h.question_id);
		}
		if (h.msg != NULL) {
			capn_free(h.msg);
			free(h.msg);
		}
		free(h.path);
		free_cap_paths(h.caps, h.ncaps);
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
	ex.reason.seg = NULL;
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
		struct capn_rpc_import *other;
		uint32_t pid = r.promiseId;
		uint32_t other_id = cd.which == CapDescriptor_senderHosted ?
		                    cd.senderHosted : cd.senderPromise;
		int calls;
		im->has_import = 1;
		im->to_import = other_id;
		calls = im->calls;
		/* import_slot may realloc the table. im is stale after this. */
		other = import_slot(c, other_id);
		im = import_slot(c, pid);
		if (other == NULL || im == NULL)
			return -1;
		/* A still-open promise absorbs the calls so its own later
		 * reflection performs one embargo. rpc.c++ PromiseClient
		 * ::resolve merges receivedCall in that case. A promise that
		 * has already resolved back to us needs its own embargo. */
		if (cd.which == CapDescriptor_senderPromise && !other->resolved)
			other->calls += calls;
		if (cd.which == CapDescriptor_senderPromise && other->resolved &&
		    other->has_export && calls > 0) {
			im->has_import = 0;
			im->has_export = 1;
			im->to_export = other->to_export;
			im->embargoed = 1;
			im->embargo_id = ++c->next_embargo;
			return send_disembargo(c, pid, im->embargo_id);
		}
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
		int n;
		read_PromisedAnswer(&pa, cd.receiverAnswer);
		im->has_question = 1;
		im->to_question = pa.questionId;
		free(im->to_path);
		im->to_path = NULL;
		im->to_npath = 0;
		n = ops_to_path(pa.transform, &im->to_path, &im->to_npath);
		if (n == -2) {
			capn_rpc_send_abort(c, kBadPipelineOp);
			return -1;
		}
		if (n != 0)
			return -1;
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
			struct capn_rpc_import *nxt = NULL;
			mode = CAPN_RPC_HELD_REMOTE;
			target = im->to_import;
			if (target < (uint32_t) c->imports_cap &&
			    c->imports[target].used)
				nxt = &c->imports[target];
			/* The promise we were merged with already came back to a
			 * capability we host. Embargo before delivering locally,
			 * which is PromiseClient::resolve when the other side is
			 * already REFLECTED. */
			if (nxt != NULL && nxt->resolved && nxt->has_export &&
			    (im->calls > 0 || nxt->calls > 0) && !im->embargoed) {
				im->has_import = 0;
				im->has_export = 1;
				im->to_export = nxt->to_export;
				im->embargoed = 1;
				im->embargo_id = ++c->next_embargo;
				send_disembargo(c, id, im->embargo_id);
				return hold_call(c, CAPN_RPC_HELD_LOCAL, id, nxt->to_export,
				                 NULL, 0, interface_id, method_id,
				                 params_datasz, params_ptrs, fill, fill_ctx);
			}
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
	uint32_t qid;
	if (npath < 0 || (npath > 0 && path == NULL))
		return (uint32_t)-1;
	qid = send_call_impl(c, 1, question_id, path, npath, interface_id, method_id,
	                     params_datasz, params_ptrs, fill, fill_ctx, NULL, 0);
	if (qid != (uint32_t) -1)
		note_pipe(c, question_id, path, npath);
	return qid;
}

/* Release each import in the peer's Return that the application did
 * not read, as rpc.c++ does when an unused response is dropped. */
static void release_unread_results(struct capn_rpc_conn *c,
                                   struct capn_rpc_question *q)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	struct Payload pl;
	Message_ptr mp;
	int i, j, n;

	if (!q->peer_results || q->bootstrap || q->reply == NULL)
		return;
	if (capn_init_mem(&msg, q->reply, q->reply_len, 0) != 0)
		return;
	mp.p = capn_getp(capn_root(&msg), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message__return)
		goto out;
	read_Return(&r, m._return);
	if (r.which != Return_results || r.results.p.type == CAPN_NULL)
		goto out;
	read_Payload(&pl, r.results);
	n = capn_len(pl.capTable);
	for (i = 0; i < n; i++) {
		struct CapDescriptor cd;

		for (j = 0; j < q->ntaken; j++)
			if (q->taken[j] == (uint32_t) i)
				break;
		if (j < q->ntaken)
			continue;
		get_CapDescriptor(&cd, pl.capTable, i);
		if (cd.which == CapDescriptor_senderHosted)
			capn_rpc_send_release(c, cd.senderHosted, 1);
		else if (cd.which == CapDescriptor_senderPromise)
			capn_rpc_send_release(c, cd.senderPromise, 1);
	}
out:
	capn_free(&msg);
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
	int awaiting = 0;
	int rc;

	q = question_find(c, question_id);
	if (q) {
		int skip = q->no_finish;

		awaiting = !q->answered && !q->tail_wait;
		release_unread_results(c, q);
		/* Finish before Return still owes the peer a release of the
		 * params' caps. rpc.c++ ~QuestionRef leaves the question up. */
		if (!q->answered)
			stash_save(c, q);
		question_clear(q);
		if (skip)
			return 0;
	}

	capn_init_malloc(&msg);
	cs = capn_root(&msg).seg;
	memset(&m, 0, sizeof m);
	memset(&f, 0, sizeof f);
	f.questionId = question_id;
	/* rpc.c++ ~QuestionRef: a call canceled before its Return asks the
	 * peer to drop the results' caps. */
	f.releaseResultCaps = awaiting;
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

int64_t capn_rpc_answer_cap_id(struct capn_rpc_conn *c, uint32_t question_id)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	struct Payload pl;
	struct CapDescriptor cd;
	Message_ptr mp;
	struct capn_rpc_question *q = question_find(c, question_id);
	int64_t id = -1;

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
		id = (int64_t) cd.senderHosted;
	else if (cd.which == CapDescriptor_senderPromise)
		id = (int64_t) cd.senderPromise;
	else if (cd.which == CapDescriptor_receiverHosted)
		id = (int64_t) cd.receiverHosted;
	if (cd.which == CapDescriptor_senderHosted ||
	    cd.which == CapDescriptor_senderPromise)
		note_taken(q, pl.content.cap_idx);

out:
	capn_free(&msg);
	return id;
}

int64_t capn_rpc_answer_cap(struct capn_rpc_conn *c, uint32_t question_id, capn_ptr cap,
                        int *is_export)
{
	struct capn msg;
	struct Message m;
	struct Return r;
	struct Payload pl;
	Message_ptr mp;
	struct capn_rpc_question *q = question_find(c, question_id);
	int64_t id = -1;
	int kind = -1;

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
			id = cap_lookup(pl.capTable, cap, &kind);
			if (id >= 0 && is_export)
				*is_export = kind;
			capn_resolve(&cap);
			if (id >= 0 && kind == 0)
				note_taken(q, cap.cap_idx);
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

/* The reflected call's results were kept locally. Tell the peer it
 * does not need to forward them, which is what rpc.c++
 * RpcCallContext::sendRedirectReturn sends. */
static int send_results_elsewhere(struct capn_rpc_conn *c, uint32_t qid)
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
	rp = new_Return(cs);
	r.answerId = qid;
	r.releaseParamCaps = 0;
	r.which = Return_resultsSentElsewhere;
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

/* Use the results we computed for the peer's question as the answer to
 * one of ours. The stored frame is a Return(results) whose answerId is
 * theirs; answer_content only reads the payload. */
/* The stored Return was written for the peer. We are now the side that
 * reads it, so hosted fields swap, the same rename rewrite_cap_table
 * does for a reflected call. */
static uint8_t *swap_return_frame(struct capn_rpc_conn *c, const uint8_t *frame,
                                  size_t len, size_t *out_len)
{
	struct capn in, out;
	struct Message m, om;
	struct Return r, orr;
	struct Payload pl, opl;
	Message_ptr mp, omp;
	Return_ptr rp;
	Payload_ptr plp;
	struct capn_segment *cs;

	if (capn_init_mem(&in, frame, len, 0) != 0)
		return NULL;
	mp.p = capn_getp(capn_root(&in), 0, 1);
	read_Message(&m, mp);
	if (m.which != Message__return) {
		capn_free(&in);
		return NULL;
	}
	read_Return(&r, m._return);
	if (r.which != Return_results || r.results.p.type == CAPN_NULL) {
		capn_free(&in);
		return NULL;
	}
	read_Payload(&pl, r.results);
	if (capn_len(pl.capTable) <= 0) {
		capn_free(&in);
		return NULL;
	}
	capn_init_malloc(&out);
	cs = capn_root(&out).seg;
	memset(&om, 0, sizeof om);
	memset(&orr, 0, sizeof orr);
	memset(&opl, 0, sizeof opl);
	capn_resolve(&pl.content);
	opl.content = copy_ptr(cs, pl.content);
	opl.capTable = rewrite_cap_table(c, cs, pl.capTable);
	plp = new_Payload(cs);
	write_Payload(&opl, plp);
	orr.answerId = r.answerId;
	orr.releaseParamCaps = r.releaseParamCaps;
	orr.which = Return_results;
	orr.results = plp;
	rp = new_Return(cs);
	write_Return(&orr, rp);
	om.which = Message__return;
	om._return = rp;
	omp = new_Message(cs);
	write_Message(&om, omp);
	capn_setp(capn_root(&out), 0, omp.p);
	{
		uint8_t *buf = frame_alloc(&out, out_len);
		capn_free(&in);
		capn_free(&out);
		return buf;
	}
}

static void adopt_answer(struct capn_rpc_conn *c, struct capn_rpc_question *q,
                         struct capn_rpc_answer *a)
{
	struct capn stored;
	struct Message m;
	struct Return r;
	Message_ptr mp;
	uint8_t *copy;
	size_t copy_len = 0;

	copy = swap_return_frame(c, a->frame, a->len, &copy_len);
	if (copy == NULL) {
		copy = (uint8_t *) malloc(a->len ? a->len : 1);
		if (copy == NULL)
			return;
		memcpy(copy, a->frame, a->len);
		copy_len = a->len;
	}
	free(q->reply);
	q->reply = copy;
	q->reply_len = copy_len;
	q->answered = 1;
	q->failed = 1;
	q->tail_wait = 0;
	if (capn_init_mem(&stored, copy, copy_len, 0) != 0)
		return;
	mp.p = capn_getp(capn_root(&stored), 0, 1);
	read_Message(&m, mp);
	if (m.which == Message__return) {
		read_Return(&r, m._return);
		q->failed = (r.which != Return_results);
		if (r.which == Return_results && r.results.p.type != CAPN_NULL) {
			struct Payload pl;
			read_Payload(&pl, r.results);
			note_introductions(c, &pl);
			apply_pipeline_notes(c, q, &pl);
		}
	}
	capn_free(&stored);
}

static void deliver_tail(struct capn_rpc_conn *c, uint32_t peer_qid)
{
	struct capn_rpc_answer *a = answer_find(c, peer_qid);
	int i;

	if (a == NULL || !a->redirected || a->taken)
		return;
	for (i = 0; i < c->questions_cap; i++) {
		struct capn_rpc_question *q = &c->questions[i];
		if (!q->used || !q->tail_wait || q->answered || q->tail_from != peer_qid)
			continue;
		a->taken = 1;
		adopt_answer(c, q, a);
		send_results_elsewhere(c, peer_qid);
		return;
	}
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
	if (q == NULL) {
		/* Finish already dropped the question. The exports it sent
		 * are still here, and releaseParamCaps drops them. */
		stash_drop(c, r.answerId, r.releaseParamCaps);
		return;
	}
	if (q->answered)
		return;
	/* rpc.c++ handleReturn: a level 0 peer asks the caller to drop the
	 * params' caps, and a Return without caps needs no Finish. */
	if (r.releaseParamCaps) {
		int i;
		for (i = 0; i < q->nparam_exports; i++)
			release_export_ref(c, q->param_exports[i]);
	}
	free(q->param_exports);
	q->param_exports = NULL;
	q->nparam_exports = 0;
	if (r.noFinishNeeded)
		q->no_finish = 1;
	/* The peer forwarded a pipelined call back to us and is telling us
	 * to use that call's results. rpc.c++ handleReturn
	 * TAKE_FROM_OTHER_QUESTION. */
	if (r.which == Return_takeFromOtherQuestion) {
		struct capn_rpc_answer *a = answer_find(c, r.takeFromOtherQuestion);
		if (a == NULL) {
			q->tail_wait = 1;
			q->tail_from = r.takeFromOtherQuestion;
			return;
		}
		/* rpc.c++ moves the redirected answer to Finished, so a second
		 * take is not another copy of the same payload. */
		if (!a->redirected || a->taken) {
			capn_rpc_send_abort(c,
				"`Return.takeFromOtherQuestion` referenced a call that did not "
				"use `sendResultsTo.yourself`.");
			return;
		}
		a->taken = 1;
		adopt_answer(c, q, a);
		send_results_elsewhere(c, r.takeFromOtherQuestion);
		return;
	}
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
	q->peer_results = (r.which == Return_results);
	if (r.which == Return_results && r.results.p.type != CAPN_NULL) {
		struct Payload pl;
		read_Payload(&pl, r.results);
		note_introductions(c, &pl);
		apply_pipeline_notes(c, q, &pl);
	}
	relay_on_return(c, r.answerId, frame, len);
}

static int payload_cap(struct Payload *pl, const uint16_t *path, int npath,
                       struct CapDescriptor *cd)
{
	capn_ptr cursor;
	int i;

	cursor = pl->content;
	for (i = 0; i < npath; i++) {
		capn_resolve(&cursor);
		if (cursor.type != CAPN_STRUCT)
			return -1;
		cursor = capn_getp(cursor, path[i], 0);
	}
	capn_resolve(&cursor);
	if (cursor.type != CAPN_INTERFACE)
		return -1;
	if (capn_len(pl->capTable) < 0 ||
	    cursor.cap_idx > (uint32_t) INT_MAX ||
	    (int) cursor.cap_idx >= capn_len(pl->capTable))
		return -1;
	get_CapDescriptor(cd, pl->capTable, (int) cursor.cap_idx);
	return 0;
}

static void note_pipe(struct capn_rpc_conn *c, uint32_t question_id,
                      const uint16_t *path, int npath)
{
	struct capn_rpc_question *q = question_find(c, question_id);
	int i;

	if (q == NULL)
		return;
	if (npath < 0)
		npath = 0;
	for (i = 0; i < q->npipes; i++) {
		if (q->pipes[i].npath == npath &&
		    (npath == 0 ||
		     memcmp(q->pipes[i].path, path,
		            (size_t) npath * sizeof path[0]) == 0)) {
			q->pipes[i].calls++;
			return;
		}
	}
	if (q->npipes >= q->pipes_cap) {
		int cap;
		struct capn_rpc_pipe_note *grown;
		if (q->pipes_cap > INT_MAX / 2)
			return;
		cap = q->pipes_cap > 0 ? q->pipes_cap * 2 : CAPN_RPC_PIPE_NOTES_INIT;
		if (cap <= 0 || (size_t) cap > SIZE_MAX / sizeof *grown)
			return;
		grown = (struct capn_rpc_pipe_note *) realloc(
			q->pipes, (size_t) cap * sizeof *grown);
		if (grown == NULL)
			return;
		memset(grown + q->pipes_cap, 0,
		       (size_t) (cap - q->pipes_cap) * sizeof *grown);
		q->pipes = grown;
		q->pipes_cap = cap;
	}
	i = q->npipes;
	q->pipes[i].path = NULL;
	q->pipes[i].npath = 0;
	if (npath > 0 && path != NULL) {
		q->pipes[i].path = (uint16_t *) malloc((size_t) npath * sizeof path[0]);
		if (q->pipes[i].path == NULL)
			return;
		memcpy(q->pipes[i].path, path, (size_t) npath * sizeof path[0]);
		q->pipes[i].npath = npath;
	}
	q->pipes[i].calls = 1;
	q->npipes++;
}

/* Pipelined calls were sent at the question, not at the import the
 * Return names. Count them on that import so a later Resolve back to
 * us still sends Disembargo. rpc.c++ PromiseClient::newCall sets
 * receivedCall, and resolve() embargos when the replacement is local. */
static void apply_pipeline_notes(struct capn_rpc_conn *c,
                                 struct capn_rpc_question *q,
                                 struct Payload *pl)
{
	int i;

	if (q == NULL)
		return;
	for (i = 0; i < q->npipes; i++) {
		struct CapDescriptor cd;
		struct capn_rpc_import *im;
		uint32_t id;

		if (q->pipes[i].calls <= 0)
			continue;
		if (payload_cap(pl, q->pipes[i].path, q->pipes[i].npath, &cd) != 0)
			continue;
		if (cd.which != CapDescriptor_senderHosted &&
		    cd.which != CapDescriptor_senderPromise)
			continue;
		id = cd.which == CapDescriptor_senderPromise ? cd.senderPromise
		                                             : cd.senderHosted;
		im = import_slot(c, id);
		if (im == NULL)
			continue;
		im->calls += q->pipes[i].calls;
		if (cd.which == CapDescriptor_senderPromise && im->resolved &&
		    im->has_export && im->calls > 0 && !im->embargoed) {
			im->embargoed = 1;
			im->embargo_id = ++c->next_embargo;
			send_disembargo(c, id, im->embargo_id);
		}
	}
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
	rc = send_answer(c, qid, &msg, 1);
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
	rc = send_answer(c, qid, &msg, 1);
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
			struct capn_rpc_conn *dst = c->vat->provisions[i].accept_conn;
			memset(&c->vat->provisions[i], 0, sizeof c->vat->provisions[i]);
			if (!dst)
				return 0;
			return send_accepted_cap(dst, qid, eid);
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
	if (question_id_in_use(c, pv.questionId))
		return abort_live_question(c, NULL);
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
			c->vat->provisions[i].intro_conn = c;
			c->vat->provisions[i].intro_export = eid;
			c->vat->provisions[i].has_recipient = 0;
			c->vat->provisions[i].recipient_port = 0;
			c->vat->provisions[i].recipient_host[0] = '\0';
			if (rid.vat.p.type != CAPN_NULL) {
				struct VatId vat;
				int n;
				read_VatId(&vat, rid.vat);
				c->vat->provisions[i].has_recipient = 1;
				c->vat->provisions[i].recipient_port = vat.port;
				n = vat.host.len;
				if (n < 0)
					n = 0;
				if (n >= CAPN_RPC_MAX_HOST)
					n = CAPN_RPC_MAX_HOST - 1;
				if (n > 0 && vat.host.str)
					memcpy(c->vat->provisions[i].recipient_host,
					       vat.host.str, (size_t) n);
				c->vat->provisions[i].recipient_host[n] = '\0';
			}
			/* Pin the export until Accept. The pin is dropped
			 * when the recipient takes her own export. */
			c->exports[eid].refcount++;
			return send_empty_return(c, pv.questionId);
		}
	}
	return send_return_exception(c, pv.questionId, "provide: table full");
}

/* Drop the reference `Provide` took on the introducer's export. */
static void drop_intro_hold(struct capn_rpc_provision *pr)
{
	struct capn_rpc_conn *ic = pr->intro_conn;
	int eid = pr->intro_export;

	pr->intro_conn = NULL;
	pr->intro_export = -1;
	if (!ic || eid < 0 || eid >= ic->exports_cap)
		return;
	if (!ic->exports[eid].used || ic->exports[eid].refcount <= 0)
		return;
	if (ic->exports[eid].refcount == 1)
		memset(&ic->exports[eid], 0, sizeof ic->exports[eid]);
	else
		ic->exports[eid].refcount--;
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
	if (question_id_in_use(c, ac.questionId))
		return abort_live_question(c, NULL);
	if (ac.provision.type == CAPN_NULL)
		return send_return_exception(c, ac.questionId,
		                             "accept: no provision");
	pp.p = ac.provision;
	read_ProvisionId(&pid, pp);

	for (i = 0; i < CAPN_RPC_MAX_PROVISIONS; i++) {
		if (c->vat->provisions[i].used && !c->vat->provisions[i].embargoed &&
		    c->vat->provisions[i].nonce == pid.nonce) {
			/* ProvisionId is the nonce. A pointer beside it is
			 * not a vat and is not compared. */
			eid = capn_rpc_export(c, c->vat->provisions[i].server,
			                      c->vat->provisions[i].dispatch);
			if (eid < 0)
				return send_return_exception(c, ac.questionId,
				                             "accept: export table full");
			drop_intro_hold(&c->vat->provisions[i]);
			c->vat->provisions[i].export_id = eid;
			if (ac.embargo) {
				/* Claimed, but the Return waits: the recipient
				 * has pipelined calls in flight through the
				 * introducer, and answering now would let a
				 * later call overtake them. The introducer
				 * lifts it with Disembargo.provide. The
				 * Return goes to this connection. */
				c->vat->provisions[i].embargoed = 1;
				c->vat->provisions[i].accept_question_id = ac.questionId;
				c->vat->provisions[i].accept_conn = c;
			} else {
				c->vat->provisions[i].used = 0;
				c->vat->provisions[i].has_recipient = 0;
				c->vat->provisions[i].recipient_port = 0;
				c->vat->provisions[i].recipient_host[0] = '\0';
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
	rc = send_answer(c, ac.questionId, &msg, 1);
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

int capn_rpc_provision_recipient(struct capn_rpc_conn *c, uint64_t nonce,
                                char *host, size_t hostcap, uint16_t *port)
{
	int i;

	if (c == NULL || c->vat == NULL)
		return -1;
	for (i = 0; i < CAPN_RPC_MAX_PROVISIONS; i++) {
		struct capn_rpc_provision *pr = &c->vat->provisions[i];
		size_t n;

		if (!pr->used || pr->nonce != nonce)
			continue;
		if (!pr->has_recipient)
			return 0;
		if (port)
			*port = pr->recipient_port;
		if (host != NULL && hostcap > 0) {
			n = strlen(pr->recipient_host);
			if (n >= hostcap)
				n = hostcap - 1;
			if (n)
				memcpy(host, pr->recipient_host, n);
			host[n] = '\0';
		}
		return 1;
	}
	return -1;
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
	if (m.which == Message_resolve) {
		/* rpc.c++ handleUnimplemented: the peer never took the
		 * reference the Resolve gave it. */
		struct Resolve res;
		struct CapDescriptor cd;

		read_Resolve(&res, m.resolve);
		if (res.which != Resolve_cap || res.cap.p.type == CAPN_NULL)
			return;
		read_CapDescriptor(&cd, res.cap);
		/* capn_rpc_resolve names the promise itself without a
		 * new reference when it resolves to the same server. */
		if (cd.which == CapDescriptor_senderHosted &&
		    cd.senderHosted != res.promiseId)
			release_export_ref(c, cd.senderHosted);
		else if (cd.which == CapDescriptor_senderPromise)
			release_export_ref(c, cd.senderPromise);
		else if (cd.which == CapDescriptor_thirdPartyHosted &&
		         cd.thirdPartyHosted.p.type != CAPN_NULL) {
			struct ThirdPartyCapDescriptor tp;

			read_ThirdPartyCapDescriptor(&tp, cd.thirdPartyHosted);
			release_export_ref(c, tp.vineId);
		}
		return;
	}
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
		 * have pipelined against can go. releaseResultCaps defaults
		 * to true and drops the exports that Return handed out. */
		struct Finish fin;
		struct capn_rpc_answer *answer;
		read_Finish(&fin, m.finish);
		answer = answer_find(c, fin.questionId);
		if (answer != NULL && fin.releaseResultCaps)
			release_answer_result_caps(c, answer);
		answer_drop(c, fin.questionId);
		/* Finish before the Return cancels it. The message stays until
		 * the matching return, which then sends nothing. */
		pending_cancel(c, fin.questionId);
		cancel_queued(c, fin.questionId);
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
		rc = handle_disembargo(c, m.disembargo, data, len);
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
	/* Same cap as the stream reader: 512 segments is too many. */
	if (nsegs >= 511u)
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
