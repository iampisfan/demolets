#ifndef _NL80211_H_
#define _NL80211_H_

#include <linux/nl80211.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "type.h"

struct nl80211_global {
	struct nl_sock *nl_sock;
	struct nl_sock *nl_event;
	struct nl_cb *nl_cb;
	int nl80211_id;
};

int nl80211_init_nl_global(void);
void nl80211_cleanup_nl_global(void);
struct nl_msg *nl80211_new_msg(int flags, u8 cmd);
int nl80211_send_and_recv(struct nl_msg *msg,
		int (*handler)(struct nl_msg *, void *),
		void *data);
#endif
