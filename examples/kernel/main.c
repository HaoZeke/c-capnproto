/* Sample kernel module: encode an AddressBook Person with c-capnproto.
 *
 * Examples-only. See KERNEL.md. Build with the GNUmakefile in this
 * directory against a kernel tree (KDIR).
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>

#include "capnp_c.h"
#include "addressbook.capnp.h"

static capn_text chars_to_text(const char *chars)
{
	return (capn_text) {
		.len = (int) strlen(chars),
		.str = chars,
		.seg = NULL,
	};
}

static void test_capn(void)
{
	uint8_t buf[4096];
	ssize_t sz;
	const char *name = "Firstname Lastname";
	const char *email = "username@domain.com";
	const char *school = "of life";
	struct capn c;
	capn_ptr cr;
	struct capn_segment *cs;
	struct Person p;
	struct Person_PhoneNumber pn0, pn1;
	Person_ptr pp;
	int setp_ret;

	capn_init_malloc(&c);
	cr = capn_root(&c);
	cs = cr.seg;

	memset(&p, 0, sizeof(p));
	p.id = 17;
	p.name = chars_to_text(name);
	p.email = chars_to_text(email);
	p.employment_which = Person_employment_school;
	p.employment.school = chars_to_text(school);

	p.phones = new_Person_PhoneNumber_list(cs, 2);
	memset(&pn0, 0, sizeof(pn0));
	pn0.number = chars_to_text("123");
	pn0.type = Person_PhoneNumber_Type_work;
	set_Person_PhoneNumber(&pn0, p.phones, 0);
	memset(&pn1, 0, sizeof(pn1));
	pn1.number = chars_to_text("234");
	pn1.type = Person_PhoneNumber_Type_home;
	set_Person_PhoneNumber(&pn1, p.phones, 1);

	pp = new_Person(cs);
	write_Person(&p, pp);
	setp_ret = capn_setp(capn_root(&c), 0, pp.p);
	sz = capn_write_mem(&c, buf, sizeof(buf), 0);
	pr_info("c-capnproto sample: setp_ret=%d bytes=%zd\n", setp_ret, sz);
	capn_free(&c);
}

static int __init capnp_addressbook_init(void)
{
	pr_info("c-capnproto sample: loading\n");
	test_capn();
	return 0;
}

static void __exit capnp_addressbook_exit(void)
{
	pr_info("c-capnproto sample: unloaded\n");
}

module_init(capnp_addressbook_init);
module_exit(capnp_addressbook_exit);

MODULE_LICENSE("Dual MIT/GPL");
MODULE_DESCRIPTION("c-capnproto addressbook sample (examples only)");
MODULE_AUTHOR("c-capnproto");
