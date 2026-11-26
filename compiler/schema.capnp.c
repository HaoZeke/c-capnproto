#include "schema.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

static const char capn_val0_str[1] = {0};
static const capn_text capn_val0 = {0,capn_val0_str,0};
static const capn_ptr capn_null = {CAPN_NULL,0,0,0,0,0,0,0,0,0,0,0};
static const capn_text capn_text_null = {0,0,0};
const uint16_t Field_noDiscriminant = FIELD_NO_DISCRIMINANT;

Node_ptr new_Node(struct capn_segment *s) {
	Node_ptr p;
	p.p = capn_new_struct(s, 48, 6);
	return p;
}
Node_list new_Node_list(struct capn_segment *s, int len) {
	Node_list p;
	p.p = capn_new_struct_list(s, len, 48, 6);
	return p;
}
void read_Node(struct Node *s capnp_unused, Node_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->id = capn_read64(p.p, 0);
	s->displayName = capn_get_text(p.p, 0, capn_val0);
	s->displayNamePrefixLength = capn_read32(p.p, 8);
	s->scopeId = capn_read64(p.p, 16);
	s->parameters.p = capn_getp(p.p, 5, 0);
if (s->parameters.p.type == CAPN_FAR_POINTER && s->parameters.p.data != NULL && *(const uint64_t*)s->parameters.p.data == 0) s->parameters.p.type = CAPN_NULL;
	s->isGeneric = (capn_read8(p.p, 36) & 1) != 0;
	s->nestedNodes.p = capn_getp(p.p, 1, 0);
if (s->nestedNodes.p.type == CAPN_FAR_POINTER && s->nestedNodes.p.data != NULL && *(const uint64_t*)s->nestedNodes.p.data == 0) s->nestedNodes.p.type = CAPN_NULL;
	s->annotations.p = capn_getp(p.p, 2, 0);
if (s->annotations.p.type == CAPN_FAR_POINTER && s->annotations.p.data != NULL && *(const uint64_t*)s->annotations.p.data == 0) s->annotations.p.type = CAPN_NULL;
	s->which = (enum Node_which)(int) capn_read16(p.p, 12);
	switch (s->which) {
	case Node__struct:
		s->_struct.dataWordCount = capn_read16(p.p, 14);
		s->_struct.pointerCount = capn_read16(p.p, 24);
		s->_struct.preferredListEncoding = (enum ElementSize)(int) capn_read16(p.p, 26);
		s->_struct.isGroup = (capn_read8(p.p, 28) & 1) != 0;
		s->_struct.discriminantCount = capn_read16(p.p, 30);
		s->_struct.discriminantOffset = capn_read32(p.p, 32);
		s->_struct.fields.p = capn_getp(p.p, 3, 0);
if (s->_struct.fields.p.type == CAPN_FAR_POINTER && s->_struct.fields.p.data != NULL && *(const uint64_t*)s->_struct.fields.p.data == 0) s->_struct.fields.p.type = CAPN_NULL;
		break;
	case Node__enum:
		s->_enum.enumerants.p = capn_getp(p.p, 3, 0);
if (s->_enum.enumerants.p.type == CAPN_FAR_POINTER && s->_enum.enumerants.p.data != NULL && *(const uint64_t*)s->_enum.enumerants.p.data == 0) s->_enum.enumerants.p.type = CAPN_NULL;
		break;
	case Node__interface:
		s->_interface.methods.p = capn_getp(p.p, 3, 0);
if (s->_interface.methods.p.type == CAPN_FAR_POINTER && s->_interface.methods.p.data != NULL && *(const uint64_t*)s->_interface.methods.p.data == 0) s->_interface.methods.p.type = CAPN_NULL;
		s->_interface.superclasses.p = capn_getp(p.p, 4, 0);
if (s->_interface.superclasses.p.type == CAPN_FAR_POINTER && s->_interface.superclasses.p.data != NULL && *(const uint64_t*)s->_interface.superclasses.p.data == 0) s->_interface.superclasses.p.type = CAPN_NULL;
		break;
	case Node__const:
		s->_const.type.p = capn_getp(p.p, 3, 0);
if (s->_const.type.p.type == CAPN_FAR_POINTER && s->_const.type.p.data != NULL && *(const uint64_t*)s->_const.type.p.data == 0) s->_const.type.p.type = CAPN_NULL;
		s->_const.value.p = capn_getp(p.p, 4, 0);
if (s->_const.value.p.type == CAPN_FAR_POINTER && s->_const.value.p.data != NULL && *(const uint64_t*)s->_const.value.p.data == 0) s->_const.value.p.type = CAPN_NULL;
		break;
	case Node_annotation:
		s->annotation.type.p = capn_getp(p.p, 3, 0);
if (s->annotation.type.p.type == CAPN_FAR_POINTER && s->annotation.type.p.data != NULL && *(const uint64_t*)s->annotation.type.p.data == 0) s->annotation.type.p.type = CAPN_NULL;
		s->annotation.targetsFile = (capn_read8(p.p, 14) & 1) != 0;
		s->annotation.targetsConst = (capn_read8(p.p, 14) & 2) != 0;
		s->annotation.targetsEnum = (capn_read8(p.p, 14) & 4) != 0;
		s->annotation.targetsEnumerant = (capn_read8(p.p, 14) & 8) != 0;
		s->annotation.targetsStruct = (capn_read8(p.p, 14) & 16) != 0;
		s->annotation.targetsField = (capn_read8(p.p, 14) & 32) != 0;
		s->annotation.targetsUnion = (capn_read8(p.p, 14) & 64) != 0;
		s->annotation.targetsGroup = (capn_read8(p.p, 14) & 128) != 0;
		s->annotation.targetsInterface = (capn_read8(p.p, 15) & 1) != 0;
		s->annotation.targetsMethod = (capn_read8(p.p, 15) & 2) != 0;
		s->annotation.targetsParam = (capn_read8(p.p, 15) & 4) != 0;
		s->annotation.targetsAnnotation = (capn_read8(p.p, 15) & 8) != 0;
		break;
	default:
		break;
	}
	s->startByte = capn_read32(p.p, 40);
	s->endByte = capn_read32(p.p, 44);
}
void write_Node(const struct Node *s capnp_unused, Node_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->id);
	capn_set_text(p.p, 0, (s->displayName.str != capn_val0.str) ? s->displayName : capn_text_null);
	capn_write32(p.p, 8, s->displayNamePrefixLength);
	capn_write64(p.p, 16, s->scopeId);
	capn_setp(p.p, 5, (s->parameters.p.type != CAPN_NULL) ? s->parameters.p : capn_null);
	capn_write1(p.p, 288, s->isGeneric != 0);
	capn_setp(p.p, 1, (s->nestedNodes.p.type != CAPN_NULL) ? s->nestedNodes.p : capn_null);
	capn_setp(p.p, 2, (s->annotations.p.type != CAPN_NULL) ? s->annotations.p : capn_null);
	capn_write16(p.p, 12, s->which);
	switch (s->which) {
	case Node__struct:
		capn_write16(p.p, 14, s->_struct.dataWordCount);
		capn_write16(p.p, 24, s->_struct.pointerCount);
		capn_write16(p.p, 26, (uint16_t) (s->_struct.preferredListEncoding));
		capn_write1(p.p, 224, s->_struct.isGroup != 0);
		capn_write16(p.p, 30, s->_struct.discriminantCount);
		capn_write32(p.p, 32, s->_struct.discriminantOffset);
		capn_setp(p.p, 3, (s->_struct.fields.p.type != CAPN_NULL) ? s->_struct.fields.p : capn_null);
		break;
	case Node__enum:
		capn_setp(p.p, 3, (s->_enum.enumerants.p.type != CAPN_NULL) ? s->_enum.enumerants.p : capn_null);
		break;
	case Node__interface:
		capn_setp(p.p, 3, (s->_interface.methods.p.type != CAPN_NULL) ? s->_interface.methods.p : capn_null);
		capn_setp(p.p, 4, (s->_interface.superclasses.p.type != CAPN_NULL) ? s->_interface.superclasses.p : capn_null);
		break;
	case Node__const:
		capn_setp(p.p, 3, (s->_const.type.p.type != CAPN_NULL) ? s->_const.type.p : capn_null);
		capn_setp(p.p, 4, (s->_const.value.p.type != CAPN_NULL) ? s->_const.value.p : capn_null);
		break;
	case Node_annotation:
		capn_setp(p.p, 3, (s->annotation.type.p.type != CAPN_NULL) ? s->annotation.type.p : capn_null);
		capn_write1(p.p, 112, s->annotation.targetsFile != 0);
		capn_write1(p.p, 113, s->annotation.targetsConst != 0);
		capn_write1(p.p, 114, s->annotation.targetsEnum != 0);
		capn_write1(p.p, 115, s->annotation.targetsEnumerant != 0);
		capn_write1(p.p, 116, s->annotation.targetsStruct != 0);
		capn_write1(p.p, 117, s->annotation.targetsField != 0);
		capn_write1(p.p, 118, s->annotation.targetsUnion != 0);
		capn_write1(p.p, 119, s->annotation.targetsGroup != 0);
		capn_write1(p.p, 120, s->annotation.targetsInterface != 0);
		capn_write1(p.p, 121, s->annotation.targetsMethod != 0);
		capn_write1(p.p, 122, s->annotation.targetsParam != 0);
		capn_write1(p.p, 123, s->annotation.targetsAnnotation != 0);
		break;
	default:
		break;
	}
	capn_write32(p.p, 40, s->startByte);
	capn_write32(p.p, 44, s->endByte);
}
void get_Node(struct Node *s, Node_list l, int i) {
	Node_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Node(s, p);
}
void set_Node(const struct Node *s, Node_list l, int i) {
	Node_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Node(s, p);
}

uint64_t Node_get_id(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t id;
	id = capn_read64(p.p, 0);
	return id;
}

capn_text Node_get_displayName(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text displayName;
	displayName = capn_get_text(p.p, 0, capn_val0);
	return displayName;
}

int Node_has_displayName(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr displayName;
	displayName = capn_getp(p.p, 0, 1);
	return displayName.type != CAPN_NULL;
}

uint32_t Node_get_displayNamePrefixLength(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	uint32_t displayNamePrefixLength;
	displayNamePrefixLength = capn_read32(p.p, 8);
	return displayNamePrefixLength;
}

uint64_t Node_get_scopeId(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t scopeId;
	scopeId = capn_read64(p.p, 16);
	return scopeId;
}

Node_Parameter_list Node_get_parameters(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	Node_Parameter_list parameters;
	parameters.p = capn_getp(p.p, 5, 1);
	capn_resolve_struct_list(&parameters.p);
	return parameters;
}

int Node_has_parameters(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr parameters;
	parameters = capn_getp(p.p, 5, 1);
	return parameters.type != CAPN_NULL;
}

unsigned Node_get_isGeneric(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	unsigned isGeneric;
	isGeneric = (capn_read8(p.p, 36) & 1) != 0;
	return isGeneric;
}

Node_NestedNode_list Node_get_nestedNodes(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	Node_NestedNode_list nestedNodes;
	nestedNodes.p = capn_getp(p.p, 1, 1);
	capn_resolve_struct_list(&nestedNodes.p);
	return nestedNodes;
}

int Node_has_nestedNodes(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr nestedNodes;
	nestedNodes = capn_getp(p.p, 1, 1);
	return nestedNodes.type != CAPN_NULL;
}

Annotation_list Node_get_annotations(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	Annotation_list annotations;
	annotations.p = capn_getp(p.p, 2, 1);
	capn_resolve_struct_list(&annotations.p);
	return annotations;
}

int Node_has_annotations(Node_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr annotations;
	annotations = capn_getp(p.p, 2, 1);
	return annotations.type != CAPN_NULL;
}

void Node_set_id(Node_ptr p, uint64_t id)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 0, id);
}

void Node_set_displayName(Node_ptr p, capn_text displayName)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (displayName.str != capn_val0.str) ? displayName : capn_text_null);
}

void Node_set_displayNamePrefixLength(Node_ptr p, uint32_t displayNamePrefixLength)
{
	capn_resolve(&p.p);
	capn_write32(p.p, 8, displayNamePrefixLength);
}

void Node_set_scopeId(Node_ptr p, uint64_t scopeId)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 16, scopeId);
}

void Node_set_parameters(Node_ptr p, Node_Parameter_list parameters)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 5, (parameters.p.type != CAPN_NULL) ? parameters.p : capn_null);
}

void Node_set_isGeneric(Node_ptr p, unsigned isGeneric)
{
	capn_resolve(&p.p);
	capn_write1(p.p, 288, isGeneric != 0);
}

void Node_set_nestedNodes(Node_ptr p, Node_NestedNode_list nestedNodes)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (nestedNodes.p.type != CAPN_NULL) ? nestedNodes.p : capn_null);
}

void Node_set_annotations(Node_ptr p, Annotation_list annotations)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 2, (annotations.p.type != CAPN_NULL) ? annotations.p : capn_null);
}

Node_Parameter_ptr new_Node_Parameter(struct capn_segment *s) {
	Node_Parameter_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Node_Parameter_list new_Node_Parameter_list(struct capn_segment *s, int len) {
	Node_Parameter_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
	return p;
}
void read_Node_Parameter(struct Node_Parameter *s capnp_unused, Node_Parameter_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->name = capn_get_text(p.p, 0, capn_val0);
}
void write_Node_Parameter(const struct Node_Parameter *s capnp_unused, Node_Parameter_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, (s->name.str != capn_val0.str) ? s->name : capn_text_null);
}
void get_Node_Parameter(struct Node_Parameter *s, Node_Parameter_list l, int i) {
	Node_Parameter_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Node_Parameter(s, p);
}
void set_Node_Parameter(const struct Node_Parameter *s, Node_Parameter_list l, int i) {
	Node_Parameter_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Node_Parameter(s, p);
}

capn_text Node_Parameter_get_name(Node_Parameter_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

int Node_Parameter_has_name(Node_Parameter_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr name;
	name = capn_getp(p.p, 0, 1);
	return name.type != CAPN_NULL;
}

void Node_Parameter_set_name(Node_Parameter_ptr p, capn_text name)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (name.str != capn_val0.str) ? name : capn_text_null);
}

Node_NestedNode_ptr new_Node_NestedNode(struct capn_segment *s) {
	Node_NestedNode_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Node_NestedNode_list new_Node_NestedNode_list(struct capn_segment *s, int len) {
	Node_NestedNode_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Node_NestedNode(struct Node_NestedNode *s capnp_unused, Node_NestedNode_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->name = capn_get_text(p.p, 0, capn_val0);
	s->id = capn_read64(p.p, 0);
}
void write_Node_NestedNode(const struct Node_NestedNode *s capnp_unused, Node_NestedNode_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, (s->name.str != capn_val0.str) ? s->name : capn_text_null);
	capn_write64(p.p, 0, s->id);
}
void get_Node_NestedNode(struct Node_NestedNode *s, Node_NestedNode_list l, int i) {
	Node_NestedNode_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Node_NestedNode(s, p);
}
void set_Node_NestedNode(const struct Node_NestedNode *s, Node_NestedNode_list l, int i) {
	Node_NestedNode_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Node_NestedNode(s, p);
}

capn_text Node_NestedNode_get_name(Node_NestedNode_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

int Node_NestedNode_has_name(Node_NestedNode_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr name;
	name = capn_getp(p.p, 0, 1);
	return name.type != CAPN_NULL;
}

uint64_t Node_NestedNode_get_id(Node_NestedNode_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t id;
	id = capn_read64(p.p, 0);
	return id;
}

void Node_NestedNode_set_name(Node_NestedNode_ptr p, capn_text name)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (name.str != capn_val0.str) ? name : capn_text_null);
}

void Node_NestedNode_set_id(Node_NestedNode_ptr p, uint64_t id)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 0, id);
}

Node_SourceInfo_ptr new_Node_SourceInfo(struct capn_segment *s) {
	Node_SourceInfo_ptr p;
	p.p = capn_new_struct(s, 16, 2);
	return p;
}
Node_SourceInfo_list new_Node_SourceInfo_list(struct capn_segment *s, int len) {
	Node_SourceInfo_list p;
	p.p = capn_new_struct_list(s, len, 16, 2);
	return p;
}
void read_Node_SourceInfo(struct Node_SourceInfo *s capnp_unused, Node_SourceInfo_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->id = capn_read64(p.p, 0);
	s->docComment = capn_get_text(p.p, 0, capn_val0);
	s->members.p = capn_getp(p.p, 1, 0);
if (s->members.p.type == CAPN_FAR_POINTER && s->members.p.data != NULL && *(const uint64_t*)s->members.p.data == 0) s->members.p.type = CAPN_NULL;
	s->startByte = capn_read32(p.p, 8);
	s->endByte = capn_read32(p.p, 12);
}
void write_Node_SourceInfo(const struct Node_SourceInfo *s capnp_unused, Node_SourceInfo_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->id);
	capn_set_text(p.p, 0, (s->docComment.str != capn_val0.str) ? s->docComment : capn_text_null);
	capn_setp(p.p, 1, (s->members.p.type != CAPN_NULL) ? s->members.p : capn_null);
	capn_write32(p.p, 8, s->startByte);
	capn_write32(p.p, 12, s->endByte);
}
void get_Node_SourceInfo(struct Node_SourceInfo *s, Node_SourceInfo_list l, int i) {
	Node_SourceInfo_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Node_SourceInfo(s, p);
}
void set_Node_SourceInfo(const struct Node_SourceInfo *s, Node_SourceInfo_list l, int i) {
	Node_SourceInfo_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Node_SourceInfo(s, p);
}

uint64_t Node_SourceInfo_get_id(Node_SourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t id;
	id = capn_read64(p.p, 0);
	return id;
}

capn_text Node_SourceInfo_get_docComment(Node_SourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text docComment;
	docComment = capn_get_text(p.p, 0, capn_val0);
	return docComment;
}

int Node_SourceInfo_has_docComment(Node_SourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr docComment;
	docComment = capn_getp(p.p, 0, 1);
	return docComment.type != CAPN_NULL;
}

Node_SourceInfo_Member_list Node_SourceInfo_get_members(Node_SourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	Node_SourceInfo_Member_list members;
	members.p = capn_getp(p.p, 1, 1);
	capn_resolve_struct_list(&members.p);
	return members;
}

int Node_SourceInfo_has_members(Node_SourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr members;
	members = capn_getp(p.p, 1, 1);
	return members.type != CAPN_NULL;
}

uint32_t Node_SourceInfo_get_startByte(Node_SourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	uint32_t startByte;
	startByte = capn_read32(p.p, 8);
	return startByte;
}

uint32_t Node_SourceInfo_get_endByte(Node_SourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	uint32_t endByte;
	endByte = capn_read32(p.p, 12);
	return endByte;
}

void Node_SourceInfo_set_id(Node_SourceInfo_ptr p, uint64_t id)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 0, id);
}

void Node_SourceInfo_set_docComment(Node_SourceInfo_ptr p, capn_text docComment)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (docComment.str != capn_val0.str) ? docComment : capn_text_null);
}

void Node_SourceInfo_set_members(Node_SourceInfo_ptr p, Node_SourceInfo_Member_list members)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (members.p.type != CAPN_NULL) ? members.p : capn_null);
}

void Node_SourceInfo_set_startByte(Node_SourceInfo_ptr p, uint32_t startByte)
{
	capn_resolve(&p.p);
	capn_write32(p.p, 8, startByte);
}

void Node_SourceInfo_set_endByte(Node_SourceInfo_ptr p, uint32_t endByte)
{
	capn_resolve(&p.p);
	capn_write32(p.p, 12, endByte);
}

Node_SourceInfo_Member_ptr new_Node_SourceInfo_Member(struct capn_segment *s) {
	Node_SourceInfo_Member_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Node_SourceInfo_Member_list new_Node_SourceInfo_Member_list(struct capn_segment *s, int len) {
	Node_SourceInfo_Member_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
	return p;
}
void read_Node_SourceInfo_Member(struct Node_SourceInfo_Member *s capnp_unused, Node_SourceInfo_Member_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->docComment = capn_get_text(p.p, 0, capn_val0);
}
void write_Node_SourceInfo_Member(const struct Node_SourceInfo_Member *s capnp_unused, Node_SourceInfo_Member_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, (s->docComment.str != capn_val0.str) ? s->docComment : capn_text_null);
}
void get_Node_SourceInfo_Member(struct Node_SourceInfo_Member *s, Node_SourceInfo_Member_list l, int i) {
	Node_SourceInfo_Member_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Node_SourceInfo_Member(s, p);
}
void set_Node_SourceInfo_Member(const struct Node_SourceInfo_Member *s, Node_SourceInfo_Member_list l, int i) {
	Node_SourceInfo_Member_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Node_SourceInfo_Member(s, p);
}

capn_text Node_SourceInfo_Member_get_docComment(Node_SourceInfo_Member_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text docComment;
	docComment = capn_get_text(p.p, 0, capn_val0);
	return docComment;
}

int Node_SourceInfo_Member_has_docComment(Node_SourceInfo_Member_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr docComment;
	docComment = capn_getp(p.p, 0, 1);
	return docComment.type != CAPN_NULL;
}

void Node_SourceInfo_Member_set_docComment(Node_SourceInfo_Member_ptr p, capn_text docComment)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (docComment.str != capn_val0.str) ? docComment : capn_text_null);
}

Field_ptr new_Field(struct capn_segment *s) {
	Field_ptr p;
	p.p = capn_new_struct(s, 24, 4);
	return p;
}
Field_list new_Field_list(struct capn_segment *s, int len) {
	Field_list p;
	p.p = capn_new_struct_list(s, len, 24, 4);
	return p;
}
void read_Field(struct Field *s capnp_unused, Field_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->name = capn_get_text(p.p, 0, capn_val0);
	s->codeOrder = capn_read16(p.p, 0);
	s->annotations.p = capn_getp(p.p, 1, 0);
if (s->annotations.p.type == CAPN_FAR_POINTER && s->annotations.p.data != NULL && *(const uint64_t*)s->annotations.p.data == 0) s->annotations.p.type = CAPN_NULL;
	s->discriminantValue = capn_read16(p.p, 2) ^ 65535u;
	s->which = (enum Field_which)(int) capn_read16(p.p, 8);
	switch (s->which) {
	case Field_slot:
		s->slot.offset = capn_read32(p.p, 4);
		s->slot.type.p = capn_getp(p.p, 2, 0);
if (s->slot.type.p.type == CAPN_FAR_POINTER && s->slot.type.p.data != NULL && *(const uint64_t*)s->slot.type.p.data == 0) s->slot.type.p.type = CAPN_NULL;
		s->slot.defaultValue.p = capn_getp(p.p, 3, 0);
if (s->slot.defaultValue.p.type == CAPN_FAR_POINTER && s->slot.defaultValue.p.data != NULL && *(const uint64_t*)s->slot.defaultValue.p.data == 0) s->slot.defaultValue.p.type = CAPN_NULL;
		s->slot.hadExplicitDefault = (capn_read8(p.p, 16) & 1) != 0;
		break;
	case Field_group:
		s->group.typeId = capn_read64(p.p, 16);
		break;
	default:
		break;
	}
	s->ordinal_which = (enum Field_ordinal_which)(int) capn_read16(p.p, 10);
	switch (s->ordinal_which) {
	case Field_ordinal__explicit:
		s->ordinal._explicit = capn_read16(p.p, 12);
		break;
	default:
		break;
	}
}
void write_Field(const struct Field *s capnp_unused, Field_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, (s->name.str != capn_val0.str) ? s->name : capn_text_null);
	capn_write16(p.p, 0, s->codeOrder);
	capn_setp(p.p, 1, (s->annotations.p.type != CAPN_NULL) ? s->annotations.p : capn_null);
	capn_write16(p.p, 2, s->discriminantValue ^ 65535u);
	capn_write16(p.p, 8, s->which);
	switch (s->which) {
	case Field_slot:
		capn_write32(p.p, 4, s->slot.offset);
		capn_setp(p.p, 2, (s->slot.type.p.type != CAPN_NULL) ? s->slot.type.p : capn_null);
		capn_setp(p.p, 3, (s->slot.defaultValue.p.type != CAPN_NULL) ? s->slot.defaultValue.p : capn_null);
		capn_write1(p.p, 128, s->slot.hadExplicitDefault != 0);
		break;
	case Field_group:
		capn_write64(p.p, 16, s->group.typeId);
		break;
	default:
		break;
	}
	capn_write16(p.p, 10, s->ordinal_which);
	switch (s->ordinal_which) {
	case Field_ordinal__explicit:
		capn_write16(p.p, 12, s->ordinal._explicit);
		break;
	default:
		break;
	}
}
void get_Field(struct Field *s, Field_list l, int i) {
	Field_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Field(s, p);
}
void set_Field(const struct Field *s, Field_list l, int i) {
	Field_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Field(s, p);
}

capn_text Field_get_name(Field_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

int Field_has_name(Field_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr name;
	name = capn_getp(p.p, 0, 1);
	return name.type != CAPN_NULL;
}

uint16_t Field_get_codeOrder(Field_ptr p)
{
	capn_resolve_struct(&p.p);
	uint16_t codeOrder;
	codeOrder = capn_read16(p.p, 0);
	return codeOrder;
}

Annotation_list Field_get_annotations(Field_ptr p)
{
	capn_resolve_struct(&p.p);
	Annotation_list annotations;
	annotations.p = capn_getp(p.p, 1, 1);
	capn_resolve_struct_list(&annotations.p);
	return annotations;
}

int Field_has_annotations(Field_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr annotations;
	annotations = capn_getp(p.p, 1, 1);
	return annotations.type != CAPN_NULL;
}

uint16_t Field_get_discriminantValue(Field_ptr p)
{
	capn_resolve_struct(&p.p);
	uint16_t discriminantValue;
	discriminantValue = capn_read16(p.p, 2) ^ 65535u;
	return discriminantValue;
}

void Field_set_name(Field_ptr p, capn_text name)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (name.str != capn_val0.str) ? name : capn_text_null);
}

void Field_set_codeOrder(Field_ptr p, uint16_t codeOrder)
{
	capn_resolve(&p.p);
	capn_write16(p.p, 0, codeOrder);
}

void Field_set_annotations(Field_ptr p, Annotation_list annotations)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (annotations.p.type != CAPN_NULL) ? annotations.p : capn_null);
}

void Field_set_discriminantValue(Field_ptr p, uint16_t discriminantValue)
{
	capn_resolve(&p.p);
	capn_write16(p.p, 2, discriminantValue ^ 65535u);
}

Enumerant_ptr new_Enumerant(struct capn_segment *s) {
	Enumerant_ptr p;
	p.p = capn_new_struct(s, 8, 2);
	return p;
}
Enumerant_list new_Enumerant_list(struct capn_segment *s, int len) {
	Enumerant_list p;
	p.p = capn_new_struct_list(s, len, 8, 2);
	return p;
}
void read_Enumerant(struct Enumerant *s capnp_unused, Enumerant_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->name = capn_get_text(p.p, 0, capn_val0);
	s->codeOrder = capn_read16(p.p, 0);
	s->annotations.p = capn_getp(p.p, 1, 0);
if (s->annotations.p.type == CAPN_FAR_POINTER && s->annotations.p.data != NULL && *(const uint64_t*)s->annotations.p.data == 0) s->annotations.p.type = CAPN_NULL;
}
void write_Enumerant(const struct Enumerant *s capnp_unused, Enumerant_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, (s->name.str != capn_val0.str) ? s->name : capn_text_null);
	capn_write16(p.p, 0, s->codeOrder);
	capn_setp(p.p, 1, (s->annotations.p.type != CAPN_NULL) ? s->annotations.p : capn_null);
}
void get_Enumerant(struct Enumerant *s, Enumerant_list l, int i) {
	Enumerant_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Enumerant(s, p);
}
void set_Enumerant(const struct Enumerant *s, Enumerant_list l, int i) {
	Enumerant_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Enumerant(s, p);
}

capn_text Enumerant_get_name(Enumerant_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

int Enumerant_has_name(Enumerant_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr name;
	name = capn_getp(p.p, 0, 1);
	return name.type != CAPN_NULL;
}

uint16_t Enumerant_get_codeOrder(Enumerant_ptr p)
{
	capn_resolve_struct(&p.p);
	uint16_t codeOrder;
	codeOrder = capn_read16(p.p, 0);
	return codeOrder;
}

Annotation_list Enumerant_get_annotations(Enumerant_ptr p)
{
	capn_resolve_struct(&p.p);
	Annotation_list annotations;
	annotations.p = capn_getp(p.p, 1, 1);
	capn_resolve_struct_list(&annotations.p);
	return annotations;
}

int Enumerant_has_annotations(Enumerant_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr annotations;
	annotations = capn_getp(p.p, 1, 1);
	return annotations.type != CAPN_NULL;
}

void Enumerant_set_name(Enumerant_ptr p, capn_text name)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (name.str != capn_val0.str) ? name : capn_text_null);
}

void Enumerant_set_codeOrder(Enumerant_ptr p, uint16_t codeOrder)
{
	capn_resolve(&p.p);
	capn_write16(p.p, 0, codeOrder);
}

void Enumerant_set_annotations(Enumerant_ptr p, Annotation_list annotations)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (annotations.p.type != CAPN_NULL) ? annotations.p : capn_null);
}

Superclass_ptr new_Superclass(struct capn_segment *s) {
	Superclass_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Superclass_list new_Superclass_list(struct capn_segment *s, int len) {
	Superclass_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Superclass(struct Superclass *s capnp_unused, Superclass_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->id = capn_read64(p.p, 0);
	s->brand.p = capn_getp(p.p, 0, 0);
if (s->brand.p.type == CAPN_FAR_POINTER && s->brand.p.data != NULL && *(const uint64_t*)s->brand.p.data == 0) s->brand.p.type = CAPN_NULL;
}
void write_Superclass(const struct Superclass *s capnp_unused, Superclass_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->id);
	capn_setp(p.p, 0, (s->brand.p.type != CAPN_NULL) ? s->brand.p : capn_null);
}
void get_Superclass(struct Superclass *s, Superclass_list l, int i) {
	Superclass_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Superclass(s, p);
}
void set_Superclass(const struct Superclass *s, Superclass_list l, int i) {
	Superclass_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Superclass(s, p);
}

uint64_t Superclass_get_id(Superclass_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t id;
	id = capn_read64(p.p, 0);
	return id;
}

Brand_ptr Superclass_get_brand(Superclass_ptr p)
{
	capn_resolve_struct(&p.p);
	Brand_ptr brand;
	brand.p = capn_getp(p.p, 0, 1);
	capn_resolve_struct(&brand.p);
	return brand;
}

int Superclass_has_brand(Superclass_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr brand;
	brand = capn_getp(p.p, 0, 1);
	return brand.type != CAPN_NULL;
}

void Superclass_set_id(Superclass_ptr p, uint64_t id)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 0, id);
}

void Superclass_set_brand(Superclass_ptr p, Brand_ptr brand)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, (brand.p.type != CAPN_NULL) ? brand.p : capn_null);
}

Method_ptr new_Method(struct capn_segment *s) {
	Method_ptr p;
	p.p = capn_new_struct(s, 24, 5);
	return p;
}
Method_list new_Method_list(struct capn_segment *s, int len) {
	Method_list p;
	p.p = capn_new_struct_list(s, len, 24, 5);
	return p;
}
void read_Method(struct Method *s capnp_unused, Method_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->name = capn_get_text(p.p, 0, capn_val0);
	s->codeOrder = capn_read16(p.p, 0);
	s->implicitParameters.p = capn_getp(p.p, 4, 0);
if (s->implicitParameters.p.type == CAPN_FAR_POINTER && s->implicitParameters.p.data != NULL && *(const uint64_t*)s->implicitParameters.p.data == 0) s->implicitParameters.p.type = CAPN_NULL;
	s->paramStructType = capn_read64(p.p, 8);
	s->paramBrand.p = capn_getp(p.p, 2, 0);
if (s->paramBrand.p.type == CAPN_FAR_POINTER && s->paramBrand.p.data != NULL && *(const uint64_t*)s->paramBrand.p.data == 0) s->paramBrand.p.type = CAPN_NULL;
	s->resultStructType = capn_read64(p.p, 16);
	s->resultBrand.p = capn_getp(p.p, 3, 0);
if (s->resultBrand.p.type == CAPN_FAR_POINTER && s->resultBrand.p.data != NULL && *(const uint64_t*)s->resultBrand.p.data == 0) s->resultBrand.p.type = CAPN_NULL;
	s->annotations.p = capn_getp(p.p, 1, 0);
if (s->annotations.p.type == CAPN_FAR_POINTER && s->annotations.p.data != NULL && *(const uint64_t*)s->annotations.p.data == 0) s->annotations.p.type = CAPN_NULL;
}
void write_Method(const struct Method *s capnp_unused, Method_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, (s->name.str != capn_val0.str) ? s->name : capn_text_null);
	capn_write16(p.p, 0, s->codeOrder);
	capn_setp(p.p, 4, (s->implicitParameters.p.type != CAPN_NULL) ? s->implicitParameters.p : capn_null);
	capn_write64(p.p, 8, s->paramStructType);
	capn_setp(p.p, 2, (s->paramBrand.p.type != CAPN_NULL) ? s->paramBrand.p : capn_null);
	capn_write64(p.p, 16, s->resultStructType);
	capn_setp(p.p, 3, (s->resultBrand.p.type != CAPN_NULL) ? s->resultBrand.p : capn_null);
	capn_setp(p.p, 1, (s->annotations.p.type != CAPN_NULL) ? s->annotations.p : capn_null);
}
void get_Method(struct Method *s, Method_list l, int i) {
	Method_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Method(s, p);
}
void set_Method(const struct Method *s, Method_list l, int i) {
	Method_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Method(s, p);
}

capn_text Method_get_name(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

int Method_has_name(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr name;
	name = capn_getp(p.p, 0, 1);
	return name.type != CAPN_NULL;
}

uint16_t Method_get_codeOrder(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	uint16_t codeOrder;
	codeOrder = capn_read16(p.p, 0);
	return codeOrder;
}

Node_Parameter_list Method_get_implicitParameters(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	Node_Parameter_list implicitParameters;
	implicitParameters.p = capn_getp(p.p, 4, 1);
	capn_resolve_struct_list(&implicitParameters.p);
	return implicitParameters;
}

int Method_has_implicitParameters(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr implicitParameters;
	implicitParameters = capn_getp(p.p, 4, 1);
	return implicitParameters.type != CAPN_NULL;
}

uint64_t Method_get_paramStructType(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t paramStructType;
	paramStructType = capn_read64(p.p, 8);
	return paramStructType;
}

Brand_ptr Method_get_paramBrand(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	Brand_ptr paramBrand;
	paramBrand.p = capn_getp(p.p, 2, 1);
	capn_resolve_struct(&paramBrand.p);
	return paramBrand;
}

int Method_has_paramBrand(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr paramBrand;
	paramBrand = capn_getp(p.p, 2, 1);
	return paramBrand.type != CAPN_NULL;
}

uint64_t Method_get_resultStructType(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t resultStructType;
	resultStructType = capn_read64(p.p, 16);
	return resultStructType;
}

Brand_ptr Method_get_resultBrand(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	Brand_ptr resultBrand;
	resultBrand.p = capn_getp(p.p, 3, 1);
	capn_resolve_struct(&resultBrand.p);
	return resultBrand;
}

int Method_has_resultBrand(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr resultBrand;
	resultBrand = capn_getp(p.p, 3, 1);
	return resultBrand.type != CAPN_NULL;
}

Annotation_list Method_get_annotations(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	Annotation_list annotations;
	annotations.p = capn_getp(p.p, 1, 1);
	capn_resolve_struct_list(&annotations.p);
	return annotations;
}

int Method_has_annotations(Method_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr annotations;
	annotations = capn_getp(p.p, 1, 1);
	return annotations.type != CAPN_NULL;
}

void Method_set_name(Method_ptr p, capn_text name)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (name.str != capn_val0.str) ? name : capn_text_null);
}

void Method_set_codeOrder(Method_ptr p, uint16_t codeOrder)
{
	capn_resolve(&p.p);
	capn_write16(p.p, 0, codeOrder);
}

void Method_set_implicitParameters(Method_ptr p, Node_Parameter_list implicitParameters)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 4, (implicitParameters.p.type != CAPN_NULL) ? implicitParameters.p : capn_null);
}

void Method_set_paramStructType(Method_ptr p, uint64_t paramStructType)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 8, paramStructType);
}

void Method_set_paramBrand(Method_ptr p, Brand_ptr paramBrand)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 2, (paramBrand.p.type != CAPN_NULL) ? paramBrand.p : capn_null);
}

void Method_set_resultStructType(Method_ptr p, uint64_t resultStructType)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 16, resultStructType);
}

void Method_set_resultBrand(Method_ptr p, Brand_ptr resultBrand)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 3, (resultBrand.p.type != CAPN_NULL) ? resultBrand.p : capn_null);
}

void Method_set_annotations(Method_ptr p, Annotation_list annotations)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (annotations.p.type != CAPN_NULL) ? annotations.p : capn_null);
}

Type_ptr new_Type(struct capn_segment *s) {
	Type_ptr p;
	p.p = capn_new_struct(s, 24, 1);
	return p;
}
Type_list new_Type_list(struct capn_segment *s, int len) {
	Type_list p;
	p.p = capn_new_struct_list(s, len, 24, 1);
	return p;
}
void read_Type(struct Type *s capnp_unused, Type_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->which = (enum Type_which)(int) capn_read16(p.p, 0);
	switch (s->which) {
	case Type__list:
		s->_list.elementType.p = capn_getp(p.p, 0, 0);
if (s->_list.elementType.p.type == CAPN_FAR_POINTER && s->_list.elementType.p.data != NULL && *(const uint64_t*)s->_list.elementType.p.data == 0) s->_list.elementType.p.type = CAPN_NULL;
		break;
	case Type__enum:
		s->_enum.typeId = capn_read64(p.p, 8);
		s->_enum.brand.p = capn_getp(p.p, 0, 0);
if (s->_enum.brand.p.type == CAPN_FAR_POINTER && s->_enum.brand.p.data != NULL && *(const uint64_t*)s->_enum.brand.p.data == 0) s->_enum.brand.p.type = CAPN_NULL;
		break;
	case Type__struct:
		s->_struct.typeId = capn_read64(p.p, 8);
		s->_struct.brand.p = capn_getp(p.p, 0, 0);
if (s->_struct.brand.p.type == CAPN_FAR_POINTER && s->_struct.brand.p.data != NULL && *(const uint64_t*)s->_struct.brand.p.data == 0) s->_struct.brand.p.type = CAPN_NULL;
		break;
	case Type__interface:
		s->_interface.typeId = capn_read64(p.p, 8);
		s->_interface.brand.p = capn_getp(p.p, 0, 0);
if (s->_interface.brand.p.type == CAPN_FAR_POINTER && s->_interface.brand.p.data != NULL && *(const uint64_t*)s->_interface.brand.p.data == 0) s->_interface.brand.p.type = CAPN_NULL;
		break;
	case Type_anyPointer:
		s->anyPointer.which = (enum Type_anyPointer_which)(int) capn_read16(p.p, 8);
		switch (s->anyPointer.which) {
		case Type_anyPointer_unconstrained:
			s->anyPointer.unconstrained.which = (enum Type_anyPointer_unconstrained_which)(int) capn_read16(p.p, 10);
			switch (s->anyPointer.unconstrained.which) {
			default:
				break;
			}
			break;
		case Type_anyPointer_parameter:
			s->anyPointer.parameter.scopeId = capn_read64(p.p, 16);
			s->anyPointer.parameter.parameterIndex = capn_read16(p.p, 10);
			break;
		case Type_anyPointer_implicitMethodParameter:
			s->anyPointer.implicitMethodParameter.parameterIndex = capn_read16(p.p, 10);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
void write_Type(const struct Type *s capnp_unused, Type_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, s->which);
	switch (s->which) {
	case Type__list:
		capn_setp(p.p, 0, (s->_list.elementType.p.type != CAPN_NULL) ? s->_list.elementType.p : capn_null);
		break;
	case Type__enum:
		capn_write64(p.p, 8, s->_enum.typeId);
		capn_setp(p.p, 0, (s->_enum.brand.p.type != CAPN_NULL) ? s->_enum.brand.p : capn_null);
		break;
	case Type__struct:
		capn_write64(p.p, 8, s->_struct.typeId);
		capn_setp(p.p, 0, (s->_struct.brand.p.type != CAPN_NULL) ? s->_struct.brand.p : capn_null);
		break;
	case Type__interface:
		capn_write64(p.p, 8, s->_interface.typeId);
		capn_setp(p.p, 0, (s->_interface.brand.p.type != CAPN_NULL) ? s->_interface.brand.p : capn_null);
		break;
	case Type_anyPointer:
		capn_write16(p.p, 8, s->anyPointer.which);
		switch (s->anyPointer.which) {
		case Type_anyPointer_unconstrained:
			capn_write16(p.p, 10, s->anyPointer.unconstrained.which);
			switch (s->anyPointer.unconstrained.which) {
			default:
				break;
			}
			break;
		case Type_anyPointer_parameter:
			capn_write64(p.p, 16, s->anyPointer.parameter.scopeId);
			capn_write16(p.p, 10, s->anyPointer.parameter.parameterIndex);
			break;
		case Type_anyPointer_implicitMethodParameter:
			capn_write16(p.p, 10, s->anyPointer.implicitMethodParameter.parameterIndex);
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
void get_Type(struct Type *s, Type_list l, int i) {
	Type_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Type(s, p);
}
void set_Type(const struct Type *s, Type_list l, int i) {
	Type_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Type(s, p);
}

Brand_ptr new_Brand(struct capn_segment *s) {
	Brand_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
Brand_list new_Brand_list(struct capn_segment *s, int len) {
	Brand_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
	return p;
}
void read_Brand(struct Brand *s capnp_unused, Brand_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->scopes.p = capn_getp(p.p, 0, 0);
if (s->scopes.p.type == CAPN_FAR_POINTER && s->scopes.p.data != NULL && *(const uint64_t*)s->scopes.p.data == 0) s->scopes.p.type = CAPN_NULL;
}
void write_Brand(const struct Brand *s capnp_unused, Brand_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->scopes.p.type != CAPN_NULL) ? s->scopes.p : capn_null);
}
void get_Brand(struct Brand *s, Brand_list l, int i) {
	Brand_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Brand(s, p);
}
void set_Brand(const struct Brand *s, Brand_list l, int i) {
	Brand_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Brand(s, p);
}

Brand_Scope_list Brand_get_scopes(Brand_ptr p)
{
	capn_resolve_struct(&p.p);
	Brand_Scope_list scopes;
	scopes.p = capn_getp(p.p, 0, 1);
	capn_resolve_struct_list(&scopes.p);
	return scopes;
}

int Brand_has_scopes(Brand_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr scopes;
	scopes = capn_getp(p.p, 0, 1);
	return scopes.type != CAPN_NULL;
}

void Brand_set_scopes(Brand_ptr p, Brand_Scope_list scopes)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, (scopes.p.type != CAPN_NULL) ? scopes.p : capn_null);
}

Brand_Scope_ptr new_Brand_Scope(struct capn_segment *s) {
	Brand_Scope_ptr p;
	p.p = capn_new_struct(s, 16, 1);
	return p;
}
Brand_Scope_list new_Brand_Scope_list(struct capn_segment *s, int len) {
	Brand_Scope_list p;
	p.p = capn_new_struct_list(s, len, 16, 1);
	return p;
}
void read_Brand_Scope(struct Brand_Scope *s capnp_unused, Brand_Scope_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->scopeId = capn_read64(p.p, 0);
	s->which = (enum Brand_Scope_which)(int) capn_read16(p.p, 8);
	switch (s->which) {
	case Brand_Scope_bind:
		s->bind.p = capn_getp(p.p, 0, 0);
if (s->bind.p.type == CAPN_FAR_POINTER && s->bind.p.data != NULL && *(const uint64_t*)s->bind.p.data == 0) s->bind.p.type = CAPN_NULL;
		break;
	default:
		break;
	}
}
void write_Brand_Scope(const struct Brand_Scope *s capnp_unused, Brand_Scope_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->scopeId);
	capn_write16(p.p, 8, s->which);
	switch (s->which) {
	case Brand_Scope_bind:
		capn_setp(p.p, 0, (s->bind.p.type != CAPN_NULL) ? s->bind.p : capn_null);
		break;
	default:
		break;
	}
}
void get_Brand_Scope(struct Brand_Scope *s, Brand_Scope_list l, int i) {
	Brand_Scope_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Brand_Scope(s, p);
}
void set_Brand_Scope(const struct Brand_Scope *s, Brand_Scope_list l, int i) {
	Brand_Scope_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Brand_Scope(s, p);
}

uint64_t Brand_Scope_get_scopeId(Brand_Scope_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t scopeId;
	scopeId = capn_read64(p.p, 0);
	return scopeId;
}

int Brand_Scope_has_bind(Brand_Scope_ptr p)
{
	capn_resolve_struct(&p.p);
	if ((enum Brand_Scope_which)(int) capn_read16(p.p, 8) != Brand_Scope_bind)
		return 0;
	capn_ptr bind;
	bind = capn_getp(p.p, 0, 1);
	return bind.type != CAPN_NULL;
}

void Brand_Scope_set_scopeId(Brand_Scope_ptr p, uint64_t scopeId)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 0, scopeId);
}

Brand_Binding_ptr new_Brand_Binding(struct capn_segment *s) {
	Brand_Binding_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
Brand_Binding_list new_Brand_Binding_list(struct capn_segment *s, int len) {
	Brand_Binding_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_Brand_Binding(struct Brand_Binding *s capnp_unused, Brand_Binding_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->which = (enum Brand_Binding_which)(int) capn_read16(p.p, 0);
	switch (s->which) {
	case Brand_Binding_type:
		s->type.p = capn_getp(p.p, 0, 0);
if (s->type.p.type == CAPN_FAR_POINTER && s->type.p.data != NULL && *(const uint64_t*)s->type.p.data == 0) s->type.p.type = CAPN_NULL;
		break;
	default:
		break;
	}
}
void write_Brand_Binding(const struct Brand_Binding *s capnp_unused, Brand_Binding_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, s->which);
	switch (s->which) {
	case Brand_Binding_type:
		capn_setp(p.p, 0, (s->type.p.type != CAPN_NULL) ? s->type.p : capn_null);
		break;
	default:
		break;
	}
}
void get_Brand_Binding(struct Brand_Binding *s, Brand_Binding_list l, int i) {
	Brand_Binding_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Brand_Binding(s, p);
}
void set_Brand_Binding(const struct Brand_Binding *s, Brand_Binding_list l, int i) {
	Brand_Binding_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Brand_Binding(s, p);
}

int Brand_Binding_has_type(Brand_Binding_ptr p)
{
	capn_resolve_struct(&p.p);
	if ((enum Brand_Binding_which)(int) capn_read16(p.p, 0) != Brand_Binding_type)
		return 0;
	capn_ptr type;
	type = capn_getp(p.p, 0, 1);
	return type.type != CAPN_NULL;
}

Value_ptr new_Value(struct capn_segment *s) {
	Value_ptr p;
	p.p = capn_new_struct(s, 16, 1);
	return p;
}
Value_list new_Value_list(struct capn_segment *s, int len) {
	Value_list p;
	p.p = capn_new_struct_list(s, len, 16, 1);
	return p;
}
void read_Value(struct Value *s capnp_unused, Value_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->which = (enum Value_which)(int) capn_read16(p.p, 0);
	switch (s->which) {
	case Value__bool:
		s->_bool = (capn_read8(p.p, 2) & 1) != 0;
		break;
	case Value_int8:
		s->int8 = (int8_t) ((int8_t)capn_read8(p.p, 2));
		break;
	case Value_uint8:
		s->uint8 = capn_read8(p.p, 2);
		break;
	case Value_int16:
		s->int16 = (int16_t) ((int16_t)capn_read16(p.p, 2));
		break;
	case Value_uint16:
		s->uint16 = capn_read16(p.p, 2);
		break;
	case Value__enum:
		s->_enum = capn_read16(p.p, 2);
		break;
	case Value_int32:
		s->int32 = (int32_t) ((int32_t)capn_read32(p.p, 4));
		break;
	case Value_uint32:
		s->uint32 = capn_read32(p.p, 4);
		break;
	case Value_float32:
		s->float32 = capn_to_f32(capn_read32(p.p, 4));
		break;
	case Value_int64:
		s->int64 = (int64_t) ((int64_t)(capn_read64(p.p, 8)));
		break;
	case Value_uint64:
		s->uint64 = capn_read64(p.p, 8);
		break;
	case Value_float64:
		s->float64 = capn_to_f64(capn_read64(p.p, 8));
		break;
	case Value_text:
		s->text = capn_get_text(p.p, 0, capn_val0);
		break;
	case Value_data:
		s->data = capn_get_data(p.p, 0);
		break;
	case Value__list:
		s->_list = capn_getp(p.p, 0, 0);
if (s->_list.type == CAPN_FAR_POINTER && s->_list.data != NULL && *(const uint64_t*)s->_list.data == 0) s->_list.type = CAPN_NULL;
		break;
	case Value__struct:
		s->_struct = capn_getp(p.p, 0, 0);
if (s->_struct.type == CAPN_FAR_POINTER && s->_struct.data != NULL && *(const uint64_t*)s->_struct.data == 0) s->_struct.type = CAPN_NULL;
		break;
	case Value_anyPointer:
		s->anyPointer = capn_getp(p.p, 0, 0);
if (s->anyPointer.type == CAPN_FAR_POINTER && s->anyPointer.data != NULL && *(const uint64_t*)s->anyPointer.data == 0) s->anyPointer.type = CAPN_NULL;
		break;
	default:
		break;
	}
}
void write_Value(const struct Value *s capnp_unused, Value_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, s->which);
	switch (s->which) {
	case Value__bool:
		capn_write1(p.p, 16, s->_bool != 0);
		break;
	case Value_int8:
		capn_write8(p.p, 2, (uint8_t) (s->int8));
		break;
	case Value_uint8:
		capn_write8(p.p, 2, s->uint8);
		break;
	case Value_int16:
		capn_write16(p.p, 2, (uint16_t) (s->int16));
		break;
	case Value_uint16:
		capn_write16(p.p, 2, s->uint16);
		break;
	case Value__enum:
		capn_write16(p.p, 2, s->_enum);
		break;
	case Value_int32:
		capn_write32(p.p, 4, (uint32_t) (s->int32));
		break;
	case Value_uint32:
		capn_write32(p.p, 4, s->uint32);
		break;
	case Value_float32:
		capn_write32(p.p, 4, capn_from_f32(s->float32));
		break;
	case Value_int64:
		capn_write64(p.p, 8, (uint64_t) (s->int64));
		break;
	case Value_uint64:
		capn_write64(p.p, 8, s->uint64);
		break;
	case Value_float64:
		capn_write64(p.p, 8, capn_from_f64(s->float64));
		break;
	case Value_text:
		capn_set_text(p.p, 0, (s->text.str != capn_val0.str) ? s->text : capn_text_null);
		break;
	case Value_data:
		capn_set_data(p.p, 0, s->data);
		break;
	case Value__list:
		capn_setp(p.p, 0, (s->_list.type != CAPN_NULL) ? s->_list : capn_null);
		break;
	case Value__struct:
		capn_setp(p.p, 0, (s->_struct.type != CAPN_NULL) ? s->_struct : capn_null);
		break;
	case Value_anyPointer:
		capn_setp(p.p, 0, (s->anyPointer.type != CAPN_NULL) ? s->anyPointer : capn_null);
		break;
	default:
		break;
	}
}
void get_Value(struct Value *s, Value_list l, int i) {
	Value_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Value(s, p);
}
void set_Value(const struct Value *s, Value_list l, int i) {
	Value_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Value(s, p);
}

int Value_has_text(Value_ptr p)
{
	capn_resolve_struct(&p.p);
	if ((enum Value_which)(int) capn_read16(p.p, 0) != Value_text)
		return 0;
	capn_ptr text;
	text = capn_getp(p.p, 0, 1);
	return text.type != CAPN_NULL;
}

int Value_has_data(Value_ptr p)
{
	capn_resolve_struct(&p.p);
	if ((enum Value_which)(int) capn_read16(p.p, 0) != Value_data)
		return 0;
	capn_ptr data;
	data = capn_getp(p.p, 0, 1);
	return data.type != CAPN_NULL;
}

int Value_has__list(Value_ptr p)
{
	capn_resolve_struct(&p.p);
	if ((enum Value_which)(int) capn_read16(p.p, 0) != Value__list)
		return 0;
	capn_ptr _list;
	_list = capn_getp(p.p, 0, 1);
	return _list.type != CAPN_NULL;
}

int Value_has__struct(Value_ptr p)
{
	capn_resolve_struct(&p.p);
	if ((enum Value_which)(int) capn_read16(p.p, 0) != Value__struct)
		return 0;
	capn_ptr _struct;
	_struct = capn_getp(p.p, 0, 1);
	return _struct.type != CAPN_NULL;
}

int Value_has_anyPointer(Value_ptr p)
{
	capn_resolve_struct(&p.p);
	if ((enum Value_which)(int) capn_read16(p.p, 0) != Value_anyPointer)
		return 0;
	capn_ptr anyPointer;
	anyPointer = capn_getp(p.p, 0, 1);
	return anyPointer.type != CAPN_NULL;
}

Annotation_ptr new_Annotation(struct capn_segment *s) {
	Annotation_ptr p;
	p.p = capn_new_struct(s, 8, 2);
	return p;
}
Annotation_list new_Annotation_list(struct capn_segment *s, int len) {
	Annotation_list p;
	p.p = capn_new_struct_list(s, len, 8, 2);
	return p;
}
void read_Annotation(struct Annotation *s capnp_unused, Annotation_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->id = capn_read64(p.p, 0);
	s->brand.p = capn_getp(p.p, 1, 0);
if (s->brand.p.type == CAPN_FAR_POINTER && s->brand.p.data != NULL && *(const uint64_t*)s->brand.p.data == 0) s->brand.p.type = CAPN_NULL;
	s->value.p = capn_getp(p.p, 0, 0);
if (s->value.p.type == CAPN_FAR_POINTER && s->value.p.data != NULL && *(const uint64_t*)s->value.p.data == 0) s->value.p.type = CAPN_NULL;
}
void write_Annotation(const struct Annotation *s capnp_unused, Annotation_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->id);
	capn_setp(p.p, 1, (s->brand.p.type != CAPN_NULL) ? s->brand.p : capn_null);
	capn_setp(p.p, 0, (s->value.p.type != CAPN_NULL) ? s->value.p : capn_null);
}
void get_Annotation(struct Annotation *s, Annotation_list l, int i) {
	Annotation_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_Annotation(s, p);
}
void set_Annotation(const struct Annotation *s, Annotation_list l, int i) {
	Annotation_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Annotation(s, p);
}

uint64_t Annotation_get_id(Annotation_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t id;
	id = capn_read64(p.p, 0);
	return id;
}

Brand_ptr Annotation_get_brand(Annotation_ptr p)
{
	capn_resolve_struct(&p.p);
	Brand_ptr brand;
	brand.p = capn_getp(p.p, 1, 1);
	capn_resolve_struct(&brand.p);
	return brand;
}

int Annotation_has_brand(Annotation_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr brand;
	brand = capn_getp(p.p, 1, 1);
	return brand.type != CAPN_NULL;
}

Value_ptr Annotation_get_value(Annotation_ptr p)
{
	capn_resolve_struct(&p.p);
	Value_ptr value;
	value.p = capn_getp(p.p, 0, 1);
	capn_resolve_struct(&value.p);
	return value;
}

int Annotation_has_value(Annotation_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr value;
	value = capn_getp(p.p, 0, 1);
	return value.type != CAPN_NULL;
}

void Annotation_set_id(Annotation_ptr p, uint64_t id)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 0, id);
}

void Annotation_set_brand(Annotation_ptr p, Brand_ptr brand)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (brand.p.type != CAPN_NULL) ? brand.p : capn_null);
}

void Annotation_set_value(Annotation_ptr p, Value_ptr value)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, (value.p.type != CAPN_NULL) ? value.p : capn_null);
}

CapnpVersion_ptr new_CapnpVersion(struct capn_segment *s) {
	CapnpVersion_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
CapnpVersion_list new_CapnpVersion_list(struct capn_segment *s, int len) {
	CapnpVersion_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_CapnpVersion(struct CapnpVersion *s capnp_unused, CapnpVersion_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->major = capn_read16(p.p, 0);
	s->minor = capn_read8(p.p, 2);
	s->micro = capn_read8(p.p, 3);
}
void write_CapnpVersion(const struct CapnpVersion *s capnp_unused, CapnpVersion_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, s->major);
	capn_write8(p.p, 2, s->minor);
	capn_write8(p.p, 3, s->micro);
}
void get_CapnpVersion(struct CapnpVersion *s, CapnpVersion_list l, int i) {
	CapnpVersion_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_CapnpVersion(s, p);
}
void set_CapnpVersion(const struct CapnpVersion *s, CapnpVersion_list l, int i) {
	CapnpVersion_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CapnpVersion(s, p);
}

uint16_t CapnpVersion_get_major(CapnpVersion_ptr p)
{
	capn_resolve_struct(&p.p);
	uint16_t major;
	major = capn_read16(p.p, 0);
	return major;
}

uint8_t CapnpVersion_get_minor(CapnpVersion_ptr p)
{
	capn_resolve_struct(&p.p);
	uint8_t minor;
	minor = capn_read8(p.p, 2);
	return minor;
}

uint8_t CapnpVersion_get_micro(CapnpVersion_ptr p)
{
	capn_resolve_struct(&p.p);
	uint8_t micro;
	micro = capn_read8(p.p, 3);
	return micro;
}

void CapnpVersion_set_major(CapnpVersion_ptr p, uint16_t major)
{
	capn_resolve(&p.p);
	capn_write16(p.p, 0, major);
}

void CapnpVersion_set_minor(CapnpVersion_ptr p, uint8_t minor)
{
	capn_resolve(&p.p);
	capn_write8(p.p, 2, minor);
}

void CapnpVersion_set_micro(CapnpVersion_ptr p, uint8_t micro)
{
	capn_resolve(&p.p);
	capn_write8(p.p, 3, micro);
}

CodeGeneratorRequest_ptr new_CodeGeneratorRequest(struct capn_segment *s) {
	CodeGeneratorRequest_ptr p;
	p.p = capn_new_struct(s, 0, 4);
	return p;
}
CodeGeneratorRequest_list new_CodeGeneratorRequest_list(struct capn_segment *s, int len) {
	CodeGeneratorRequest_list p;
	p.p = capn_new_struct_list(s, len, 0, 4);
	return p;
}
void read_CodeGeneratorRequest(struct CodeGeneratorRequest *s capnp_unused, CodeGeneratorRequest_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->capnpVersion.p = capn_getp(p.p, 2, 0);
if (s->capnpVersion.p.type == CAPN_FAR_POINTER && s->capnpVersion.p.data != NULL && *(const uint64_t*)s->capnpVersion.p.data == 0) s->capnpVersion.p.type = CAPN_NULL;
	s->nodes.p = capn_getp(p.p, 0, 0);
if (s->nodes.p.type == CAPN_FAR_POINTER && s->nodes.p.data != NULL && *(const uint64_t*)s->nodes.p.data == 0) s->nodes.p.type = CAPN_NULL;
	s->sourceInfo.p = capn_getp(p.p, 3, 0);
if (s->sourceInfo.p.type == CAPN_FAR_POINTER && s->sourceInfo.p.data != NULL && *(const uint64_t*)s->sourceInfo.p.data == 0) s->sourceInfo.p.type = CAPN_NULL;
	s->requestedFiles.p = capn_getp(p.p, 1, 0);
if (s->requestedFiles.p.type == CAPN_FAR_POINTER && s->requestedFiles.p.data != NULL && *(const uint64_t*)s->requestedFiles.p.data == 0) s->requestedFiles.p.type = CAPN_NULL;
}
void write_CodeGeneratorRequest(const struct CodeGeneratorRequest *s capnp_unused, CodeGeneratorRequest_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 2, (s->capnpVersion.p.type != CAPN_NULL) ? s->capnpVersion.p : capn_null);
	capn_setp(p.p, 0, (s->nodes.p.type != CAPN_NULL) ? s->nodes.p : capn_null);
	capn_setp(p.p, 3, (s->sourceInfo.p.type != CAPN_NULL) ? s->sourceInfo.p : capn_null);
	capn_setp(p.p, 1, (s->requestedFiles.p.type != CAPN_NULL) ? s->requestedFiles.p : capn_null);
}
void get_CodeGeneratorRequest(struct CodeGeneratorRequest *s, CodeGeneratorRequest_list l, int i) {
	CodeGeneratorRequest_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_CodeGeneratorRequest(s, p);
}
void set_CodeGeneratorRequest(const struct CodeGeneratorRequest *s, CodeGeneratorRequest_list l, int i) {
	CodeGeneratorRequest_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CodeGeneratorRequest(s, p);
}

CapnpVersion_ptr CodeGeneratorRequest_get_capnpVersion(CodeGeneratorRequest_ptr p)
{
	capn_resolve_struct(&p.p);
	CapnpVersion_ptr capnpVersion;
	capnpVersion.p = capn_getp(p.p, 2, 1);
	capn_resolve_struct(&capnpVersion.p);
	return capnpVersion;
}

int CodeGeneratorRequest_has_capnpVersion(CodeGeneratorRequest_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr capnpVersion;
	capnpVersion = capn_getp(p.p, 2, 1);
	return capnpVersion.type != CAPN_NULL;
}

Node_list CodeGeneratorRequest_get_nodes(CodeGeneratorRequest_ptr p)
{
	capn_resolve_struct(&p.p);
	Node_list nodes;
	nodes.p = capn_getp(p.p, 0, 1);
	capn_resolve_struct_list(&nodes.p);
	return nodes;
}

int CodeGeneratorRequest_has_nodes(CodeGeneratorRequest_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr nodes;
	nodes = capn_getp(p.p, 0, 1);
	return nodes.type != CAPN_NULL;
}

Node_SourceInfo_list CodeGeneratorRequest_get_sourceInfo(CodeGeneratorRequest_ptr p)
{
	capn_resolve_struct(&p.p);
	Node_SourceInfo_list sourceInfo;
	sourceInfo.p = capn_getp(p.p, 3, 1);
	capn_resolve_struct_list(&sourceInfo.p);
	return sourceInfo;
}

int CodeGeneratorRequest_has_sourceInfo(CodeGeneratorRequest_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr sourceInfo;
	sourceInfo = capn_getp(p.p, 3, 1);
	return sourceInfo.type != CAPN_NULL;
}

CodeGeneratorRequest_RequestedFile_list CodeGeneratorRequest_get_requestedFiles(CodeGeneratorRequest_ptr p)
{
	capn_resolve_struct(&p.p);
	CodeGeneratorRequest_RequestedFile_list requestedFiles;
	requestedFiles.p = capn_getp(p.p, 1, 1);
	capn_resolve_struct_list(&requestedFiles.p);
	return requestedFiles;
}

int CodeGeneratorRequest_has_requestedFiles(CodeGeneratorRequest_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr requestedFiles;
	requestedFiles = capn_getp(p.p, 1, 1);
	return requestedFiles.type != CAPN_NULL;
}

void CodeGeneratorRequest_set_capnpVersion(CodeGeneratorRequest_ptr p, CapnpVersion_ptr capnpVersion)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 2, (capnpVersion.p.type != CAPN_NULL) ? capnpVersion.p : capn_null);
}

void CodeGeneratorRequest_set_nodes(CodeGeneratorRequest_ptr p, Node_list nodes)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, (nodes.p.type != CAPN_NULL) ? nodes.p : capn_null);
}

void CodeGeneratorRequest_set_sourceInfo(CodeGeneratorRequest_ptr p, Node_SourceInfo_list sourceInfo)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 3, (sourceInfo.p.type != CAPN_NULL) ? sourceInfo.p : capn_null);
}

void CodeGeneratorRequest_set_requestedFiles(CodeGeneratorRequest_ptr p, CodeGeneratorRequest_RequestedFile_list requestedFiles)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (requestedFiles.p.type != CAPN_NULL) ? requestedFiles.p : capn_null);
}

CodeGeneratorRequest_RequestedFile_ptr new_CodeGeneratorRequest_RequestedFile(struct capn_segment *s) {
	CodeGeneratorRequest_RequestedFile_ptr p;
	p.p = capn_new_struct(s, 8, 3);
	return p;
}
CodeGeneratorRequest_RequestedFile_list new_CodeGeneratorRequest_RequestedFile_list(struct capn_segment *s, int len) {
	CodeGeneratorRequest_RequestedFile_list p;
	p.p = capn_new_struct_list(s, len, 8, 3);
	return p;
}
void read_CodeGeneratorRequest_RequestedFile(struct CodeGeneratorRequest_RequestedFile *s capnp_unused, CodeGeneratorRequest_RequestedFile_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->id = capn_read64(p.p, 0);
	s->filename = capn_get_text(p.p, 0, capn_val0);
	s->imports.p = capn_getp(p.p, 1, 0);
if (s->imports.p.type == CAPN_FAR_POINTER && s->imports.p.data != NULL && *(const uint64_t*)s->imports.p.data == 0) s->imports.p.type = CAPN_NULL;
	s->fileSourceInfo.p = capn_getp(p.p, 2, 0);
if (s->fileSourceInfo.p.type == CAPN_FAR_POINTER && s->fileSourceInfo.p.data != NULL && *(const uint64_t*)s->fileSourceInfo.p.data == 0) s->fileSourceInfo.p.type = CAPN_NULL;
}
void write_CodeGeneratorRequest_RequestedFile(const struct CodeGeneratorRequest_RequestedFile *s capnp_unused, CodeGeneratorRequest_RequestedFile_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->id);
	capn_set_text(p.p, 0, (s->filename.str != capn_val0.str) ? s->filename : capn_text_null);
	capn_setp(p.p, 1, (s->imports.p.type != CAPN_NULL) ? s->imports.p : capn_null);
	capn_setp(p.p, 2, (s->fileSourceInfo.p.type != CAPN_NULL) ? s->fileSourceInfo.p : capn_null);
}
void get_CodeGeneratorRequest_RequestedFile(struct CodeGeneratorRequest_RequestedFile *s, CodeGeneratorRequest_RequestedFile_list l, int i) {
	CodeGeneratorRequest_RequestedFile_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_CodeGeneratorRequest_RequestedFile(s, p);
}
void set_CodeGeneratorRequest_RequestedFile(const struct CodeGeneratorRequest_RequestedFile *s, CodeGeneratorRequest_RequestedFile_list l, int i) {
	CodeGeneratorRequest_RequestedFile_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CodeGeneratorRequest_RequestedFile(s, p);
}

uint64_t CodeGeneratorRequest_RequestedFile_get_id(CodeGeneratorRequest_RequestedFile_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t id;
	id = capn_read64(p.p, 0);
	return id;
}

capn_text CodeGeneratorRequest_RequestedFile_get_filename(CodeGeneratorRequest_RequestedFile_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text filename;
	filename = capn_get_text(p.p, 0, capn_val0);
	return filename;
}

int CodeGeneratorRequest_RequestedFile_has_filename(CodeGeneratorRequest_RequestedFile_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr filename;
	filename = capn_getp(p.p, 0, 1);
	return filename.type != CAPN_NULL;
}

CodeGeneratorRequest_RequestedFile_Import_list CodeGeneratorRequest_RequestedFile_get_imports(CodeGeneratorRequest_RequestedFile_ptr p)
{
	capn_resolve_struct(&p.p);
	CodeGeneratorRequest_RequestedFile_Import_list imports;
	imports.p = capn_getp(p.p, 1, 1);
	capn_resolve_struct_list(&imports.p);
	return imports;
}

int CodeGeneratorRequest_RequestedFile_has_imports(CodeGeneratorRequest_RequestedFile_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr imports;
	imports = capn_getp(p.p, 1, 1);
	return imports.type != CAPN_NULL;
}

CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr CodeGeneratorRequest_RequestedFile_get_fileSourceInfo(CodeGeneratorRequest_RequestedFile_ptr p)
{
	capn_resolve_struct(&p.p);
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr fileSourceInfo;
	fileSourceInfo.p = capn_getp(p.p, 2, 1);
	capn_resolve_struct(&fileSourceInfo.p);
	return fileSourceInfo;
}

int CodeGeneratorRequest_RequestedFile_has_fileSourceInfo(CodeGeneratorRequest_RequestedFile_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr fileSourceInfo;
	fileSourceInfo = capn_getp(p.p, 2, 1);
	return fileSourceInfo.type != CAPN_NULL;
}

void CodeGeneratorRequest_RequestedFile_set_id(CodeGeneratorRequest_RequestedFile_ptr p, uint64_t id)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 0, id);
}

void CodeGeneratorRequest_RequestedFile_set_filename(CodeGeneratorRequest_RequestedFile_ptr p, capn_text filename)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (filename.str != capn_val0.str) ? filename : capn_text_null);
}

void CodeGeneratorRequest_RequestedFile_set_imports(CodeGeneratorRequest_RequestedFile_ptr p, CodeGeneratorRequest_RequestedFile_Import_list imports)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 1, (imports.p.type != CAPN_NULL) ? imports.p : capn_null);
}

void CodeGeneratorRequest_RequestedFile_set_fileSourceInfo(CodeGeneratorRequest_RequestedFile_ptr p, CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr fileSourceInfo)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 2, (fileSourceInfo.p.type != CAPN_NULL) ? fileSourceInfo.p : capn_null);
}

CodeGeneratorRequest_RequestedFile_Import_ptr new_CodeGeneratorRequest_RequestedFile_Import(struct capn_segment *s) {
	CodeGeneratorRequest_RequestedFile_Import_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
CodeGeneratorRequest_RequestedFile_Import_list new_CodeGeneratorRequest_RequestedFile_Import_list(struct capn_segment *s, int len) {
	CodeGeneratorRequest_RequestedFile_Import_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_CodeGeneratorRequest_RequestedFile_Import(struct CodeGeneratorRequest_RequestedFile_Import *s capnp_unused, CodeGeneratorRequest_RequestedFile_Import_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->id = capn_read64(p.p, 0);
	s->name = capn_get_text(p.p, 0, capn_val0);
}
void write_CodeGeneratorRequest_RequestedFile_Import(const struct CodeGeneratorRequest_RequestedFile_Import *s capnp_unused, CodeGeneratorRequest_RequestedFile_Import_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->id);
	capn_set_text(p.p, 0, (s->name.str != capn_val0.str) ? s->name : capn_text_null);
}
void get_CodeGeneratorRequest_RequestedFile_Import(struct CodeGeneratorRequest_RequestedFile_Import *s, CodeGeneratorRequest_RequestedFile_Import_list l, int i) {
	CodeGeneratorRequest_RequestedFile_Import_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_CodeGeneratorRequest_RequestedFile_Import(s, p);
}
void set_CodeGeneratorRequest_RequestedFile_Import(const struct CodeGeneratorRequest_RequestedFile_Import *s, CodeGeneratorRequest_RequestedFile_Import_list l, int i) {
	CodeGeneratorRequest_RequestedFile_Import_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CodeGeneratorRequest_RequestedFile_Import(s, p);
}

uint64_t CodeGeneratorRequest_RequestedFile_Import_get_id(CodeGeneratorRequest_RequestedFile_Import_ptr p)
{
	capn_resolve_struct(&p.p);
	uint64_t id;
	id = capn_read64(p.p, 0);
	return id;
}

capn_text CodeGeneratorRequest_RequestedFile_Import_get_name(CodeGeneratorRequest_RequestedFile_Import_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

int CodeGeneratorRequest_RequestedFile_Import_has_name(CodeGeneratorRequest_RequestedFile_Import_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr name;
	name = capn_getp(p.p, 0, 1);
	return name.type != CAPN_NULL;
}

void CodeGeneratorRequest_RequestedFile_Import_set_id(CodeGeneratorRequest_RequestedFile_Import_ptr p, uint64_t id)
{
	capn_resolve(&p.p);
	capn_write64(p.p, 0, id);
}

void CodeGeneratorRequest_RequestedFile_Import_set_name(CodeGeneratorRequest_RequestedFile_Import_ptr p, capn_text name)
{
	capn_resolve(&p.p);
	capn_set_text(p.p, 0, (name.str != capn_val0.str) ? name : capn_text_null);
}

CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr new_CodeGeneratorRequest_RequestedFile_FileSourceInfo(struct capn_segment *s) {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p;
	p.p = capn_new_struct(s, 0, 1);
	return p;
}
CodeGeneratorRequest_RequestedFile_FileSourceInfo_list new_CodeGeneratorRequest_RequestedFile_FileSourceInfo_list(struct capn_segment *s, int len) {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_list p;
	p.p = capn_new_struct_list(s, len, 0, 1);
	return p;
}
void read_CodeGeneratorRequest_RequestedFile_FileSourceInfo(struct CodeGeneratorRequest_RequestedFile_FileSourceInfo *s capnp_unused, CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->identifiers.p = capn_getp(p.p, 0, 0);
if (s->identifiers.p.type == CAPN_FAR_POINTER && s->identifiers.p.data != NULL && *(const uint64_t*)s->identifiers.p.data == 0) s->identifiers.p.type = CAPN_NULL;
}
void write_CodeGeneratorRequest_RequestedFile_FileSourceInfo(const struct CodeGeneratorRequest_RequestedFile_FileSourceInfo *s capnp_unused, CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_setp(p.p, 0, (s->identifiers.p.type != CAPN_NULL) ? s->identifiers.p : capn_null);
}
void get_CodeGeneratorRequest_RequestedFile_FileSourceInfo(struct CodeGeneratorRequest_RequestedFile_FileSourceInfo *s, CodeGeneratorRequest_RequestedFile_FileSourceInfo_list l, int i) {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_CodeGeneratorRequest_RequestedFile_FileSourceInfo(s, p);
}
void set_CodeGeneratorRequest_RequestedFile_FileSourceInfo(const struct CodeGeneratorRequest_RequestedFile_FileSourceInfo *s, CodeGeneratorRequest_RequestedFile_FileSourceInfo_list l, int i) {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CodeGeneratorRequest_RequestedFile_FileSourceInfo(s, p);
}

CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list CodeGeneratorRequest_RequestedFile_FileSourceInfo_get_identifiers(CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list identifiers;
	identifiers.p = capn_getp(p.p, 0, 1);
	capn_resolve_struct_list(&identifiers.p);
	return identifiers;
}

int CodeGeneratorRequest_RequestedFile_FileSourceInfo_has_identifiers(CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p)
{
	capn_resolve_struct(&p.p);
	capn_ptr identifiers;
	identifiers = capn_getp(p.p, 0, 1);
	return identifiers.type != CAPN_NULL;
}

void CodeGeneratorRequest_RequestedFile_FileSourceInfo_set_identifiers(CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list identifiers)
{
	capn_resolve(&p.p);
	capn_setp(p.p, 0, (identifiers.p.type != CAPN_NULL) ? identifiers.p : capn_null);
}

CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr new_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(struct capn_segment *s) {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p;
	p.p = capn_new_struct(s, 24, 0);
	return p;
}
CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list new_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list(struct capn_segment *s, int len) {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list p;
	p.p = capn_new_struct_list(s, len, 24, 0);
	return p;
}
void read_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier *s capnp_unused, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p) {
	capn_resolve_struct(&p.p);
	capnp_use(s);
	s->startByte = capn_read32(p.p, 0);
	s->endByte = capn_read32(p.p, 4);
	s->which = (enum CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_which)(int) capn_read16(p.p, 16);
	switch (s->which) {
	case CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_typeId:
		s->typeId = capn_read64(p.p, 8);
		break;
	case CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_member:
		s->member.parentTypeId = capn_read64(p.p, 8);
		s->member.ordinal = capn_read16(p.p, 18);
		break;
	default:
		break;
	}
}
void write_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(const struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier *s capnp_unused, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->startByte);
	capn_write32(p.p, 4, s->endByte);
	capn_write16(p.p, 16, s->which);
	switch (s->which) {
	case CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_typeId:
		capn_write64(p.p, 8, s->typeId);
		break;
	case CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_member:
		capn_write64(p.p, 8, s->member.parentTypeId);
		capn_write16(p.p, 18, s->member.ordinal);
		break;
	default:
		break;
	}
}
void get_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier *s, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list l, int i) {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p;
	capn_resolve_struct_list(&l.p);
	p.p = capn_getp(l.p, i, 0);
	read_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(s, p);
}
void set_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(const struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier *s, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list l, int i) {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(s, p);
}

uint32_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_get_startByte(CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p)
{
	capn_resolve_struct(&p.p);
	uint32_t startByte;
	startByte = capn_read32(p.p, 0);
	return startByte;
}

uint32_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_get_endByte(CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p)
{
	capn_resolve_struct(&p.p);
	uint32_t endByte;
	endByte = capn_read32(p.p, 4);
	return endByte;
}

void CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_set_startByte(CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p, uint32_t startByte)
{
	capn_resolve(&p.p);
	capn_write32(p.p, 0, startByte);
}

void CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_set_endByte(CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p, uint32_t endByte)
{
	capn_resolve(&p.p);
	capn_write32(p.p, 4, endByte);
}
