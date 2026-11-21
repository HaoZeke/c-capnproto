/* Loopback bind and a nonblocking recv with the macros removed.
 * Apple headers omit INADDR_LOOPBACK and MSG_DONTWAIT under
 * _POSIX_C_SOURCE=200809L. The helpers must still compile. */

/* Force the branch Apple headers take when these macros are absent. */
#define POSIX_NET_FORCE_FALLBACK 1
#undef INADDR_LOOPBACK
#undef MSG_DONTWAIT

#include "posix_net.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(void)
{
	struct sockaddr_in addr;
	char buf[8];
	int lfd, pair[2], one = 1;
	ssize_t n;

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd < 0)
		return 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = posix_loopback_addr();
	addr.sin_port = 0;
	if (bind(lfd, (struct sockaddr *)&addr, sizeof addr) != 0) {
		perror("bind");
		close(lfd);
		return 1;
	}
	close(lfd);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0)
		return 1;
	{
		int before = fcntl(pair[0], F_GETFL, 0);
		if (before < 0 || (before & O_NONBLOCK) != 0)
			return 1;
		n = posix_recv_nonblock(pair[0], buf, sizeof buf);
		{
			int after = fcntl(pair[0], F_GETFL, 0);
			if (after < 0 || (after & O_NONBLOCK) != 0) {
				fprintf(stderr, "O_NONBLOCK left set\n");
				return 1;
			}
		}
	}
	n = posix_recv_nonblock(pair[0], buf, sizeof buf);
	if (n >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
		fprintf(stderr, "nonblocking recv returned %zd errno %d\n", n, errno);
		close(pair[0]);
		close(pair[1]);
		return 1;
	}
	close(pair[0]);
	close(pair[1]);
	return 0;
}
