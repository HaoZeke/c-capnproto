#ifndef CAPN_8831780CA65DBB5E
#define CAPN_8831780CA65DBB5E
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>

#if CAPN_VERSION != 1
#error "version mismatch between capnp_c.h and generated code"
#endif

#ifndef capnp_nowarn
# ifdef __GNUC__
#  define capnp_nowarn __extension__
# else
#  define capnp_nowarn
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct Leaf;
struct Inner;
struct TreeNode;
struct Bag;
struct Lists;

typedef struct {capn_ptr p;} Leaf_ptr;
typedef struct {capn_ptr p;} Inner_ptr;
typedef struct {capn_ptr p;} TreeNode_ptr;
typedef struct {capn_ptr p;} Bag_ptr;
typedef struct {capn_ptr p;} Lists_ptr;

typedef struct {capn_ptr p;} Leaf_list;
typedef struct {capn_ptr p;} Inner_list;
typedef struct {capn_ptr p;} TreeNode_list;
typedef struct {capn_ptr p;} Bag_list;
typedef struct {capn_ptr p;} Lists_list;

struct Leaf {
	int32_t value;
};

static const size_t Leaf_word_count = 1;

static const size_t Leaf_pointer_count = 0;

static const size_t Leaf_struct_bytes_count = 8;


int32_t Leaf_get_value(Leaf_ptr p);

void Leaf_set_value(Leaf_ptr p, int32_t value);

struct Inner {
	TreeNode_ptr left;
	TreeNode_ptr right;
};

static const size_t Inner_word_count = 0;

static const size_t Inner_pointer_count = 2;

static const size_t Inner_struct_bytes_count = 16;


TreeNode_ptr Inner_get_left(Inner_ptr p);

int Inner_has_left(Inner_ptr p);

TreeNode_ptr Inner_get_right(Inner_ptr p);

int Inner_has_right(Inner_ptr p);

void Inner_set_left(Inner_ptr p, TreeNode_ptr left);

void Inner_set_right(Inner_ptr p, TreeNode_ptr right);

struct TreeNode {
	int32_t nodeType;
	Leaf_ptr leaf;
	Inner_ptr inner;
};

static const size_t TreeNode_word_count = 1;

static const size_t TreeNode_pointer_count = 2;

static const size_t TreeNode_struct_bytes_count = 24;


int32_t TreeNode_get_nodeType(TreeNode_ptr p);

Leaf_ptr TreeNode_get_leaf(TreeNode_ptr p);

int TreeNode_has_leaf(TreeNode_ptr p);

Inner_ptr TreeNode_get_inner(TreeNode_ptr p);

int TreeNode_has_inner(TreeNode_ptr p);

void TreeNode_set_nodeType(TreeNode_ptr p, int32_t nodeType);

void TreeNode_set_leaf(TreeNode_ptr p, Leaf_ptr leaf);

void TreeNode_set_inner(TreeNode_ptr p, Inner_ptr inner);

struct Bag {
	capn_text note;
	capn_data blob;
};

static const size_t Bag_word_count = 0;

static const size_t Bag_pointer_count = 2;

static const size_t Bag_struct_bytes_count = 16;


capn_text Bag_get_note(Bag_ptr p);

int Bag_has_note(Bag_ptr p);

capn_data Bag_get_blob(Bag_ptr p);

int Bag_has_blob(Bag_ptr p);

void Bag_set_note(Bag_ptr p, capn_text note);

void Bag_set_blob(Bag_ptr p, capn_data blob);

struct Lists {
	capn_list32 u32;
	capn_list1 flags;
	capn_ptr_list names;
	Leaf_list leaves;
};

static const size_t Lists_word_count = 0;

static const size_t Lists_pointer_count = 4;

static const size_t Lists_struct_bytes_count = 32;


capn_list32 Lists_get_u32(Lists_ptr p);

int Lists_has_u32(Lists_ptr p);

capn_list1 Lists_get_flags(Lists_ptr p);

int Lists_has_flags(Lists_ptr p);

capn_ptr_list Lists_get_names(Lists_ptr p);

int Lists_has_names(Lists_ptr p);

Leaf_list Lists_get_leaves(Lists_ptr p);

int Lists_has_leaves(Lists_ptr p);

void Lists_set_u32(Lists_ptr p, capn_list32 u32);

void Lists_set_flags(Lists_ptr p, capn_list1 flags);

void Lists_set_names(Lists_ptr p, capn_ptr_list names);

void Lists_set_leaves(Lists_ptr p, Leaf_list leaves);

Leaf_ptr new_Leaf(struct capn_segment*);
Inner_ptr new_Inner(struct capn_segment*);
TreeNode_ptr new_TreeNode(struct capn_segment*);
Bag_ptr new_Bag(struct capn_segment*);
Lists_ptr new_Lists(struct capn_segment*);

Leaf_list new_Leaf_list(struct capn_segment*, int len);
Inner_list new_Inner_list(struct capn_segment*, int len);
TreeNode_list new_TreeNode_list(struct capn_segment*, int len);
Bag_list new_Bag_list(struct capn_segment*, int len);
Lists_list new_Lists_list(struct capn_segment*, int len);

void read_Leaf(struct Leaf*, Leaf_ptr);
void read_Inner(struct Inner*, Inner_ptr);
void read_TreeNode(struct TreeNode*, TreeNode_ptr);
void read_Bag(struct Bag*, Bag_ptr);
void read_Lists(struct Lists*, Lists_ptr);

void write_Leaf(const struct Leaf*, Leaf_ptr);
void write_Inner(const struct Inner*, Inner_ptr);
void write_TreeNode(const struct TreeNode*, TreeNode_ptr);
void write_Bag(const struct Bag*, Bag_ptr);
void write_Lists(const struct Lists*, Lists_ptr);

void get_Leaf(struct Leaf*, Leaf_list, int i);
void get_Inner(struct Inner*, Inner_list, int i);
void get_TreeNode(struct TreeNode*, TreeNode_list, int i);
void get_Bag(struct Bag*, Bag_list, int i);
void get_Lists(struct Lists*, Lists_list, int i);

void set_Leaf(const struct Leaf*, Leaf_list, int i);
void set_Inner(const struct Inner*, Inner_list, int i);
void set_TreeNode(const struct TreeNode*, TreeNode_list, int i);
void set_Bag(const struct Bag*, Bag_list, int i);
void set_Lists(const struct Lists*, Lists_list, int i);

#ifdef __cplusplus
}
#endif
#endif
