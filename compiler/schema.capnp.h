#ifndef CAPN_A93FC509624C72D9
#define CAPN_A93FC509624C72D9
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

struct Node;
struct Node_Parameter;
struct Node_NestedNode;
struct Node_SourceInfo;
struct Node_SourceInfo_Member;
struct Field;
struct Enumerant;
struct Superclass;
struct Method;
struct Type;
struct Brand;
struct Brand_Scope;
struct Brand_Binding;
struct Value;
struct Annotation;
struct CapnpVersion;
struct CodeGeneratorRequest;
struct CodeGeneratorRequest_RequestedFile;
struct CodeGeneratorRequest_RequestedFile_Import;
struct CodeGeneratorRequest_RequestedFile_FileSourceInfo;
struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier;

typedef struct {capn_ptr p;} Node_ptr;
typedef struct {capn_ptr p;} Node_Parameter_ptr;
typedef struct {capn_ptr p;} Node_NestedNode_ptr;
typedef struct {capn_ptr p;} Node_SourceInfo_ptr;
typedef struct {capn_ptr p;} Node_SourceInfo_Member_ptr;
typedef struct {capn_ptr p;} Field_ptr;
typedef struct {capn_ptr p;} Enumerant_ptr;
typedef struct {capn_ptr p;} Superclass_ptr;
typedef struct {capn_ptr p;} Method_ptr;
typedef struct {capn_ptr p;} Type_ptr;
typedef struct {capn_ptr p;} Brand_ptr;
typedef struct {capn_ptr p;} Brand_Scope_ptr;
typedef struct {capn_ptr p;} Brand_Binding_ptr;
typedef struct {capn_ptr p;} Value_ptr;
typedef struct {capn_ptr p;} Annotation_ptr;
typedef struct {capn_ptr p;} CapnpVersion_ptr;
typedef struct {capn_ptr p;} CodeGeneratorRequest_ptr;
typedef struct {capn_ptr p;} CodeGeneratorRequest_RequestedFile_ptr;
typedef struct {capn_ptr p;} CodeGeneratorRequest_RequestedFile_Import_ptr;
typedef struct {capn_ptr p;} CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr;
typedef struct {capn_ptr p;} CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr;

typedef struct {capn_ptr p;} Node_list;
typedef struct {capn_ptr p;} Node_Parameter_list;
typedef struct {capn_ptr p;} Node_NestedNode_list;
typedef struct {capn_ptr p;} Node_SourceInfo_list;
typedef struct {capn_ptr p;} Node_SourceInfo_Member_list;
typedef struct {capn_ptr p;} Field_list;
typedef struct {capn_ptr p;} Enumerant_list;
typedef struct {capn_ptr p;} Superclass_list;
typedef struct {capn_ptr p;} Method_list;
typedef struct {capn_ptr p;} Type_list;
typedef struct {capn_ptr p;} Brand_list;
typedef struct {capn_ptr p;} Brand_Scope_list;
typedef struct {capn_ptr p;} Brand_Binding_list;
typedef struct {capn_ptr p;} Value_list;
typedef struct {capn_ptr p;} Annotation_list;
typedef struct {capn_ptr p;} CapnpVersion_list;
typedef struct {capn_ptr p;} CodeGeneratorRequest_list;
typedef struct {capn_ptr p;} CodeGeneratorRequest_RequestedFile_list;
typedef struct {capn_ptr p;} CodeGeneratorRequest_RequestedFile_Import_list;
typedef struct {capn_ptr p;} CodeGeneratorRequest_RequestedFile_FileSourceInfo_list;
typedef struct {capn_ptr p;} CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list;

enum ElementSize {
	ElementSize_empty = 0,
	ElementSize_bit = 1,
	ElementSize_byte = 2,
	ElementSize_twoBytes = 3,
	ElementSize_fourBytes = 4,
	ElementSize_eightBytes = 5,
	ElementSize_pointer = 6,
	ElementSize_inlineComposite = 7
};
#define FIELD_NO_DISCRIMINANT (65535)
extern const uint16_t Field_noDiscriminant;
enum Node_which {
	Node_file = 0,
	Node__struct = 1,
	Node__enum = 2,
	Node__interface = 3,
	Node__const = 4,
	Node_annotation = 5
};

struct Node {
	uint64_t id;
	capn_text displayName;
	uint32_t displayNamePrefixLength;
	uint64_t scopeId;
	Node_Parameter_list parameters;
	unsigned isGeneric : 1;
	Node_NestedNode_list nestedNodes;
	Annotation_list annotations;
	enum Node_which which;
	capnp_nowarn union {
		capnp_nowarn struct {
			uint16_t dataWordCount;
			uint16_t pointerCount;
			enum ElementSize preferredListEncoding;
			unsigned isGroup : 1;
			uint16_t discriminantCount;
			uint32_t discriminantOffset;
			Field_list fields;
		} _struct;
		capnp_nowarn struct {
			Enumerant_list enumerants;
		} _enum;
		capnp_nowarn struct {
			Method_list methods;
			Superclass_list superclasses;
		} _interface;
		capnp_nowarn struct {
			Type_ptr type;
			Value_ptr value;
		} _const;
		capnp_nowarn struct {
			Type_ptr type;
			unsigned targetsFile : 1;
			unsigned targetsConst : 1;
			unsigned targetsEnum : 1;
			unsigned targetsEnumerant : 1;
			unsigned targetsStruct : 1;
			unsigned targetsField : 1;
			unsigned targetsUnion : 1;
			unsigned targetsGroup : 1;
			unsigned targetsInterface : 1;
			unsigned targetsMethod : 1;
			unsigned targetsParam : 1;
			unsigned targetsAnnotation : 1;
		} annotation;
	};
	uint32_t startByte;
	uint32_t endByte;
};

static const size_t Node_word_count = 6;

static const size_t Node_pointer_count = 6;

static const size_t Node_struct_bytes_count = 96;


uint64_t Node_get_id(Node_ptr p);

capn_text Node_get_displayName(Node_ptr p);

int Node_has_displayName(Node_ptr p);

uint32_t Node_get_displayNamePrefixLength(Node_ptr p);

uint64_t Node_get_scopeId(Node_ptr p);

Node_Parameter_list Node_get_parameters(Node_ptr p);

int Node_has_parameters(Node_ptr p);

unsigned Node_get_isGeneric(Node_ptr p);

Node_NestedNode_list Node_get_nestedNodes(Node_ptr p);

int Node_has_nestedNodes(Node_ptr p);

Annotation_list Node_get_annotations(Node_ptr p);

int Node_has_annotations(Node_ptr p);

void Node_set_id(Node_ptr p, uint64_t id);

void Node_set_displayName(Node_ptr p, capn_text displayName);

void Node_set_displayNamePrefixLength(Node_ptr p, uint32_t displayNamePrefixLength);

void Node_set_scopeId(Node_ptr p, uint64_t scopeId);

void Node_set_parameters(Node_ptr p, Node_Parameter_list parameters);

void Node_set_isGeneric(Node_ptr p, unsigned isGeneric);

void Node_set_nestedNodes(Node_ptr p, Node_NestedNode_list nestedNodes);

void Node_set_annotations(Node_ptr p, Annotation_list annotations);

struct Node_Parameter {
	capn_text name;
};

static const size_t Node_Parameter_word_count = 0;

static const size_t Node_Parameter_pointer_count = 1;

static const size_t Node_Parameter_struct_bytes_count = 8;


capn_text Node_Parameter_get_name(Node_Parameter_ptr p);

int Node_Parameter_has_name(Node_Parameter_ptr p);

void Node_Parameter_set_name(Node_Parameter_ptr p, capn_text name);

struct Node_NestedNode {
	capn_text name;
	uint64_t id;
};

static const size_t Node_NestedNode_word_count = 1;

static const size_t Node_NestedNode_pointer_count = 1;

static const size_t Node_NestedNode_struct_bytes_count = 16;


capn_text Node_NestedNode_get_name(Node_NestedNode_ptr p);

int Node_NestedNode_has_name(Node_NestedNode_ptr p);

uint64_t Node_NestedNode_get_id(Node_NestedNode_ptr p);

void Node_NestedNode_set_name(Node_NestedNode_ptr p, capn_text name);

void Node_NestedNode_set_id(Node_NestedNode_ptr p, uint64_t id);

struct Node_SourceInfo {
	uint64_t id;
	capn_text docComment;
	Node_SourceInfo_Member_list members;
	uint32_t startByte;
	uint32_t endByte;
};

static const size_t Node_SourceInfo_word_count = 2;

static const size_t Node_SourceInfo_pointer_count = 2;

static const size_t Node_SourceInfo_struct_bytes_count = 32;


uint64_t Node_SourceInfo_get_id(Node_SourceInfo_ptr p);

capn_text Node_SourceInfo_get_docComment(Node_SourceInfo_ptr p);

int Node_SourceInfo_has_docComment(Node_SourceInfo_ptr p);

Node_SourceInfo_Member_list Node_SourceInfo_get_members(Node_SourceInfo_ptr p);

int Node_SourceInfo_has_members(Node_SourceInfo_ptr p);

uint32_t Node_SourceInfo_get_startByte(Node_SourceInfo_ptr p);

uint32_t Node_SourceInfo_get_endByte(Node_SourceInfo_ptr p);

void Node_SourceInfo_set_id(Node_SourceInfo_ptr p, uint64_t id);

void Node_SourceInfo_set_docComment(Node_SourceInfo_ptr p, capn_text docComment);

void Node_SourceInfo_set_members(Node_SourceInfo_ptr p, Node_SourceInfo_Member_list members);

void Node_SourceInfo_set_startByte(Node_SourceInfo_ptr p, uint32_t startByte);

void Node_SourceInfo_set_endByte(Node_SourceInfo_ptr p, uint32_t endByte);

struct Node_SourceInfo_Member {
	capn_text docComment;
};

static const size_t Node_SourceInfo_Member_word_count = 0;

static const size_t Node_SourceInfo_Member_pointer_count = 1;

static const size_t Node_SourceInfo_Member_struct_bytes_count = 8;


capn_text Node_SourceInfo_Member_get_docComment(Node_SourceInfo_Member_ptr p);

int Node_SourceInfo_Member_has_docComment(Node_SourceInfo_Member_ptr p);

void Node_SourceInfo_Member_set_docComment(Node_SourceInfo_Member_ptr p, capn_text docComment);
enum Field_which {
	Field_slot = 0,
	Field_group = 1
};
enum Field_ordinal_which {
	Field_ordinal_implicit = 0,
	Field_ordinal__explicit = 1
};

struct Field {
	capn_text name;
	uint16_t codeOrder;
	Annotation_list annotations;
	uint16_t discriminantValue;
	enum Field_which which;
	capnp_nowarn union {
		capnp_nowarn struct {
			uint32_t offset;
			Type_ptr type;
			Value_ptr defaultValue;
			unsigned hadExplicitDefault : 1;
		} slot;
		capnp_nowarn struct {
			uint64_t typeId;
		} group;
	};
	enum Field_ordinal_which ordinal_which;
	capnp_nowarn union {
		uint16_t _explicit;
	} ordinal;
};

static const size_t Field_word_count = 3;

static const size_t Field_pointer_count = 4;

static const size_t Field_struct_bytes_count = 56;


capn_text Field_get_name(Field_ptr p);

int Field_has_name(Field_ptr p);

uint16_t Field_get_codeOrder(Field_ptr p);

Annotation_list Field_get_annotations(Field_ptr p);

int Field_has_annotations(Field_ptr p);

uint16_t Field_get_discriminantValue(Field_ptr p);

void Field_set_name(Field_ptr p, capn_text name);

void Field_set_codeOrder(Field_ptr p, uint16_t codeOrder);

void Field_set_annotations(Field_ptr p, Annotation_list annotations);

void Field_set_discriminantValue(Field_ptr p, uint16_t discriminantValue);

struct Enumerant {
	capn_text name;
	uint16_t codeOrder;
	Annotation_list annotations;
};

static const size_t Enumerant_word_count = 1;

static const size_t Enumerant_pointer_count = 2;

static const size_t Enumerant_struct_bytes_count = 24;


capn_text Enumerant_get_name(Enumerant_ptr p);

int Enumerant_has_name(Enumerant_ptr p);

uint16_t Enumerant_get_codeOrder(Enumerant_ptr p);

Annotation_list Enumerant_get_annotations(Enumerant_ptr p);

int Enumerant_has_annotations(Enumerant_ptr p);

void Enumerant_set_name(Enumerant_ptr p, capn_text name);

void Enumerant_set_codeOrder(Enumerant_ptr p, uint16_t codeOrder);

void Enumerant_set_annotations(Enumerant_ptr p, Annotation_list annotations);

struct Superclass {
	uint64_t id;
	Brand_ptr brand;
};

static const size_t Superclass_word_count = 1;

static const size_t Superclass_pointer_count = 1;

static const size_t Superclass_struct_bytes_count = 16;


uint64_t Superclass_get_id(Superclass_ptr p);

Brand_ptr Superclass_get_brand(Superclass_ptr p);

int Superclass_has_brand(Superclass_ptr p);

void Superclass_set_id(Superclass_ptr p, uint64_t id);

void Superclass_set_brand(Superclass_ptr p, Brand_ptr brand);

struct Method {
	capn_text name;
	uint16_t codeOrder;
	Node_Parameter_list implicitParameters;
	uint64_t paramStructType;
	Brand_ptr paramBrand;
	uint64_t resultStructType;
	Brand_ptr resultBrand;
	Annotation_list annotations;
};

static const size_t Method_word_count = 3;

static const size_t Method_pointer_count = 5;

static const size_t Method_struct_bytes_count = 64;


capn_text Method_get_name(Method_ptr p);

int Method_has_name(Method_ptr p);

uint16_t Method_get_codeOrder(Method_ptr p);

Node_Parameter_list Method_get_implicitParameters(Method_ptr p);

int Method_has_implicitParameters(Method_ptr p);

uint64_t Method_get_paramStructType(Method_ptr p);

Brand_ptr Method_get_paramBrand(Method_ptr p);

int Method_has_paramBrand(Method_ptr p);

uint64_t Method_get_resultStructType(Method_ptr p);

Brand_ptr Method_get_resultBrand(Method_ptr p);

int Method_has_resultBrand(Method_ptr p);

Annotation_list Method_get_annotations(Method_ptr p);

int Method_has_annotations(Method_ptr p);

void Method_set_name(Method_ptr p, capn_text name);

void Method_set_codeOrder(Method_ptr p, uint16_t codeOrder);

void Method_set_implicitParameters(Method_ptr p, Node_Parameter_list implicitParameters);

void Method_set_paramStructType(Method_ptr p, uint64_t paramStructType);

void Method_set_paramBrand(Method_ptr p, Brand_ptr paramBrand);

void Method_set_resultStructType(Method_ptr p, uint64_t resultStructType);

void Method_set_resultBrand(Method_ptr p, Brand_ptr resultBrand);

void Method_set_annotations(Method_ptr p, Annotation_list annotations);
enum Type_anyPointer_unconstrained_which {
	Type_anyPointer_unconstrained_anyKind = 0,
	Type_anyPointer_unconstrained__struct = 1,
	Type_anyPointer_unconstrained__list = 2,
	Type_anyPointer_unconstrained_capability = 3
};
enum Type_anyPointer_which {
	Type_anyPointer_unconstrained = 0,
	Type_anyPointer_parameter = 1,
	Type_anyPointer_implicitMethodParameter = 2
};
enum Type_which {
	Type__void = 0,
	Type__bool = 1,
	Type_int8 = 2,
	Type_int16 = 3,
	Type_int32 = 4,
	Type_int64 = 5,
	Type_uint8 = 6,
	Type_uint16 = 7,
	Type_uint32 = 8,
	Type_uint64 = 9,
	Type_float32 = 10,
	Type_float64 = 11,
	Type_text = 12,
	Type_data = 13,
	Type__list = 14,
	Type__enum = 15,
	Type__struct = 16,
	Type__interface = 17,
	Type_anyPointer = 18
};

struct Type {
	enum Type_which which;
	capnp_nowarn union {
		capnp_nowarn struct {
			Type_ptr elementType;
		} _list;
		capnp_nowarn struct {
			uint64_t typeId;
			Brand_ptr brand;
		} _enum;
		capnp_nowarn struct {
			uint64_t typeId;
			Brand_ptr brand;
		} _struct;
		capnp_nowarn struct {
			uint64_t typeId;
			Brand_ptr brand;
		} _interface;
		capnp_nowarn struct {
			enum Type_anyPointer_which which;
			capnp_nowarn union {
				capnp_nowarn struct {
					enum Type_anyPointer_unconstrained_which which;
				} unconstrained;
				capnp_nowarn struct {
					uint64_t scopeId;
					uint16_t parameterIndex;
				} parameter;
				capnp_nowarn struct {
					uint16_t parameterIndex;
				} implicitMethodParameter;
			};
		} anyPointer;
	};
};

static const size_t Type_word_count = 3;

static const size_t Type_pointer_count = 1;

static const size_t Type_struct_bytes_count = 32;


struct Brand {
	Brand_Scope_list scopes;
};

static const size_t Brand_word_count = 0;

static const size_t Brand_pointer_count = 1;

static const size_t Brand_struct_bytes_count = 8;


Brand_Scope_list Brand_get_scopes(Brand_ptr p);

int Brand_has_scopes(Brand_ptr p);

void Brand_set_scopes(Brand_ptr p, Brand_Scope_list scopes);
enum Brand_Scope_which {
	Brand_Scope_bind = 0,
	Brand_Scope_inherit = 1
};

struct Brand_Scope {
	uint64_t scopeId;
	enum Brand_Scope_which which;
	capnp_nowarn union {
		Brand_Binding_list bind;
	};
};

static const size_t Brand_Scope_word_count = 2;

static const size_t Brand_Scope_pointer_count = 1;

static const size_t Brand_Scope_struct_bytes_count = 24;


uint64_t Brand_Scope_get_scopeId(Brand_Scope_ptr p);

void Brand_Scope_set_scopeId(Brand_Scope_ptr p, uint64_t scopeId);
enum Brand_Binding_which {
	Brand_Binding_unbound = 0,
	Brand_Binding_type = 1
};

struct Brand_Binding {
	enum Brand_Binding_which which;
	capnp_nowarn union {
		Type_ptr type;
	};
};

static const size_t Brand_Binding_word_count = 1;

static const size_t Brand_Binding_pointer_count = 1;

static const size_t Brand_Binding_struct_bytes_count = 16;

enum Value_which {
	Value__void = 0,
	Value__bool = 1,
	Value_int8 = 2,
	Value_int16 = 3,
	Value_int32 = 4,
	Value_int64 = 5,
	Value_uint8 = 6,
	Value_uint16 = 7,
	Value_uint32 = 8,
	Value_uint64 = 9,
	Value_float32 = 10,
	Value_float64 = 11,
	Value_text = 12,
	Value_data = 13,
	Value__list = 14,
	Value__enum = 15,
	Value__struct = 16,
	Value__interface = 17,
	Value_anyPointer = 18
};

struct Value {
	enum Value_which which;
	capnp_nowarn union {
		unsigned _bool : 1;
		int8_t int8;
		int16_t int16;
		int32_t int32;
		int64_t int64;
		uint8_t uint8;
		uint16_t uint16;
		uint32_t uint32;
		uint64_t uint64;
		float float32;
		double float64;
		capn_text text;
		capn_data data;
		capn_ptr _list;
		uint16_t _enum;
		capn_ptr _struct;
		capn_ptr anyPointer;
	};
};

static const size_t Value_word_count = 2;

static const size_t Value_pointer_count = 1;

static const size_t Value_struct_bytes_count = 24;


struct Annotation {
	uint64_t id;
	Brand_ptr brand;
	Value_ptr value;
};

static const size_t Annotation_word_count = 1;

static const size_t Annotation_pointer_count = 2;

static const size_t Annotation_struct_bytes_count = 24;


uint64_t Annotation_get_id(Annotation_ptr p);

Brand_ptr Annotation_get_brand(Annotation_ptr p);

int Annotation_has_brand(Annotation_ptr p);

Value_ptr Annotation_get_value(Annotation_ptr p);

int Annotation_has_value(Annotation_ptr p);

void Annotation_set_id(Annotation_ptr p, uint64_t id);

void Annotation_set_brand(Annotation_ptr p, Brand_ptr brand);

void Annotation_set_value(Annotation_ptr p, Value_ptr value);

struct CapnpVersion {
	uint16_t major;
	uint8_t minor;
	uint8_t micro;
};

static const size_t CapnpVersion_word_count = 1;

static const size_t CapnpVersion_pointer_count = 0;

static const size_t CapnpVersion_struct_bytes_count = 8;


uint16_t CapnpVersion_get_major(CapnpVersion_ptr p);

uint8_t CapnpVersion_get_minor(CapnpVersion_ptr p);

uint8_t CapnpVersion_get_micro(CapnpVersion_ptr p);

void CapnpVersion_set_major(CapnpVersion_ptr p, uint16_t major);

void CapnpVersion_set_minor(CapnpVersion_ptr p, uint8_t minor);

void CapnpVersion_set_micro(CapnpVersion_ptr p, uint8_t micro);

struct CodeGeneratorRequest {
	CapnpVersion_ptr capnpVersion;
	Node_list nodes;
	Node_SourceInfo_list sourceInfo;
	CodeGeneratorRequest_RequestedFile_list requestedFiles;
};

static const size_t CodeGeneratorRequest_word_count = 0;

static const size_t CodeGeneratorRequest_pointer_count = 4;

static const size_t CodeGeneratorRequest_struct_bytes_count = 32;


CapnpVersion_ptr CodeGeneratorRequest_get_capnpVersion(CodeGeneratorRequest_ptr p);

int CodeGeneratorRequest_has_capnpVersion(CodeGeneratorRequest_ptr p);

Node_list CodeGeneratorRequest_get_nodes(CodeGeneratorRequest_ptr p);

int CodeGeneratorRequest_has_nodes(CodeGeneratorRequest_ptr p);

Node_SourceInfo_list CodeGeneratorRequest_get_sourceInfo(CodeGeneratorRequest_ptr p);

int CodeGeneratorRequest_has_sourceInfo(CodeGeneratorRequest_ptr p);

CodeGeneratorRequest_RequestedFile_list CodeGeneratorRequest_get_requestedFiles(CodeGeneratorRequest_ptr p);

int CodeGeneratorRequest_has_requestedFiles(CodeGeneratorRequest_ptr p);

void CodeGeneratorRequest_set_capnpVersion(CodeGeneratorRequest_ptr p, CapnpVersion_ptr capnpVersion);

void CodeGeneratorRequest_set_nodes(CodeGeneratorRequest_ptr p, Node_list nodes);

void CodeGeneratorRequest_set_sourceInfo(CodeGeneratorRequest_ptr p, Node_SourceInfo_list sourceInfo);

void CodeGeneratorRequest_set_requestedFiles(CodeGeneratorRequest_ptr p, CodeGeneratorRequest_RequestedFile_list requestedFiles);

struct CodeGeneratorRequest_RequestedFile {
	uint64_t id;
	capn_text filename;
	CodeGeneratorRequest_RequestedFile_Import_list imports;
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr fileSourceInfo;
};

static const size_t CodeGeneratorRequest_RequestedFile_word_count = 1;

static const size_t CodeGeneratorRequest_RequestedFile_pointer_count = 3;

static const size_t CodeGeneratorRequest_RequestedFile_struct_bytes_count = 32;


uint64_t CodeGeneratorRequest_RequestedFile_get_id(CodeGeneratorRequest_RequestedFile_ptr p);

capn_text CodeGeneratorRequest_RequestedFile_get_filename(CodeGeneratorRequest_RequestedFile_ptr p);

int CodeGeneratorRequest_RequestedFile_has_filename(CodeGeneratorRequest_RequestedFile_ptr p);

CodeGeneratorRequest_RequestedFile_Import_list CodeGeneratorRequest_RequestedFile_get_imports(CodeGeneratorRequest_RequestedFile_ptr p);

int CodeGeneratorRequest_RequestedFile_has_imports(CodeGeneratorRequest_RequestedFile_ptr p);

CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr CodeGeneratorRequest_RequestedFile_get_fileSourceInfo(CodeGeneratorRequest_RequestedFile_ptr p);

int CodeGeneratorRequest_RequestedFile_has_fileSourceInfo(CodeGeneratorRequest_RequestedFile_ptr p);

void CodeGeneratorRequest_RequestedFile_set_id(CodeGeneratorRequest_RequestedFile_ptr p, uint64_t id);

void CodeGeneratorRequest_RequestedFile_set_filename(CodeGeneratorRequest_RequestedFile_ptr p, capn_text filename);

void CodeGeneratorRequest_RequestedFile_set_imports(CodeGeneratorRequest_RequestedFile_ptr p, CodeGeneratorRequest_RequestedFile_Import_list imports);

void CodeGeneratorRequest_RequestedFile_set_fileSourceInfo(CodeGeneratorRequest_RequestedFile_ptr p, CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr fileSourceInfo);

struct CodeGeneratorRequest_RequestedFile_Import {
	uint64_t id;
	capn_text name;
};

static const size_t CodeGeneratorRequest_RequestedFile_Import_word_count = 1;

static const size_t CodeGeneratorRequest_RequestedFile_Import_pointer_count = 1;

static const size_t CodeGeneratorRequest_RequestedFile_Import_struct_bytes_count = 16;


uint64_t CodeGeneratorRequest_RequestedFile_Import_get_id(CodeGeneratorRequest_RequestedFile_Import_ptr p);

capn_text CodeGeneratorRequest_RequestedFile_Import_get_name(CodeGeneratorRequest_RequestedFile_Import_ptr p);

int CodeGeneratorRequest_RequestedFile_Import_has_name(CodeGeneratorRequest_RequestedFile_Import_ptr p);

void CodeGeneratorRequest_RequestedFile_Import_set_id(CodeGeneratorRequest_RequestedFile_Import_ptr p, uint64_t id);

void CodeGeneratorRequest_RequestedFile_Import_set_name(CodeGeneratorRequest_RequestedFile_Import_ptr p, capn_text name);

struct CodeGeneratorRequest_RequestedFile_FileSourceInfo {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list identifiers;
};

static const size_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_word_count = 0;

static const size_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_pointer_count = 1;

static const size_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_struct_bytes_count = 8;


CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list CodeGeneratorRequest_RequestedFile_FileSourceInfo_get_identifiers(CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p);

int CodeGeneratorRequest_RequestedFile_FileSourceInfo_has_identifiers(CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p);

void CodeGeneratorRequest_RequestedFile_FileSourceInfo_set_identifiers(CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr p, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list identifiers);
enum CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_which {
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_typeId = 0,
	CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_member = 1
};

struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier {
	uint32_t startByte;
	uint32_t endByte;
	enum CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_which which;
	capnp_nowarn union {
		uint64_t typeId;
		capnp_nowarn struct {
			uint64_t parentTypeId;
			uint16_t ordinal;
		} member;
	};
};

static const size_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_word_count = 3;

static const size_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_pointer_count = 0;

static const size_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_struct_bytes_count = 24;


uint32_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_get_startByte(CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p);

uint32_t CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_get_endByte(CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p);

void CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_set_startByte(CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p, uint32_t startByte);

void CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_set_endByte(CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr p, uint32_t endByte);

Node_ptr new_Node(struct capn_segment*);
Node_Parameter_ptr new_Node_Parameter(struct capn_segment*);
Node_NestedNode_ptr new_Node_NestedNode(struct capn_segment*);
Node_SourceInfo_ptr new_Node_SourceInfo(struct capn_segment*);
Node_SourceInfo_Member_ptr new_Node_SourceInfo_Member(struct capn_segment*);
Field_ptr new_Field(struct capn_segment*);
Enumerant_ptr new_Enumerant(struct capn_segment*);
Superclass_ptr new_Superclass(struct capn_segment*);
Method_ptr new_Method(struct capn_segment*);
Type_ptr new_Type(struct capn_segment*);
Brand_ptr new_Brand(struct capn_segment*);
Brand_Scope_ptr new_Brand_Scope(struct capn_segment*);
Brand_Binding_ptr new_Brand_Binding(struct capn_segment*);
Value_ptr new_Value(struct capn_segment*);
Annotation_ptr new_Annotation(struct capn_segment*);
CapnpVersion_ptr new_CapnpVersion(struct capn_segment*);
CodeGeneratorRequest_ptr new_CodeGeneratorRequest(struct capn_segment*);
CodeGeneratorRequest_RequestedFile_ptr new_CodeGeneratorRequest_RequestedFile(struct capn_segment*);
CodeGeneratorRequest_RequestedFile_Import_ptr new_CodeGeneratorRequest_RequestedFile_Import(struct capn_segment*);
CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr new_CodeGeneratorRequest_RequestedFile_FileSourceInfo(struct capn_segment*);
CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr new_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(struct capn_segment*);

Node_list new_Node_list(struct capn_segment*, int len);
Node_Parameter_list new_Node_Parameter_list(struct capn_segment*, int len);
Node_NestedNode_list new_Node_NestedNode_list(struct capn_segment*, int len);
Node_SourceInfo_list new_Node_SourceInfo_list(struct capn_segment*, int len);
Node_SourceInfo_Member_list new_Node_SourceInfo_Member_list(struct capn_segment*, int len);
Field_list new_Field_list(struct capn_segment*, int len);
Enumerant_list new_Enumerant_list(struct capn_segment*, int len);
Superclass_list new_Superclass_list(struct capn_segment*, int len);
Method_list new_Method_list(struct capn_segment*, int len);
Type_list new_Type_list(struct capn_segment*, int len);
Brand_list new_Brand_list(struct capn_segment*, int len);
Brand_Scope_list new_Brand_Scope_list(struct capn_segment*, int len);
Brand_Binding_list new_Brand_Binding_list(struct capn_segment*, int len);
Value_list new_Value_list(struct capn_segment*, int len);
Annotation_list new_Annotation_list(struct capn_segment*, int len);
CapnpVersion_list new_CapnpVersion_list(struct capn_segment*, int len);
CodeGeneratorRequest_list new_CodeGeneratorRequest_list(struct capn_segment*, int len);
CodeGeneratorRequest_RequestedFile_list new_CodeGeneratorRequest_RequestedFile_list(struct capn_segment*, int len);
CodeGeneratorRequest_RequestedFile_Import_list new_CodeGeneratorRequest_RequestedFile_Import_list(struct capn_segment*, int len);
CodeGeneratorRequest_RequestedFile_FileSourceInfo_list new_CodeGeneratorRequest_RequestedFile_FileSourceInfo_list(struct capn_segment*, int len);
CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list new_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list(struct capn_segment*, int len);

void read_Node(struct Node*, Node_ptr);
void read_Node_Parameter(struct Node_Parameter*, Node_Parameter_ptr);
void read_Node_NestedNode(struct Node_NestedNode*, Node_NestedNode_ptr);
void read_Node_SourceInfo(struct Node_SourceInfo*, Node_SourceInfo_ptr);
void read_Node_SourceInfo_Member(struct Node_SourceInfo_Member*, Node_SourceInfo_Member_ptr);
void read_Field(struct Field*, Field_ptr);
void read_Enumerant(struct Enumerant*, Enumerant_ptr);
void read_Superclass(struct Superclass*, Superclass_ptr);
void read_Method(struct Method*, Method_ptr);
void read_Type(struct Type*, Type_ptr);
void read_Brand(struct Brand*, Brand_ptr);
void read_Brand_Scope(struct Brand_Scope*, Brand_Scope_ptr);
void read_Brand_Binding(struct Brand_Binding*, Brand_Binding_ptr);
void read_Value(struct Value*, Value_ptr);
void read_Annotation(struct Annotation*, Annotation_ptr);
void read_CapnpVersion(struct CapnpVersion*, CapnpVersion_ptr);
void read_CodeGeneratorRequest(struct CodeGeneratorRequest*, CodeGeneratorRequest_ptr);
void read_CodeGeneratorRequest_RequestedFile(struct CodeGeneratorRequest_RequestedFile*, CodeGeneratorRequest_RequestedFile_ptr);
void read_CodeGeneratorRequest_RequestedFile_Import(struct CodeGeneratorRequest_RequestedFile_Import*, CodeGeneratorRequest_RequestedFile_Import_ptr);
void read_CodeGeneratorRequest_RequestedFile_FileSourceInfo(struct CodeGeneratorRequest_RequestedFile_FileSourceInfo*, CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr);
void read_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier*, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr);

void write_Node(const struct Node*, Node_ptr);
void write_Node_Parameter(const struct Node_Parameter*, Node_Parameter_ptr);
void write_Node_NestedNode(const struct Node_NestedNode*, Node_NestedNode_ptr);
void write_Node_SourceInfo(const struct Node_SourceInfo*, Node_SourceInfo_ptr);
void write_Node_SourceInfo_Member(const struct Node_SourceInfo_Member*, Node_SourceInfo_Member_ptr);
void write_Field(const struct Field*, Field_ptr);
void write_Enumerant(const struct Enumerant*, Enumerant_ptr);
void write_Superclass(const struct Superclass*, Superclass_ptr);
void write_Method(const struct Method*, Method_ptr);
void write_Type(const struct Type*, Type_ptr);
void write_Brand(const struct Brand*, Brand_ptr);
void write_Brand_Scope(const struct Brand_Scope*, Brand_Scope_ptr);
void write_Brand_Binding(const struct Brand_Binding*, Brand_Binding_ptr);
void write_Value(const struct Value*, Value_ptr);
void write_Annotation(const struct Annotation*, Annotation_ptr);
void write_CapnpVersion(const struct CapnpVersion*, CapnpVersion_ptr);
void write_CodeGeneratorRequest(const struct CodeGeneratorRequest*, CodeGeneratorRequest_ptr);
void write_CodeGeneratorRequest_RequestedFile(const struct CodeGeneratorRequest_RequestedFile*, CodeGeneratorRequest_RequestedFile_ptr);
void write_CodeGeneratorRequest_RequestedFile_Import(const struct CodeGeneratorRequest_RequestedFile_Import*, CodeGeneratorRequest_RequestedFile_Import_ptr);
void write_CodeGeneratorRequest_RequestedFile_FileSourceInfo(const struct CodeGeneratorRequest_RequestedFile_FileSourceInfo*, CodeGeneratorRequest_RequestedFile_FileSourceInfo_ptr);
void write_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(const struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier*, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_ptr);

void get_Node(struct Node*, Node_list, int i);
void get_Node_Parameter(struct Node_Parameter*, Node_Parameter_list, int i);
void get_Node_NestedNode(struct Node_NestedNode*, Node_NestedNode_list, int i);
void get_Node_SourceInfo(struct Node_SourceInfo*, Node_SourceInfo_list, int i);
void get_Node_SourceInfo_Member(struct Node_SourceInfo_Member*, Node_SourceInfo_Member_list, int i);
void get_Field(struct Field*, Field_list, int i);
void get_Enumerant(struct Enumerant*, Enumerant_list, int i);
void get_Superclass(struct Superclass*, Superclass_list, int i);
void get_Method(struct Method*, Method_list, int i);
void get_Type(struct Type*, Type_list, int i);
void get_Brand(struct Brand*, Brand_list, int i);
void get_Brand_Scope(struct Brand_Scope*, Brand_Scope_list, int i);
void get_Brand_Binding(struct Brand_Binding*, Brand_Binding_list, int i);
void get_Value(struct Value*, Value_list, int i);
void get_Annotation(struct Annotation*, Annotation_list, int i);
void get_CapnpVersion(struct CapnpVersion*, CapnpVersion_list, int i);
void get_CodeGeneratorRequest(struct CodeGeneratorRequest*, CodeGeneratorRequest_list, int i);
void get_CodeGeneratorRequest_RequestedFile(struct CodeGeneratorRequest_RequestedFile*, CodeGeneratorRequest_RequestedFile_list, int i);
void get_CodeGeneratorRequest_RequestedFile_Import(struct CodeGeneratorRequest_RequestedFile_Import*, CodeGeneratorRequest_RequestedFile_Import_list, int i);
void get_CodeGeneratorRequest_RequestedFile_FileSourceInfo(struct CodeGeneratorRequest_RequestedFile_FileSourceInfo*, CodeGeneratorRequest_RequestedFile_FileSourceInfo_list, int i);
void get_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier*, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list, int i);

void set_Node(const struct Node*, Node_list, int i);
void set_Node_Parameter(const struct Node_Parameter*, Node_Parameter_list, int i);
void set_Node_NestedNode(const struct Node_NestedNode*, Node_NestedNode_list, int i);
void set_Node_SourceInfo(const struct Node_SourceInfo*, Node_SourceInfo_list, int i);
void set_Node_SourceInfo_Member(const struct Node_SourceInfo_Member*, Node_SourceInfo_Member_list, int i);
void set_Field(const struct Field*, Field_list, int i);
void set_Enumerant(const struct Enumerant*, Enumerant_list, int i);
void set_Superclass(const struct Superclass*, Superclass_list, int i);
void set_Method(const struct Method*, Method_list, int i);
void set_Type(const struct Type*, Type_list, int i);
void set_Brand(const struct Brand*, Brand_list, int i);
void set_Brand_Scope(const struct Brand_Scope*, Brand_Scope_list, int i);
void set_Brand_Binding(const struct Brand_Binding*, Brand_Binding_list, int i);
void set_Value(const struct Value*, Value_list, int i);
void set_Annotation(const struct Annotation*, Annotation_list, int i);
void set_CapnpVersion(const struct CapnpVersion*, CapnpVersion_list, int i);
void set_CodeGeneratorRequest(const struct CodeGeneratorRequest*, CodeGeneratorRequest_list, int i);
void set_CodeGeneratorRequest_RequestedFile(const struct CodeGeneratorRequest_RequestedFile*, CodeGeneratorRequest_RequestedFile_list, int i);
void set_CodeGeneratorRequest_RequestedFile_Import(const struct CodeGeneratorRequest_RequestedFile_Import*, CodeGeneratorRequest_RequestedFile_Import_list, int i);
void set_CodeGeneratorRequest_RequestedFile_FileSourceInfo(const struct CodeGeneratorRequest_RequestedFile_FileSourceInfo*, CodeGeneratorRequest_RequestedFile_FileSourceInfo_list, int i);
void set_CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier(const struct CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier*, CodeGeneratorRequest_RequestedFile_FileSourceInfo_Identifier_list, int i);

#ifdef __cplusplus
}
#endif
#endif
