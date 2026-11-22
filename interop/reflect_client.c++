// C++ client. Pipelines three calls on a capability the C server
// reflects, and checks that a parameter cap is destroyed after drop.

#include "mirror.capnp.h"

#include <capnp/ez-rpc.h>
#include <kj/debug.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace {

class Ordered final : public Seq::Server {
public:
	std::vector<int> seen;

	kj::Promise<void> next(NextContext ctx) override {
		int n = ctx.getParams().getN();
		seen.push_back(n);
		ctx.getResults().setN(n + 1);
		return kj::READY_NOW;
	}
};

class Dying final : public Seq::Server {
public:
	explicit Dying(bool *flag) : flag(flag) {}
	~Dying() noexcept(false) { if (flag) *flag = true; }

	kj::Promise<void> next(NextContext) override { return kj::READY_NOW; }

	bool *flag;
};

int fail(const char *what) {
	fprintf(stderr, "reflect: %s\n", what);
	return 1;
}

}

int main(int argc, char **argv) {
	std::string port = argc > 1 ? argv[1] : "43120";
	capnp::EzRpcClient client("127.0.0.1:" + port);
	auto &waitScope = client.getWaitScope();
	Mirror::Client mirror = client.getMain<Mirror>();

	{
		auto impl = kj::heap<Ordered>();
		Ordered *raw = impl.get();
		auto req = mirror.reflectRequest();
		req.setCap(kj::mv(impl));
		auto echoed = req.send();
		auto pipe = echoed.getCap();
		auto a = pipe.nextRequest();
		auto b = pipe.nextRequest();
		auto c = pipe.nextRequest();
		a.setN(1);
		b.setN(2);
		c.setN(3);
		auto pa = a.send();
		auto pb = b.send();
		auto pc = c.send();
		if (pa.wait(waitScope).getN() != 2)
			return fail("first result");
		if (pb.wait(waitScope).getN() != 3)
			return fail("second result");
		if (pc.wait(waitScope).getN() != 4)
			return fail("third result");
		if (raw->seen.size() != 3 || raw->seen[0] != 1 ||
		    raw->seen[1] != 2 || raw->seen[2] != 3)
			return fail("call order");
	}
	{
		bool destroyed = false;
		auto req = mirror.dropRequest();
		req.setCap(kj::heap<Dying>(&destroyed));
		req.send().wait(waitScope);
		if (!destroyed)
			return fail("param cap was not released");
	}
	printf("reflect ok\n");
	return 0;
}
