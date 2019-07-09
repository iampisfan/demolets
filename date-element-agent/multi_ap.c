#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <unistd.h>
#include <sys/uio.h>
#include <string.h>

#include "multi_ap.h"
#include "debug.h"

int collect_de_from_easymesh(unsigned int msgtype, char **resp)
{
	struct sockaddr_un sun = {.sun_family = AF_UNIX};
	const int one = 1;
	struct iovec iov;
	struct message msg;
	struct pollfd pollfds[1];
	int ret = 0;
	int client_socket_fd = -1;

	strcpy(sun.sun_path, CONTROLLER_SOCKET_PATH);

	client_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (client_socket_fd < 0) {
		debug("create de-agent client socket failed\n");
		return -1;
	}

	setsockopt(client_socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (connect(client_socket_fd, (struct sockaddr *)&sun, sizeof(sun)) < 0) {
		ret = -1;
		debug("connect to easymesh socket server failed\n");
		goto exit;
	}

	memset(msg.alid, 0xff, 6);
	memset(msg.macaddr, 0xff, 6);
	msg.messagetype = msgtype;

	if (write(client_socket_fd, &msg, sizeof(msg)) != sizeof(msg)) {
		ret = -1;
		debug("send msg %x to easymesh failed\n", msgtype);
		goto exit;
	}

	pollfds[0].fd = client_socket_fd;
	pollfds[0].events = POLLIN;
	poll(pollfds, 2, TIMEOUT);
	if (pollfds[0].events & POLLIN) {
		*resp = calloc(1, 256*1024);
		iov.iov_base = *resp;
		iov.iov_len = 256*1024;
		if (readv(client_socket_fd, &iov, 1) > 0) {
			ret = 0;
			debug("recv from eashmesh:\n%s\n", *resp);
		} else {
			ret = -1;
			debug("recv from eashmesh with empty\n");
		}
	} else {
		ret = -1;
		debug("wait for easymesh response timeout\n");
	}

exit:
	close(client_socket_fd);
	return ret;
}

