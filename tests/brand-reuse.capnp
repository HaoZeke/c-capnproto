@0xc0ffee19a1b2c3d4;

using A = import "a.capnp";
using B = import "b.capnp";

struct F {
  t @0 :A.Box(Text);
  nest @1 :B.Nest;
}
