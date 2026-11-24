@0x8a1c0b5e4d3f2a02;

using HolderFile = import "nested-const.capnp";

struct Uses {
  held @0 :HolderFile.Holder;
}
