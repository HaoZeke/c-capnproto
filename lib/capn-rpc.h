/* Two-party Cap'n Proto RPC vat.
 *
 * Answers the level 1 messages a peer sends to a capability this vat
 * hosts (Bootstrap, Call, Finish, Release, pipelined calls, and the
 * echo half of the Disembargo handshake), level 3 Provide and Accept for
 * handing a capability to a third vat, and level 4 Join. Level 3 needs a network layer that can
 * name a third vat, which rpc-twoparty.capnp cannot, so the vat speaks
 * the one in rpc-threeparty.capnp instead.
 *
 * The vat owns no transport. A caller feeds it whole framed messages and
 * receives whole framed messages back through the callbacks below, so it
 * works the same over a socket, a pipe, or a test harness in one process.
 */

#ifndef CAPN_RPC_H
#define CAPN_RPC_H

#include "capnp_c.h"
/* CapDescriptor_ptr, for the level 3 descriptor writer below. */
#include "rpc.capnp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CAPN_RPC_INITIAL_TABLE_CAPACITY 8
/* Export, answer and question tables double until this many slots.
 * The next insert fails and leaves the table as it was. */
#define CAPN_RPC_MAX_TABLE_CAPACITY 1024
#define CAPN_RPC_MAX_PROVISIONS 32
#define CAPN_RPC_MAX_INTRODUCTIONS 8
/* Long enough for a hostname; a longer one is refused, not truncated. */
#define CAPN_RPC_MAX_HOST 64
#define CAPN_RPC_MAX_JOINS 8
/* Outstanding unacknowledged calls a stream may hold. */
#define CAPN_RPC_STREAM_MAX_WINDOW 64
#define CAPN_RPC_MAX_JOIN_PARTS 16
/* Capabilities one params or results payload can carry. */
#define CAPN_RPC_MAX_PENDING_CAPS 16
#define CAPN_RPC_MAX_PENDING_CALLS 64
#define CAPN_RPC_MAX_QUEUED 32
#define CAPN_RPC_MAX_HELD 16
#define CAPN_RPC_MAX_RELAYS 32
#define CAPN_RPC_MAX_IMPORTS 4096
#define CAPN_RPC_PATH_MAX 8
/* First batch of pipeline notes on a question. The table doubles
 * from here. A fixed cap of four used to drop later paths. */
#define CAPN_RPC_PIPE_NOTES_INIT 4
/* Disembargo messages held until the promise they name has resolved
 * and its queued calls are on the wire. */
#define CAPN_RPC_MAX_ECHOES 8
/* What a pending capability becomes in the cap table. */
#define CAPN_RPC_CAP_HOSTED 0
#define CAPN_RPC_CAP_PROMISE 1
#define CAPN_RPC_CAP_ANSWER 2
/* Return this from dispatch to answer later with capn_rpc_return.
 * A plain 1 is an error, so this value is not 1. */
#define CAPN_RPC_PENDING (-2)

struct capn_rpc_conn;

/* Handle one call on an exported capability.
 *
 * `params` is the resolved parameter struct; write the reply into
 * `results`, a struct already allocated in the reply message. Return 0
 * to answer with those results, CAPN_RPC_PENDING to answer later, and
 * any other value to answer with an exception.
 */
typedef int (*capn_rpc_dispatch_fn)(void *server, uint64_t interface_id,
                                    uint16_t method_id, capn_ptr params,
                                    capn_ptr results);

/* Send one framed message to the peer. Return 0 on success. */
typedef int (*capn_rpc_send_fn)(void *ctx, const uint8_t *data, size_t len);

struct capn_rpc_export {
	int used;
	int refcount;
	void *server;
	capn_rpc_dispatch_fn dispatch;
	/* Calls stay queued until Resolve. A settled promise keeps the id
	 * and forwards, because the peer may still address the old id. */
	int promise;
	int broken;
	int has_forward;
	int forward_export;
	int has_forward_import;
	uint32_t forward_import;
};

/* A Return already sent, kept until the peer sends `Finish`.
 *
 * Promise pipelining is the reason: a caller may address a capability
 * inside an answer before it has seen the answer, so the answer has to
 * still be here when the pipelined call arrives.
 */
struct capn_rpc_answer {
	int used;
	uint32_t question_id;
	uint8_t *frame;
	size_t len;
	/* Kept locally because the call set sendResultsTo to yourself.
	 * takeFromOtherQuestion may adopt the payload once. */
	int redirected;
	int taken;
	/* Imports from the params that the results handed back. They stay
	 * until Finish, so a pipelined call can still name them. Owned
	 * by the answer. */
	uint32_t *held_imports;
	int nheld;
};

/* One pipelined call path sent against a question that had not
 * returned yet. `calls` is how many were sent along `path`. */
struct capn_rpc_pipe_note {
	int calls;
	int npath;
	/* Heap path. A fixed 8-slot array used to drop the tail. */
	uint16_t *path;
};

/* A question this vat asked, from send until its Return arrives. */
struct capn_rpc_question {
	int used;
	uint32_t question_id;
	int answered;
	int failed;
	uint8_t *reply;
	size_t reply_len;
	/* Growable. A fixed table of four used to drop further paths. */
	struct capn_rpc_pipe_note *pipes;
	int npipes;
	int pipes_cap;
	/* Return(takeFromOtherQuestion) arrived before the reflected call
	 * finished. tail_from is the peer's question id whose results are
	 * the answer. rpc.capnp Call.sendResultsTo.yourself. */
	int tail_wait;
	uint32_t tail_from;
	/* Exports the Call's params named, one reference each, which a
	 * Return with releaseParamCaps hands back. */
	uint32_t *param_exports;
	int nparam_exports;
	/* The Return said noFinishNeeded, so no Finish is sent. */
	int no_finish;
};

/* A capability promised to a third vat, awaiting its Accept.
 *
 * Level 3: the introducer told us to expect someone, and the nonce is
 * the whole of the arrangement. Matching on it alone is what lets the
 * recipient claim the capability without us having to trust her account
 * of who sent her. */
struct capn_rpc_provision {
	int used;
	uint64_t nonce;
	/* The capability itself, not this connection's id for it: the
	 * recipient may well arrive on another connection, where that id
	 * means nothing. */
	void *server;
	capn_rpc_dispatch_fn dispatch;
	int export_id;
	/* The introducer's Provide question, which is how a later
	 * Disembargo names this arrangement (rpc.capnp, Disembargo.context
	 * .provide). */
	uint32_t question_id;
	/* An embargoed Accept has claimed the capability but must not be
	 * answered until the introducer lifts the embargo, so the slot
	 * outlives the claim and carries the answer to send. */
	int embargoed;
	uint32_t accept_question_id;
	/* The connection that sent the embargoed Accept. The Return
	 * goes there, not to the connection that sent Disembargo. */
	struct capn_rpc_conn *accept_conn;
	/* The introducer's export, pinned until Accept. The pin is
	 * one reference and is dropped when the recipient claims it. */
	struct capn_rpc_conn *intro_conn;
	int intro_export;
	/* Host and port from Provide.recipient, when that message
	 * named a vat. handle_accept matches the nonce. */
	int has_recipient;
	uint16_t recipient_port;
	char recipient_host[CAPN_RPC_MAX_HOST];
};

/* An introduction we have been handed but not yet picked up.
 *
 * Level 3: a payload can name a capability hosted by a third vat, as a
 * `thirdPartyHosted` CapDescriptor carrying where to go and a vine. The
 * vine is an ordinary import through the introducer, so calls work
 * before we ever reach the third party; that is the fallback the spec
 * gives level 1 and 2 receivers, and it is why the vine must not be
 * released until the pickup succeeds. Connecting is the network layer's
 * job, so the vat records the introduction and hands it over.
 */
struct capn_rpc_introduction {
	int used;
	uint64_t nonce;
	uint32_t vine_id;
	uint16_t port;
	char host[CAPN_RPC_MAX_HOST];
};

/* Client-side flow control for `-> stream` methods: a bounded window of
 * unacknowledged stream calls. The wire carries ordinary Call/Return
 * pairs; the window is policy, as in capnp-C++. After any stream call
 * fails, later sends fail immediately and the failure surfaces at finish,
 * which is the streaming error-propagation rule. */
struct capn_rpc_stream {
	int window;
	int nout;
	uint32_t qids[CAPN_RPC_STREAM_MAX_WINDOW];
	int failed;
	uint32_t first_failure;
	int have_failure;
};

/* One in-flight Join, keyed by the sender's joinId.
 *
 * A Join asks whether several capabilities are the same object. Each part
 * is its own question, so no part is answerable when it arrives: the
 * answer depends on the whole set. Parts accumulate here and every
 * question is answered once the last one lands, which is the order
 * rpc-twoparty.capnp's JoinResult describes.
 */
struct capn_rpc_join {
	int used;
	uint32_t join_id;
	int part_count;
	int nseen;
	int seen[CAPN_RPC_MAX_JOIN_PARTS];
	uint32_t qids[CAPN_RPC_MAX_JOIN_PARTS];
	int eids[CAPN_RPC_MAX_JOIN_PARTS];
};

/* What a vat knows across all its connections.
 *
 * A level 3 handoff is arranged on one connection and claimed on
 * another: the introducer sends `Provide` over its own, and the
 * recipient arrives on hers. Holding the arrangement on the connection
 * would make it claimable only by the introducer, which is no handoff at
 * all. Connections given no vat get one to themselves, which is what a
 * two-party deployment wants.
 */
struct capn_rpc_vat {
	struct capn_rpc_provision provisions[CAPN_RPC_MAX_PROVISIONS];
};

/* A capability made while a payload is being filled, kept until the
 * payload's capTable is written. */
struct capn_rpc_pending_cap {
	int is_import;
	int kind;
	uint32_t id;
	uint32_t question_id;
	uint16_t *path;
	int npath;
};

/* An import id the peer handed us, including a promise not yet resolved. */
struct capn_rpc_import {
	int used;
	int resolved;
	int broken;
	int calls;
	int embargoed;
	uint32_t embargo_id;
	int has_import;
	uint32_t to_import;
	int has_export;
	uint32_t to_export;
	int has_question;
	uint32_t to_question;
	uint16_t *to_path;
	int to_npath;
};

/* A call that arrived for a promise (or an embargoed export) and cannot
 * be answered until the resolution is on the wire. `on_question` waits
 * for the Return of `question_id` instead of an export. */
struct capn_rpc_queued {
	int used;
	int on_question;
	int export_id;
	uint32_t question_id;
	uint8_t *frame;
	size_t len;
	/* Arrival order. A later call may reuse a lower slot, so the slot
	 * index is not the order the frames must be replayed. */
	uint64_t seq;
};

/* A call this vat made after Resolve but before the embargo echo. */
struct capn_rpc_held {
	int used;
	int mode;
	uint32_t import_id;
	uint32_t target;
	uint32_t question_id;
	uint64_t interface_id;
	uint16_t method_id;
	int datasz;
	int ptrs;
	uint16_t *path;
	int npath;
	struct capn *msg;
	capn_ptr params;
	/* Caps the fill function registered, in cap-table order. The
	 * outgoing call rebuilds its table from these; a local dispatch
	 * swaps hosted fields so param_cap reads them. */
	int ncaps;
	struct capn_rpc_pending_cap caps[CAPN_RPC_MAX_PENDING_CAPS];
	/* Arrival order. A later call may reuse a lower slot. */
	uint64_t seq;
};

/* A senderLoopback Disembargo that arrived while its target was still
 * a promise. Echoed after the queued calls, not before. */
struct capn_rpc_echo {
	int used;
	uint32_t export_id;
	uint8_t *frame;
	size_t len;
	/* Arrival order. A later disembargo may reuse a lower slot. */
	uint64_t seq;
};

/* A call forwarded to a resolved import, so its Return can be rewritten
 * onto the question the original caller is waiting on. */
struct capn_rpc_relay {
	int used;
	uint32_t local_qid;
	uint32_t remote_qid;
};

/* A call dispatch has not answered yet, with the message its results are
 * being built in. */
struct capn_rpc_pending_call {
	int used;
	/* Finish arrived before the server answered. The message stays
	 * until return so a fill of `results` is not a use after free.
	 * return then sends nothing. */
	int cancelled;
	uint32_t question_id;
	/* Distinguishes a question id the peer reused after Finish. */
	uint32_t generation;
	struct capn *msg;
	capn_ptr results;
	/* The caller is on this connection. The answer fills that question
	 * and is not sent to the peer. */
	int local_question;
	/* Caps this answer will carry. They stay here until return, so a
	 * later call cannot overwrite them or drop the export refs. A cap
	 * built in this message after dispatch returned CAPN_RPC_PENDING is
	 * appended here too. */
	struct capn_rpc_pending_cap caps[CAPN_RPC_MAX_PENDING_CAPS];
	int ncaps;
	/* The peer asked us to keep the results; a later Return names them
	 * with takeFromOtherQuestion. */
	int redirect;
	/* senderHosted ids from the params, released when the call ends.
	 * Owned by the pending call. */
	uint32_t *param_imports;
	int nparam_imports;
};

struct capn_rpc_conn {
	struct capn_rpc_export *exports;
	int exports_cap;
	struct capn_rpc_answer *answers;
	int answers_cap;
	struct capn_rpc_question *questions;
	int questions_cap;
	/* Shared with this vat's other connections when one is attached;
	 * otherwise `own_vat` below. */
	struct capn_rpc_vat *vat;
	struct capn_rpc_vat own_vat;
	struct capn_rpc_introduction introductions[CAPN_RPC_MAX_INTRODUCTIONS];
	uint32_t next_question_id;
	struct capn_rpc_join joins[CAPN_RPC_MAX_JOINS];
	/* Capability returned for `Bootstrap`; NULL answers with an exception. */
	void *bootstrap;
	capn_rpc_dispatch_fn bootstrap_dispatch;
	capn_rpc_send_fn send;
	void *send_ctx;
	/* Set once the peer sent Abort, or this side called capn_rpc_send_abort.
	 * A connection in this state accepts no more messages. */
	int aborted;
	/* Size of the results struct handed to dispatch, in bytes of data and
	 * pointer slots. capn_rpc_init sets 8 and 1. */
	int results_datasz;
	int results_ptrs;
	struct capn_rpc_pending_cap pending_caps[CAPN_RPC_MAX_PENDING_CAPS];
	int npending;
	/* While a fill callback or dispatch runs, new caps are recorded
	 * here instead of `pending_caps`. NULL outside those callbacks. */
	struct capn_rpc_pending_cap *cap_dest;
	int *cap_dest_n;
	/* Param imports the running dispatch looked up with
	 * capn_rpc_param_cap. The application owns those references. */
	uint32_t **param_taken;
	int *param_taken_n;
	/* The capTable of the call being dispatched, valid during dispatch. */
	CapDescriptor_list in_caps;
	struct capn_rpc_pending_call pending_calls[CAPN_RPC_MAX_PENDING_CALLS];
	uint32_t current_question;
	/* Generation of the call being dispatched. 0 outside dispatch. */
	uint32_t current_generation;
	uint32_t answer_clock;
	struct capn_rpc_import *imports;
	int imports_cap;
	struct capn_rpc_queued queued[CAPN_RPC_MAX_QUEUED];
	/* Next arrival stamp for `queued`. Starts at 0; the first frame is 1. */
	uint64_t queue_clock;
	struct capn_rpc_held held[CAPN_RPC_MAX_HELD];
	struct capn_rpc_relay relays[CAPN_RPC_MAX_RELAYS];
	uint32_t next_embargo;
	struct capn_rpc_echo echoes[CAPN_RPC_MAX_ECHOES];
};

/* Zero the tables and install the transmit callback. */
CAPN_EXPORT void capn_rpc_init(struct capn_rpc_conn *c, capn_rpc_send_fn send,
                   void *send_ctx);

/* Release the frames a connection still holds for pipelining and for
 * unclaimed answers. Call it when the connection is finished; the
 * connection must not be used afterwards without capn_rpc_init. */
CAPN_EXPORT void capn_rpc_free(struct capn_rpc_conn *c);

/* Deferred replies. A dispatch function that cannot answer yet returns
 * CAPN_RPC_PENDING after noting capn_rpc_current_question() and
 * capn_rpc_current_generation(), and keeping the `results` pointer.
 * Later it fills `results` and calls capn_rpc_return, or
 * capn_rpc_return_exception, passing that same id and generation.
 * `params` points into the incoming message and is gone once dispatch
 * returns, so copy what is needed first. If the peer sends Finish in
 * the meantime the call is cancelled: filling `results` is still
 * allowed, and return sends nothing and reports -1. A later call that
 * reuses the question id has a new generation, so the old return cannot
 * complete it. */
CAPN_EXPORT uint32_t capn_rpc_current_question(const struct capn_rpc_conn *c);
CAPN_EXPORT uint32_t capn_rpc_current_generation(const struct capn_rpc_conn *c);
/* Results of a deferred call, or a null pointer when that question and
 * generation are not waiting. Valid until capn_rpc_return, including
 * after Finish. */
CAPN_EXPORT capn_ptr capn_rpc_pending_results(struct capn_rpc_conn *c, uint32_t question_id,
                                  uint32_t generation);
CAPN_EXPORT int capn_rpc_return(struct capn_rpc_conn *c, uint32_t question_id,
                    uint32_t generation);
CAPN_EXPORT int capn_rpc_return_exception(struct capn_rpc_conn *c, uint32_t question_id,
                              uint32_t generation, const char *reason);

/* Size the results struct dispatch receives. It must cover the largest
 * results of any method this connection serves; the generated
 * `<Interface>_<method>_method` descriptors give the sizes per method. */
CAPN_EXPORT void capn_rpc_set_result_size(struct capn_rpc_conn *c, int datasz, int ptrs);

/* Share level 3 arrangements with this vat's other connections. Call
 * after capn_rpc_init and before any Provide; `vat` must outlive the
 * connection. */
CAPN_EXPORT void capn_rpc_set_vat(struct capn_rpc_conn *c, struct capn_rpc_vat *vat);

/* Ask the peer to hold `imported_cap` for a third vat, and return the
 * question id, which is also what a later Disembargo.provide names. The
 * nonce is the whole of the arrangement: the recipient presents it in an
 * Accept, and the host matches on it alone. */
CAPN_EXPORT uint32_t capn_rpc_send_provide(struct capn_rpc_conn *c, uint32_t imported_cap,
                               const char *recipient_host, uint16_t recipient_port,
                               uint64_t nonce);

/* Claim a capability a third vat provided for us. Returns the question
 * id; the answer carries the capability. */
CAPN_EXPORT uint32_t capn_rpc_send_accept(struct capn_rpc_conn *c, uint64_t nonce, int embargo);

/* Lift the embargo on the Accept this vat arranged with Provide. */
CAPN_EXPORT int capn_rpc_send_disembargo_provide(struct capn_rpc_conn *c, uint32_t provide_qid);

/* Tell the peer this connection is being torn down because of `reason`,
 * and refuse further messages on it. Returns the send callback's result. */
CAPN_EXPORT int capn_rpc_send_abort(struct capn_rpc_conn *c, const char *reason);

/* 1 once the connection has aborted, from either side. */
CAPN_EXPORT int capn_rpc_is_aborted(const struct capn_rpc_conn *c);

/* Set the capability answered to `Bootstrap`. */
CAPN_EXPORT void capn_rpc_set_bootstrap(struct capn_rpc_conn *c, void *server,
                            capn_rpc_dispatch_fn dispatch);

/* Export a capability, or return the id it already holds. -1 when the
 * table is full. */
CAPN_EXPORT int capn_rpc_export(struct capn_rpc_conn *c, void *server,
                    capn_rpc_dispatch_fn dispatch);

/* Export a promise. Calls are held until capn_rpc_resolve,
 * capn_rpc_resolve_import, or capn_rpc_resolve_exception. The id is what
 * a senderPromise descriptor names. -1 when the table cannot grow. */
CAPN_EXPORT int capn_rpc_export_promise(struct capn_rpc_conn *c, void *server,
                                        capn_rpc_dispatch_fn dispatch);

/* Resolve `promise_id` to a capability this vat hosts. Queued calls are
 * delivered before the Resolve message, so their Returns precede it. */
CAPN_EXPORT int capn_rpc_resolve(struct capn_rpc_conn *c, int promise_id,
                                 void *server, capn_rpc_dispatch_fn dispatch);

/* Resolve `promise_id` to a capability the peer hosts. Queued calls are
 * forwarded to `import_id` before Resolve, which carries receiverHosted. */
CAPN_EXPORT int capn_rpc_resolve_import(struct capn_rpc_conn *c,
                                        int promise_id, uint32_t import_id);

/* Reject `promise_id`. Queued calls are answered with `reason`, then a
 * Resolve carrying that exception is sent. */
CAPN_EXPORT int capn_rpc_resolve_exception(struct capn_rpc_conn *c,
                                           int promise_id,
                                           const char *reason);

/* 1 when a Resolve has been applied to `import_id`. `*broken` is set when
 * the promise rejected. `*to_export` is 1 when `*target` is a local export,
 * 0 when it is an import id, and -1 when the target is a pipeline or absent. */
CAPN_EXPORT int capn_rpc_promise_state(const struct capn_rpc_conn *c,
                                       uint32_t import_id, int *broken,
                                       int *to_export, uint32_t *target);

/* Handle one framed message. Returns 0 when handled, non-zero on a
 * malformed frame. Replies go out through the send callback. */
CAPN_EXPORT int capn_rpc_handle(struct capn_rpc_conn *c, const uint8_t *data, size_t len);

/* --- client side -------------------------------------------------- */

/* Ask for the peer's bootstrap capability. Returns the questionId, or
 * (uint32_t)-1 when the question table is full. */
CAPN_EXPORT uint32_t capn_rpc_send_bootstrap(struct capn_rpc_conn *c);

/* Write a call's parameter struct. */
typedef void (*capn_rpc_fill_fn)(void *ctx, capn_ptr params);

/* Call a method on an imported capability. `fill` may be NULL. Returns
 * the questionId, or (uint32_t)-1 on failure. */
/* `params_datasz` is in bytes and `params_ptrs` in pointer slots: the
 * caller knows its own method signature, and a struct sized here rather
 * than guessed silently drops any field past the end. */
CAPN_EXPORT uint32_t capn_rpc_send_call(struct capn_rpc_conn *c, uint32_t imported_cap,
                            uint64_t interface_id, uint16_t method_id,
                            int params_datasz, int params_ptrs,
                            capn_rpc_fill_fn fill, void *fill_ctx);

/* Call a method on a capability the answer to `question_id` will carry,
 * before that answer has arrived (promise pipelining). `path` is the
 * list of pointer indices to follow into the answer's results, as
 * PromisedAnswer.transform. Returns the questionId or (uint32_t)-1. */
CAPN_EXPORT uint32_t capn_rpc_send_pipelined_call(struct capn_rpc_conn *c, uint32_t question_id,
                                      const uint16_t *path, int npath,
                                      uint64_t interface_id, uint16_t method_id,
                                      int params_datasz, int params_ptrs,
                                      capn_rpc_fill_fn fill, void *fill_ctx);

/* Capabilities in payloads. Inside a fill callback (params) or dispatch
 * (results), these make a capability pointer to store in a pointer slot
 * with capn_setp (or in a list), and the payload's capTable is written
 * for you. `new_export_cap` hands the peer one of this vat's objects;
 * `new_import_cap` hands back a capability the peer hosts. Both return a
 * null pointer when the payload already holds CAPN_RPC_MAX_PENDING_CAPS.
 *
 * The cap is recorded on the payload being built: the fill or dispatch
 * that is running, or, when dispatch has already returned
 * CAPN_RPC_PENDING, the parked call whose message contains `seg`. A cap
 * built in any other message uses the connection pending list, which
 * the next outgoing payload consumes. */
CAPN_EXPORT capn_ptr capn_rpc_new_export_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 void *server, capn_rpc_dispatch_fn dispatch);
CAPN_EXPORT capn_ptr capn_rpc_new_import_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 uint32_t import_id);
/* senderPromise: the peer must call this id before it resolves. */
CAPN_EXPORT capn_ptr capn_rpc_new_promise_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                  void *server, capn_rpc_dispatch_fn dispatch);
/* receiverAnswer: the capability is the result of a question this vat
 * asked the peer. `path` is the PromisedAnswer transform. */
CAPN_EXPORT capn_ptr capn_rpc_new_answer_cap(struct capn_rpc_conn *c, struct capn_segment *seg,
                                 uint32_t question_id, const uint16_t *path,
                                 int npath);

/* What a capability pointer received in params (during dispatch) or in an
 * answer names. Returns the wire id, which is a uint32, or -1. A value
 * at or above 2^31 is not folded into a negative int. `*is_export` is 0
 * for a capability the peer hosts, whose id is what capn_rpc_send_call
 * takes as `imported_cap`, and 1 for one of this vat's own exports
 * passed back. */
/* A senderHosted import looked up during dispatch is the application's
 * reference: release it with capn_rpc_send_release when done. The
 * other senderHosted params are released when the call ends. */
CAPN_EXPORT int64_t capn_rpc_param_cap(struct capn_rpc_conn *c, capn_ptr cap, int *is_export);
CAPN_EXPORT int64_t capn_rpc_answer_cap(struct capn_rpc_conn *c, uint32_t question_id, capn_ptr cap,
                        int *is_export);

/* Tell the peer we are done with an answer, and drop our copy. No
 * Finish goes out when the Return said noFinishNeeded. */
CAPN_EXPORT int capn_rpc_send_finish(struct capn_rpc_conn *c, uint32_t question_id);

/* Drop `count` references to an import. */
CAPN_EXPORT int capn_rpc_send_release(struct capn_rpc_conn *c, uint32_t import_id,
                          uint32_t count);

/* 1 once the Return for `question_id` has arrived. */
CAPN_EXPORT int capn_rpc_is_answered(struct capn_rpc_conn *c, uint32_t question_id);

/* 1 when that Return carried an exception. */
CAPN_EXPORT int capn_rpc_is_failed(struct capn_rpc_conn *c, uint32_t question_id);

/* Results of an answered question. Returns 0 and fills `msg_out` / `out`
 * on success; non-zero while the question is outstanding or if it
 * failed. The caller frees `msg_out` with capn_free. */
/* The import id of a capability an answer returned, or -1 when the
 * answer carries none. The id is the wire uint32, not a truncated int.
 * A returned capability arrives as a pointer into the answer's capTable;
 * calling it needs the id the descriptor beside it names, which is what
 * this reads. */
CAPN_EXPORT int64_t capn_rpc_answer_cap_id(struct capn_rpc_conn *c, uint32_t question_id);

CAPN_EXPORT int capn_rpc_answer_content(struct capn_rpc_conn *c, uint32_t question_id,
                            struct capn *msg_out, capn_ptr *out);

/* Nonces of capabilities held for a third vat, for tests and shutdown
 * accounting. Writes up to `cap` entries and returns how many exist. */
/* Introductions handed to us and not yet picked up. Copies up to `max`
 * into `out` (may be NULL) and returns how many are held. */
CAPN_EXPORT int capn_rpc_pending_introductions(struct capn_rpc_conn *c,
                                   struct capn_rpc_introduction *out, int max);

/* Finish the pickup for `nonce`: releases the vine, which the sender
 * treats as the signal to close its Provide. Call this only once the
 * third party has actually handed the capability over; releasing early
 * drops the fallback path with nothing in its place. Returns 0 on
 * success, -1 if no such introduction is held. */
CAPN_EXPORT int capn_rpc_introduction_done(struct capn_rpc_conn *c, uint64_t nonce);

/* Write a `thirdPartyHosted` descriptor into `cd`: where the recipient
 * should go, which pending Provide to claim once there, and the vine we
 * export so calls work in the meantime. */
CAPN_EXPORT int capn_rpc_write_third_party_cap(struct capn_rpc_conn *c, CapDescriptor_ptr cd,
                                   const char *host, uint16_t port,
                                   uint64_t nonce, uint32_t vine_id);

/* Accepts claimed but still embargoed, awaiting Disembargo.provide. */
CAPN_EXPORT int capn_rpc_embargoed_accepts(struct capn_rpc_conn *c);

CAPN_EXPORT int capn_rpc_pending_provisions(struct capn_rpc_conn *c, uint64_t *out,
                                int cap);

/* 1 when the provision for `nonce` stored a recipient, 0 when the
 * slot is in use and names none, -1 when no such provision exists.
 * handle_accept matches the nonce and does not call this. */
CAPN_EXPORT int capn_rpc_provision_recipient(struct capn_rpc_conn *c, uint64_t nonce,
                                char *host, size_t hostcap, uint16_t *port);

/* --- blocking file descriptor transport -----------------------------------
 *
 * The vat owns no transport, but a stream socket or pipe needs only these:
 * pass capn_rpc_fd_send with `send_ctx` pointing at an int holding the
 * descriptor, and call capn_rpc_fd_pump whenever it is readable. They
 * block, and exist only on Unix-like systems. */

/* A capn_rpc_send_fn that writes the frame to the descriptor *ctx. */
CAPN_EXPORT int capn_rpc_fd_send(void *ctx, const uint8_t *data, size_t len);

/* Read one stream-framed message into a malloc'd buffer. Returns 0, 1 on
 * a clean end of stream before any byte, or -1 on error or a message
 * over 64 MiB. */
CAPN_EXPORT int capn_rpc_fd_read_frame(int fd, uint8_t **frame, size_t *len);

/* Read one frame and handle it. Returns 0, 1 at end of stream, or -1. */
CAPN_EXPORT int capn_rpc_fd_pump(struct capn_rpc_conn *c, int fd);

/* --- stream flow control ------------------------------------------- */

CAPN_EXPORT void capn_rpc_stream_init(struct capn_rpc_stream *s, int window);

/* Send one stream call, blocking only when the window is full. Returns 0
 * on success, non-zero once the stream has failed. */
CAPN_EXPORT int capn_rpc_stream_send(struct capn_rpc_conn *c, struct capn_rpc_stream *s,
                         uint32_t imported_cap, uint64_t interface_id,
                         uint16_t method_id, int params_datasz,
                         int params_ptrs, capn_rpc_fill_fn fill,
                         void *fill_ctx);

/* Wait for every outstanding call. Returns 0 when all succeeded. */
CAPN_EXPORT int capn_rpc_stream_finish(struct capn_rpc_conn *c,
                           struct capn_rpc_stream *s);


#ifdef __cplusplus
}
#endif

#endif /* CAPN_RPC_H */
