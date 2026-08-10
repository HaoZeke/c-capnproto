@0xd4a9e4f5b8c72a1e;

using C = import "/c.capnp";
$C.fieldgetset;
$C.codecgen;
$C.extraheader("#include \"data-codec.h\"");

struct DataMessage $C.mapname("data_message_t") {
  payload  @0 :Data $C.mapname("payload") $C.maplistcount("payload_len");
  name     @1 :Text;
  chunks   @2 :List(Data) $C.mapname("chunks") $C.maplistcount("n_chunks");
  tag      @3 :UInt32;
}
