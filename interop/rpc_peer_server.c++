// Reference RPC peer: capnp-C++ serving CountLeaf over rpc-twoparty on
// 127.0.0.1:<argv[1]>. CountLeaf extends CountMid, which extends CountBase,
// and also extends Adder. A call may carry any of those interface ids.
#include "adder.capnp.h"
#include <capnp/ez-rpc.h>
#include <kj/async.h>
#include <string>

class CountLeafImpl final : public CountLeaf::Server {
public:
	kj::Promise<void> add(AddContext ctx) override {
		auto p = ctx.getParams();
		ctx.getResults().setSum(p.getA() + p.getB());
		return kj::READY_NOW;
	}

	kj::Promise<void> tag(TagContext ctx) override {
		auto p = ctx.getParams();
		auto r = ctx.getResults();
		r.setN(p.getN());
		r.setLabel(p.getLabel());
		return kj::READY_NOW;
	}

	kj::Promise<void> inc(IncContext ctx) override {
		auto p = ctx.getParams();
		ctx.getResults().setN(p.getN() + 1);
		return kj::READY_NOW;
	}

	kj::Promise<void> twice(TwiceContext ctx) override {
		auto p = ctx.getParams();
		ctx.getResults().setN(p.getN() * 2);
		return kj::READY_NOW;
	}

	kj::Promise<void> dec(DecContext ctx) override {
		auto p = ctx.getParams();
		ctx.getResults().setN(p.getN() - 1);
		return kj::READY_NOW;
	}
};

int main(int argc, char **argv) {
	std::string port = argc > 1 ? argv[1] : "43117";
	capnp::EzRpcServer server(kj::heap<CountLeafImpl>(), "127.0.0.1:" + port);
	auto &waitScope = server.getWaitScope();
	kj::NEVER_DONE.wait(waitScope);
}
