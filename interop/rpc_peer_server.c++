// Reference RPC peer: capnp-C++ serving CountLeaf over rpc-twoparty on
// 127.0.0.1:<argv[1]>. CountLeaf extends CountMid, which extends CountBase,
// and also extends Adder. A call may carry any of those interface ids.
// `client <port>` connects to a C vat instead and pipelines add() through
// the promise defer() returns.
#include "adder.capnp.h"
#include <capnp/ez-rpc.h>
#include <kj/async.h>
#include <kj/memory.h>
#include <cstdio>
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

	kj::Promise<void> defer(DeferContext ctx) override {
		auto paf = kj::newPromiseAndFulfiller<Adder::Client>();
		fulfiller = kj::mv(paf.fulfiller);
		ctx.getResults().setCap(kj::mv(paf.promise));
		return kj::READY_NOW;
	}

	kj::Promise<void> fulfill(FulfillContext) override {
		if (fulfiller.get() == nullptr)
			return KJ_EXCEPTION(FAILED, "no deferred adder");
		if (have_watch)
			fulfiller->fulfill(kj::mv(watched));
		else
			fulfiller->fulfill(kj::heap<CountLeafImpl>());
		fulfiller = nullptr;
		have_watch = false;
		return kj::READY_NOW;
	}

	kj::Promise<void> watch(WatchContext ctx) override {
		watched = ctx.getParams().getCap();
		have_watch = true;
		return kj::READY_NOW;
	}

private:
	kj::Own<kj::PromiseFulfiller<Adder::Client>> fulfiller;
	Adder::Client watched = nullptr;
	bool have_watch = false;
};

/* A capability the C++ client hosts. The C vat reflects pipelined calls
 * back to it, and echo() returns the cap it was given so the caller can
 * check the cap table survived the trip. seq() records call order. */
class EchoSeq final : public Adder::Server {
public:
	kj::Promise<void> echo(EchoContext ctx) override {
		ctx.getResults().setCap(ctx.getParams().getCap());
		echoes++;
		return kj::READY_NOW;
	}

	kj::Promise<void> seq(SeqContext ctx) override {
		int64_t n = ctx.getParams().getN();
		if (n != expect)
			bad = 1;
		ctx.getResults().setPrev(last);
		last = n;
		expect = n + 1;
		return kj::READY_NOW;
	}

	int echoes = 0;
	int bad = 0;
	int64_t last = 0;
	int64_t expect = 1;
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

	/* Pass a capability we host, pipeline echo() and seq(1) on the
	 * promise before fulfill(), then seq(2) after the promise comes
	 * back to us. seq(2) has to wait for the disembargo echo, and the
	 * cap echo() returns has to be the one we passed in. */
	auto owned = kj::heap<EchoSeq>();
	EchoSeq *raw = owned.get();
	Adder::Client mine(kj::mv(owned));

	auto watch = adder.watchRequest();
	watch.setCap(mine);
	watch.send().wait(wait);

	auto deferred2 = adder.deferRequest().send();
	auto echoReq = deferred2.getCap().echoRequest();
	echoReq.setCap(mine);
	auto echoed = echoReq.send();
	auto seq1req = deferred2.getCap().seqRequest();
	seq1req.setN(1);
	auto seq1 = seq1req.send();
	deferred2.wait(wait);
	adder.fulfillRequest().send().wait(wait);

	auto seq2req = deferred2.getCap().seqRequest();
	seq2req.setN(2);
	auto seq2 = seq2req.send();

	int64_t prev1 = seq1.wait(wait).getPrev();
	int64_t prev2 = seq2.wait(wait).getPrev();
	auto returned = echoed.wait(wait).getCap();
	auto seq3req = returned.seqRequest();
	seq3req.setN(3);
	int64_t prev3 = seq3req.send().wait(wait).getPrev();
	if (raw->bad || raw->echoes != 1 || prev1 != 0 || prev2 != 1 || prev3 != 2) {
		std::fprintf(stderr,
		             "embargo order bad=%d echoes=%d prev=%lld,%lld,%lld\n",
		             raw->bad, raw->echoes,
		             static_cast<long long>(prev1),
		             static_cast<long long>(prev2),
		             static_cast<long long>(prev3));
		return 1;
	}
	std::printf("C++ client embargo interop passed.\n");
	return 0;
}

int main(int argc, char **argv) {
	if (argc > 2 && std::string(argv[1]) == "client")
		return run_client(argv[2]);
	std::string port = argc > 1 ? argv[1] : "43117";
	capnp::EzRpcServer server(kj::heap<CountLeafImpl>(), "127.0.0.1:" + port);
	auto &waitScope = server.getWaitScope();
	kj::NEVER_DONE.wait(waitScope);
}
