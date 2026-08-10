# Nested struct accessors for $C.fieldgetset.
#
# TreeNode.leaf / TreeNode.inner are pointer fields. Get-after-set on a
# nested Leaf must return the value written via Leaf_set_value.

@0x8831780ca65dbb5e;

using C = import "/c.capnp";
$C.fieldgetset;

struct Leaf {
  value @0 :Int32;
}

struct Inner {
  left @0 :TreeNode;
  right @1 :TreeNode;
}

struct TreeNode {
  nodeType @0 :Int32;
  leaf @1 :Leaf;
  inner @2 :Inner;
}

# Pointer has_ accessors: empty Text/Data is present, null is not.
struct Bag {
  note @0 :Text;
  blob @1 :Data;
}
