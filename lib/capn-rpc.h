/* Two-party Cap'n Proto RPC vat.
 *
 * Answers the level 1 messages a peer sends to a capability this vat
 * hosts, plus level 4 `Join`. Level 3 is absent by construction rather
 * than by omission: `Provide` and `Accept` introduce a capability to a
 * third vat, and a two-party connection has no way to name one --
 * rpc-twoparty.capnp declares `ThirdPartyCapId` and `RecipientId` empty,
 * "never used, because there is no third party".
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

#define CAPN_RPC_MAX_EXPORTS 64
#define CAPN_RPC_MAX_ANSWERS 64
#define CAPN_RPC_MAX_ANSWER_BYTES 8192
#define CAPN_RPC_MAX_QUESTIONS 64
#define CAPN_RPC_MAX_PROVISIONS 32
#define CAPN_RPC_MAX_INTRODUCTIONS 8
/* Long enough for a hostname; a longer one is refused, not truncated. */
#define CAPN_RPC_MAX_HOST 64
#define CAPN_RPC_MAX_JOINS 8
/* Outstanding unacknowledged calls a stream may hold. */
#define CAPN_RPC_STREAM_MAX_WINDOW 64
#define CAPN_RPC_MAX_JOIN_PARTS 16

struct capn_rpc_conn;

/* Handle one call on an exported capability.
 *
 * `params` is the resolved parameter struct; write the reply into
 * `results`, a struct already allocated in the reply message. Return 0
 * to answer with those results, non-zero to answer with an exception.
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
	uint8_t frame[CAPN_RPC_MAX_ANSWER_BYTES];
	size_t len;
};

/* A question this vat asked, from send until its Return arrives. */
struct capn_rpc_question {
	int used;
	uint32_t question_id;
	int answered;
	int failed;
	uint8_t reply[CAPN_RPC_MAX_ANSWER_BYTES];
	size_t reply_len;
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

struct capn_rpc_conn {
	struct capn_rpc_export exports[CAPN_RPC_MAX_EXPORTS];
	struct capn_rpc_answer answers[CAPN_RPC_MAX_ANSWERS];
	struct capn_rpc_question questions[CAPN_RPC_MAX_QUESTIONS];
	struct capn_rpc_provision provisions[CAPN_RPC_MAX_PROVISIONS];
	struct capn_rpc_introduction introductions[CAPN_RPC_MAX_INTRODUCTIONS];
	uint32_t next_question_id;
	struct capn_rpc_join joins[CAPN_RPC_MAX_JOINS];
	/* Capability returned for `Bootstrap`; NULL answers with an exception. */
	void *bootstrap;
	capn_rpc_dispatch_fn bootstrap_dispatch;
	capn_rpc_send_fn send;
	void *send_ctx;
};

/* Zero the tables and install the transmit callback. */
void capn_rpc_init(struct capn_rpc_conn *c, capn_rpc_send_fn send,
                   void *send_ctx);

/* Set the capability answered to `Bootstrap`. */
void capn_rpc_set_bootstrap(struct capn_rpc_conn *c, void *server,
                            capn_rpc_dispatch_fn dispatch);

/* Export a capability, or return the id it already holds. -1 when the
 * table is full. */
int capn_rpc_export(struct capn_rpc_conn *c, void *server,
                    capn_rpc_dispatch_fn dispatch);

/* Handle one framed message. Returns 0 when handled, non-zero on a
 * malformed frame. Replies go out through the send callback. */
int capn_rpc_handle(struct capn_rpc_conn *c, const uint8_t *data, size_t len);

/* --- client side -------------------------------------------------- */

/* Ask for the peer's bootstrap capability. Returns the questionId, or
 * (uint32_t)-1 when the question table is full. */
uint32_t capn_rpc_send_bootstrap(struct capn_rpc_conn *c);

/* Write a call's parameter struct. */
typedef void (*capn_rpc_fill_fn)(void *ctx, capn_ptr params);

/* Call a method on an imported capability. `fill` may be NULL. Returns
 * the questionId, or (uint32_t)-1 on failure. */
/* `params_datasz` is in bytes and `params_ptrs` in pointer slots: the
 * caller knows its own method signature, and a struct sized here rather
 * than guessed silently drops any field past the end. */
uint32_t capn_rpc_send_call(struct capn_rpc_conn *c, uint32_t imported_cap,
                            uint64_t interface_id, uint16_t method_id,
                            int params_datasz, int params_ptrs,
                            capn_rpc_fill_fn fill, void *fill_ctx);

/* Tell the peer we are done with an answer, and drop our copy. */
int capn_rpc_send_finish(struct capn_rpc_conn *c, uint32_t question_id);

/* Drop `count` references to an import. */
int capn_rpc_send_release(struct capn_rpc_conn *c, uint32_t import_id,
                          uint32_t count);

/* 1 once the Return for `question_id` has arrived. */
int capn_rpc_is_answered(struct capn_rpc_conn *c, uint32_t question_id);

/* 1 when that Return carried an exception. */
int capn_rpc_is_failed(struct capn_rpc_conn *c, uint32_t question_id);

/* Results of an answered question. Returns 0 and fills `msg_out` / `out`
 * on success; non-zero while the question is outstanding or if it
 * failed. The caller frees `msg_out` with capn_free. */
int capn_rpc_answer_content(struct capn_rpc_conn *c, uint32_t question_id,
                            struct capn *msg_out, capn_ptr *out);

/* Nonces of capabilities held for a third vat, for tests and shutdown
 * accounting. Writes up to `cap` entries and returns how many exist. */
/* Introductions handed to us and not yet picked up. Copies up to `max`
 * into `out` (may be NULL) and returns how many are held. */
int capn_rpc_pending_introductions(struct capn_rpc_conn *c,
                                   struct capn_rpc_introduction *out, int max);

/* Finish the pickup for `nonce`: releases the vine, which the sender
 * treats as the signal to close its Provide. Call this only once the
 * third party has actually handed the capability over; releasing early
 * drops the fallback path with nothing in its place. Returns 0 on
 * success, -1 if no such introduction is held. */
int capn_rpc_introduction_done(struct capn_rpc_conn *c, uint64_t nonce);

/* Write a `thirdPartyHosted` descriptor into `cd`: where the recipient
 * should go, which pending Provide to claim once there, and the vine we
 * export so calls work in the meantime. */
int capn_rpc_write_third_party_cap(struct capn_rpc_conn *c, CapDescriptor_ptr cd,
                                   const char *host, uint16_t port,
                                   uint64_t nonce, uint32_t vine_id);

/* Accepts claimed but still embargoed, awaiting Disembargo.provide. */
int capn_rpc_embargoed_accepts(struct capn_rpc_conn *c);

int capn_rpc_pending_provisions(struct capn_rpc_conn *c, uint64_t *out,
                                int cap);

/* --- stream flow control ------------------------------------------- */

void capn_rpc_stream_init(struct capn_rpc_stream *s, int window);

/* Send one stream call, blocking only when the window is full. Returns 0
 * on success, non-zero once the stream has failed. */
int capn_rpc_stream_send(struct capn_rpc_conn *c, struct capn_rpc_stream *s,
                         uint32_t imported_cap, uint64_t interface_id,
                         uint16_t method_id, int params_datasz,
                         int params_ptrs, capn_rpc_fill_fn fill,
                         void *fill_ctx);

/* Wait for every outstanding call. Returns 0 when all succeeded. */
int capn_rpc_stream_finish(struct capn_rpc_conn *c,
                           struct capn_rpc_stream *s);


#ifdef __cplusplus
}
#endif

#endif /* CAPN_RPC_H */
