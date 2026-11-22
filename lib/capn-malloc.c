/* vim: set sw=8 ts=8 sts=8 noet: */
/* capn-malloc.c
 *
 * Copyright (C) 2013 James McKaskill
 * Copyright (C) 2014 Steve Dee
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "capnp_c.h"
#include "capnp_priv.h"
#ifndef __KERNEL__
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#else /* __KERNEL__ */
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/limits.h>
#define malloc(sz) kmalloc((sz), GFP_KERNEL)
#define calloc(n, sz) kcalloc((n), (sz), GFP_KERNEL)
#define free(p) kfree(p)
#endif /* __KERNEL__ */

/*
 * 8-byte alignment is required for struct capn_segment (Cap'n Proto
 * words are 8 bytes). This is the ARM/Sparc (and 32-bit x86) compile-time
 * check: a negative bitfield width fails the build if
 * sizeof(struct capn_segment) is not a multiple of 8.
 *
 * Field ALIGNED_(8) on data/len/cap/user (see capnp_c.h) is the portable
 * fix. Do not apply aligned(64) to the whole struct.
 *
 * Without 8-byte size/alignment, (sizeof(struct capn_segment)&7) is 4 on
 * 32-bit pointers (e.g. 44 & 7) and 0 on 64-bit (e.g. 80 & 7).
 */
struct check_segment_alignment {
	unsigned int foo : (sizeof(struct capn_segment)&7) ? -1 : 1;
};

/* Floor and rounding unit for create() allocations. Must be a power of two.
 * Override at compile time: -DCAPN_CREATE_MIN_SZ=512 */
#ifndef CAPN_CREATE_MIN_SZ
#define CAPN_CREATE_MIN_SZ 4096
#endif

static struct capn_segment *create_sz(size_t need) {
	struct capn_segment *s;
	size_t total, unit;
	if (need > (size_t) -1 - sizeof(*s))
		return NULL;
	total = need + sizeof(*s);
	unit = (size_t) CAPN_CREATE_MIN_SZ;
	if (total < unit) {
		total = unit;
	} else {
		if (unit == 0 || (unit & (unit - 1)) != 0)
			return NULL;
		if (total > (size_t) -1 - (unit - 1))
			return NULL;
		total = (total + unit - 1) & ~(unit - 1);
	}
	s = (struct capn_segment*) calloc(1, total);
	if (!s)
		return NULL;
	s->data = (char*) (s+1);
	s->cap = total - sizeof(*s);
	s->user = s;
	return s;
}

static struct capn_segment *create(void *u, uint32_t id, int sz) {
	(void) u;
	(void) id;
	/* sz is the caller's int (the create callback). A request near
	 * INT_MAX plus this header used to wrap the signed add and the
	 * 4096-byte round-up, then calloc a 4096-byte segment for a
	 * multi-gigabyte list. Count in size_t and refuse a wrap. */
	if (sz < 0)
		return NULL;
	return create_sz((size_t) sz);
}

int capn_using_malloc(const struct capn *c) {
	return c != NULL && c->create == create;
}

struct capn_segment *capn_malloc_segment(size_t sz) {
	return create_sz(sz);
}

static struct capn_segment *create_local(void *u, int sz) {
	return create(u, 0, sz);
}

void capn_init_malloc(struct capn *c) {
	memset(c, 0, sizeof(*c));
	c->create = &create;
	c->create_local = &create_local;
}

void capn_free(struct capn *c) {
	struct capn_segment *s = c->seglist;
	while (s != NULL) {
		struct capn_segment *n = s->next;
		free(s->user);
		s = n;
	}
	free(c->dup_busy);
	free(c->dup_at);
	capn_reset_copy(c);
}

void capn_reset_copy(struct capn *c) {
	struct capn_segment *s = c->copylist;
	while (s != NULL) {
		struct capn_segment *n = s->next;
		free(s->user);
		s = n;
	}
	c->copy = NULL;
	c->copylist = NULL;
}

#define ZBUF_SZ 4096
/* Framing header is a stack array of this many segment-size words.
 * The first word of the stream is (segnum - 1). C++
 * InputStreamMessageReader rejects a count of 512 or more, so the
 * wire word must stay below 511. */
#define CAPN_MAX_SEGS 511

/* Largest framed message the stream readers will allocate for, matching
 * the 64 MiB default traversal limit (C++ refuses a larger message at
 * the framing layer, before it allocates). Override at compile time. */
#ifndef CAPN_MAX_MESSAGE_BYTES
#define CAPN_MAX_MESSAGE_BYTES ((uint32_t) 64u * 1024u * 1024u)
#endif

#ifndef __KERNEL__
static int read_fd_all(ssize_t (*read_fd)(int fd, void *p, size_t count), int fd, void *p, size_t count)
{
	ssize_t ret;
	size_t got = 0;

	while (got < count) {
		ret = read_fd(fd, ((uint8_t*)p) + got, count - got);
		if (ret < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			return -1;
		}
		if (ret == 0)
			return -1;
		got += (size_t)ret;
	}
	return 0;
}
#endif /* !__KERNEL__ */

static int read_fp(void *p, size_t sz, FILE *f,
		   ssize_t (*read_fd)(int fd, void *p, size_t count), int fd,
		   struct capn_stream *z, uint8_t* zbuf, int packed) {
#ifndef __KERNEL__
	if ((f || read_fd) && packed) {
		z->next_out = (uint8_t*) p;
		z->avail_out = sz;

		/* capn_inflate returns CAPN_NEED_MORE when more packed
		 * input is required to fill avail_out. Refill zbuf from f
		 * / read_fd and keep leftover next_in on zbuf after each fill. */

		while (z->avail_out) {
			int inf;

			inf = capn_inflate(z);
			if (inf != 0 && inf != CAPN_NEED_MORE)
				return -1;
			if (!z->avail_out)
				return 0;

			if (z->avail_in && z->next_in != NULL)
				memmove(zbuf, z->next_in, z->avail_in);
			z->next_in = zbuf;
			if (f) {
				int r = fread(zbuf + z->avail_in, 1, ZBUF_SZ - z->avail_in, f);
				if (r <= 0)
					return -1;
				z->avail_in += r;
			} else {
				ssize_t r;
				for (;;) {
					r = read_fd(fd, zbuf + z->avail_in, ZBUF_SZ - z->avail_in);
					if (r < 0) {
						if (errno == EAGAIN || errno == EINTR)
							continue;
						return -1;
					}
					break;
				}
				if (r <= 0)
					return -1;
				z->avail_in += (size_t)r;
			}
		}
		return 0;

	} else if (f && !packed) {
		return fread(p, sz, 1, f) != 1;

	} else if (read_fd && !packed) {
		return read_fd_all(read_fd, fd, p, sz);

	} else
#endif /* !__KERNEL__ */
	if (packed) {
		z->next_out = (uint8_t*) p;
		z->avail_out = sz;
		if (capn_inflate(z) != 0)
			return -1;
		return z->avail_out != 0;

	} else {
		if (z->avail_in < sz)
			return -1;
		memcpy(p, z->next_in, sz);
		z->next_in += sz;
		z->avail_in -= sz;
		return 0;
	}
}

static int init_fp(struct capn *c, FILE *f,
		   ssize_t (*read_fd)(int fd, void *p, size_t count), int fd,
		   struct capn_stream *z, int packed, size_t max_bytes) {
	/*
	 * Initialize 'c' from the contents of 'f', assuming the message has been
	 * serialized with the standard framing format. From https://capnproto.org/encoding.html:
	 *
	 * When transmitting over a stream, the following should be sent. All integers are unsigned and little-endian.
	 *   (4 bytes) The number of segments, minus one (since there is always at least one segment).
	 *   (N * 4 bytes) The size of each segment, in words.
	 *   (0 or 4 bytes) Padding up to the next word boundary.
	 *   The content of each segment, in order.
	 */

	struct capn_segment *s = NULL;
	uint32_t i, segnum;
	size_t total = 0;
	uint32_t hdr[CAPN_MAX_SEGS];
	uint8_t zbuf[ZBUF_SZ];
	char *data = NULL;

	capn_init_malloc(c);
	if (max_bytes == 0)
		max_bytes = CAPN_MAX_MESSAGE_BYTES;
	else
		c->traversal_limit = max_bytes;

	/* C++ FlatArrayMessageReader treats a zero-length buffer as an
	 * empty message. The root read is what fails, not the open. */
	if (!packed && !f && !read_fd && z->avail_in == 0) {
		s = (struct capn_segment *) calloc(1, sizeof(*s));
		if (!s)
			goto err;
		s->user = s;
		capn_append_segment(c, s);
		return 0;
	}

	/* Read the first four bytes to know how many headers we have.
	 * A packed stream is a sequence of words. C++ reads the first
	 * word, then the rest of the table, and a zero or raw run that
	 * continues past that read is an error. */
	if (packed) {
		uint8_t word[8];
		if (read_fp(word, 8, f, read_fd, fd, z, zbuf, 1) ||
		    z->zeros || z->raw || z->avail_buf)
			goto err;
		memcpy(&segnum, word, 4);
		memcpy(&hdr[0], word + 4, 4);
	} else if (read_fp(&segnum, 4, f, read_fd, fd, z, zbuf, 0))
		goto err;

	segnum = capn_flip32(segnum);
	if (segnum > CAPN_MAX_SEGS - 1)
		goto err;
	segnum++; /* The wire encoding was zero-based */

	/* Read the header list */
	if (packed) {
		if (segnum > 1) {
			size_t rest = (size_t) (segnum & ~1u) * 4u;
			if (read_fp(hdr + 1, rest, f, read_fd, fd, z, zbuf, 1) ||
			    z->zeros || z->raw || z->avail_buf)
				goto err;
		}
	} else if (read_fp(hdr, 8 * (segnum/2) + 4, f, read_fd, fd, z, zbuf, 0))
		goto err;

	for (i = 0; i < segnum; i++) {
		uint32_t n = capn_flip32(hdr[i]);
		size_t nbytes;
		/* 2^29-1 words is the largest segment C++ will read. */
		if (n > 0x1fffffffu)
			goto err;
		nbytes = (size_t) n * 8u;
		if (total > (size_t) -1 - nbytes)
			goto err;
		hdr[i] = (uint32_t) nbytes;
		total += nbytes;
	}

	if (total > max_bytes)
		goto err;
	/* A buffer cannot hold more than it was given, or than the longest
	 * run a packed stream can inflate to (255 zero words per two bytes).
	 * The inflate product is capped so a large avail_in cannot wrap. */
	if (!f && !read_fd) {
		size_t bound = z->avail_in;
		if (packed) {
			if (z->avail_in > (SIZE_MAX - 8u) / 1024u)
				bound = SIZE_MAX;
			else
				bound = z->avail_in * 1024u + 8u;
		}
		if (total > bound)
			goto err;
	}

	/* Allocate space for the data and the capn_segment structs.
	 * total is at most CAPN_MAX_MESSAGE_BYTES and segnum at most
	 * CAPN_MAX_SEGS, so the sum fits in size_t. */
	s = (struct capn_segment*) calloc(1, (size_t) total + (sizeof(*s) * segnum));
	if (!s)
		goto err;

	/* The segment bodies are one packed stream. A zero run or a raw
	 * run continues from one segment into the next. A run that still
	 * has bytes after those bodies is not part of this message. */
	data = (char*) (s+segnum);
	if (packed) {
		if (total && (read_fp(data, total, f, read_fd, fd, z, zbuf, 1) ||
		    z->zeros || z->raw || z->avail_buf))
			goto err;
	} else if (read_fp(data, total, f, read_fd, fd, z, zbuf, 0))
		goto err;

	for (i = 0; i < segnum; i++) {
		s[i].len = s[i].cap = hdr[i];
		s[i].data = data;
		data += s[i].len;
		capn_append_segment(c, &s[i]);
	}

    /* Set the entire region to be freed on the last segment */
	s[segnum-1].user = s;

	/* A segment of zero words is a message the reader can be
	 * built from. The missing root is reported when it is read. */
	return 0;

err:
	memset(c, 0, sizeof(*c));
	free(s);
	return -1;
}

int capn_init_fp_max(struct capn *c, FILE *f, int packed, size_t max_bytes) {
	struct capn_stream z;
	memset(&z, 0, sizeof(z));
	return init_fp(c, f, NULL, -1, &z, packed, max_bytes);
}

int capn_init_fp(struct capn *c, FILE *f, int packed) {
	return capn_init_fp_max(c, f, packed, 0);
}

int capn_init_mem_max(struct capn *c, const uint8_t *p, size_t sz, int packed,
		      size_t max_bytes) {
	struct capn_stream z;
	memset(&z, 0, sizeof(z));
	z.next_in = p;
	z.avail_in = sz;
	return init_fp(c, NULL, NULL, -1, &z, packed, max_bytes);
}

int capn_init_mem(struct capn *c, const uint8_t *p, size_t sz, int packed) {
	return capn_init_mem_max(c, p, sz, packed, 0);
}

#ifndef __KERNEL__
int capn_init_fd(struct capn *c, ssize_t (*read_fd)(int fd, void *p, size_t count), int fd, int packed) {
	struct capn_stream z;
	if (read_fd == NULL)
		return -1;
	memset(&z, 0, sizeof(z));
	return init_fp(c, NULL, read_fd, fd, &z, packed, 0);
}
#else /* __KERNEL__ */
int capn_init_fd(struct capn *c, ssize_t (*read_fd)(int fd, void *p, size_t count), int fd, int packed) {
	return -1;
}
#endif /* !__KERNEL__ */

static void header_calc(struct capn *c, uint32_t *headerlen, size_t *headersz)
{
	/* segnum == 1:
	 *   [segnum][segsiz]
	 * segnum == 2:
	 *   [segnum][segsiz][segsiz][zeroes]
	 * segnum == 3:
	 *   [segnum][segsiz][segsiz][segsiz]
	 * segnum == 4:
	 *   [segnum][segsiz][segsiz][segsiz][segsiz][zeroes]
	 */
	*headerlen = ((2 + c->segnum) / 2) * 2;
	*headersz = 4 * *headerlen;
}

static int add_size(size_t *sum, size_t n)
{
	if (*sum > (size_t) -1 - n)
		return -1;
	*sum += n;
	return 0;
}

static int header_render(struct capn *c, struct capn_segment *seg, uint8_t *header, uint32_t headerlen, size_t *datasz)
{
	size_t i;
	uint32_t val;

	/* Byte copies: packed framing places this header at a 2-byte
	 * offset, which is not a valid uint32_t store. */
	val = capn_flip32(c->segnum - 1);
	memcpy(header, &val, sizeof(val));
	val = 0;
	memcpy(header + (size_t)(headerlen - 1) * 4, &val, sizeof(val)); /* Zero out the spare position in the header sizes */
	for (i = 0; i < c->segnum; i++, seg = seg->next) {
		if (0 == seg)
			return -1;
		if (seg->len / 8 > 0xffffffffu)
			return -1;
		if (add_size(datasz, seg->len))
			return -1;
		val = capn_flip32((uint32_t) (seg->len / 8));
		memcpy(header + (1 + i) * 4, &val, sizeof(val));
	}
	if (0 != seg)
		return -1;

	return 0;
}

/* The zero-length buffer reader keeps one segment with no data, so
 * its root read fails. C++ has no segment to write for it. */
static int no_segments(const struct capn *c)
{
	return c->segnum == 0 || (c->segnum == 1 && c->seglist &&
		c->seglist->data == NULL && c->seglist->cap == 0);
}

/* A builder whose first segment has room but no root yet is padded
 * to one word, which is what writing used to do by reading the root.
 * A wire segment is left alone, including one with no words. */
static void pad_builder_root(struct capn *c)
{
	struct capn_segment *seg = c->seglist;

	if (seg && seg->cap > seg->len && seg->len < 8 && seg->cap >= 8)
		seg->len = 8;
}

static int64_t capn_write_mem_packed(struct capn *c, uint8_t *p, size_t sz)
{
	struct capn_segment *seg;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	uint8_t *header;
	struct capn_stream z;
	int ret;

	pad_builder_root(c);
	header_calc(c, &headerlen, &headersz);

	/* Uncompressed header is written at p + headersz + 2 (two bytes of
	 * worst-case pack expansion), then header_render stores headerlen
	 * little-endian uint32s. Reject before any store.
	 * Need: headersz + 2 + headerlen*4 <= sz
	 */
	if (headersz > sz || sz - headersz < 2)
		return -1;
	if (headerlen > (sz - headersz - 2) / 4)
		return -1;

	header = p + headersz + 2;

	ret = header_render(c, c->seglist, header, headerlen, &datasz);
	if (ret != 0)
		return -1;

	memset(&z, 0, sizeof(z));
	z.next_in = header;
	z.avail_in = headersz;
	z.next_out = p;
	z.avail_out = sz;

	// pack the headers
	ret = capn_deflate(&z);
	if (ret != 0 || z.avail_in != 0)
		return -1;

	for (seg = c->seglist; seg; seg = seg->next) {
		z.next_in = (uint8_t *)seg->data;
		z.avail_in = seg->len;
		ret = capn_deflate(&z);
		if (ret != 0 || z.avail_in != 0)
			return -1;
	}

	return (int64_t)(sz - z.avail_out);
}

/* Copy the root into one segment. A null root leaves dst empty.
 * A broken pointer, a spent traversal budget, or a capability is an
 * error. Shared objects are duplicated, which is what a re-encode
 * does. The first segment starts large enough for the source, and
 * grows when a shared copy needs more than that. */
static int copy_for_flat(struct capn *src, struct capn *dst)
{
	capn_ptr root, obj;
	int null_word;
	size_t guess, saved_used;
	struct capn_segment *seg;
	int tries;

	memset(dst, 0, sizeof *dst);
	if (!src || !src->seglist)
		return 0;
	if (capn_validate(src) != 0 || !capn_ok(src))
		return -1;
	root = capn_root(src);
	obj = capn_getp(root, 0, 1);
	if (!capn_ok(src))
		return -1;
	null_word = root.seg && root.data &&
		capn_flip64(*(uint64_t *) root.data) == 0;
	if (obj.type == CAPN_NULL) {
		if (!null_word)
			return -1;
		return 0;
	}
	/* A capability has no single-segment encoding. Lists and structs do. */
	if (obj.type == CAPN_INTERFACE)
		return -1;

	saved_used = src->traversal_used;
	guess = 8;
	for (seg = src->seglist; seg; seg = seg->next) {
		if (guess > (size_t) -1 - seg->len)
			return -1;
		guess += seg->len;
	}
	for (tries = 0; tries < 8; tries++) {
		struct capn_segment *made;
		size_t used;
		int req;

		if (guess > (size_t) INT_MAX)
			return -1;
		req = (int) guess;
		src->traversal_used = saved_used;
		capn_init_malloc(dst);
		dst->traversal_limit = src->traversal_limit;
		dst->nesting_limit = src->nesting_limit;
		made = dst->create ? dst->create(dst->user, 0, req) : NULL;
		if (!made) {
			capn_free(dst);
			memset(dst, 0, sizeof *dst);
			return -1;
		}
		capn_append_segment(dst, made);
		dst->dup_copy = 1;
		if (capn_set_root(dst, obj) != 0 || !capn_ok(src)) {
			capn_free(dst);
			memset(dst, 0, sizeof *dst);
			return -1;
		}
		if (dst->segnum == 1)
			return 0;
		used = 0;
		for (seg = dst->seglist; seg; seg = seg->next) {
			if (used > (size_t) -1 - seg->len) {
				capn_free(dst);
				memset(dst, 0, sizeof *dst);
				return -1;
			}
			used += seg->len;
		}
		capn_free(dst);
		memset(dst, 0, sizeof *dst);
		if (used > (size_t) -1 / 2)
			return -1;
		guess = used * 2;
	}
	return -1;
}

int64_t capn_write_flat(struct capn *c, uint8_t *p, size_t sz)
{
	struct capn dst;
	struct capn_segment *seg;
	int64_t n;

	if (!c || !p)
		return -1;
	if (copy_for_flat(c, &dst) != 0)
		return -1;
	if (dst.segnum == 0) {
		/* A null root re-encodes as a canonical empty struct. */
		static const uint8_t empty_struct[8] = {
			0xfc, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
		};
		capn_free(&dst);
		if (sz < 8)
			return -1;
		memcpy(p, empty_struct, 8);
		return 8;
	}
	seg = dst.seglist;
	if (!seg || seg->len > sz) {
		capn_free(&dst);
		return -1;
	}
	memcpy(p, seg->data, seg->len);
	n = (int64_t) seg->len;
	capn_free(&dst);
	return n;
}

int64_t capn_write_flat_packed(struct capn *c, uint8_t *p, size_t sz)
{
	uint8_t *raw;
	int64_t n;
	struct capn_stream z;
	int ret;

	if (!c || !p)
		return -1;
	{
		struct capn dst;
		struct capn_segment *one;
		static const uint8_t empty_struct[8] = {
			0xfc, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
		};

		if (copy_for_flat(c, &dst) != 0)
			return -1;
		if (dst.segnum == 0) {
			raw = (uint8_t *) malloc(8);
			if (!raw)
				return -1;
			memcpy(raw, empty_struct, 8);
			n = 8;
		} else {
			one = dst.seglist;
			if (!one) {
				capn_free(&dst);
				return -1;
			}
			raw = (uint8_t *) malloc(one->len ? one->len : 1);
			if (!raw) {
				capn_free(&dst);
				return -1;
			}
			memcpy(raw, one->data, one->len);
			n = (int64_t) one->len;
			capn_free(&dst);
		}
	}
	memset(&z, 0, sizeof z);
	z.next_in = raw;
	z.avail_in = (size_t) n;
	z.next_out = p;
	z.avail_out = sz;
	ret = capn_deflate(&z);
	free(raw);
	if (ret != 0 || z.avail_in != 0)
		return -1;
	return (int64_t) (sz - z.avail_out);
}

int64_t
capn_write_mem(struct capn *c, uint8_t *p, size_t sz, int packed)
{
	struct capn_segment *seg;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	uint8_t *header;
	int ret;

	if (no_segments(c))
		return -1;

	if (packed)
		return capn_write_mem_packed(c, p, sz);

	pad_builder_root(c);
	header_calc(c, &headerlen, &headersz);
	header = p;

	if (sz < headersz)
		return -1;

	ret = header_render(c, c->seglist, header, headerlen, &datasz);
	if (ret != 0)
		return -1;

	if (datasz > (size_t) -1 - headersz || sz < headersz + datasz)
		return -1;

	p += headersz;

	for (seg = c->seglist; seg; seg = seg->next) {
		memcpy(p, seg->data, seg->len);
		p += seg->len;
	}

	return (int64_t)(headersz + datasz);
}

static int _write_fd(ssize_t (*write_fd)(int fd, const void *p, size_t count), int fd, void *p, size_t count)
{
	ssize_t ret;
	size_t sent = 0;

	while (sent < count) {
		ret = write_fd(fd, ((uint8_t*)p)+sent, count-sent);
		if (ret < 0) {
#ifndef __KERNEL__
			if (errno == EAGAIN || errno == EINTR)
				continue;
			else
#endif /* !__KERNEL__ */
				return -1;
		}
		sent += ret;
	}

	return 0;
}

/* Packed encoding can grow (a dense 4096-byte segment becomes 4098).
 * Stream through buf: on CAPN_NEED_MORE, emit the filled chunk and
 * reuse the buffer. Keep z so a raw run that spans a chunk continues. */
static int packed_emit_seg(struct capn_stream *z,
	unsigned char *buf, size_t bufcap,
	const uint8_t *src, size_t len,
	int (*emit)(void *ctx, const void *p, size_t n),
	void *ctx,
	size_t *written)
{
	memset(z, 0, sizeof(*z));
	z->next_in = src;
	z->avail_in = len;

	while (z->avail_in) {
		size_t n;
		int ret;

		z->next_out = buf;
		z->avail_out = bufcap;
		ret = capn_deflate(z);
		if (ret != 0 && ret != CAPN_NEED_MORE)
			return -1;
		n = bufcap - z->avail_out;
		if (n == 0)
			return -1;
		if (emit(ctx, buf, n) < 0)
			return -1;
		*written += n;
		if (ret == 0)
			return 0;
	}

	return 0;
}

struct write_fd_ctx {
	ssize_t (*write_fd)(int fd, const void *p, size_t count);
	int fd;
};

static int emit_fd(void *ctx, const void *p, size_t n)
{
	struct write_fd_ctx *e = (struct write_fd_ctx *)ctx;
	return _write_fd(e->write_fd, e->fd, (void *)p, n);
}

/* The segment table grows with the segment count. A packed header
 * needs a second copy of that table plus two bytes. The 4096 byte
 * floor is the chunk size a small message already used. */
static unsigned char *header_buf(size_t headersz, int packed, size_t *cap_out)
{
	size_t need = headersz;
	size_t cap = 4096;

	if (packed) {
		if (headersz > (SIZE_MAX - 2) / 2)
			return NULL;
		need = headersz * 2 + 2;
	}
	if (need > cap)
		cap = need;
	if (cap == 0)
		cap = 1;
	*cap_out = cap;
	return (unsigned char *) malloc(cap);
}

int capn_write_fd(struct capn *c, ssize_t (*write_fd)(int fd, const void *p, size_t count), int fd, int packed)
{
	unsigned char *buf = NULL;
	size_t bufcap = 0;
	struct capn_segment *seg;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	int ret;
	struct capn_stream z;
	unsigned char *p;

	if (no_segments(c))
		return -1;

	/* A zero-word segment is a real segment. C++ writeMessage
	 * writes it. The root read is a separate step. */
	pad_builder_root(c);
	header_calc(c, &headerlen, &headersz);

	buf = header_buf(headersz, packed, &bufcap);
	if (buf == NULL)
		return -1;

	ret = header_render(c, c->seglist, buf, headerlen, &datasz);
	if (ret != 0)
		goto out;

	if (packed) {
		size_t headerrem = bufcap - headersz;
		size_t maxpack = headersz + 2;
		if (headerrem < maxpack) {
			ret = -1;
			goto out;
		}

		memset(&z, 0, sizeof(z));
		z.next_in = buf;
		z.avail_in = headersz;
		z.next_out = buf + headersz;
		z.avail_out = headerrem;
		ret = capn_deflate(&z);
		if (ret != 0)
			goto out;

		p = buf + headersz;
		headersz = headerrem - z.avail_out;
	} else {
		p = buf;
	}

	ret = _write_fd(write_fd, fd, p, headersz);
	if (ret < 0)
		goto out;

	datasz = headersz;
	for (seg = c->seglist; seg; seg = seg->next) {
		if (packed) {
			struct write_fd_ctx e;
			e.write_fd = write_fd;
			e.fd = fd;
			ret = packed_emit_seg(&z, buf, bufcap,
				(const uint8_t *)seg->data, seg->len,
				emit_fd, &e, &datasz);
			if (ret < 0)
				goto out;
		} else {
			ret = _write_fd(write_fd, fd, (uint8_t *)seg->data, seg->len);
			if (ret < 0)
				goto out;
			if (add_size(&datasz, seg->len)) {
				ret = -1;
				goto out;
			}
		}
	}

	ret = (int) datasz;
out:
	free(buf);
	return ret;
}

#ifndef __KERNEL__
static int _write_fp(FILE *f, const void *p, size_t count)
{
	size_t sent = 0;

	while (sent < count) {
		size_t ret = fwrite(((const uint8_t*)p)+sent, 1, count-sent, f);
		if (ret == 0)
			return -1;
		sent += ret;
	}

	return 0;
}

static int emit_fp(void *ctx, const void *p, size_t n)
{
	return _write_fp((FILE *)ctx, p, n);
}

int capn_write_fp(struct capn *c, FILE *f, int packed)
{
	unsigned char *buf = NULL;
	size_t bufcap = 0;
	struct capn_segment *seg;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	int ret;
	struct capn_stream z;
	unsigned char *p;

	if (no_segments(c) || f == NULL)
		return -1;

	pad_builder_root(c);
	header_calc(c, &headerlen, &headersz);

	buf = header_buf(headersz, packed, &bufcap);
	if (buf == NULL)
		return -1;

	ret = header_render(c, c->seglist, buf, headerlen, &datasz);
	if (ret != 0)
		goto out;

	if (packed) {
		size_t headerrem = bufcap - headersz;
		size_t maxpack = headersz + 2;
		if (headerrem < maxpack) {
			ret = -1;
			goto out;
		}

		memset(&z, 0, sizeof(z));
		z.next_in = buf;
		z.avail_in = headersz;
		z.next_out = buf + headersz;
		z.avail_out = headerrem;
		ret = capn_deflate(&z);
		if (ret != 0)
			goto out;

		p = buf + headersz;
		headersz = headerrem - z.avail_out;
	} else {
		p = buf;
	}

	ret = _write_fp(f, p, headersz);
	if (ret < 0)
		goto out;

	datasz = headersz;
	for (seg = c->seglist; seg; seg = seg->next) {
		if (packed) {
			ret = packed_emit_seg(&z, buf, bufcap,
				(const uint8_t *)seg->data, seg->len,
				emit_fp, f, &datasz);
			if (ret < 0)
				goto out;
		} else {
			ret = _write_fp(f, seg->data, seg->len);
			if (ret < 0)
				goto out;
			if (add_size(&datasz, seg->len)) {
				ret = -1;
				goto out;
			}
		}
	}

	if (fflush(f) != 0) {
		ret = -1;
		goto out;
	}

	ret = (int) datasz;
out:
	free(buf);
	return ret;
}
#else /* __KERNEL__ */
int capn_write_fp(struct capn *c, FILE *f, int packed)
{
	return -1;
}
#endif /* !__KERNEL__ */

int64_t capn_size(struct capn *c)
{
	size_t headersz, datasz = 0;
	struct capn_segment *seg;
	uint32_t i;

	if (no_segments(c))
		return -1;

	pad_builder_root(c);
	seg = c->seglist;

	headersz = 8 * ((2 + c->segnum) / 2);

	for (i = 0; i < c->segnum; i++, seg = seg->next) {
		if (0 == seg)
			return -1;
		if (add_size(&datasz, seg->len))
			return -1;
	}
	if (0 != seg)
		return -1;
	if (datasz > (size_t) -1 - headersz)
		return -1;

	return (int64_t)(headersz + datasz);
}
