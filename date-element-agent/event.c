#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <net/if.h>
#include <poll.h>
#include <pthread.h>
#include <libubus.h>

#include "event.h"

static struct dl_list nl_event_list;
extern struct nl80211_global nl_global;

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

struct handler_args {
	const char *group;
	int id;
};

static int family_handler(struct nl_msg *msg, void *arg)
{
	struct handler_args *grp = arg;
	struct nlattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *mcgrp;
	int rem_mcgrp;

	nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[CTRL_ATTR_MCAST_GROUPS])
		return NL_SKIP;

	nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
		struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

		nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
			  nla_data(mcgrp), nla_len(mcgrp), NULL);

		if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
		    !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
			continue;
		if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]),
			    grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
			continue;
		grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
		break;
	}

	return NL_SKIP;
}

static int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group)
{
	struct nl_msg *msg;
	struct nl_cb *cb;
	int ret, ctrlid;
	struct handler_args grp = {
		.group = group,
		.id = -ENOENT,
	};

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		ret = -ENOMEM;
		goto out_fail_cb;
	}

	ctrlid = genl_ctrl_resolve(sock, "nlctrl");

	genlmsg_put(msg, 0, 0, ctrlid, 0, 0, CTRL_CMD_GETFAMILY, 0);

	ret = -ENOBUFS;
	NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

	ret = nl_send_auto_complete(sock, msg);
	if (ret < 0)
		goto out;

	ret = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

	while (ret > 0)
		nl_recvmsgs(sock, cb);

	if (ret == 0)
		ret = grp.id;
 nla_put_failure:
 out:
	nl_cb_put(cb);
 out_fail_cb:
	nlmsg_free(msg);
	return ret;
}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

static int nl80211_event_handler(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct nl80211_event_list_item *item;
	u32 ifindex;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (tb[NL80211_ATTR_IFINDEX])
		ifindex = nla_get_u32(tb[NL80211_ATTR_IFINDEX]);
	else
		return NL_SKIP;

	dl_list_for_each(item, &nl_event_list,
					struct nl80211_event_list_item, dl_list) {
		if (ifindex == item->nl_event->ifindex) {
			item->nl_event->handler(msg, item->nl_event->arg, ifindex);
			break;
		}
	}

	return NL_SKIP;
}

static void *loop_pthread(void *arg)
{
	while (1)
		nl_recvmsgs(nl_global.nl_event, nl_global.nl_cb);
}

int nl80211_setup_event_monitor(void)
{
	int mcid, ret;
	struct nl80211_global *global = &nl_global;

	/* Configuration multicast group */
	mcid = nl_get_multicast_id(global->nl_sock, "nl80211", "config");
	if (mcid < 0)
		return mcid;

	ret = nl_socket_add_membership(global->nl_sock, mcid);
	if (ret)
		return ret;

	/* Scan multicast group */
	mcid = nl_get_multicast_id(global->nl_sock, "nl80211", "scan");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(global->nl_event, mcid);
		if (ret)
			return ret;
	}

	/* Regulatory multicast group */
	mcid = nl_get_multicast_id(global->nl_sock, "nl80211", "regulatory");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(global->nl_event, mcid);
		if (ret)
			return ret;
	}

	/* MLME multicast group */
	mcid = nl_get_multicast_id(global->nl_sock, "nl80211", "mlme");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(global->nl_event, mcid);
		if (ret)
			return ret;
	}

	mcid = nl_get_multicast_id(global->nl_sock, "nl80211", "vendor");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(global->nl_event, mcid);
		if (ret)
			return ret;
	}

	/* no sequence checking for multicast messages */
	nl_cb_set(global->nl_cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_set(global->nl_cb, NL_CB_VALID, NL_CB_CUSTOM, nl80211_event_handler, NULL);

	dl_list_init(&nl_event_list);
}

void nl80211_register_event_handler(u32 ifindex,
				int (*handler)(struct nl_msg *msg, void *arg, u32 ifindex),
				void *arg)
{
	struct nl80211_event_list_item *item;

	item = calloc(1, sizeof(struct nl80211_event_list_item));
	if (!item) {
		printf("nl80211_register_event_handler allocate nl80211_event_list_item failed\n");
		return;
	}

	item->nl_event = calloc(1, sizeof(struct nl80211_event_data));
	if (!item->nl_event) {
		printf("nl80211_register_event_handler allocate nl80211_event_data failed\n");
		return;
	}

	item->nl_event->ifindex = ifindex;
	item->nl_event->handler = handler;
	item->nl_event->arg = arg;

	dl_list_add_tail(&nl_event_list, &item->dl_list);
}

void run_event_uloop(void)
{
	pthread_t pthread;
	pthread_create(&pthread, NULL, loop_pthread, NULL);
}

