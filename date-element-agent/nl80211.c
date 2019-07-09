#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "nl80211.h"

struct nl80211_global nl_global;

static void nl80211_destroy_sock(struct nl_sock **sock)
{
	if (*sock == NULL)
		return;
	nl_socket_free(*sock);
	*sock = NULL;
}

int nl80211_init_nl_global()
{
	int ret = 0;
	struct nl80211_global *global = &nl_global;

	memset(global, 0, sizeof(struct nl80211_global));

	global->nl_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!global->nl_cb) {
		printf("Failed to allocate netlink callback\n");
		return -ENOMEM;
	}

	global->nl_sock = nl_socket_alloc_cb(global->nl_cb);
	if (!global->nl_sock) {
		printf("Failed to allocate netlink command socket\n");
		ret = -ENOMEM;
		goto err;
	}
	nl_socket_set_buffer_size(global->nl_sock, 8192, 8192);
	if (genl_connect(global->nl_sock)) {
		printf("Failed to connect to generic netlink\n");
		ret = -ENOLINK;
		goto err;
	}

	global->nl80211_id = genl_ctrl_resolve(global->nl_sock, "nl80211");
	if (global->nl80211_id < 0) {
		printf("nl80211 not find\n");
		ret = -ENOENT;
		goto err;
	}

	global->nl_event = nl_socket_alloc_cb(global->nl_cb);
	if (!global->nl_event) {
		printf("Failed to allocate netlink event socket\n");
		ret = -ENOMEM;
		goto err;
	}
	nl_socket_set_buffer_size(global->nl_event, 8192, 8192);
	if (genl_connect(global->nl_event)) {
		printf("Failed to connect to generic netlink\n");
		ret = -ENOLINK;
		goto err;
	}

	return 0;

err:
	nl80211_destroy_sock(&global->nl_sock);
	nl80211_destroy_sock(&global->nl_event);
	nl_cb_put(global->nl_cb);
	global->nl_cb = NULL;
	return ret;
}

void nl80211_cleanup_nl_global()
{
	struct nl80211_global *global = &nl_global;

	nl80211_destroy_sock(&global->nl_sock);
	nl80211_destroy_sock(&global->nl_event);
	nl_cb_put(global->nl_cb);
	global->nl_cb = NULL;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
		void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

struct nl_msg *nl80211_new_msg(int flags, uint8_t cmd)
{
	struct nl_msg *msg = NULL;
	int ret = -1;

	msg = nlmsg_alloc();
	if (!msg) {
		printf("malloc nl80211_msg failed\n");
		return NULL;
	}

	if (!genlmsg_put(msg, 0, 0, nl_global.nl80211_id, 0, flags, cmd, 0)) {
		printf("genlmsg_put failed cmd = %d\n", cmd);
		nlmsg_free(msg);
		return NULL;
	}

	return msg;
}

int nl80211_send_and_recv(struct nl_msg *msg,
			int (*valid_handler)(struct nl_msg *, void *),
			void *valid_data)
{
	struct nl_cb *cb;
	int err = -1;

	if (!msg)
		return -ENOMEM;

	cb = nl_cb_clone(nl_global.nl_cb);
	if (!cb) {
		printf("failed to allocate netlink callbacks\n");
		nlmsg_free(msg);
		return -ENOMEM;
	}

	err = nl_send_auto_complete(nl_global.nl_sock, msg);
	if (err < 0)
		goto out;

	err = 1;
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, valid_data);

	while (err > 0)
		nl_recvmsgs(nl_global.nl_sock, cb);

out:
	nl_cb_put(cb);
	free(msg);
	return err;
}