@0xa5b3d80c6921471e;

using C = import "/c.capnp";
$C.codecgen;
$C.extraheader("#include \"null-text.h\"");

struct Kid $C.mapname("kid_t") {
  n @0 :UInt32;
}

struct Wrap $C.mapname("wrap_t") {
  note  @0 :Text;
  child @1 :Kid;
}
