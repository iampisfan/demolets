#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <sys/uio.h>
#include "demo.h"

static const char * socket_path = "/var/run/demo.sock";

int main(void)
{
	struct sockaddr_un sun = {.sun_family = AF_UNIX};
	int server_fd, client_fd;
	struct message msg;
	int addrlen;
	char tlv_v[100];
	struct iovec iov[2];

	strcpy(sun.sun_path, socket_path);

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (server_fd < 0) {
		printf("create server socket failed\n");
		return -1;
	}

	const int one = 1;
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (bind(server_fd, (struct sockaddr*)&sun, sizeof(sun)) < 0) {
		printf("server socket bind failed\n");
		close(server_fd);
		return -1;
	}

	memset(&msg, 0, sizeof(msg));
	if (listen(server_fd, SOMAXCONN) < 0) {
		printf("server socket start listen failed\n");
		close(server_fd);
		return -1;
	}

	addrlen = sizeof(sun);
	client_fd = accept(server_fd, (struct sockaddr*)&sun, &addrlen);
	if (client_fd < 0) {
		printf("server socket accept failed\n");
		close(server_fd);
		return -1;
	}

	iov[0].iov_base = &msg;
	iov[0].iov_len = sizeof(msg);
	iov[1].iov_base = tlv_v;
	iov[1].iov_len = sizeof(tlv_v);
	if (readv(client_fd, iov, 2) > 0) {
		printf("server receive:\n");
		printf("\talid = %x:%x:%x:%x:%x:%x\n", msg.alid[0], msg.alid[1], msg.alid[2], msg.alid[3], msg.alid[4], msg.alid[5]);
		printf("\tmacaddr = %x:%x:%x:%x:%x:%x\n", msg.macaddr[0], msg.macaddr[1], msg.macaddr[2], msg.macaddr[3], msg.macaddr[4], msg.macaddr[5]);
		printf("\tmessagetype = %d\n", msg.tlv.messagetype);
		printf("\ttlv type = %d\n", msg.tlv.tlv_t);
		printf("\ttlv length = %d\n", msg.tlv.tlv_l);
		msg.tlv.tlv_v = tlv_v;
		printf("\ttlv value = %s\n", msg.tlv.tlv_v);
	}

	close(server_fd);
	close(client_fd);
	unlink(socket_path);
	return 0;
}
