# Test schema for $C.extraheader and $C.extendedattribute.
#
# extraheader must be emitted into the generated .h after the capnp_c.h
# include. extendedattribute must prefix generated new_/get_/set_ (and
# related) functions.

@0xbc91e0a4d3f8c217;

using C = import "/c.capnp";
$C.fieldgetset;
$C.extraheader("#include \"extraheader-probe.h\"");
$C.extendedattribute("EXTRAHEADER_EXTATTR");

struct Widget {
  id @0 :UInt32;
  name @1 :Text;
}
