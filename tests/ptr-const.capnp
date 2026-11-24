@0xd1e5c0a3b2f40712;
# Pointer constants, branded or plain, read by `ptr-const-test.c`.

struct Box(T) {
  val @0 :T;
}

struct Plain {
  n @0 :Int32;
  t @1 :Text;
}

const boxed :Box(Text) = (val = "hi");
const plain :Plain = (n = 3, t = "x");
const nums :List(Int32) = [1, 2];
const plains :List(Plain) = [(n = 4), (n = 5)];
