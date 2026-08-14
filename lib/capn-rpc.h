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

#ifdef __cplusplus
extern "C" {
#endif

#define CAPN_RPC_MAX_EXPORTS 64
#define CAPN_RPC_MAX_ANSWERS 64
#define CAPN_RPC_MAX_ANSWER_BYTES 8192
#define CAPN_RPC_MAX_JOINS 8
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

#ifdef __cplusplus
}
#endif

#endif /* CAPN_RPC_H */
