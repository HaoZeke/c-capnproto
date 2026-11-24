@0xc0ffee15a1b2c3d4;

using H = import "brand-hold.capnp";

struct Box(T) {
  val @0 :T;
}

struct Use {
  t @0 :Box(Text);
  plain @1 :H.Text;
}
