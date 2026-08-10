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

typedef struct {capn_ptr p;} Leaf_ptr;
typedef struct {capn_ptr p;} Inner_ptr;
typedef struct {capn_ptr p;} TreeNode_ptr;

typedef struct {capn_ptr p;} Leaf_list;
typedef struct {capn_ptr p;} Inner_list;
typedef struct {capn_ptr p;} TreeNode_list;

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

TreeNode_ptr Inner_get_right(Inner_ptr p);

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

Inner_ptr TreeNode_get_inner(TreeNode_ptr p);

void TreeNode_set_nodeType(TreeNode_ptr p, int32_t nodeType);

void TreeNode_set_leaf(TreeNode_ptr p, Leaf_ptr leaf);

void TreeNode_set_inner(TreeNode_ptr p, Inner_ptr inner);

Leaf_ptr new_Leaf(struct capn_segment*);
Inner_ptr new_Inner(struct capn_segment*);
TreeNode_ptr new_TreeNode(struct capn_segment*);

Leaf_list new_Leaf_list(struct capn_segment*, int len);
Inner_list new_Inner_list(struct capn_segment*, int len);
TreeNode_list new_TreeNode_list(struct capn_segment*, int len);

void read_Leaf(struct Leaf*, Leaf_ptr);
void read_Inner(struct Inner*, Inner_ptr);
void read_TreeNode(struct TreeNode*, TreeNode_ptr);

void write_Leaf(const struct Leaf*, Leaf_ptr);
void write_Inner(const struct Inner*, Inner_ptr);
void write_TreeNode(const struct TreeNode*, TreeNode_ptr);

void get_Leaf(struct Leaf*, Leaf_list, int i);
void get_Inner(struct Inner*, Inner_list, int i);
void get_TreeNode(struct TreeNode*, TreeNode_list, int i);

void set_Leaf(const struct Leaf*, Leaf_list, int i);
void set_Inner(const struct Inner*, Inner_list, int i);
void set_TreeNode(const struct TreeNode*, TreeNode_list, int i);

#ifdef __cplusplus
}
#endif
#endif
