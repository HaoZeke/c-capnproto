#ifndef CAPN_A184C7885CDAF2A1
#define CAPN_A184C7885CDAF2A1
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

struct VatId;
struct ProvisionId;
struct RecipientId;
struct ThirdPartyCapId;
struct JoinKeyPart;
struct JoinResult;

typedef struct {capn_ptr p;} VatId_ptr;
typedef struct {capn_ptr p;} ProvisionId_ptr;
typedef struct {capn_ptr p;} RecipientId_ptr;
typedef struct {capn_ptr p;} ThirdPartyCapId_ptr;
typedef struct {capn_ptr p;} JoinKeyPart_ptr;
typedef struct {capn_ptr p;} JoinResult_ptr;

typedef struct {capn_ptr p;} VatId_list;
typedef struct {capn_ptr p;} ProvisionId_list;
typedef struct {capn_ptr p;} RecipientId_list;
typedef struct {capn_ptr p;} ThirdPartyCapId_list;
typedef struct {capn_ptr p;} JoinKeyPart_list;
typedef struct {capn_ptr p;} JoinResult_list;

enum Side {
	Side_server = 0,
	Side_client = 1
};

struct VatId {
	enum Side side;
};

static const size_t VatId_word_count = 1;

static const size_t VatId_pointer_count = 0;

static const size_t VatId_struct_bytes_count = 8;


struct ProvisionId {
	uint32_t joinId;
};

static const size_t ProvisionId_word_count = 1;

static const size_t ProvisionId_pointer_count = 0;

static const size_t ProvisionId_struct_bytes_count = 8;


capnp_nowarn struct RecipientId {
};

static const size_t RecipientId_word_count = 0;

static const size_t RecipientId_pointer_count = 0;

static const size_t RecipientId_struct_bytes_count = 0;


capnp_nowarn struct ThirdPartyCapId {
};

static const size_t ThirdPartyCapId_word_count = 0;

static const size_t ThirdPartyCapId_pointer_count = 0;

static const size_t ThirdPartyCapId_struct_bytes_count = 0;


struct JoinKeyPart {
	uint32_t joinId;
	uint16_t partCount;
	uint16_t partNum;
};

static const size_t JoinKeyPart_word_count = 1;

static const size_t JoinKeyPart_pointer_count = 0;

static const size_t JoinKeyPart_struct_bytes_count = 8;


struct JoinResult {
	uint32_t joinId;
	unsigned succeeded : 1;
	capn_ptr cap;
};

static const size_t JoinResult_word_count = 1;

static const size_t JoinResult_pointer_count = 1;

static const size_t JoinResult_struct_bytes_count = 16;


VatId_ptr new_VatId(struct capn_segment*);
ProvisionId_ptr new_ProvisionId(struct capn_segment*);
RecipientId_ptr new_RecipientId(struct capn_segment*);
ThirdPartyCapId_ptr new_ThirdPartyCapId(struct capn_segment*);
JoinKeyPart_ptr new_JoinKeyPart(struct capn_segment*);
JoinResult_ptr new_JoinResult(struct capn_segment*);

VatId_list new_VatId_list(struct capn_segment*, int len);
ProvisionId_list new_ProvisionId_list(struct capn_segment*, int len);
RecipientId_list new_RecipientId_list(struct capn_segment*, int len);
ThirdPartyCapId_list new_ThirdPartyCapId_list(struct capn_segment*, int len);
JoinKeyPart_list new_JoinKeyPart_list(struct capn_segment*, int len);
JoinResult_list new_JoinResult_list(struct capn_segment*, int len);

void read_VatId(struct VatId*, VatId_ptr);
void read_ProvisionId(struct ProvisionId*, ProvisionId_ptr);
void read_RecipientId(struct RecipientId*, RecipientId_ptr);
void read_ThirdPartyCapId(struct ThirdPartyCapId*, ThirdPartyCapId_ptr);
void read_JoinKeyPart(struct JoinKeyPart*, JoinKeyPart_ptr);
void read_JoinResult(struct JoinResult*, JoinResult_ptr);

void write_VatId(const struct VatId*, VatId_ptr);
void write_ProvisionId(const struct ProvisionId*, ProvisionId_ptr);
void write_RecipientId(const struct RecipientId*, RecipientId_ptr);
void write_ThirdPartyCapId(const struct ThirdPartyCapId*, ThirdPartyCapId_ptr);
void write_JoinKeyPart(const struct JoinKeyPart*, JoinKeyPart_ptr);
void write_JoinResult(const struct JoinResult*, JoinResult_ptr);

void get_VatId(struct VatId*, VatId_list, int i);
void get_ProvisionId(struct ProvisionId*, ProvisionId_list, int i);
void get_RecipientId(struct RecipientId*, RecipientId_list, int i);
void get_ThirdPartyCapId(struct ThirdPartyCapId*, ThirdPartyCapId_list, int i);
void get_JoinKeyPart(struct JoinKeyPart*, JoinKeyPart_list, int i);
void get_JoinResult(struct JoinResult*, JoinResult_list, int i);

void set_VatId(const struct VatId*, VatId_list, int i);
void set_ProvisionId(const struct ProvisionId*, ProvisionId_list, int i);
void set_RecipientId(const struct RecipientId*, RecipientId_list, int i);
void set_ThirdPartyCapId(const struct ThirdPartyCapId*, ThirdPartyCapId_list, int i);
void set_JoinKeyPart(const struct JoinKeyPart*, JoinKeyPart_list, int i);
void set_JoinResult(const struct JoinResult*, JoinResult_list, int i);

#ifdef __cplusplus
}
#endif
#endif
