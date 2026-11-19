// Reference RPC peer: capnp-C++ serving Adder over rpc-twoparty on
// 127.0.0.1:<port>. `client <port>` connects to a C vat instead and
// pipelines add() through the promise defer() returns.
#include "adder.capnp.h"
#include <capnp/ez-rpc.h>
#include <kj/async.h>
#include <kj/memory.h>
#include <cstdio>
#include <string>

class AdderImpl final : public Adder::Server {
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

	kj::Promise<void> defer(DeferContext ctx) override {
		auto paf = kj::newPromiseAndFulfiller<Adder::Client>();
		fulfiller = kj::mv(paf.fulfiller);
		ctx.getResults().setCap(kj::mv(paf.promise));
		return kj::READY_NOW;
	}

	kj::Promise<void> fulfill(FulfillContext) override {
		if (fulfiller.get() == nullptr)
			return KJ_EXCEPTION(FAILED, "no deferred adder");
		fulfiller->fulfill(kj::heap<AdderImpl>());
		fulfiller = nullptr;
		return kj::READY_NOW;
	}

private:
	kj::Own<kj::PromiseFulfiller<Adder::Client>> fulfiller;
};

static int run_client(const std::string &port) {
	capnp::EzRpcClient client("127.0.0.1:" + port);
	Adder::Client adder = client.getMain<Adder>();
	auto &wait = client.getWaitScope();

	/* Pipeline add() onto the promise before fulfill(), so the call is
	 * in flight while the C vat is still holding the promise. */
	auto deferred = adder.deferRequest().send();
	auto addReq = deferred.getCap().addRequest();
	addReq.setA(7);
	addReq.setB(8);
	auto sumPromise = addReq.send();
	deferred.wait(wait);
	adder.fulfillRequest().send().wait(wait);
	int64_t sum = sumPromise.wait(wait).getSum();
	if (sum != 15) {
		std::fprintf(stderr, "pipelined add returned %lld, expected 15\n",
		             static_cast<long long>(sum));
		return 1;
	}
	std::printf("C++ client promise interop passed.\n");
	return 0;
}

int main(int argc, char **argv) {
	if (argc > 2 && std::string(argv[1]) == "client")
		return run_client(argv[2]);
	std::string port = argc > 1 ? argv[1] : "43117";
	capnp::EzRpcServer server(kj::heap<AdderImpl>(), "127.0.0.1:" + port);
	auto &waitScope = server.getWaitScope();
	kj::NEVER_DONE.wait(waitScope);
}
