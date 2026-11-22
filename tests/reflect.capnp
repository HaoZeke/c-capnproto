# Schema for the reflection, text and JSON tests: unions, groups, defaults
# of every type, nested lists, generics, enums and a second file.

@0xb2c3d4e5f6071829;
using Other = import "reflect-other.capnp";
using C = import "/c.capnp";
$C.reflect;

annotation tag(*) :Text;
annotation flag(struct, field) :Bool;

enum Kind $tag("k") { alpha @0; beta @1; gamma @2; }

struct Defaults {
  b @0 :Bool = true;
  i8 @1 :Int8 = -8;
  i16 @2 :Int16 = -1600;
  i32 @3 :Int32 = -320000;
  i64 @4 :Int64 = -6400000000;
  u8 @5 :UInt8 = 200;
  u16 @6 :UInt16 = 60000;
  u32 @7 :UInt32 = 4000000000;
  u64 @8 :UInt64 = 18000000000000000000;
  f32 @9 :Float32 = 1.5;
  f64 @10 :Float64 = -2.25;
  t @11 :Text = "hello";
  d @12 :Data = 0x"deadbeef";
  e @13 :Kind = beta;
  lst @14 :List(Int32) = [1, 2, 3];
  lt @15 :List(Text) = ["a", "b"];
  s @16 :Other.Other = (x = 5, name = "five");
  bl @17 :List(Bool) = [true, false, true];
  v @18 :Void;
  f32b @19 :Float32 = inf;
  f64b @20 :Float64 = nan;
  ll @21 :List(List(Int16)) = [[1,2],[3]];
  ls @22 :List(Other.Other) = [(x = 1), (x = 2, name = "two")];
}

struct Unions {
  before @0 :UInt32;
  union {
    a @1 :UInt8;
    b @2 :Text;
    c :group { x @3 :Int64; y @4 :Int64; }
    d @5 :Void;
    e @6 :List(UInt16);
    f @7 :Bool;
  }
  after @8 :Float64;
  mid :union {
    p @9 :Bool;
    q @10 :Data;
    r @11 :Kind;
  }
  grp :group { m @12 :UInt16 = 3; n @13 :Text = "grp"; inner :group { z @14 :Bool = true; } }
}

struct Lists {
  v @0 :List(Void);
  b @1 :List(Bool);
  i8 @2 :List(Int8);
  u16 @3 :List(UInt16);
  i32 @4 :List(Int32);
  u64 @5 :List(UInt64);
  f32 @6 :List(Float32);
  f64 @7 :List(Float64);
  t @8 :List(Text);
  d @9 :List(Data);
  e @10 :List(Kind);
  s @11 :List(Defaults);
  ll @12 :List(List(Int32));
  lll @13 :List(List(List(Text)));
  ls @14 :List(List(Unions));
}

struct Any {
  p @0 :AnyPointer;
  s @1 :AnyStruct;
  l @2 :AnyList;
  c @3 :Capability;
  tag @4 :Int32;
}

struct Box(T) {
  val @0 :T;
  count @1 :UInt32;
}

struct Pair(A, B) { first @0 :A; second @1 :B; }

# Inner's brand on `inherited` is Scope.inherit. The parameter comes
# from Holder, not from a bind list on Inner.
struct Holder(T) {
  struct Inner {
    value @0 :T;
  }
  inherited @0 :Inner;
  rebound @1 :Holder(T).Inner;
}

struct UsesInherit {
  h @0 :Holder(Text);
}

struct UsesGenerics {
  ib @0 :Box(List(Int32));
  tb @1 :Box(Text);
  sb @2 :Box(Defaults);
  nested @3 :Box(Box(Text));
  pair @4 :Pair(Text, List(Int32));
  any @5 :Box;
}

struct Nested {
  struct Inner { struct Deep { x @0 :UInt8; } d @0 :Deep; }
  enum E { one @0; two @1; }
  const k :UInt32 = 42;
  i @0 :Inner;
  e @1 :E;
}

const cBool :Bool = true;
const cI8 :Int8 = -1;
const cU16 :UInt16 = 65535;
const cI32 :Int32 = -2147483648;
const cU64 :UInt64 = 0xffffffffffffffff;
const cI64 :Int64 = -9223372036854775808;
const cF32 :Float32 = 3.5;
const cF64 :Float64 = 1e300;
const cText :Text = "const text";
const cData :Data = 0x"01020304";
const cEnum :Kind = gamma;
const cList :List(UInt32) = [10, 20, 30];
const cTextList :List(Text) = ["x", "yy"];
const cStruct :Defaults = (i32 = 123, t = "cs");
const cStructList :List(Other.Other) = [(x = 9)];
const cVoid :Void = void;
const cFromOther :Int64 = Other.otherConst;

interface Calc {
  add @0 (a :Int32, b :Int32) -> (sum :Int32);
  noop @1 ();
  struct Params { x @0 :Int32; }
  withStruct @2 (p :Params) -> (r :Params);
  stream @3 (chunk :Data) -> stream;
}

interface SubCalc extends(Calc) {
  mul @0 (a :Int32, b :Int32) -> (prod :Int32);
}

interface GenIface(T) {
  get @0 () -> (v :T);
  put @1 (v :T) -> ();
}

struct HasCap { calc @0 :Calc; gi @1 :GenIface(Text); list @2 :List(Calc); }

struct Flagged $flag(true) { x @0 :UInt8 $flag(false); y @1 :Text $tag("why"); }

struct Big {
  a @0 :UInt64; b @1 :UInt64; c @2 :UInt64; d @3 :UInt64; e @4 :UInt64; f @5 :UInt64; g @6 :UInt64;
  txt @7 :Text; bin @8 :Data; kids @9 :List(Big); name @10 :Text;
}

# Ordinals out of code order, and union members whose pointer is unset.
struct Ordered {
  late @2 :UInt8;
  early @0 :UInt8;
  union {
    none @1 :Void;
    txt @3 :Text;
    lst @4 :List(UInt8);
    sub @5 :Other.Other;
  }
  kind @6 :Kind;
}

struct FirstIsPointer {
  union {
    t @0 :Text;
    n @1 :UInt8;
  }
  after @2 :UInt8;
}
