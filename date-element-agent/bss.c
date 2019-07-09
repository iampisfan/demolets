#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <time.h>

#include "collect.h"
#include "ubus.h"
#include "utils.h"
#include "nl80211.h"
#include "event.h"
#include "vendor.h"

enum index {
	BSSID,
	Ifname,
	LastChange,
	Enabled,
	SSID,
	UnicastBytesSent,
	UnicastBytesReceived,
	MulticastBytesSent,
	MulticastBytesReceived,
	BroadcastBytesSent,
	BroadcastBytesReceived,
	STAList,
	NumberOfSTA,
	TimeStamp,
	IndexMax,
};

struct station{
	u16 num;
	u16 num_max;
	char *mac_addr[10];
};

extern u32 start_time;
static int collect_last_change(struct data_element *de, void *arg)
{
	time_t time_t;
	struct tm *tm;
	u32 last_change;

	time(&time_t);
	tm = gmtime(&time_t);
	last_change = 
		(tm->tm_hour*60*60 + tm->tm_min*60 + tm->tm_sec) - start_time;
	fill_de_node(de, &last_change, 1);

	return 0;
}

static int collect_enabled_state(struct data_element *de, void *arg)
{
	u8 enabled = 1;
	fill_de_node(de, &enabled, 1);

	return 0;
}

static int collect_ssid(struct data_element *de, void *arg)
{
	char *ifname = arg;
	char ssid[64] = {0};

	get_bss_ssid(ifname, ssid);
	if (strlen(ssid))
		fill_de_node(de, ssid, 1);

	return 0;
}

static int collect_sta_list(struct data_element *de, void *arg)
{
	struct station stations;
	char *ifname = arg;
	int i = 0;
	struct data_element_list *list_item;
	struct data_element_container *list_data;
	struct data_element *key_node;
	u8 mac[ETH_ALEN] = {0};
	u16 num_of_sta = 0;

	stations.num = 0;
	stations.num_max = 10;
	ubus_get_clients(ifname, &stations);

	if (stations.num) {
		num_of_sta = stations.num;

		for (i = 0; i < num_of_sta; i++) {
			list_item = calloc(1, sizeof(struct data_element_list));
			if (!list_item)
				return -1;

			list_data = calloc(1, sizeof(struct data_element_container));
			if (!list_data)
				return -1;

			key_node = calloc(1, sizeof(struct data_element));
			if (!key_node)
				return -1;
			
			key_node->name = "MACAddress";
			key_node->type = TYPE_MAC_ADDRESS;
			sscanf(stations.mac_addr[i], "%02x:%02x:%02x:%02x:%02x:%02x",
					&mac[0],  &mac[1], &mac[2],  &mac[3], &mac[4],  &mac[5]);
			fill_de_node(key_node, mac, 0);

			list_data->num = 1;
			list_data->element = key_node;
			collect_sta(list_data, if_nametoindex(ifname));

			list_item->data = list_data;
			fill_de_node(de, list_item, 1);
		}
	} else {
		list_item = calloc(1, sizeof(struct data_element_list));
		if (!list_item)
			return -1;
		
		list_data = calloc(1, sizeof(struct data_element_container));
		if (!list_data)
			return -1;
		
		key_node = calloc(1, sizeof(struct data_element));
		if (!key_node)
			return -1;
		
		key_node->flags = FLAG_LIST_EMPTY;
		list_data->num = 1;
		list_data->element = key_node;
		
		list_item->data = list_data;
		fill_de_node(de, list_item, 1);
	}

	fill_de_node(de + 1, &num_of_sta, 1);

	for (i = 0; i < stations.num; i++)
		free(stations.mac_addr[i]);

	return 0;
}

static int collect_time_stamp(struct data_element *de, void *arg)
{
	char time_str[64] = {0};

	if (NULL == de)
		return -1;

	get_time_ISO8061(time_str, sizeof(time_str)-1);
	fill_de_node(de, time_str, 1);

	return 0;
}

static struct data_element bss_nodes[] = {
	[Ifname] = {
		"Ifname", FLAG_NONE, TYPE_STRING,
	},
	[BSSID] = {
		"BSSID", FLAG_LIST_KEY, TYPE_MAC_ADDRESS
	},
	[LastChange] = {
		"LastChange", FLAG_NONE, TYPE_UINT32, collect_last_change
	},
	[Enabled] = {
		"Enabled", FLAG_NONE, TYPE_BOOLEAN, collect_enabled_state
	},
	[SSID] = {
		"SSID", FLAG_NONE, TYPE_STRING, collect_ssid
	},
	[UnicastBytesSent] = {
		"UnicastBytesSent", FLAG_NONE, TYPE_UINT32
	},
	[UnicastBytesReceived] = {
		"UnicastBytesReceived", FLAG_NONE, TYPE_UINT32
	},
	[MulticastBytesSent] = {
		"MulticastBytesSent", FLAG_NONE, TYPE_UINT32
	},
	[MulticastBytesReceived] = {
		"MulticastBytesReceived", FLAG_NONE, TYPE_UINT32
	},
	[BroadcastBytesSent] = {
		"BroadcastBytesSent", FLAG_NONE, TYPE_UINT32
	},
	[BroadcastBytesReceived] = {
		"BroadcastBytesReceived", FLAG_NONE, TYPE_UINT32
	},
	[STAList] = {
		"STAList", FLAG_NONE, TYPE_LIST, collect_sta_list
	},
	[NumberOfSTA] = {
		"NumberOfSTA", FLAG_STATIC, TYPE_UINT16, .value.uint16 = 0,
	},
	[TimeStamp] = {
		"TimeStamp", FLAG_NONE, TYPE_STRING, collect_time_stamp
	},
};

static char *get_dev_ifname(char *name, char *p)
{
	char *nameend;
	char *namestart;

	while(*p == ' ' || (unsigned char)(*p - 9) <= (13 - 9))
		p++;

	namestart = p;
	nameend = namestart;
	while (*nameend && *nameend != ':' && *nameend != ' ')
		nameend++;
	if (*nameend == ':') {
		if ((nameend - namestart) < 54) {
			memcpy(name, namestart, nameend - namestart);
			name[nameend - namestart] = '\0';
			p = nameend;
		} else {
			/* Interface name too large */
			name[0] = '\0';
		}
	} else {
		/* trailing ':' not found - return empty */
		name[0] = '\0';
	}

	return p + 1;
}

static void get_dev_fields(char *buff, struct data_element *nodes)
{
//	u32 unset = 6400000;
//	u32 multicast_bytes = 0;
	struct {
		unsigned long long rx_packets;	/* total packets received		*/
		unsigned long long tx_packets;	/* total packets transmitted	*/
		unsigned long long rx_bytes;	/* total bytes received 		*/
		unsigned long long tx_bytes;	/* total bytes transmitted		*/
		unsigned long rx_errors;		/* bad packets received 		*/
		unsigned long tx_errors;		/* packet transmit problems 	*/
		unsigned long rx_dropped;		/* no space in linux buffers	*/
		unsigned long tx_dropped;		/* no space available in linux	*/
		unsigned long rx_multicast; 	/* multicast packets received	*/
		unsigned long rx_compressed;
		unsigned long tx_compressed;
		unsigned long collisions;
		/* detailed rx_errors: */
		unsigned long rx_length_errors;
		unsigned long rx_over_errors;	/* receiver ring buff overflow	*/
		unsigned long rx_crc_errors;	/* recved pkt with crc error	*/
		unsigned long rx_frame_errors;	/* recv'd frame alignment error */
		unsigned long rx_fifo_errors;	/* recv'r fifo overrun			*/
		unsigned long rx_missed_errors; /* receiver missed packet	  */
		/* detailed tx_errors */
		unsigned long tx_aborted_errors;
		unsigned long tx_carrier_errors;
		unsigned long tx_fifo_errors;
		unsigned long tx_heartbeat_errors;
		unsigned long tx_window_errors;
	} stats;
	
	memset(&stats, 0, sizeof(stats));
	sscanf(buff, "%llu%llu%u%u%u%u%u%u%llu%llu%u%u%u%u%u%u",
				&stats.rx_bytes, /* missing for 0 */
				&stats.rx_packets,
				&stats.rx_errors,
				&stats.rx_dropped,
				&stats.rx_fifo_errors,
				&stats.rx_frame_errors,
				&stats.rx_compressed, /* missing for <= 1 */
				&stats.rx_multicast, /* missing for <= 1 */
				&stats.tx_bytes, /* missing for 0 */
				&stats.tx_packets,
				&stats.tx_errors,
				&stats.tx_dropped,
				&stats.tx_fifo_errors,
				&stats.collisions,
				&stats.tx_carrier_errors,
				&stats.tx_compressed /* missing for <= 1 */);
//	multicast_bytes = stats.rx_multicast * 1460;

	fill_de_node(&nodes[UnicastBytesSent], &stats.tx_bytes, 1);
	fill_de_node(&nodes[UnicastBytesReceived], &stats.rx_bytes, 1);
//	fill_de_node(&nodes[MulticastBytesSent], &unset, 1);
//	fill_de_node(&nodes[MulticastBytesReceived], &multicast_bytes, 1);
//	fill_de_node(&nodes[BroadcastBytesSent], &unset, 1);
//	fill_de_node(&nodes[BroadcastBytesReceived], &multicast_bytes, 1);
}

static int if_read_proc(const char *ifname, struct data_element *nodes)
{
	FILE *file;
	char buf[512];
	
	file = fopen("/proc/net/dev", "r");
	if (!file)
		return -1;

	fgets(buf, sizeof(buf), file); /* eat line */
	fgets(buf, sizeof(buf), file);

	while (fgets(buf, sizeof(buf), file)) {
			char *s, name[128];
	
			s = get_dev_ifname(name, buf);
			if (strcmp(name, ifname))
				continue;

			get_dev_fields(s, nodes);
	}

	fclose(file);
	return 0;

}

static int if_read_ap8x_stats(const char *ifname, struct data_element *nodes)
{
	FILE *file;
	char buf[512];
	char path[128];
	char value[64];
	int i;

	snprintf(path, sizeof(path) - 1, "/proc/ap8x/%s_stats", ifname);
	file = fopen(path, "r");
	if (!file)
		return -1;

	while (fgets(buf, sizeof(buf), file)) {
		if (0 == strncmp(buf, "tx_multicast_bytes", strlen("tx_multicast_bytes"))) {
			for (i = 0; i < strlen(buf); i++) {
				if (buf[i] >= '0' && buf[i] <= '9') {
					strcpy(value, &buf[i]);
					u32 tx_multicast_bytes = atoi(value);
					fill_de_node(&nodes[MulticastBytesSent], &tx_multicast_bytes, 1);
					break;
				}
			}
		} else if (0 == strncmp(buf, "tx_broadcast_bytes", strlen("tx_broadcast_bytes"))) {
			for (i = 0; i < strlen(buf); i++) {
				if (buf[i] >= '0' && buf[i] <= '9') {
					strcpy(value, &buf[i]);
					u32 tx_broadcast_bytes = atoi(value);
					fill_de_node(&nodes[BroadcastBytesSent], &tx_broadcast_bytes, 1);
					break;
				}
			}
		} else if (0 == strncmp(buf, "rx_multicast_bytes", strlen("rx_multicast_bytes"))) {
			for (i = 0; i < strlen(buf); i++) {
				if (buf[i] >= '0' && buf[i] <= '9') {
					strcpy(value, &buf[i]);
					u32 rx_multicast_bytes = atoi(value);
					fill_de_node(&nodes[MulticastBytesReceived], &rx_multicast_bytes, 1);
					break;
				}
			}
		} else if (0 == strncmp(buf, "rx_broadcast_bytes", strlen("rx_broadcast_bytes"))) {
			for (i = 0; i < strlen(buf); i++) {
				if (buf[i] >= '0' && buf[i] <= '9') {
					strcpy(value, &buf[i]);
					u32 rx_broadcast_bytes = atoi(value);
					fill_de_node(&nodes[BroadcastBytesReceived], &rx_broadcast_bytes, 1);
					break;
				}
			}

		}
	}

	fclose(file);
	return 0;

}

extern struct last_event de_last_event;
static int sta_event_handle(struct nl_msg *msg, void *arg, u32 ifindex)
{
	struct data_element *nodes = arg;
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	u32 vendor_id;
	u32 subcmd;
	u8 *data = NULL;
	u32 data_len;
	char ifname[64] = {0};

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			  genlmsg_attrlen(gnlh, 0), NULL);

	if (gnlh->cmd == NL80211_CMD_VENDOR) {
			vendor_id = nla_get_u32(tb[NL80211_ATTR_VENDOR_ID]);
			subcmd = nla_get_u32(tb[NL80211_ATTR_VENDOR_SUBCMD]);
			if (tb[NL80211_ATTR_VENDOR_DATA]) {
				data = nla_data(tb[NL80211_ATTR_VENDOR_DATA]);
				data_len = nla_len(tb[NL80211_ATTR_VENDOR_DATA]);
			}
			if (vendor_id == MRVL_OUI) {
				switch (subcmd) {
				case MWL_VENDOR_EVENT_ASSOC:
					if (if_indextoname(ifindex, ifname)) {
						cleanup_de_node(&nodes[STAList]);
						collect_sta_list(&nodes[STAList], ifname);
					}

					de_last_event.id = ASSOC_EVENT;
					if (de_last_event.assoc)
						cleanup_de_container(&de_last_event.assoc);
					de_last_event.assoc = calloc(1, sizeof(struct data_element_container));
					notify_assoc(de_last_event.assoc, nodes[BSSID].value.mac_address,
								data, data_len);
					break;
				case MWL_VENDOR_EVENT_DISASSOC:
					if (if_indextoname(ifindex, ifname)) {
						cleanup_de_node(&nodes[STAList]);
						collect_sta_list(&nodes[STAList], ifname);
					}

					de_last_event.id = DISASSOC_EVENT;
					if (de_last_event.disassoc)
						cleanup_de_container(&de_last_event.disassoc);
					de_last_event.disassoc = calloc(1, sizeof(struct data_element_container));
					notify_disassoc(de_last_event.disassoc, nodes[BSSID].value.mac_address,
								data, data_len);
					break;
				default:
					break;
				}
			}
	}

	return 0;
}

int collect_bss(struct data_element_container *parent)
{
	int i = 0;
	char *ifname = NULL;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;
	struct data_element *key_node = &bss_nodes[BSSID];
	u8 mac[ETH_ALEN] = {0};

	ifname = strdup(parent->element[0].value.string);
	get_interface_macaddr(ifname, mac);
	fill_de_node(&bss_nodes[Ifname], ifname, 0);
	fill_de_node(key_node, mac, 1);
	cleanup_de_node(&parent->element[0]);

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (!parent->element) {
		printf("collect bss realloc elements failed with no memory\n");
		parent->num = 0;
		free(ifname);
		return -1;
	} else {
		parent->num = IndexMax;
		nodes = parent->element;
	}

	memcpy(nodes, bss_nodes, IndexMax*sizeof(struct data_element));

	for (i = 0; i < IndexMax; i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, (void *)ifname);
	}

	if_read_proc(ifname, nodes);

	nl80211_register_event_handler(if_nametoindex(ifname),
					sta_event_handle, (void *)nodes);

	free(ifname);
	return 0;
}

int update_bss(struct data_element_container *parent)
{
	struct data_element *nodes = parent->element;
	struct data_element *node;
	struct data_element_list *list_item;
	struct data_element_container *list_data;
	char *ifname = nodes[Ifname].value.string;

	node = &nodes[LastChange];
	collect_last_change(node, NULL);

	node = &nodes[STAList];
	if_read_proc(ifname, nodes);
	if_read_ap8x_stats(ifname, nodes);

	cleanup_de_node(node);
	collect_sta_list(node, ifname);

	return 0;
}

