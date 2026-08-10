@0x9a7c3e1d5b0842f6;

using C = import "/c.capnp";
$C.codecgen;

struct Item $C.mapname("item_t") {
  name @0 :Text;
  n @1 :Int32;
}
