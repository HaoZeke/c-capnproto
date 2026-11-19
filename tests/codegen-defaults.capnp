# Schemas for codegen default/list/const regressions.
#
# Rec: empty Data (and empty List) defaults must compile without
# referencing an undeclared capn_buf.
# Greeting: a Text field read from a null pointer must write back as a null
# pointer, so the default (or null) survives a read/write round trip.
# Event: List(Text) must work with capn_len after wrap as capn_ptr_list.
# Constants: generated as const with optional SCREAMING_SNAKE #define.

@0xa68e823569a53e2e;

using C = import "/c.capnp";
$C.fieldgetset;

struct Rec {
  info @0 :Data = "";
  tags @1 :List(Text) = [];
}

struct Greeting {
  hello @0 :Text = "hi";
  plain @1 :Text;
}

struct Event {
  args @0 :List(Text);
}

enum Color {
  red @0;
  green @1;
  blue @2;
}

const answer :Int32 = 42;
const flag :Bool = true;
const count :UInt16 = 7;
const big :UInt64 = 0x100000000;
const hue :Color = green;
const ratio :Float32 = 1.5;
const answerCamel :Int32 = 99;
