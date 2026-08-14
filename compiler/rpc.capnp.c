#include "rpc.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

static const capn_text capn_val0 = {0,"",0};
static const capn_ptr capn_null = {CAPN_NULL};

Message_ptr new_Message(struct capn_segment *s) {
	Message_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Message_list new_Message_list(struct capn_segment *s, int len) {
	Message_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Message(struct Message *s capnp_unused, Message_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->which = (enum Message_which)(int) capn_read16(p.p, 0);
	switch (s->which) {
	case Message_unimplemented:
		s->unimplemented.p = capn_getp(p.p, 0, 0);
		break;
	case Message_abort:
		s->abort.p = capn_getp(p.p, 0, 0);
		break;
	case Message_bootstrap:
		s->bootstrap.p = capn_getp(p.p, 0, 0);
		break;
	case Message_call:
		s->call.p = capn_getp(p.p, 0, 0);
		break;
	case Message__return:
		s->_return.p = capn_getp(p.p, 0, 0);
		break;
	case Message_finish:
		s->finish.p = capn_getp(p.p, 0, 0);
		break;
	case Message_resolve:
		s->resolve.p = capn_getp(p.p, 0, 0);
		break;
	case Message_release:
		s->release.p = capn_getp(p.p, 0, 0);
		break;
	case Message_disembargo:
		s->disembargo.p = capn_getp(p.p, 0, 0);
		break;
	case Message_obsoleteSave:
		s->obsoleteSave = capn_getp(p.p, 0, 0);
		break;
	case Message_obsoleteDelete:
		s->obsoleteDelete = capn_getp(p.p, 0, 0);
		break;
	case Message_provide:
		s->provide.p = capn_getp(p.p, 0, 0);
		break;
	case Message_accept:
		s->accept.p = capn_getp(p.p, 0, 0);
		break;
	case Message_join:
		s->join.p = capn_getp(p.p, 0, 0);
		break;
	default:
		break;
	}
}
void write_Message(const struct Message *s capnp_unused, Message_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, s->which);
	switch (s->which) {
	case Message_unimplemented:
		capn_setp(p.p, 0, (s->unimplemented.p.type != CAPN_NULL) ? s->unimplemented.p : capn_null);
		break;
	case Message_abort:
		capn_setp(p.p, 0, (s->abort.p.type != CAPN_NULL) ? s->abort.p : capn_null);
		break;
	case Message_bootstrap:
		capn_setp(p.p, 0, (s->bootstrap.p.type != CAPN_NULL) ? s->bootstrap.p : capn_null);
		break;
	case Message_call:
		capn_setp(p.p, 0, (s->call.p.type != CAPN_NULL) ? s->call.p : capn_null);
		break;
	case Message__return:
		capn_setp(p.p, 0, (s->_return.p.type != CAPN_NULL) ? s->_return.p : capn_null);
		break;
	case Message_finish:
		capn_setp(p.p, 0, (s->finish.p.type != CAPN_NULL) ? s->finish.p : capn_null);
		break;
	case Message_resolve:
		capn_setp(p.p, 0, (s->resolve.p.type != CAPN_NULL) ? s->resolve.p : capn_null);
		break;
	case Message_release:
		capn_setp(p.p, 0, (s->release.p.type != CAPN_NULL) ? s->release.p : capn_null);
		break;
	case Message_disembargo:
		capn_setp(p.p, 0, (s->disembargo.p.type != CAPN_NULL) ? s->disembargo.p : capn_null);
		break;
	case Message_obsoleteSave:
		capn_setp(p.p, 0, (s->obsoleteSave.type != CAPN_NULL) ? s->obsoleteSave : capn_null);
		break;
	case Message_obsoleteDelete:
		capn_setp(p.p, 0, (s->obsoleteDelete.type != CAPN_NULL) ? s->obsoleteDelete : capn_null);
		break;
	case Message_provide:
		capn_setp(p.p, 0, (s->provide.p.type != CAPN_NULL) ? s->provide.p : capn_null);
		break;
	case Message_accept:
		capn_setp(p.p, 0, (s->accept.p.type != CAPN_NULL) ? s->accept.p : capn_null);
		break;
	case Message_join:
		capn_setp(p.p, 0, (s->join.p.type != CAPN_NULL) ? s->join.p : capn_null);
		break;
	default:
		break;
	}
}
void get_Message(struct Message *s, Message_list l, int i) {
	Message_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Message(s, p);
}
void set_Message(const struct Message *s, Message_list l, int i) {
	Message_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Message(s, p);
}

Bootstrap_ptr new_Bootstrap(struct capn_segment *s) {
	Bootstrap_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Bootstrap_list new_Bootstrap_list(struct capn_segment *s, int len) {
	Bootstrap_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Bootstrap(struct Bootstrap *s capnp_unused, Bootstrap_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->questionId = capn_read32(p.p, 0);
	s->deprecatedObjectId = capn_getp(p.p, 0, 0);
}
void write_Bootstrap(const struct Bootstrap *s capnp_unused, Bootstrap_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->questionId);
	capn_setp(p.p, 0, (s->deprecatedObjectId.type != CAPN_NULL) ? s->deprecatedObjectId : capn_null);
}
void get_Bootstrap(struct Bootstrap *s, Bootstrap_list l, int i) {
	Bootstrap_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Bootstrap(s, p);
}
void set_Bootstrap(const struct Bootstrap *s, Bootstrap_list l, int i) {
	Bootstrap_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Bootstrap(s, p);
}

Call_ptr new_Call(struct capn_segment *s) {
	Call_ptr p;
	p.p = capn_new_struct(s, 24, 3);
	return p;
}
Call_list new_Call_list(struct capn_segment *s, int len) {
	Call_list p;
	p.p = capn_new_struct_list(s, len, 24, 3);
	return p;
}
void read_Call(struct Call *s capnp_unused, Call_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->questionId = capn_read32(p.p, 0);
	s->target.p = capn_getp(p.p, 0, 0);
	s->interfaceId = capn_read64(p.p, 8);
	s->methodId = capn_read16(p.p, 4);
	s->allowThirdPartyTailCall = (capn_read8(p.p, 16) & 1) != 0;
	s->noPromisePipelining = (capn_read8(p.p, 16) & 2) != 0;
	s->onlyPromisePipeline = (capn_read8(p.p, 16) & 4) != 0;
	s->params.p = capn_getp(p.p, 1, 0);
	s->sendResultsTo_which = (enum Call_sendResultsTo_which)(int) capn_read16(p.p, 6);
	switch (s->sendResultsTo_which) {
	case Call_sendResultsTo_thirdParty:
		s->sendResultsTo.thirdParty = capn_getp(p.p, 2, 0);
		break;
	default:
		break;
	}
}
void write_Call(const struct Call *s capnp_unused, Call_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->questionId);
	capn_setp(p.p, 0, (s->target.p.type != CAPN_NULL) ? s->target.p : capn_null);
	capn_write64(p.p, 8, s->interfaceId);
	capn_write16(p.p, 4, s->methodId);
	capn_write1(p.p, 128, s->allowThirdPartyTailCall != 0);
	capn_write1(p.p, 129, s->noPromisePipelining != 0);
	capn_write1(p.p, 130, s->onlyPromisePipeline != 0);
	capn_setp(p.p, 1, (s->params.p.type != CAPN_NULL) ? s->params.p : capn_null);
	capn_write16(p.p, 6, s->sendResultsTo_which);
	switch (s->sendResultsTo_which) {
	case Call_sendResultsTo_thirdParty:
		capn_setp(p.p, 2, (s->sendResultsTo.thirdParty.type != CAPN_NULL) ? s->sendResultsTo.thirdParty : capn_null);
		break;
	default:
		break;
	}
}
void get_Call(struct Call *s, Call_list l, int i) {
	Call_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Call(s, p);
}
void set_Call(const struct Call *s, Call_list l, int i) {
	Call_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Call(s, p);
}

Return_ptr new_Return(struct capn_segment *s) {
	Return_ptr p;
	p.p = capn_new_struct(s, 16, 1);
	return p;
}
Return_list new_Return_list(struct capn_segment *s, int len) {
	Return_list p;
	p.p = capn_new_struct_list(s, len, 16, 1);
	return p;
}
void read_Return(struct Return *s capnp_unused, Return_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->answerId = capn_read32(p.p, 0);
	s->releaseParamCaps = (capn_read8(p.p, 4) & 1) != 1;
	s->noFinishNeeded = (capn_read8(p.p, 4) & 2) != 0;
	s->which = (enum Return_which)(int) capn_read16(p.p, 6);
	switch (s->which) {
	case Return_takeFromOtherQuestion:
		s->takeFromOtherQuestion = capn_read32(p.p, 8);
		break;
	case Return_results:
		s->results.p = capn_getp(p.p, 0, 0);
		break;
	case Return_exception:
		s->exception.p = capn_getp(p.p, 0, 0);
		break;
	case Return_acceptFromThirdParty:
		s->acceptFromThirdParty = capn_getp(p.p, 0, 0);
		break;
	default:
		break;
	}
}
void write_Return(const struct Return *s capnp_unused, Return_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->answerId);
	capn_write1(p.p, 32, s->releaseParamCaps != 1);
	capn_write1(p.p, 33, s->noFinishNeeded != 0);
	capn_write16(p.p, 6, s->which);
	switch (s->which) {
	case Return_takeFromOtherQuestion:
		capn_write32(p.p, 8, s->takeFromOtherQuestion);
		break;
	case Return_results:
		capn_setp(p.p, 0, (s->results.p.type != CAPN_NULL) ? s->results.p : capn_null);
		break;
	case Return_exception:
		capn_setp(p.p, 0, (s->exception.p.type != CAPN_NULL) ? s->exception.p : capn_null);
		break;
	case Return_acceptFromThirdParty:
		capn_setp(p.p, 0, (s->acceptFromThirdParty.type != CAPN_NULL) ? s->acceptFromThirdParty : capn_null);
		break;
	default:
		break;
	}
}
void get_Return(struct Return *s, Return_list l, int i) {
	Return_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Return(s, p);
}
void set_Return(const struct Return *s, Return_list l, int i) {
	Return_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Return(s, p);
}

Finish_ptr new_Finish(struct capn_segment *s) {
	Finish_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
Finish_list new_Finish_list(struct capn_segment *s, int len) {
	Finish_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_Finish(struct Finish *s capnp_unused, Finish_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->questionId = capn_read32(p.p, 0);
	s->releaseResultCaps = (capn_read8(p.p, 4) & 1) != 1;
	s->requireEarlyCancellationWorkaround = (capn_read8(p.p, 4) & 2) != 2;
}
void write_Finish(const struct Finish *s capnp_unused, Finish_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->questionId);
	capn_write1(p.p, 32, s->releaseResultCaps != 1);
	capn_write1(p.p, 33, s->requireEarlyCancellationWorkaround != 1);
}
void get_Finish(struct Finish *s, Finish_list l, int i) {
	Finish_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Finish(s, p);
}
void set_Finish(const struct Finish *s, Finish_list l, int i) {
	Finish_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Finish(s, p);
}

Resolve_ptr new_Resolve(struct capn_segment *s) {
	Resolve_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Resolve_list new_Resolve_list(struct capn_segment *s, int len) {
	Resolve_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Resolve(struct Resolve *s capnp_unused, Resolve_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->promiseId = capn_read32(p.p, 0);
	s->which = (enum Resolve_which)(int) capn_read16(p.p, 4);
	switch (s->which) {
	case Resolve_cap:
		s->cap.p = capn_getp(p.p, 0, 0);
		break;
	case Resolve_exception:
		s->exception.p = capn_getp(p.p, 0, 0);
		break;
	default:
		break;
	}
}
void write_Resolve(const struct Resolve *s capnp_unused, Resolve_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->promiseId);
	capn_write16(p.p, 4, s->which);
	switch (s->which) {
	case Resolve_cap:
		capn_setp(p.p, 0, (s->cap.p.type != CAPN_NULL) ? s->cap.p : capn_null);
		break;
	case Resolve_exception:
		capn_setp(p.p, 0, (s->exception.p.type != CAPN_NULL) ? s->exception.p : capn_null);
		break;
	default:
		break;
	}
}
void get_Resolve(struct Resolve *s, Resolve_list l, int i) {
	Resolve_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Resolve(s, p);
}
void set_Resolve(const struct Resolve *s, Resolve_list l, int i) {
	Resolve_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Resolve(s, p);
}

Release_ptr new_Release(struct capn_segment *s) {
	Release_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
Release_list new_Release_list(struct capn_segment *s, int len) {
	Release_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_Release(struct Release *s capnp_unused, Release_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->id = capn_read32(p.p, 0);
	s->referenceCount = capn_read32(p.p, 4);
}
void write_Release(const struct Release *s capnp_unused, Release_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->id);
	capn_write32(p.p, 4, s->referenceCount);
}
void get_Release(struct Release *s, Release_list l, int i) {
	Release_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Release(s, p);
}
void set_Release(const struct Release *s, Release_list l, int i) {
	Release_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Release(s, p);
}

Disembargo_ptr new_Disembargo(struct capn_segment *s) {
	Disembargo_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Disembargo_list new_Disembargo_list(struct capn_segment *s, int len) {
	Disembargo_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Disembargo(struct Disembargo *s capnp_unused, Disembargo_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->target.p = capn_getp(p.p, 0, 0);
	s->context_which = (enum Disembargo_context_which)(int) capn_read16(p.p, 4);
	switch (s->context_which) {
	case Disembargo_context_senderLoopback:
		s->context.senderLoopback = capn_read32(p.p, 0);
		break;
	case Disembargo_context_receiverLoopback:
		s->context.receiverLoopback = capn_read32(p.p, 0);
		break;
	case Disembargo_context_provide:
		s->context.provide = capn_read32(p.p, 0);
		break;
	default:
		break;
	}
}
void write_Disembargo(const struct Disembargo *s capnp_unused, Disembargo_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->target.p.type != CAPN_NULL) ? s->target.p : capn_null);
	capn_write16(p.p, 4, s->context_which);
	switch (s->context_which) {
	case Disembargo_context_senderLoopback:
		capn_write32(p.p, 0, s->context.senderLoopback);
		break;
	case Disembargo_context_receiverLoopback:
		capn_write32(p.p, 0, s->context.receiverLoopback);
		break;
	case Disembargo_context_provide:
		capn_write32(p.p, 0, s->context.provide);
		break;
	default:
		break;
	}
}
void get_Disembargo(struct Disembargo *s, Disembargo_list l, int i) {
	Disembargo_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Disembargo(s, p);
}
void set_Disembargo(const struct Disembargo *s, Disembargo_list l, int i) {
	Disembargo_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Disembargo(s, p);
}

Provide_ptr new_Provide(struct capn_segment *s) {
	Provide_ptr p;
	p.p = capn_new_struct(s, 8, 2);
	return p;
}
Provide_list new_Provide_list(struct capn_segment *s, int len) {
	Provide_list p;
	p.p = capn_new_struct_list(s, len, 8, 2);
	return p;
}
void read_Provide(struct Provide *s capnp_unused, Provide_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->questionId = capn_read32(p.p, 0);
	s->target.p = capn_getp(p.p, 0, 0);
	s->recipient = capn_getp(p.p, 1, 0);
}
void write_Provide(const struct Provide *s capnp_unused, Provide_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->questionId);
	capn_setp(p.p, 0, (s->target.p.type != CAPN_NULL) ? s->target.p : capn_null);
	capn_setp(p.p, 1, (s->recipient.type != CAPN_NULL) ? s->recipient : capn_null);
}
void get_Provide(struct Provide *s, Provide_list l, int i) {
	Provide_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Provide(s, p);
}
void set_Provide(const struct Provide *s, Provide_list l, int i) {
	Provide_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Provide(s, p);
}

Accept_ptr new_Accept(struct capn_segment *s) {
	Accept_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Accept_list new_Accept_list(struct capn_segment *s, int len) {
	Accept_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Accept(struct Accept *s capnp_unused, Accept_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->questionId = capn_read32(p.p, 0);
	s->provision = capn_getp(p.p, 0, 0);
	s->embargo = (capn_read8(p.p, 4) & 1) != 0;
}
void write_Accept(const struct Accept *s capnp_unused, Accept_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->questionId);
	capn_setp(p.p, 0, (s->provision.type != CAPN_NULL) ? s->provision : capn_null);
	capn_write1(p.p, 32, s->embargo != 0);
}
void get_Accept(struct Accept *s, Accept_list l, int i) {
	Accept_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Accept(s, p);
}
void set_Accept(const struct Accept *s, Accept_list l, int i) {
	Accept_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Accept(s, p);
}

Join_ptr new_Join(struct capn_segment *s) {
	Join_ptr p;
	p.p = capn_new_struct(s, 8, 2);
	return p;
}
Join_list new_Join_list(struct capn_segment *s, int len) {
	Join_list p;
	p.p = capn_new_struct_list(s, len, 8, 2);
	return p;
}
void read_Join(struct Join *s capnp_unused, Join_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->questionId = capn_read32(p.p, 0);
	s->target.p = capn_getp(p.p, 0, 0);
	s->keyPart = capn_getp(p.p, 1, 0);
}
void write_Join(const struct Join *s capnp_unused, Join_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->questionId);
	capn_setp(p.p, 0, (s->target.p.type != CAPN_NULL) ? s->target.p : capn_null);
	capn_setp(p.p, 1, (s->keyPart.type != CAPN_NULL) ? s->keyPart : capn_null);
}
void get_Join(struct Join *s, Join_list l, int i) {
	Join_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Join(s, p);
}
void set_Join(const struct Join *s, Join_list l, int i) {
	Join_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Join(s, p);
}

MessageTarget_ptr new_MessageTarget(struct capn_segment *s) {
	MessageTarget_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
MessageTarget_list new_MessageTarget_list(struct capn_segment *s, int len) {
	MessageTarget_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_MessageTarget(struct MessageTarget *s capnp_unused, MessageTarget_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->which = (enum MessageTarget_which)(int) capn_read16(p.p, 4);
	switch (s->which) {
	case MessageTarget_importedCap:
		s->importedCap = capn_read32(p.p, 0);
		break;
	case MessageTarget_promisedAnswer:
		s->promisedAnswer.p = capn_getp(p.p, 0, 0);
		break;
	default:
		break;
	}
}
void write_MessageTarget(const struct MessageTarget *s capnp_unused, MessageTarget_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 4, s->which);
	switch (s->which) {
	case MessageTarget_importedCap:
		capn_write32(p.p, 0, s->importedCap);
		break;
	case MessageTarget_promisedAnswer:
		capn_setp(p.p, 0, (s->promisedAnswer.p.type != CAPN_NULL) ? s->promisedAnswer.p : capn_null);
		break;
	default:
		break;
	}
}
void get_MessageTarget(struct MessageTarget *s, MessageTarget_list l, int i) {
	MessageTarget_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_MessageTarget(s, p);
}
void set_MessageTarget(const struct MessageTarget *s, MessageTarget_list l, int i) {
	MessageTarget_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_MessageTarget(s, p);
}

Payload_ptr new_Payload(struct capn_segment *s) {
	Payload_ptr p;
	p.p = capn_new_struct(s, 0, 2);
	return p;
}
Payload_list new_Payload_list(struct capn_segment *s, int len) {
	Payload_list p;
	p.p = capn_new_struct_list(s, len, 0, 2);
	return p;
}
void read_Payload(struct Payload *s capnp_unused, Payload_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->content = capn_getp(p.p, 0, 0);
	s->capTable.p = capn_getp(p.p, 1, 0);
}
void write_Payload(const struct Payload *s capnp_unused, Payload_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->content.type != CAPN_NULL) ? s->content : capn_null);
	capn_setp(p.p, 1, (s->capTable.p.type != CAPN_NULL) ? s->capTable.p : capn_null);
}
void get_Payload(struct Payload *s, Payload_list l, int i) {
	Payload_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Payload(s, p);
}
void set_Payload(const struct Payload *s, Payload_list l, int i) {
	Payload_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Payload(s, p);
}

CapDescriptor_ptr new_CapDescriptor(struct capn_segment *s) {
	CapDescriptor_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
CapDescriptor_list new_CapDescriptor_list(struct capn_segment *s, int len) {
	CapDescriptor_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_CapDescriptor(struct CapDescriptor *s capnp_unused, CapDescriptor_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->which = (enum CapDescriptor_which)(int) capn_read16(p.p, 0);
	switch (s->which) {
	case CapDescriptor_senderHosted:
		s->senderHosted = capn_read32(p.p, 4);
		break;
	case CapDescriptor_senderPromise:
		s->senderPromise = capn_read32(p.p, 4);
		break;
	case CapDescriptor_receiverHosted:
		s->receiverHosted = capn_read32(p.p, 4);
		break;
	case CapDescriptor_receiverAnswer:
		s->receiverAnswer.p = capn_getp(p.p, 0, 0);
		break;
	case CapDescriptor_thirdPartyHosted:
		s->thirdPartyHosted.p = capn_getp(p.p, 0, 0);
		break;
	default:
		break;
	}
	s->attachedFd = capn_read8(p.p, 2) ^ 255u;
}
void write_CapDescriptor(const struct CapDescriptor *s capnp_unused, CapDescriptor_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, s->which);
	switch (s->which) {
	case CapDescriptor_senderHosted:
		capn_write32(p.p, 4, s->senderHosted);
		break;
	case CapDescriptor_senderPromise:
		capn_write32(p.p, 4, s->senderPromise);
		break;
	case CapDescriptor_receiverHosted:
		capn_write32(p.p, 4, s->receiverHosted);
		break;
	case CapDescriptor_receiverAnswer:
		capn_setp(p.p, 0, (s->receiverAnswer.p.type != CAPN_NULL) ? s->receiverAnswer.p : capn_null);
		break;
	case CapDescriptor_thirdPartyHosted:
		capn_setp(p.p, 0, (s->thirdPartyHosted.p.type != CAPN_NULL) ? s->thirdPartyHosted.p : capn_null);
		break;
	default:
		break;
	}
	capn_write8(p.p, 2, s->attachedFd ^ 255u);
}
void get_CapDescriptor(struct CapDescriptor *s, CapDescriptor_list l, int i) {
	CapDescriptor_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_CapDescriptor(s, p);
}
void set_CapDescriptor(const struct CapDescriptor *s, CapDescriptor_list l, int i) {
	CapDescriptor_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CapDescriptor(s, p);
}

PromisedAnswer_ptr new_PromisedAnswer(struct capn_segment *s) {
	PromisedAnswer_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
PromisedAnswer_list new_PromisedAnswer_list(struct capn_segment *s, int len) {
	PromisedAnswer_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_PromisedAnswer(struct PromisedAnswer *s capnp_unused, PromisedAnswer_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->questionId = capn_read32(p.p, 0);
	s->transform.p = capn_getp(p.p, 0, 0);
}
void write_PromisedAnswer(const struct PromisedAnswer *s capnp_unused, PromisedAnswer_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->questionId);
	capn_setp(p.p, 0, (s->transform.p.type != CAPN_NULL) ? s->transform.p : capn_null);
}
void get_PromisedAnswer(struct PromisedAnswer *s, PromisedAnswer_list l, int i) {
	PromisedAnswer_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_PromisedAnswer(s, p);
}
void set_PromisedAnswer(const struct PromisedAnswer *s, PromisedAnswer_list l, int i) {
	PromisedAnswer_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_PromisedAnswer(s, p);
}

PromisedAnswer_Op_ptr new_PromisedAnswer_Op(struct capn_segment *s) {
	PromisedAnswer_Op_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
PromisedAnswer_Op_list new_PromisedAnswer_Op_list(struct capn_segment *s, int len) {
	PromisedAnswer_Op_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_PromisedAnswer_Op(struct PromisedAnswer_Op *s capnp_unused, PromisedAnswer_Op_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->which = (enum PromisedAnswer_Op_which)(int) capn_read16(p.p, 0);
	switch (s->which) {
	case PromisedAnswer_Op_getPointerField:
		s->getPointerField = capn_read16(p.p, 2);
		break;
	default:
		break;
	}
}
void write_PromisedAnswer_Op(const struct PromisedAnswer_Op *s capnp_unused, PromisedAnswer_Op_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, s->which);
	switch (s->which) {
	case PromisedAnswer_Op_getPointerField:
		capn_write16(p.p, 2, s->getPointerField);
		break;
	default:
		break;
	}
}
void get_PromisedAnswer_Op(struct PromisedAnswer_Op *s, PromisedAnswer_Op_list l, int i) {
	PromisedAnswer_Op_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_PromisedAnswer_Op(s, p);
}
void set_PromisedAnswer_Op(const struct PromisedAnswer_Op *s, PromisedAnswer_Op_list l, int i) {
	PromisedAnswer_Op_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_PromisedAnswer_Op(s, p);
}

ThirdPartyCapDescriptor_ptr new_ThirdPartyCapDescriptor(struct capn_segment *s) {
	ThirdPartyCapDescriptor_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
ThirdPartyCapDescriptor_list new_ThirdPartyCapDescriptor_list(struct capn_segment *s, int len) {
	ThirdPartyCapDescriptor_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_ThirdPartyCapDescriptor(struct ThirdPartyCapDescriptor *s capnp_unused, ThirdPartyCapDescriptor_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->id = capn_getp(p.p, 0, 0);
	s->vineId = capn_read32(p.p, 0);
}
void write_ThirdPartyCapDescriptor(const struct ThirdPartyCapDescriptor *s capnp_unused, ThirdPartyCapDescriptor_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->id.type != CAPN_NULL) ? s->id : capn_null);
	capn_write32(p.p, 0, s->vineId);
}
void get_ThirdPartyCapDescriptor(struct ThirdPartyCapDescriptor *s, ThirdPartyCapDescriptor_list l, int i) {
	ThirdPartyCapDescriptor_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_ThirdPartyCapDescriptor(s, p);
}
void set_ThirdPartyCapDescriptor(const struct ThirdPartyCapDescriptor *s, ThirdPartyCapDescriptor_list l, int i) {
	ThirdPartyCapDescriptor_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_ThirdPartyCapDescriptor(s, p);
}

Exception_ptr new_Exception(struct capn_segment *s) {
	Exception_ptr p;
	p.p = capn_new_struct(s, 8, 2);
	return p;
}
Exception_list new_Exception_list(struct capn_segment *s, int len) {
	Exception_list p;
	p.p = capn_new_struct_list(s, len, 8, 2);
	return p;
}
void read_Exception(struct Exception *s capnp_unused, Exception_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->reason = capn_get_text(p.p, 0, capn_val0);
	s->type = (enum Exception_Type)(int) capn_read16(p.p, 4);
	s->obsoleteIsCallersFault = (capn_read8(p.p, 0) & 1) != 0;
	s->obsoleteDurability = capn_read16(p.p, 2);
	s->trace = capn_get_text(p.p, 1, capn_val0);
}
void write_Exception(const struct Exception *s capnp_unused, Exception_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, s->reason);
	capn_write16(p.p, 4, (uint16_t) (s->type));
	capn_write1(p.p, 0, s->obsoleteIsCallersFault != 0);
	capn_write16(p.p, 2, s->obsoleteDurability);
	capn_set_text(p.p, 1, s->trace);
}
void get_Exception(struct Exception *s, Exception_list l, int i) {
	Exception_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Exception(s, p);
}
void set_Exception(const struct Exception *s, Exception_list l, int i) {
	Exception_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Exception(s, p);
}
