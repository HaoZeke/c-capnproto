/* Socket helpers that stay inside POSIX.1-2008.
 *
 * INADDR_LOOPBACK and MSG_DONTWAIT are absent from Apple headers when
 * the translation unit is built with _POSIX_C_SOURCE=200809L. */

#ifndef POSIX_NET_H
#define POSIX_NET_H

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

static inline uint32_t posix_loopback_addr(void)
{
#if defined(INADDR_LOOPBACK) && !defined(POSIX_NET_FORCE_FALLBACK)
	return htonl(INADDR_LOOPBACK);
#else
	return htonl(0x7f000001u);
#endif
}

static inline ssize_t posix_recv_nonblock(int fd, void *buf, size_t len)
{
#if defined(MSG_DONTWAIT) && !defined(POSIX_NET_FORCE_FALLBACK)
	return recv(fd, buf, len, MSG_DONTWAIT);
#else
	int flags = fcntl(fd, F_GETFL, 0);
	int raised = 0;
	ssize_t n;

	if (flags < 0)
		return -1;
	if ((flags & O_NONBLOCK) == 0) {
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 0)
			return -1;
		raised = 1;
	}
	n = recv(fd, buf, len, 0);
	if (raised && fcntl(fd, F_SETFL, flags) != 0)
		return -1;
	return n;
#endif
}

#endif
