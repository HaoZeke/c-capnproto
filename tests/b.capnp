@0xc0ffee13a1b2c3d4;

using A = import "a.capnp";

const c1 :A.Box(Text) = (val = "x");

struct Nest {
  inner @0 :A.Box(A.Box(Data));
}
