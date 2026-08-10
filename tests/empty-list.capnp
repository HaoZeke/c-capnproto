@0xb6c4e91d7a30582f;

using C = import "/c.capnp";
$C.codecgen;
$C.extraheader("#include \"empty-list.h\"");

struct EmptyListThenField $C.mapname("empty_list_then_field_t") {
  names @0 :List(Text) $C.mapname("names") $C.maplistcount("n_names");
  tag   @1 :UInt32;
}
