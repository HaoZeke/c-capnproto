@0xb0fa5533ab65a835;

using C = import "/c.capnp";
$C.codecgen;

# List(struct) with a dest mapname that differs from the schema field.
# gen_call_list_decoder must emit
#   decode_Line_list(&(d->n_chapters), &(d->chapters_), s->chapters);
# Swapping dest/src yields d->chapters / s->chapters_ and fails to compile.
struct Line $C.mapname("line_t") {
  caption @0 :Text;
  n @1 :Int32;
}

struct Book $C.mapname("book_t") {
  chapters @0 :List(Line) $C.mapname("chapters_") $C.maplistcount("n_chapters");
  tag @1 :UInt32;
}
