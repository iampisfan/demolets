#ifndef _EVENT_H_
#define _EVENT_H_

#include "nl80211.h"
#include "type.h"
#include "dl_list.h"
#include "data_element.h"

struct nl80211_event_data {
	u32 ifindex;
	int (*handler)(struct nl_msg *msg, void *arg, u32 ifindex);
	void *arg;
};
struct nl80211_event_list_item {
	struct nl80211_event_data *nl_event;
	struct dl_list dl_list;
};

enum last_event_id {
	NOT_SET = 0,
	ASSOC_EVENT,
	DISASSOC_EVENT
};

struct last_event {
	enum last_event_id id;
	struct data_element_container *assoc;
	struct data_element_container *disassoc;
};

int nl80211_setup_event_monitor(void);
void nl80211_register_event_handler(u32 ifindex,
		int (*handler)(struct nl_msg *msg, void *arg, u32 ifindex),
		void *arg);
void run_event_uloop(void);

int notify_assoc(struct data_element_container *parent, u8 *bss_mac,
		u8 *data, u32 data_len);
int notify_disassoc(struct data_element_container *parent, u8 *bss_mac,
		u8 *data, u32 data_len);

#endif
