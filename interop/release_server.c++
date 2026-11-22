// Reference peer for capability release: capnp-C++ serving an Adder over
// rpc-twoparty on 127.0.0.1:<argv[1]>. borrow() returns a new Adder, and
// tag() reports in `n` how many of those are still alive, so a C client
// can see whether rpc.c++ dropped the caps it did not keep.
#include "adder.capnp.h"
#include <capnp/ez-rpc.h>
#include <kj/async.h>
#include <cstdio>
#include <string>

static int g_live;

class Borrowed final : public Adder::Server {
public:
	Borrowed() { g_live++; }
	~Borrowed() { g_live--; }

	kj::Promise<void> add(AddContext ctx) override {
		auto p = ctx.getParams();
		ctx.getResults().setSum(p.getA() + p.getB());
		return kj::READY_NOW;
	}
};

class Lender final : public Adder::Server {
public:
	kj::Promise<void> add(AddContext ctx) override {
		auto p = ctx.getParams();
		ctx.getResults().setSum(p.getA() + p.getB());
		return kj::READY_NOW;
	}

	kj::Promise<void> tag(TagContext ctx) override {
		ctx.getResults().setN(g_live);
		return kj::READY_NOW;
	}

	kj::Promise<void> borrow(BorrowContext ctx) override {
		auto r = ctx.getResults();
		r.setN(ctx.getParams().getN());
		r.setCap(kj::heap<Borrowed>());
		return kj::READY_NOW;
	}
};

// `client <port>` drives interop/release_c_server.c instead. Each borrow()
// is dropped before its Return, which the C vat delays, so rpc.c++ sends
// Finish with releaseResultCaps and ignores the caps in the Return. The C vat must
// then drop the export itself, senderHosted (odd n) or senderPromise.
static int run_client(const std::string &port) {
	capnp::EzRpcClient client("127.0.0.1:" + port);
	Adder::Client lender = client.getMain<Adder>();
	auto &waitScope = client.getWaitScope();
	auto live = [&]() { return lender.tagRequest().send().wait(waitScope).getN(); };

	if (live() != 0) {
		std::fprintf(stderr, "borrowed refs before any borrow()\n");
		return 1;
	}
	for (int n = 1; n <= 4; n++) {
		{
			auto req = lender.borrowRequest();
			req.setN(n);
			auto dropped = req.send();
			// A call dropped in the turn that made it never leaves
			// rpc.c++, so let one turn write it first.
			waitScope.poll();
		}
		int got = live();
		if (got != 0) {
			std::fprintf(stderr, "cancelled borrow(%d): %d %s refs left\n", n, got,
			             n % 2 ? "senderHosted" : "senderPromise");
			return 1;
		}
	}
	{
		auto req = lender.borrowRequest();
		req.setN(2);
		auto kept = req.send().wait(waitScope).getCap();
		if (live() != 1) {
			std::fprintf(stderr, "a read senderPromise result was dropped\n");
			return 1;
		}
	}
	if (live() != 0) {
		std::fprintf(stderr, "Release of a senderPromise result left a ref\n");
		return 1;
	}
	std::printf("C++ client release interop passed.\n");
	return 0;
}

int main(int argc, char **argv) {
	if (argc > 2 && std::string(argv[1]) == "client")
		return run_client(argv[2]);
	std::string port = argc > 1 ? argv[1] : "43117";
	capnp::EzRpcServer server(kj::heap<Lender>(), "127.0.0.1:" + port);
	auto &waitScope = server.getWaitScope();
	kj::NEVER_DONE.wait(waitScope);
}
