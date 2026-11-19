# $Json annotations exercised by the dynamic codec: renamed fields,
# flattened groups, discriminators, base64 and hex.

@0xe7c3a91b04d25f68;
using C = import "/c.capnp";
$C.reflect;

struct FlattenOptions {
  prefix @0 :Text = "";
}

struct DefaultNest {
  value @0 :Text;
}

struct DiscriminatorOptions {
  name @0 :Text;
  valueName @1 :Text;
}

annotation name @0xfa5b1fd61c2e7c3d (field, enumerant, group, union) :Text;
annotation flatten @0x82d3e852af0336bf (field, group, union) :FlattenOptions;
annotation discriminator @0xcfa794e8d19a0162 (struct, union) :DiscriminatorOptions;
annotation base64 @0xd7d879450a253e4b (field) :Void;
annotation hex @0xf061e22f0ae5c7b5 (field) :Void;

enum Hue {
  red @0;
  green @1 $name("verde");
  blue @2;
}

struct Doc $discriminator(name = "kind") {
  fullName @0 :Text $name("full_name");
  age @1 :UInt16;

  flat :group $flatten() {
    foo @2 :UInt32;
    bar @3 :Text;
    inner :group $flatten() {
      qux @4 :Text;
    }
  }

  pref :group $flatten(prefix = "p.") {
    a @5 :Text;
    b @6 :UInt32 $name("bee");
  }

  union {
    n @7 :UInt32;
    t @8 :Text;
    v @9 :Void;
  }

  tagged :union $flatten() $discriminator(valueName = "payload") {
    left @10 :Text;
    right @11 :UInt32 $name("right_n");
  }

  raw @12 :Data;
  b64 @13 :Data $base64;
  hx @14 :Data $hex;
  hue @15 :Hue;

  nest :group {
    city @16 :Text $name("town");
  }

  defaultNest @17 :DefaultNest $flatten() = (value = "default");
}
