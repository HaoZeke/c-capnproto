@0xbf5e831ac9f0d2a1;
# RPC interop test interface: served by capnp-C++ (interop/rpc_peer_server.c++),
# called by the C vat (interop/rpc_client.c). The explicit interface id is
# mirrored as ADDER_IFACE in the Fortran client.

using C = import "/c.capnp";
$C.rpcstubs;

interface Adder @0xea01e10cbc414411 {
  add @0 (a :Int64, b :Int64) -> (sum :Int64);
  # One data word and one pointer, so a stub that counts pointer bytes
  # as data shifts the text on the wire.
  tag @1 (n :Int32, label :Text) -> (n :Int32, label :Text);
}

# Three interfaces. A call on the leaf carries the root's interface id.
interface CountBase {
  inc @0 (n :Int32) -> (n :Int32);
}

interface CountMid extends(CountBase) {
  twice @0 (n :Int32) -> (n :Int32);
}

interface CountLeaf extends(CountMid, Adder) {
  dec @0 (n :Int32) -> (n :Int32);
}
