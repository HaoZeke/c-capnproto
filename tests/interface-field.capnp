# Interface (capability) pointer field. Encoding.html: A=3, B=0, C = table
# index. The C runtime stores that index in capn_ptr.len. No RPC table is
# provided; get/set copy the pointer as-is.

@0x9c1e0b7a4d2f1803;

using C = import "/c.capnp";
$C.fieldgetset;

interface Echo {
}

# `-> stream` makes capnp add an import of /capnp/stream.capnp, which has
# no C header; the generated header must still compile.
interface Sink {
  write @0 (chunk :Data) -> stream;
  done @1 () -> ();
}

struct Holder {
  cap @0 :Echo;
  caps @1 :List(Echo);
}
