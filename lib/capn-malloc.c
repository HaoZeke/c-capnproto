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

static struct capn_segment *create(void *u, uint32_t id, int sz) {
	struct capn_segment *s;
	sz += sizeof(*s);
	if (sz < 4096) {
		sz = 4096;
	} else {
		sz = (sz + 4095) & ~4095;
	}
	s = (struct capn_segment*) calloc(1, sz);
	s->data = (char*) (s+1);
	s->cap = sz - sizeof(*s);
	s->user = s;
	return s;
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
 * The first word of the stream is (segnum - 1); more than CAPN_MAX_SEGS
 * segments cannot be decoded. */
#define CAPN_MAX_SEGS 1024

static int read_fp(void *p, size_t sz, FILE *f, struct capn_stream *z, uint8_t* zbuf, int packed) {
#ifndef __KERNEL__
	if (f && packed) {
		z->next_out = (uint8_t*) p;
		z->avail_out = sz;

		while (z->avail_out && capn_inflate(z) == CAPN_NEED_MORE) {
			int r;
			memmove(zbuf, z->next_in, z->avail_in);
			r = fread(zbuf+z->avail_in, 1, ZBUF_SZ - z->avail_in, f);
			if (r <= 0)
				return -1;
			z->avail_in += r;
		}
		return 0;

	} else if (f && !packed) {
		return fread(p, sz, 1, f) != 1;

	} else
#endif /* !__KERNEL__ */
	if (packed) {
		z->next_out = (uint8_t*) p;
		z->avail_out = sz;
		return capn_inflate(z) != 0;

	} else {
		if (z->avail_in < sz)
			return -1;
		memcpy(p, z->next_in, sz);
		z->next_in += sz;
		z->avail_in -= sz;
		return 0;
	}
}

static int init_fp(struct capn *c, FILE *f, struct capn_stream *z, int packed) {
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
	uint32_t i, segnum, total = 0;
	uint32_t hdr[CAPN_MAX_SEGS];
	uint8_t zbuf[ZBUF_SZ];
	char *data = NULL;

	capn_init_malloc(c);

	/* Read the first four bytes to know how many headers we have */
	if (read_fp(&segnum, 4, f, z, zbuf, packed))
		goto err;

	segnum = capn_flip32(segnum);
	if (segnum > CAPN_MAX_SEGS - 1)
		goto err;
	segnum++; /* The wire encoding was zero-based */

	/* Read the header list */
	if (read_fp(hdr, 8 * (segnum/2) + 4, f, z, zbuf, packed))
		goto err;

	for (i = 0; i < segnum; i++) {
		uint32_t n = capn_flip32(hdr[i]);
		if (n > INT_MAX/8 || n > UINT32_MAX/8 || UINT32_MAX - total < n*8)
			goto err;
		hdr[i] = n*8;
		total += hdr[i];
	}

	/* Allocate space for the data and the capn_segment structs */
	s = (struct capn_segment*) calloc(1, total + (sizeof(*s) * segnum));
	if (!s)
		goto err;

	/* Now read the data and setup the capn_segment structs */
	data = (char*) (s+segnum);
	if (read_fp(data, total, f, z, zbuf, packed))
		goto err;

	for (i = 0; i < segnum; i++) {
		s[i].len = s[i].cap = hdr[i];
		s[i].data = data;
		data += s[i].len;
		capn_append_segment(c, &s[i]);
	}

    /* Set the entire region to be freed on the last segment */
	s[segnum-1].user = s;

	return 0;

err:
	memset(c, 0, sizeof(*c));
	free(s);
	return -1;
}

int capn_init_fp(struct capn *c, FILE *f, int packed) {
	struct capn_stream z;
	memset(&z, 0, sizeof(z));
	return init_fp(c, f, &z, packed);
}

int capn_init_mem(struct capn *c, const uint8_t *p, size_t sz, int packed) {
	struct capn_stream z;
	memset(&z, 0, sizeof(z));
	z.next_in = p;
	z.avail_in = sz;
	return init_fp(c, NULL, &z, packed);
}

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

static int header_render(struct capn *c, struct capn_segment *seg, uint32_t *header, uint32_t headerlen, size_t *datasz)
{
	size_t i;
	uint32_t val;

	val = capn_flip32(c->segnum - 1);
	memcpy(&header[0], &val, sizeof(val));
	val = 0;
	memcpy(&header[headerlen-1], &val, sizeof(val)); /* Zero out the spare position in the header sizes */
	for (i = 0; i < c->segnum; i++, seg = seg->next) {
		if (0 == seg)
			return -1;
		*datasz += seg->len;
		val = capn_flip32(seg->len / 8);
		memcpy(&header[1 + i], &val, sizeof(val));
	}
	if (0 != seg)
		return -1;

	return 0;
}

static int64_t capn_write_mem_packed(struct capn *c, uint8_t *p, size_t sz)
{
	struct capn_segment *seg;
	struct capn_ptr root;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	uint32_t *header;
	struct capn_stream z;
	int ret;

	root = capn_root(c);
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

	header = (uint32_t*) (p + headersz + 2);

	ret = header_render(c, root.seg, header, headerlen, &datasz);
	if (ret != 0)
		return -1;

	memset(&z, 0, sizeof(z));
	z.next_in = (uint8_t *)header;
	z.avail_in = headersz;
	z.next_out = p;
	z.avail_out = sz;

	// pack the headers
	ret = capn_deflate(&z);
	if (ret != 0 || z.avail_in != 0)
		return -1;

	for (seg = root.seg; seg; seg = seg->next) {
		z.next_in = (uint8_t *)seg->data;
		z.avail_in = seg->len;
		ret = capn_deflate(&z);
		if (ret != 0 || z.avail_in != 0)
			return -1;
	}

	return (int64_t)(sz - z.avail_out);
}

int64_t
capn_write_mem(struct capn *c, uint8_t *p, size_t sz, int packed)
{
	struct capn_segment *seg;
	struct capn_ptr root;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	uint32_t *header;
	int ret;

	if (c->segnum == 0)
		return -1;

	if (packed)
		return capn_write_mem_packed(c, p, sz);

	root = capn_root(c);
	header_calc(c, &headerlen, &headersz);
	header = (uint32_t*) p;

	if (sz < headersz)
		return -1;

	ret = header_render(c, root.seg, header, headerlen, &datasz);
	if (ret != 0)
		return -1;

	if (sz < headersz + datasz)
		return -1;

	p += headersz;

	for (seg = root.seg; seg; seg = seg->next) {
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

int capn_write_fd(struct capn *c, ssize_t (*write_fd)(int fd, const void *p, size_t count), int fd, int packed)
{
	unsigned char buf[4096];
	struct capn_segment *seg;
	struct capn_ptr root;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	int ret;
	struct capn_stream z;
	unsigned char *p;

	if (c->segnum == 0)
		return -1;

	root = capn_root(c);
	header_calc(c, &headerlen, &headersz);

	if (sizeof(buf) < headersz)
		return -1;

	ret = header_render(c, root.seg, (uint32_t*)buf, headerlen, &datasz);
	if (ret != 0)
		return -1;

	if (packed) {
		const int headerrem = sizeof(buf) - headersz;
		const int maxpack = headersz + 2;
		if (headerrem < maxpack)
			return -1;

		memset(&z, 0, sizeof(z));
		z.next_in = buf;
		z.avail_in = headersz;
		z.next_out = buf + headersz;
		z.avail_out = headerrem;
		ret = capn_deflate(&z);
		if (ret != 0)
			return -1;

		p = buf + headersz;
		headersz = headerrem - z.avail_out;
	} else {
		p = buf;
	}

	ret = _write_fd(write_fd, fd, p, headersz);
	if (ret < 0)
		return -1;

	datasz = headersz;
	for (seg = root.seg; seg; seg = seg->next) {
		size_t bufsz;
		if (packed) {
			memset(&z, 0, sizeof(z));
			z.next_in = (uint8_t*)seg->data;
			z.avail_in = seg->len;
			z.next_out = buf;
			z.avail_out = sizeof(buf);
			ret = capn_deflate(&z);
			if (ret != 0)
				return -1;
			p = buf;
			bufsz = sizeof(buf) - z.avail_out;
		} else {
			p = (uint8_t*)seg->data;
			bufsz = seg->len;
		}
		ret = _write_fd(write_fd, fd, p, bufsz);
		if (ret < 0)
			return -1;
		datasz += bufsz;
	}

	return datasz;
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

int capn_write_fp(struct capn *c, FILE *f, int packed)
{
	unsigned char buf[4096];
	struct capn_segment *seg;
	struct capn_ptr root;
	uint32_t headerlen;
	size_t headersz, datasz = 0;
	int ret;
	struct capn_stream z;
	unsigned char *p;

	if (c->segnum == 0 || f == NULL)
		return -1;

	root = capn_root(c);
	header_calc(c, &headerlen, &headersz);

	if (sizeof(buf) < headersz)
		return -1;

	ret = header_render(c, root.seg, (uint32_t*)buf, headerlen, &datasz);
	if (ret != 0)
		return -1;

	if (packed) {
		const int headerrem = sizeof(buf) - headersz;
		const int maxpack = headersz + 2;
		if (headerrem < maxpack)
			return -1;

		memset(&z, 0, sizeof(z));
		z.next_in = buf;
		z.avail_in = headersz;
		z.next_out = buf + headersz;
		z.avail_out = headerrem;
		ret = capn_deflate(&z);
		if (ret != 0)
			return -1;

		p = buf + headersz;
		headersz = headerrem - z.avail_out;
	} else {
		p = buf;
	}

	ret = _write_fp(f, p, headersz);
	if (ret < 0)
		return -1;

	datasz = headersz;
	for (seg = root.seg; seg; seg = seg->next) {
		size_t bufsz;
		if (packed) {
			memset(&z, 0, sizeof(z));
			z.next_in = (uint8_t*)seg->data;
			z.avail_in = seg->len;
			z.next_out = buf;
			z.avail_out = sizeof(buf);
			ret = capn_deflate(&z);
			if (ret != 0)
				return -1;
			p = buf;
			bufsz = sizeof(buf) - z.avail_out;
		} else {
			p = (uint8_t*)seg->data;
			bufsz = seg->len;
		}
		ret = _write_fp(f, p, bufsz);
		if (ret < 0)
			return -1;
		datasz += bufsz;
	}

	if (fflush(f) != 0)
		return -1;

	return (int)datasz;
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
	struct capn_ptr root;
	struct capn_segment *seg;
	uint32_t i;

	if (c->segnum == 0)
		return -1;

	root = capn_root(c);
	seg = root.seg;

	headersz = 8 * ((2 + c->segnum) / 2);

	for (i = 0; i < c->segnum; i++, seg = seg->next) {
		if (0 == seg)
			return -1;
		datasz += seg->len;
	}
	if (0 != seg)
		return -1;

	return (int64_t)(headersz + datasz);
}
