@0xb14a11ee00000001;

using C = import "/c.capnp";
$C.rpcstubs;

interface Seq @0xb14a11ee00000002 {
  next @0 (n :Int32) -> (n :Int32);
}

interface Mirror @0xb14a11ee00000003 {
  reflect @0 (cap :Seq) -> (cap :Seq);
  drop @1 (cap :Seq) -> (cap :Seq);
}
