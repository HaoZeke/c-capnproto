@0xc15a0e6b9d4f2701;

using C = import "/c.capnp";
$C.fieldgetset;

struct UDef {
  before @0 :UInt8 = 1;
  union {
    a @1 :Int32 = 5;
    b @2 :Text = "bee";
    c @3 :Void;
    g :group { x @4 :Int16 = 9; y @5 :Text; }
  }
  after @6 :UInt8;
}
