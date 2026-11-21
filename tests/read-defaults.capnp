@0xe7c0a11d4e5a1901;

struct Inner {
  x @0 :UInt32;
  s @1 :Text;
}

struct WithDefaults {
  l @0 :List(UInt32) = [10, 20, 30];
  st @1 :Inner = (x = 99, s = "custom");
}

struct Outer {
  x @0 :UInt32;
  inner @1 :Inner;
}
