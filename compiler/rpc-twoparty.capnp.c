#include "rpc-twoparty.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) (x);
#else
# define capnp_unused
# define capnp_use(x)
#endif

static const capn_ptr capn_null = {CAPN_NULL};

VatId_ptr new_VatId(struct capn_segment *s) {
	VatId_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
VatId_list new_VatId_list(struct capn_segment *s, int len) {
	VatId_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_VatId(struct VatId *s capnp_unused, VatId_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->side = (enum Side)(int) capn_read16(p.p, 0);
}
void write_VatId(const struct VatId *s capnp_unused, VatId_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, (uint16_t) (s->side));
}
void get_VatId(struct VatId *s, VatId_list l, int i) {
	VatId_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_VatId(s, p);
}
void set_VatId(const struct VatId *s, VatId_list l, int i) {
	VatId_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_VatId(s, p);
}

ProvisionId_ptr new_ProvisionId(struct capn_segment *s) {
	ProvisionId_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
ProvisionId_list new_ProvisionId_list(struct capn_segment *s, int len) {
	ProvisionId_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_ProvisionId(struct ProvisionId *s capnp_unused, ProvisionId_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->joinId = capn_read32(p.p, 0);
}
void write_ProvisionId(const struct ProvisionId *s capnp_unused, ProvisionId_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->joinId);
}
void get_ProvisionId(struct ProvisionId *s, ProvisionId_list l, int i) {
	ProvisionId_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_ProvisionId(s, p);
}
void set_ProvisionId(const struct ProvisionId *s, ProvisionId_list l, int i) {
	ProvisionId_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_ProvisionId(s, p);
}

RecipientId_ptr new_RecipientId(struct capn_segment *s) {
	RecipientId_ptr p;
	p.p = capn_new_struct(s, 0, 0);
	return p;
}
RecipientId_list new_RecipientId_list(struct capn_segment *s, int len) {
	RecipientId_list p;
	p.p = capn_new_struct_list(s, len, 0, 0);
	return p;
}
void read_RecipientId(struct RecipientId *s capnp_unused, RecipientId_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void write_RecipientId(const struct RecipientId *s capnp_unused, RecipientId_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void get_RecipientId(struct RecipientId *s, RecipientId_list l, int i) {
	RecipientId_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_RecipientId(s, p);
}
void set_RecipientId(const struct RecipientId *s, RecipientId_list l, int i) {
	RecipientId_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_RecipientId(s, p);
}

ThirdPartyCapId_ptr new_ThirdPartyCapId(struct capn_segment *s) {
	ThirdPartyCapId_ptr p;
	p.p = capn_new_struct(s, 0, 0);
	return p;
}
ThirdPartyCapId_list new_ThirdPartyCapId_list(struct capn_segment *s, int len) {
	ThirdPartyCapId_list p;
	p.p = capn_new_struct_list(s, len, 0, 0);
	return p;
}
void read_ThirdPartyCapId(struct ThirdPartyCapId *s capnp_unused, ThirdPartyCapId_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void write_ThirdPartyCapId(const struct ThirdPartyCapId *s capnp_unused, ThirdPartyCapId_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
}
void get_ThirdPartyCapId(struct ThirdPartyCapId *s, ThirdPartyCapId_list l, int i) {
	ThirdPartyCapId_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_ThirdPartyCapId(s, p);
}
void set_ThirdPartyCapId(const struct ThirdPartyCapId *s, ThirdPartyCapId_list l, int i) {
	ThirdPartyCapId_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_ThirdPartyCapId(s, p);
}

JoinKeyPart_ptr new_JoinKeyPart(struct capn_segment *s) {
	JoinKeyPart_ptr p;
	p.p = capn_new_struct(s, 8, 0);
	return p;
}
JoinKeyPart_list new_JoinKeyPart_list(struct capn_segment *s, int len) {
	JoinKeyPart_list p;
	p.p = capn_new_struct_list(s, len, 8, 0);
	return p;
}
void read_JoinKeyPart(struct JoinKeyPart *s capnp_unused, JoinKeyPart_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->joinId = capn_read32(p.p, 0);
	s->partCount = capn_read16(p.p, 4);
	s->partNum = capn_read16(p.p, 6);
}
void write_JoinKeyPart(const struct JoinKeyPart *s capnp_unused, JoinKeyPart_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->joinId);
	capn_write16(p.p, 4, s->partCount);
	capn_write16(p.p, 6, s->partNum);
}
void get_JoinKeyPart(struct JoinKeyPart *s, JoinKeyPart_list l, int i) {
	JoinKeyPart_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_JoinKeyPart(s, p);
}
void set_JoinKeyPart(const struct JoinKeyPart *s, JoinKeyPart_list l, int i) {
	JoinKeyPart_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_JoinKeyPart(s, p);
}

JoinResult_ptr new_JoinResult(struct capn_segment *s) {
	JoinResult_ptr p;
	p.p = capn_new_struct(s, 8, 1);
	return p;
}
JoinResult_list new_JoinResult_list(struct capn_segment *s, int len) {
	JoinResult_list p;
	p.p = capn_new_struct_list(s, len, 8, 1);
	return p;
}
void read_JoinResult(struct JoinResult *s capnp_unused, JoinResult_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->joinId = capn_read32(p.p, 0);
	s->succeeded = (capn_read8(p.p, 4) & 1) != 0;
	s->cap = capn_getp(p.p, 0, 0);
}
void write_JoinResult(const struct JoinResult *s capnp_unused, JoinResult_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write32(p.p, 0, s->joinId);
	capn_write1(p.p, 32, s->succeeded != 0);
	capn_setp(p.p, 0, (s->cap.type != CAPN_NULL) ? s->cap : capn_null);
}
void get_JoinResult(struct JoinResult *s, JoinResult_list l, int i) {
	JoinResult_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_JoinResult(s, p);
}
void set_JoinResult(const struct JoinResult *s, JoinResult_list l, int i) {
	JoinResult_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_JoinResult(s, p);
}
