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
  # A promise of an Adder. fulfill() resolves it to a working Adder,
  # or to the capability passed to watch() when one was given.
  defer @2 () -> (cap :Adder);
  fulfill @3 () -> ();
  # Remember a capability this vat hosts. A later fulfill() resolves
  # the deferred promise to it, so calls pipelined on the promise are
  # reflected back here with the cap table swapped.
  watch @4 (cap :Adder) -> ();
  # Return the capability that was passed in. A reflected echo is how a
  # cap handed through a promise comes back to the caller.
  echo @5 (cap :Adder) -> (cap :Adder);
  # prev is the n of the previous seq() on this object, or 0 if none.
  # Calls that cross an embargo have to arrive in the order they were sent.
  seq @6 (n :Int64) -> (prev :Int64);
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
