@0xc0ffee1aa1b2c3d4;

struct Box(T) {
  val @0 :T;
}

struct Text {
  n @0 :Int32;
}

struct Use {
  t @0 :Box(Text);
}
