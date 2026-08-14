#ifndef CAPN_B312981B2552A250
#define CAPN_B312981B2552A250
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>

#if CAPN_VERSION != 1
#error "version mismatch between capnp_c.h and generated code"
#endif

#ifndef capnp_nowarn
# ifdef __GNUC__
#  define capnp_nowarn __extension__
# else
#  define capnp_nowarn
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct Message;
struct Bootstrap;
struct Call;
struct Return;
struct Finish;
struct Resolve;
struct Release;
struct Disembargo;
struct Provide;
struct Accept;
struct Join;
struct MessageTarget;
struct Payload;
struct CapDescriptor;
struct PromisedAnswer;
struct PromisedAnswer_Op;
struct ThirdPartyCapDescriptor;
struct Exception;

typedef struct {capn_ptr p;} Message_ptr;
typedef struct {capn_ptr p;} Bootstrap_ptr;
typedef struct {capn_ptr p;} Call_ptr;
typedef struct {capn_ptr p;} Return_ptr;
typedef struct {capn_ptr p;} Finish_ptr;
typedef struct {capn_ptr p;} Resolve_ptr;
typedef struct {capn_ptr p;} Release_ptr;
typedef struct {capn_ptr p;} Disembargo_ptr;
typedef struct {capn_ptr p;} Provide_ptr;
typedef struct {capn_ptr p;} Accept_ptr;
typedef struct {capn_ptr p;} Join_ptr;
typedef struct {capn_ptr p;} MessageTarget_ptr;
typedef struct {capn_ptr p;} Payload_ptr;
typedef struct {capn_ptr p;} CapDescriptor_ptr;
typedef struct {capn_ptr p;} PromisedAnswer_ptr;
typedef struct {capn_ptr p;} PromisedAnswer_Op_ptr;
typedef struct {capn_ptr p;} ThirdPartyCapDescriptor_ptr;
typedef struct {capn_ptr p;} Exception_ptr;

typedef struct {capn_ptr p;} Message_list;
typedef struct {capn_ptr p;} Bootstrap_list;
typedef struct {capn_ptr p;} Call_list;
typedef struct {capn_ptr p;} Return_list;
typedef struct {capn_ptr p;} Finish_list;
typedef struct {capn_ptr p;} Resolve_list;
typedef struct {capn_ptr p;} Release_list;
typedef struct {capn_ptr p;} Disembargo_list;
typedef struct {capn_ptr p;} Provide_list;
typedef struct {capn_ptr p;} Accept_list;
typedef struct {capn_ptr p;} Join_list;
typedef struct {capn_ptr p;} MessageTarget_list;
typedef struct {capn_ptr p;} Payload_list;
typedef struct {capn_ptr p;} CapDescriptor_list;
typedef struct {capn_ptr p;} PromisedAnswer_list;
typedef struct {capn_ptr p;} PromisedAnswer_Op_list;
typedef struct {capn_ptr p;} ThirdPartyCapDescriptor_list;
typedef struct {capn_ptr p;} Exception_list;

enum Exception_Type {
	Exception_Type_failed = 0,
	Exception_Type_overloaded = 1,
	Exception_Type_disconnected = 2,
	Exception_Type_unimplemented = 3
};
enum Message_which {
	Message_unimplemented = 0,
	Message_abort = 1,
	Message_bootstrap = 8,
	Message_call = 2,
	Message__return = 3,
	Message_finish = 4,
	Message_resolve = 5,
	Message_release = 6,
	Message_disembargo = 13,
	Message_obsoleteSave = 7,
	Message_obsoleteDelete = 9,
	Message_provide = 10,
	Message_accept = 11,
	Message_join = 12
};

struct Message {
	enum Message_which which;
	capnp_nowarn union {
		Message_ptr unimplemented;
		Exception_ptr abort;
		Bootstrap_ptr bootstrap;
		Call_ptr call;
		Return_ptr _return;
		Finish_ptr finish;
		Resolve_ptr resolve;
		Release_ptr release;
		Disembargo_ptr disembargo;
		capn_ptr obsoleteSave;
		capn_ptr obsoleteDelete;
		Provide_ptr provide;
		Accept_ptr accept;
		Join_ptr join;
	};
};

static const size_t Message_word_count = 1;

static const size_t Message_pointer_count = 1;

static const size_t Message_struct_bytes_count = 16;


struct Bootstrap {
	uint32_t questionId;
	capn_ptr deprecatedObjectId;
};

static const size_t Bootstrap_word_count = 1;

static const size_t Bootstrap_pointer_count = 1;

static const size_t Bootstrap_struct_bytes_count = 16;

enum Call_sendResultsTo_which {
	Call_sendResultsTo_caller = 0,
	Call_sendResultsTo_yourself = 1,
	Call_sendResultsTo_thirdParty = 2
};

struct Call {
	uint32_t questionId;
	MessageTarget_ptr target;
	uint64_t interfaceId;
	uint16_t methodId;
	unsigned allowThirdPartyTailCall : 1;
	unsigned noPromisePipelining : 1;
	unsigned onlyPromisePipeline : 1;
	Payload_ptr params;
	enum Call_sendResultsTo_which sendResultsTo_which;
	capnp_nowarn union {
		capn_ptr thirdParty;
	} sendResultsTo;
};

static const size_t Call_word_count = 3;

static const size_t Call_pointer_count = 3;

static const size_t Call_struct_bytes_count = 48;

enum Return_which {
	Return_results = 0,
	Return_exception = 1,
	Return_canceled = 2,
	Return_resultsSentElsewhere = 3,
	Return_takeFromOtherQuestion = 4,
	Return_acceptFromThirdParty = 5
};

struct Return {
	uint32_t answerId;
	unsigned releaseParamCaps : 1;
	unsigned noFinishNeeded : 1;
	enum Return_which which;
	capnp_nowarn union {
		Payload_ptr results;
		Exception_ptr exception;
		uint32_t takeFromOtherQuestion;
		capn_ptr acceptFromThirdParty;
	};
};

static const size_t Return_word_count = 2;

static const size_t Return_pointer_count = 1;

static const size_t Return_struct_bytes_count = 24;


struct Finish {
	uint32_t questionId;
	unsigned releaseResultCaps : 1;
	unsigned requireEarlyCancellationWorkaround : 1;
};

static const size_t Finish_word_count = 1;

static const size_t Finish_pointer_count = 0;

static const size_t Finish_struct_bytes_count = 8;

enum Resolve_which {
	Resolve_cap = 0,
	Resolve_exception = 1
};

struct Resolve {
	uint32_t promiseId;
	enum Resolve_which which;
	capnp_nowarn union {
		CapDescriptor_ptr cap;
		Exception_ptr exception;
	};
};

static const size_t Resolve_word_count = 1;

static const size_t Resolve_pointer_count = 1;

static const size_t Resolve_struct_bytes_count = 16;


struct Release {
	uint32_t id;
	uint32_t referenceCount;
};

static const size_t Release_word_count = 1;

static const size_t Release_pointer_count = 0;

static const size_t Release_struct_bytes_count = 8;

enum Disembargo_context_which {
	Disembargo_context_senderLoopback = 0,
	Disembargo_context_receiverLoopback = 1,
	Disembargo_context_accept = 2,
	Disembargo_context_provide = 3
};

struct Disembargo {
	MessageTarget_ptr target;
	enum Disembargo_context_which context_which;
	capnp_nowarn union {
		uint32_t senderLoopback;
		uint32_t receiverLoopback;
		uint32_t provide;
	} context;
};

static const size_t Disembargo_word_count = 1;

static const size_t Disembargo_pointer_count = 1;

static const size_t Disembargo_struct_bytes_count = 16;


struct Provide {
	uint32_t questionId;
	MessageTarget_ptr target;
	capn_ptr recipient;
};

static const size_t Provide_word_count = 1;

static const size_t Provide_pointer_count = 2;

static const size_t Provide_struct_bytes_count = 24;


struct Accept {
	uint32_t questionId;
	capn_ptr provision;
	unsigned embargo : 1;
};

static const size_t Accept_word_count = 1;

static const size_t Accept_pointer_count = 1;

static const size_t Accept_struct_bytes_count = 16;


struct Join {
	uint32_t questionId;
	MessageTarget_ptr target;
	capn_ptr keyPart;
};

static const size_t Join_word_count = 1;

static const size_t Join_pointer_count = 2;

static const size_t Join_struct_bytes_count = 24;

enum MessageTarget_which {
	MessageTarget_importedCap = 0,
	MessageTarget_promisedAnswer = 1
};

struct MessageTarget {
	enum MessageTarget_which which;
	capnp_nowarn union {
		uint32_t importedCap;
		PromisedAnswer_ptr promisedAnswer;
	};
};

static const size_t MessageTarget_word_count = 1;

static const size_t MessageTarget_pointer_count = 1;

static const size_t MessageTarget_struct_bytes_count = 16;


struct Payload {
	capn_ptr content;
	CapDescriptor_list capTable;
};

static const size_t Payload_word_count = 0;

static const size_t Payload_pointer_count = 2;

static const size_t Payload_struct_bytes_count = 16;

enum CapDescriptor_which {
	CapDescriptor_none = 0,
	CapDescriptor_senderHosted = 1,
	CapDescriptor_senderPromise = 2,
	CapDescriptor_receiverHosted = 3,
	CapDescriptor_receiverAnswer = 4,
	CapDescriptor_thirdPartyHosted = 5
};

struct CapDescriptor {
	enum CapDescriptor_which which;
	capnp_nowarn union {
		uint32_t senderHosted;
		uint32_t senderPromise;
		uint32_t receiverHosted;
		PromisedAnswer_ptr receiverAnswer;
		ThirdPartyCapDescriptor_ptr thirdPartyHosted;
	};
	uint8_t attachedFd;
};

static const size_t CapDescriptor_word_count = 1;

static const size_t CapDescriptor_pointer_count = 1;

static const size_t CapDescriptor_struct_bytes_count = 16;


struct PromisedAnswer {
	uint32_t questionId;
	PromisedAnswer_Op_list transform;
};

static const size_t PromisedAnswer_word_count = 1;

static const size_t PromisedAnswer_pointer_count = 1;

static const size_t PromisedAnswer_struct_bytes_count = 16;

enum PromisedAnswer_Op_which {
	PromisedAnswer_Op_noop = 0,
	PromisedAnswer_Op_getPointerField = 1
};

struct PromisedAnswer_Op {
	enum PromisedAnswer_Op_which which;
	capnp_nowarn union {
		uint16_t getPointerField;
	};
};

static const size_t PromisedAnswer_Op_word_count = 1;

static const size_t PromisedAnswer_Op_pointer_count = 0;

static const size_t PromisedAnswer_Op_struct_bytes_count = 8;


struct ThirdPartyCapDescriptor {
	capn_ptr id;
	uint32_t vineId;
};

static const size_t ThirdPartyCapDescriptor_word_count = 1;

static const size_t ThirdPartyCapDescriptor_pointer_count = 1;

static const size_t ThirdPartyCapDescriptor_struct_bytes_count = 16;


struct Exception {
	capn_text reason;
	enum Exception_Type type;
	unsigned obsoleteIsCallersFault : 1;
	uint16_t obsoleteDurability;
	capn_text trace;
};

static const size_t Exception_word_count = 1;

static const size_t Exception_pointer_count = 2;

static const size_t Exception_struct_bytes_count = 24;


Message_ptr new_Message(struct capn_segment*);
Bootstrap_ptr new_Bootstrap(struct capn_segment*);
Call_ptr new_Call(struct capn_segment*);
Return_ptr new_Return(struct capn_segment*);
Finish_ptr new_Finish(struct capn_segment*);
Resolve_ptr new_Resolve(struct capn_segment*);
Release_ptr new_Release(struct capn_segment*);
Disembargo_ptr new_Disembargo(struct capn_segment*);
Provide_ptr new_Provide(struct capn_segment*);
Accept_ptr new_Accept(struct capn_segment*);
Join_ptr new_Join(struct capn_segment*);
MessageTarget_ptr new_MessageTarget(struct capn_segment*);
Payload_ptr new_Payload(struct capn_segment*);
CapDescriptor_ptr new_CapDescriptor(struct capn_segment*);
PromisedAnswer_ptr new_PromisedAnswer(struct capn_segment*);
PromisedAnswer_Op_ptr new_PromisedAnswer_Op(struct capn_segment*);
ThirdPartyCapDescriptor_ptr new_ThirdPartyCapDescriptor(struct capn_segment*);
Exception_ptr new_Exception(struct capn_segment*);

Message_list new_Message_list(struct capn_segment*, int len);
Bootstrap_list new_Bootstrap_list(struct capn_segment*, int len);
Call_list new_Call_list(struct capn_segment*, int len);
Return_list new_Return_list(struct capn_segment*, int len);
Finish_list new_Finish_list(struct capn_segment*, int len);
Resolve_list new_Resolve_list(struct capn_segment*, int len);
Release_list new_Release_list(struct capn_segment*, int len);
Disembargo_list new_Disembargo_list(struct capn_segment*, int len);
Provide_list new_Provide_list(struct capn_segment*, int len);
Accept_list new_Accept_list(struct capn_segment*, int len);
Join_list new_Join_list(struct capn_segment*, int len);
MessageTarget_list new_MessageTarget_list(struct capn_segment*, int len);
Payload_list new_Payload_list(struct capn_segment*, int len);
CapDescriptor_list new_CapDescriptor_list(struct capn_segment*, int len);
PromisedAnswer_list new_PromisedAnswer_list(struct capn_segment*, int len);
PromisedAnswer_Op_list new_PromisedAnswer_Op_list(struct capn_segment*, int len);
ThirdPartyCapDescriptor_list new_ThirdPartyCapDescriptor_list(struct capn_segment*, int len);
Exception_list new_Exception_list(struct capn_segment*, int len);

void read_Message(struct Message*, Message_ptr);
void read_Bootstrap(struct Bootstrap*, Bootstrap_ptr);
void read_Call(struct Call*, Call_ptr);
void read_Return(struct Return*, Return_ptr);
void read_Finish(struct Finish*, Finish_ptr);
void read_Resolve(struct Resolve*, Resolve_ptr);
void read_Release(struct Release*, Release_ptr);
void read_Disembargo(struct Disembargo*, Disembargo_ptr);
void read_Provide(struct Provide*, Provide_ptr);
void read_Accept(struct Accept*, Accept_ptr);
void read_Join(struct Join*, Join_ptr);
void read_MessageTarget(struct MessageTarget*, MessageTarget_ptr);
void read_Payload(struct Payload*, Payload_ptr);
void read_CapDescriptor(struct CapDescriptor*, CapDescriptor_ptr);
void read_PromisedAnswer(struct PromisedAnswer*, PromisedAnswer_ptr);
void read_PromisedAnswer_Op(struct PromisedAnswer_Op*, PromisedAnswer_Op_ptr);
void read_ThirdPartyCapDescriptor(struct ThirdPartyCapDescriptor*, ThirdPartyCapDescriptor_ptr);
void read_Exception(struct Exception*, Exception_ptr);

void write_Message(const struct Message*, Message_ptr);
void write_Bootstrap(const struct Bootstrap*, Bootstrap_ptr);
void write_Call(const struct Call*, Call_ptr);
void write_Return(const struct Return*, Return_ptr);
void write_Finish(const struct Finish*, Finish_ptr);
void write_Resolve(const struct Resolve*, Resolve_ptr);
void write_Release(const struct Release*, Release_ptr);
void write_Disembargo(const struct Disembargo*, Disembargo_ptr);
void write_Provide(const struct Provide*, Provide_ptr);
void write_Accept(const struct Accept*, Accept_ptr);
void write_Join(const struct Join*, Join_ptr);
void write_MessageTarget(const struct MessageTarget*, MessageTarget_ptr);
void write_Payload(const struct Payload*, Payload_ptr);
void write_CapDescriptor(const struct CapDescriptor*, CapDescriptor_ptr);
void write_PromisedAnswer(const struct PromisedAnswer*, PromisedAnswer_ptr);
void write_PromisedAnswer_Op(const struct PromisedAnswer_Op*, PromisedAnswer_Op_ptr);
void write_ThirdPartyCapDescriptor(const struct ThirdPartyCapDescriptor*, ThirdPartyCapDescriptor_ptr);
void write_Exception(const struct Exception*, Exception_ptr);

void get_Message(struct Message*, Message_list, int i);
void get_Bootstrap(struct Bootstrap*, Bootstrap_list, int i);
void get_Call(struct Call*, Call_list, int i);
void get_Return(struct Return*, Return_list, int i);
void get_Finish(struct Finish*, Finish_list, int i);
void get_Resolve(struct Resolve*, Resolve_list, int i);
void get_Release(struct Release*, Release_list, int i);
void get_Disembargo(struct Disembargo*, Disembargo_list, int i);
void get_Provide(struct Provide*, Provide_list, int i);
void get_Accept(struct Accept*, Accept_list, int i);
void get_Join(struct Join*, Join_list, int i);
void get_MessageTarget(struct MessageTarget*, MessageTarget_list, int i);
void get_Payload(struct Payload*, Payload_list, int i);
void get_CapDescriptor(struct CapDescriptor*, CapDescriptor_list, int i);
void get_PromisedAnswer(struct PromisedAnswer*, PromisedAnswer_list, int i);
void get_PromisedAnswer_Op(struct PromisedAnswer_Op*, PromisedAnswer_Op_list, int i);
void get_ThirdPartyCapDescriptor(struct ThirdPartyCapDescriptor*, ThirdPartyCapDescriptor_list, int i);
void get_Exception(struct Exception*, Exception_list, int i);

void set_Message(const struct Message*, Message_list, int i);
void set_Bootstrap(const struct Bootstrap*, Bootstrap_list, int i);
void set_Call(const struct Call*, Call_list, int i);
void set_Return(const struct Return*, Return_list, int i);
void set_Finish(const struct Finish*, Finish_list, int i);
void set_Resolve(const struct Resolve*, Resolve_list, int i);
void set_Release(const struct Release*, Release_list, int i);
void set_Disembargo(const struct Disembargo*, Disembargo_list, int i);
void set_Provide(const struct Provide*, Provide_list, int i);
void set_Accept(const struct Accept*, Accept_list, int i);
void set_Join(const struct Join*, Join_list, int i);
void set_MessageTarget(const struct MessageTarget*, MessageTarget_list, int i);
void set_Payload(const struct Payload*, Payload_list, int i);
void set_CapDescriptor(const struct CapDescriptor*, CapDescriptor_list, int i);
void set_PromisedAnswer(const struct PromisedAnswer*, PromisedAnswer_list, int i);
void set_PromisedAnswer_Op(const struct PromisedAnswer_Op*, PromisedAnswer_Op_list, int i);
void set_ThirdPartyCapDescriptor(const struct ThirdPartyCapDescriptor*, ThirdPartyCapDescriptor_list, int i);
void set_Exception(const struct Exception*, Exception_list, int i);

#ifdef __cplusplus
}
#endif
#endif
