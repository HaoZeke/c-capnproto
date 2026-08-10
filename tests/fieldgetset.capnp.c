#include "fieldgetset.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif


Leaf_ptr new_Leaf(struct capn_segment *s) {
	Leaf_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
Leaf_list new_Leaf_list(struct capn_segment *s, int len) {
	Leaf_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_Leaf(struct Leaf *s capnp_unused, Leaf_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->value = (int32_t) ((int32_t)capn_read32(p.p, 0));
}
void write_Leaf(const struct Leaf *s capnp_unused, Leaf_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->value));
}
void get_Leaf(struct Leaf *s, Leaf_list l, int i) {
	Leaf_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Leaf(s, p);
}
void set_Leaf(const struct Leaf *s, Leaf_list l, int i) {
	Leaf_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Leaf(s, p);
}

int32_t Leaf_get_value(Leaf_ptr p)
{
	capn_resolve(&p.p);
	int32_t value;
	value = (int32_t) ((int32_t)capn_read32(p.p, 0));
	return value;
}

void Leaf_set_value(Leaf_ptr p, int32_t value)
{
	capn_resolve(&p.p);
	capn_write32(p.p, 0, (uint32_t) (value));
}

Inner_ptr new_Inner(struct capn_segment *s) {
	Inner_ptr p;
	p.p = capn_new_struct(s, 0, 2);
	return p;
}
Inner_list new_Inner_list(struct capn_segment *s, int len) {
	Inner_list p;
	p.p = capn_new_struct_list(s, len, 0, 2);
	return p;
}
void read_Inner(struct Inner *s capnp_unused, Inner_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->left.p = capn_getp(p.p, 0, 0);
	s->right.p = capn_getp(p.p, 1, 0);
}
void write_Inner(const struct Inner *s capnp_unused, Inner_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, s->left.p);
	capn_setp(p.p, 1, s->right.p);
}
void get_Inner(struct Inner *s, Inner_list l, int i) {
	Inner_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Inner(s, p);
}
void set_Inner(const struct Inner *s, Inner_list l, int i) {
	Inner_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Inner(s, p);
}

TreeNode_ptr Inner_get_left(Inner_ptr p)
{
	capn_resolve(&p.p);
	TreeNode_ptr left;
	left.p = capn_getp(p.p, 0, 1);
	return left;
}

TreeNode_ptr Inner_get_right(Inner_ptr p)
{
	capn_resolve(&p.p);
	TreeNode_ptr right;
	right.p = capn_getp(p.p, 1, 1);
	return right;
}

void Inner_set_left(Inner_ptr p, TreeNode_ptr left)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, left.p);
}

void Inner_set_right(Inner_ptr p, TreeNode_ptr right)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, right.p);
}

TreeNode_ptr new_TreeNode(struct capn_segment *s) {
	TreeNode_ptr p;
	p.p = capn_new_struct(s, 8, 2);
	return p;
}
TreeNode_list new_TreeNode_list(struct capn_segment *s, int len) {
	TreeNode_list p;
	p.p = capn_new_struct_list(s, len, 8, 2);
	return p;
}
void read_TreeNode(struct TreeNode *s capnp_unused, TreeNode_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->nodeType = (int32_t) ((int32_t)capn_read32(p.p, 0));
	s->leaf.p = capn_getp(p.p, 0, 0);
	s->inner.p = capn_getp(p.p, 1, 0);
}
void write_TreeNode(const struct TreeNode *s capnp_unused, TreeNode_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, (uint32_t) (s->nodeType));
	capn_setp(p.p, 0, s->leaf.p);
	capn_setp(p.p, 1, s->inner.p);
}
void get_TreeNode(struct TreeNode *s, TreeNode_list l, int i) {
	TreeNode_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_TreeNode(s, p);
}
void set_TreeNode(const struct TreeNode *s, TreeNode_list l, int i) {
	TreeNode_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_TreeNode(s, p);
}

int32_t TreeNode_get_nodeType(TreeNode_ptr p)
{
	capn_resolve(&p.p);
	int32_t nodeType;
	nodeType = (int32_t) ((int32_t)capn_read32(p.p, 0));
	return nodeType;
}

Leaf_ptr TreeNode_get_leaf(TreeNode_ptr p)
{
	capn_resolve(&p.p);
	Leaf_ptr leaf;
	leaf.p = capn_getp(p.p, 0, 1);
	return leaf;
}

Inner_ptr TreeNode_get_inner(TreeNode_ptr p)
{
	capn_resolve(&p.p);
	Inner_ptr inner;
	inner.p = capn_getp(p.p, 1, 1);
	return inner;
}

void TreeNode_set_nodeType(TreeNode_ptr p, int32_t nodeType)
{
	capn_resolve(&p.p);
	capn_write32(p.p, 0, (uint32_t) (nodeType));
}

void TreeNode_set_leaf(TreeNode_ptr p, Leaf_ptr leaf)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, leaf.p);
}

void TreeNode_set_inner(TreeNode_ptr p, Inner_ptr inner)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, inner.p);
}
