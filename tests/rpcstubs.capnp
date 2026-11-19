# Typed RPC stubs: client helpers, result readers and a server vtable
# generated from the interface, used by rpcstubs-test.

@0xf6071829304a5b6c;

using C = import "/c.capnp";
$C.rpcstubs;

interface Calc {
  add @0 (a :Int32, b :Int32) -> (sum :Int32);
  join @1 (left :Text, right :Text) -> (joined :Text, length :UInt32);
  noop @2 () -> ();
  ticks @3 (n :UInt64) -> stream;
}

interface Echo {
  echo @0 (data :Data, times :UInt16 = 2) -> (data :Data, times :UInt16);
}
