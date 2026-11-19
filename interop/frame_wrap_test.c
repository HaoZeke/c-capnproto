/* Fake peer for interop/rpc_client.c's frame reader.
 *
 * Accept the client, write a segment count of 0xffffffff, and leave the
 * socket open. That count is one less than the segment total, so adding
 * one in a uint32 wraps to zero and the old reader then blocks waiting
 * for a table that never arrives. The client must exit instead.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static int listen_port(int *port)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof addr;
	int fd, one = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = 0;
	if (bind(fd, (struct sockaddr *)&addr, sizeof addr) != 0 ||
	    listen(fd, 1) != 0 ||
	    getsockname(fd, (struct sockaddr *)&addr, &len) != 0) {
		close(fd);
		return -1;
	}
	*port = ntohs(addr.sin_port);
	return fd;
}

static void drain(int fd)
{
	char buf[4096];
	struct timespec ts;
	int i;

	ts.tv_sec = 0;
	ts.tv_nsec = 20000000L;
	for (i = 0; i < 25; i++) {
		ssize_t n = recv(fd, buf, sizeof buf, MSG_DONTWAIT);
		if (n > 0)
			continue;
		if (n == 0)
			return;
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			return;
		nanosleep(&ts, NULL);
	}
}

int main(int argc, char **argv)
{
	char portbuf[16];
	const uint8_t wrapped[4] = {0xff, 0xff, 0xff, 0xff};
	struct timespec step;
	int lfd, cfd, port, status;
	pid_t pid;
	int i;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <rpc_client_c>\n", argv[0]);
		return 2;
	}
	lfd = listen_port(&port);
	if (lfd < 0) {
		perror("listen");
		return 1;
	}
	snprintf(portbuf, sizeof portbuf, "%d", port);
	pid = fork();
	if (pid < 0) {
		perror("fork");
		return 1;
	}
	if (pid == 0) {
		int devnull = open("/dev/null", O_WRONLY);
		close(lfd);
		if (devnull >= 0) {
			dup2(devnull, STDOUT_FILENO);
			dup2(devnull, STDERR_FILENO);
			if (devnull > STDERR_FILENO)
				close(devnull);
		}
		execl(argv[1], argv[1], portbuf, (char *)NULL);
		perror("execl");
		_exit(127);
	}

	cfd = accept(lfd, NULL, NULL);
	if (cfd < 0) {
		perror("accept");
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		return 1;
	}
	drain(cfd);
	if (write(cfd, wrapped, sizeof wrapped) != (ssize_t)sizeof wrapped) {
		perror("write");
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		return 1;
	}

	step.tv_sec = 0;
	step.tv_nsec = 50000000L;
	status = -1;
	for (i = 0; i < 40; i++) {
		pid_t got = waitpid(pid, &status, WNOHANG);
		if (got == pid)
			break;
		nanosleep(&step, NULL);
	}
	close(cfd);
	close(lfd);
	if (i == 40) {
		fprintf(stderr, "client blocked on a wrapping segment count\n");
		kill(pid, SIGKILL);
		waitpid(pid, NULL, 0);
		return 1;
	}
	if (!WIFEXITED(status) || WEXITSTATUS(status) == 0) {
		fprintf(stderr, "client did not fail the wrapping frame\n");
		return 1;
	}
	return 0;
}
