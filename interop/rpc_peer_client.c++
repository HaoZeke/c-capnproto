// capnp-C++ client for the C CountLeaf vat (interop/rpc_server.c).
// Calls Adder methods and the two ancestors, so a grandparent interface
// id is dispatched by the C leaf.
#include "adder.capnp.h"
#include <capnp/ez-rpc.h>
#include <stdio.h>
#include <string>

int main(int argc, char **argv) {
	std::string port = argc > 1 ? argv[1] : "43118";
	capnp::EzRpcClient client("127.0.0.1:" + port);
	auto &waitScope = client.getWaitScope();
	CountLeaf::Client leaf = client.getMain<CountLeaf>();

	{
		auto req = leaf.addRequest();
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
		auto req = leaf.tagRequest();
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
	{
		auto req = leaf.incRequest();
		req.setN(7);
		auto resp = req.send().wait(waitScope);
		if (resp.getN() != 8) {
			fprintf(stderr, "inc(7) returned %d\n", resp.getN());
			return 1;
		}
	}
	{
		auto req = leaf.twiceRequest();
		req.setN(3);
		auto resp = req.send().wait(waitScope);
		if (resp.getN() != 6) {
			fprintf(stderr, "twice(3) returned %d\n", resp.getN());
			return 1;
		}
	}
	{
		auto req = leaf.decRequest();
		req.setN(10);
		auto resp = req.send().wait(waitScope);
		if (resp.getN() != 9) {
			fprintf(stderr, "dec(10) returned %d\n", resp.getN());
			return 1;
		}
	}
	printf("All reverse rpc interop assertions passed.\n");
	return 0;
}
