@0xa1b2c3d4e5f60718;

using C = import "/c.capnp";
$C.reflect;
struct Other { x @0 :Int32 = 7; name @1 :Text = "oth"; }
enum Color { red @0; green @1; blue @2; }
const otherConst :Int64 = -99;
