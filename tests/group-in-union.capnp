@0xd4a4f1b8e2c6a390;

using C = import "/c.capnp";
$C.codecgen;

struct GroupInUnion $C.mapname("group_in_union_t") {
  data :union $C.mapuniontag("kind") {
    foo :group {
      x @0 :Int32;
      y @1 :Int64;
    }
    bar :group {
      name @2 :Text;
      value @3 :UInt32;
    }
    baz @4 :Text;
  }
}
