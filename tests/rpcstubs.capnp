# Typed RPC stubs: client helpers, result readers and a server vtable
# generated from the interface, used by rpcstubs-test.

@0xf6071829304a5b6c;

using C = import "/c.capnp";
$C.rpcstubs;

interface Calc {
  add @0 (a :Int32, b :Int32) -> (sum :Int32);
  join @1 (left :Text, right :Text) -> (joined :Text, length :UInt32);
  noop @2 () -> ();
  ticks @3 (n :UInt64) -> stream;
}

interface Echo {
  echo @0 (data :Data, times :UInt16 = 2) -> (data :Data, times :UInt16);
}

interface SubCalc extends(Calc) {
  multiply @0 (a :Int32, b :Int32) -> (product :Int32);
}

struct Note {
  n @0 :Int32;
  label @1 :Text;
}

interface Noted {
  post @0 Note -> Note;
  mix @1 (n :Int32, label :Text) -> (echo :Text, n :Int32);
}

# The leaf's own results are one byte. The grandparent's are three words.
interface WideRoot {
  span @0 () -> (a :UInt64, b :UInt64, c :UInt64);
}

interface WideMid extends(WideRoot) {
  mid @0 () -> (x :UInt8);
}

interface WideLeaf extends(WideMid) {
  leaf @0 () -> (x :UInt8);
}

# The pointer lives only on the grandparent.
interface TextRoot {
  name @0 () -> (t :Text);
}

interface TextMid extends(TextRoot) {
  n @0 () -> (n :Int32);
}

interface TextLeaf extends(TextMid) {
  n2 @0 () -> (n :Int32);
}

interface ChainRoot {
  inc @0 (n :Int32) -> (n :Int32);
}

interface ChainMid extends(ChainRoot) {
  twice @0 (n :Int32) -> (n :Int32);
}

interface ChainLeaf extends(ChainMid) {
  dec @0 (n :Int32) -> (n :Int32);
}

# BareMid and SideMid declare no methods. They exist so a leaf can
# forward a grandparent id, including through a second superclass.
interface BareRoot {
  inc @0 (n :Int32) -> (n :Int32);
  label @1 (t :Text) -> (t :Text);
}

interface BareMid extends(BareRoot) {
}

interface SideRoot {
  ping @0 (n :Int32) -> (n :Int32);
}

interface SideMid extends(SideRoot) {
}

interface BareLeaf extends(BareMid, SideMid) {
  dec @0 (n :Int32) -> (n :Int32);
}
