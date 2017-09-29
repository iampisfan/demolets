#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/uio.h>
#include "demo.h"

static char * alid_str = NULL;
static char * macaddr_str = NULL;
static char * tlv_str = NULL;
static struct message msg;

static const char * socket_path = "/var/run/demo.sock";

static int usage()
{
	fprintf(stderr, "Usage:\n"
		"Options:\n"
		" -a <alid>:		alid mac address 00:11:22:33:44:55\n"
		" -m <macaddr>:		mac addr 00:11:22:33:44:55\n"
		" -t <tlv>:		tlv string\n");

	return -1;
}

static void fill_msg() {
	char addr[6];
	if (alid_str) {
		sscanf(alid_str, "%2x:%2x:%2x:%2x:%2x:%2x", &(msg.alid[0]), &(msg.alid[1]), &(msg.alid[2]),
				&(msg.alid[3]), &(msg.alid[4]), &(msg.alid[5]));
	}
	if (macaddr_str) {
		sscanf(macaddr_str, "%2x:%2x:%2x:%2x:%2x:%2x", &(msg.macaddr[0]), &(msg.macaddr[1]), &(msg.macaddr[2]),
                                &(msg.macaddr[3]), &(msg.macaddr[4]), &(msg.macaddr[5]));
	}
	if (tlv_str) {
		msg.tlv.messagetype = (*tlv_str) - '0';
		msg.tlv.tlv_t = (*(tlv_str + 1)) - '0';
		msg.tlv.tlv_l = (*(tlv_str + 2)) - '0';
		msg.tlv.tlv_v = strdup(tlv_str + 3);
	}
}

int main(int argc, char **argv)
{
        struct sockaddr_un sun = {.sun_family = AF_UNIX};
        int sock;
	int ch;
	struct iovec iov[2];

	while ((ch = getopt(argc, argv, "a:m:t:")) != -1) {
		switch(ch) {
		case 'a':
			alid_str = optarg;
			break;
		case 'm':
			macaddr_str = optarg;
			break;
		case 't':
			tlv_str = optarg;
			break;
		default:
			return usage();
		}
	}
	fill_msg();

        strcpy(sun.sun_path, socket_path);

        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sock < 0)
                return -1;

        const int one = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	if (connect(sock, (struct sockaddr *)&sun, sizeof(sun)) < 0) {
		close(sock);
		return -1;
	}

	iov[0].iov_base = &msg;
	iov[0].iov_len = sizeof(msg);
	iov[1].iov_base = msg.tlv.tlv_v;
	iov[1].iov_len = msg.tlv.tlv_l;
	if (writev(sock, iov, 2) < 0) {
		close(sock);
		return -1;
	}

	close(sock);
	return 0;
}
