// capnp-C++ client for the C Adder vat (interop/rpc_server.c).
// Calls add() and tag() so a params struct with both a data word and a
// pointer is read back by the C stubs.
#include "adder.capnp.h"
#include <capnp/ez-rpc.h>
#include <stdio.h>
#include <string>

int main(int argc, char **argv) {
	std::string port = argc > 1 ? argv[1] : "43118";
	capnp::EzRpcClient client("127.0.0.1:" + port);
	auto &waitScope = client.getWaitScope();
	Adder::Client adder = client.getMain<Adder>();

	{
		auto req = adder.addRequest();
		req.setA(20);
		req.setB(22);
		auto resp = req.send().wait(waitScope);
		if (resp.getSum() != 42) {
			fprintf(stderr, "add(20, 22) returned %lld\n",
			        (long long) resp.getSum());
			return 1;
		}
	}
	{
		auto req = adder.tagRequest();
		req.setN(9);
		req.setLabel("wire");
		auto resp = req.send().wait(waitScope);
		auto label = resp.getLabel();
		if (resp.getN() != 9 || label != "wire") {
			fprintf(stderr, "tag(9, \"wire\") came back as n=%d label=%s\n",
			        resp.getN(), label.cStr());
			return 1;
		}
	}
	printf("All reverse rpc interop assertions passed.\n");
	return 0;
}
