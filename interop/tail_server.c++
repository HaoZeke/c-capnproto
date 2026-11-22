#include "tail.capnp.h"

#include <capnp/ez-rpc.h>
#include <kj/async.h>
#include <string>

class HubImpl final : public Hub::Server {
public:
	kj::Promise<void> relay(RelayContext ctx) override {
		auto params = ctx.getParams();
		auto req = params.getCb().echoRequest();
		req.setN(params.getN());
		return ctx.tailCall(kj::mv(req));
	}
};

int main(int argc, char **argv) {
	std::string port = argc > 1 ? argv[1] : "43118";
	capnp::EzRpcServer server(kj::heap<HubImpl>(), "127.0.0.1:" + port);
	auto &waitScope = server.getWaitScope();
	kj::NEVER_DONE.wait(waitScope);
}
