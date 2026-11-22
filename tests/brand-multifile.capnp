@0xc0ffee18a1b2c3d4;

using A = import "a.capnp";

struct Local {
  n @0 :Int32;
}

struct Uses {
  boxed @0 :A.Box(Local);
}
