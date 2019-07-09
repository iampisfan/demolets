#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "data_element.h"
#include "collect.h"
#include "utils.h"
#include "vendor.h"
#include "wext.h"
#include "opclass.h"

struct offchan_req
{
	u32	id;
	u32 channel;
	u32 lifetime;
	u32	start_ts;
	u32 dwell_time;
	u32 channel_width;
	void *pak;
	u8 radio_slot;
	u8 req_type;
	u8 priority;
	u8 status;
} __attribute__ ((packed));

static int NB_LIST_MAX_NUM = 256;
static struct neighbor_list_entrie_t *nlist = NULL;

/* grouping NeighborBSS */
static struct data_element neighbor_bss_nodes[] = {
	{ "BSSID", FLAG_LIST_KEY, TYPE_MAC_ADDRESS },
	{ "SSID", FLAG_ALLOW_EMPTY_STR, TYPE_STRING },
	{ "SignalStrength", FLAG_NONE, TYPE_UINT8 },
	{ "ChannelBandwidth", FLAG_NONE, TYPE_UINT8 },
	{ "ChannelUtilization", FLAG_NONE, TYPE_UINT8 },
	{ "StationCount", FLAG_NONE, TYPE_UINT16 }
};

enum nr_chan_width {
	NR_CHAN_WIDTH_20 = 0,
	NR_CHAN_WIDTH_40 = 1,
	NR_CHAN_WIDTH_80 = 2,
	NR_CHAN_WIDTH_160 = 3,
	NR_CHAN_WIDTH_80P80 = 4,
};

static int collect_neighbor_signal_strength(struct data_element *de,
			struct neighbor_list_entrie_t *neighbor)
{
	s8 s8_signal = (s8) neighbor->rssi;
	u8 u8_signal = 0;

	s8_signal = (s8_signal > 0) ? (s8_signal*-1):s8_signal;
	if (s8_signal <= -110)
		u8_signal = 0;
	else if (s8_signal > -110 && s8_signal < 0)
		u8_signal = (s8_signal+110)*2;
	else
		u8_signal = 220;

	fill_de_node(de, &u8_signal, 1);
}

static int collect_neighbor_qbss(struct data_element *de,
            struct neighbor_list_entrie_t *neighbor)
{
	u8 chan_width;
	u8 channel_util;
	u16 sta_cnt;

	if (neighbor->width == NR_CHAN_WIDTH_160) {
		chan_width = 160;
	} else if (neighbor->width == NR_CHAN_WIDTH_80) {
		chan_width = 80;
	} else if (neighbor->width == NR_CHAN_WIDTH_40) {
		chan_width = 40;
	} else {
		chan_width = 20;
	}

	fill_de_node(de, &chan_width, 1); /* ChannelBandwidth */
	channel_util = neighbor->channel_util;
	sta_cnt = neighbor->sta_cnt;
	if (sta_cnt == 65535) {
		(de + 1)->flags = FLAG_NONE;
		(de + 2)->flags = FLAG_NONE;
	} else {
		fill_de_node(de + 1, &channel_util, 1); /* ChannelUtilization */
		fill_de_node(de + 2, &sta_cnt, 1); /* StationCount */
	}

	return 0;
}

static int collect_neighbor_bss(struct data_element_container *parent,
				struct neighbor_list_entrie_t *neighbor)
{
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;

	fill_de_node(&neighbor_bss_nodes[0], neighbor->bssid, 1);
	fill_de_node(&neighbor_bss_nodes[1], neighbor->ssid, 1);
	collect_neighbor_signal_strength(&neighbor_bss_nodes[2], neighbor);
	collect_neighbor_qbss(&neighbor_bss_nodes[3], neighbor);

	parent->element = realloc(parent->element, sizeof(neighbor_bss_nodes));
	if (parent->element) {
		parent->num = sizeof(neighbor_bss_nodes)/sizeof(struct data_element);
		nodes = parent->element;
	} else {
		printf("collect neighbor_bss realloc failed with no memory\n");
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, neighbor_bss_nodes, sizeof(neighbor_bss_nodes));

	return 0;

}

/* grouping ChannelScan */
static int collect_channel_scan_time_stamp(struct data_element *de, void *arg)
{
	char time_str[64] = {0};

	if (NULL == de)
		return -1;

	get_time_ISO8061(time_str, sizeof(time_str)-1);
	fill_de_node(de, time_str, 1);

	return 0;
}

static int collect_neighbor_list(struct data_element *de, void *arg)
{
	struct data_element_list *list_item;
	struct data_element_container *list_data;
	struct data_element *key_node;
	u8 *channel = (u8 *)arg;
	struct neighbor_list_entrie_t *neighbor;
	int i = 0;
	u8 num_of_neighbor = 0;

	for (i = 0; i < NB_LIST_MAX_NUM; i++) {
		neighbor = &nlist[i];
		if (neighbor->chan != *channel)
			continue;
		list_item = calloc(1, sizeof(struct data_element_list));
		list_data = calloc(1, sizeof(struct data_element_container));
		key_node = calloc(1, sizeof(struct data_element));
		key_node->name = "BSSID";
		key_node->flags = FLAG_LIST_KEY;
		key_node->type = TYPE_MAC_ADDRESS;
		fill_de_node(key_node, neighbor->bssid, 1);
		list_data->num = 1;
		list_data->element = key_node;
		collect_neighbor_bss(list_data, neighbor);
		list_item->data = list_data;
		fill_de_node(de, list_item, 1);
		num_of_neighbor++;
	}

	if (0 == num_of_neighbor) {
		list_item = calloc(1, sizeof(struct data_element_list));
		list_data = calloc(1, sizeof(struct data_element_container));
		key_node = calloc(1, sizeof(struct data_element));
		key_node->flags = FLAG_LIST_EMPTY;
		list_data->num = 1;
		list_data->element = key_node;
		list_item->data = list_data;
		fill_de_node(de, list_item, 1);
	}

	fill_de_node(de + 1, &num_of_neighbor, 1);

	return 0;

}

static struct data_element channel_scan_nodes[] = {
	{ "Channel", FLAG_LIST_KEY, TYPE_UINT8 },
	{ "TimeStamp", FLAG_NONE, TYPE_STRING, collect_channel_scan_time_stamp },
	{ "Utilization", FLAG_STATIC, TYPE_UINT8, .value.uint8 = 56 },
	{ "Noise", FLAG_STATIC, TYPE_UINT8, .value.uint8 = 20 },
	{ "NeighborList", FLAG_NONE, TYPE_LIST, collect_neighbor_list },
	{ "NumberOfNeighbors", FLAG_STATIC, TYPE_UINT8, .value.uint8 = 0 },
};

static int collect_channel_scan(struct data_element_container *parent, u8 *channel)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;

	fill_de_node(&channel_scan_nodes[0], channel, 1);
	parent->element = realloc(parent->element, sizeof(channel_scan_nodes));
	if (parent->element) {
		parent->num = sizeof(channel_scan_nodes)/sizeof(struct data_element);
		nodes = parent->element;
	} else {
		printf("collect channel_scan realloc failed with no memory\n");
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, channel_scan_nodes, sizeof(channel_scan_nodes));

	for (i = 0; i < sizeof(channel_scan_nodes)/sizeof(struct data_element); i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, channel);
	}

	return 0;

}

/* grouping OpClassScan */
static int collect_channel_scan_list(struct data_element *de, void *arg)
{
	struct data_element_list *list_item;
	struct data_element_container *list_data;
	struct data_element *key_node;
	u8 *opclass = arg;
	const struct oper_class_map *oper_class_map = get_opclass_channels(*opclass);
	u8 channel;
	u8 num_of_chan_scans = 0;

	if (!oper_class_map)
		return -1;

	for (channel = oper_class_map->min_chan; channel <= oper_class_map->max_chan;
				channel+= oper_class_map->inc) {
		list_item = calloc(1, sizeof(struct data_element_list));
		list_data = calloc(1, sizeof(struct data_element_container));
		key_node = calloc(1, sizeof(struct data_element));
		key_node->name = "Channel";
		key_node->flags = FLAG_LIST_KEY;
		key_node->type = TYPE_UINT8;
		fill_de_node(key_node, &channel, 1);
		list_data->num = 1;
		list_data->element = key_node;
		collect_channel_scan(list_data, &channel);
		list_item->data = list_data;
		fill_de_node(de, list_item, 1);
		num_of_chan_scans++;
	}

	fill_de_node(de + 1, &num_of_chan_scans, 1);

	return 0;
}

static struct data_element opclass_scan_nodes[] = {
		{ "OperatingClass", FLAG_LIST_KEY, TYPE_UINT8 },
		{ "ChannelScanList", FLAG_NONE, TYPE_LIST, collect_channel_scan_list },
		{ "NumberOfChannelScans", FLAG_STATIC, TYPE_UINT8, .value.uint8 = 1 },
};

static int collect_opclass_scan(struct data_element_container *parent, u8 opclass)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;

	fill_de_node(&opclass_scan_nodes[0], &opclass, 1);
	parent->element = realloc(parent->element, sizeof(opclass_scan_nodes));
	if (parent->element) {
		parent->num = sizeof(opclass_scan_nodes)/sizeof(struct data_element);
		nodes = parent->element;
	} else {
		printf("collect opclass_scan realloc failed with no memory\n");
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, opclass_scan_nodes, sizeof(opclass_scan_nodes));

	for (i = 0; i < sizeof(opclass_scan_nodes)/sizeof(struct data_element); i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, &opclass);
	}

	return 0;
}


/* grouping ScanResult */
static int collect_scan_result_time_stamp(struct data_element *de, void *arg)
{
	char time_str[64] = {0};

	if (NULL == de)
		return -1;

	get_time_ISO8061(time_str, sizeof(time_str)-1);
	fill_de_node(de, time_str, 1);

	return 0;
}

static int collect_scan_result_opclass_scan_list(struct data_element *de, void *arg)
{
	struct neighbor_list_entrie_t *nlist = arg;
	int i = 0;
	u8 opclass;
	struct data_element_list *list_item = de->value.list;
	struct data_element_container *list_data = NULL;
	struct data_element *key_node = NULL;
	/* hardcode */
	u8 opclass_list[5] = {81, 115, 118, 121, 125};
	u8 num_of_opclass_scans = sizeof(opclass_list)/sizeof(u8);

	for (i = 0; i < num_of_opclass_scans; i++) {
		list_item = calloc(1, sizeof(struct data_element_list));
		list_data = calloc(1, sizeof(struct data_element_container));
		key_node = calloc(1, sizeof(struct data_element));
		key_node->name = "OperatingClass";
		key_node->flags = FLAG_LIST_KEY;
		key_node->type = TYPE_UINT8;
		fill_de_node(key_node, &opclass_list[i], 1);
		list_data->num = 1;
		list_data->element = key_node;
		collect_opclass_scan(list_data, opclass_list[i]);
		list_item->data = list_data;
		fill_de_node(de, list_item, 1);
	}
/*
	for (i = 0; i < NB_LIST_MAX_NUM; i++) {
		if (0 == channel_to_opclass(nlist[i].chan, &opclass)) {
			int find = 0;
			while (list_item) {
				list_data = list_item->data;
				if (list_data && list_data->num) {
					if (list_data->element[0].value.uint8 == opclass) {
						find = 1;
						break;
					}
				}
				list_item = list_item->next;
			}
			if (!find) {
				list_item = calloc(1, sizeof(struct data_element_list));
				list_data = calloc(1, sizeof(struct data_element_container));
				key_node = calloc(1, sizeof(struct data_element));
				key_node->name = "OperatingClass";
				key_node->flags = FLAG_LIST_KEY;
				key_node->type = TYPE_UINT8;
				fill_de_node(key_node, &opclass, 1);
				list_data->num = 1;
				list_data->element = key_node;
				collect_opclass_scan(list_data);
				list_item->data = list_data;
				fill_de_node(de, list_item, 1);
				num_of_opclass_scans++;
			}
		}
	}
*/
	fill_de_node(de + 1, &num_of_opclass_scans, 1);
	return 0;	
}

static struct data_element scan_result_nodes[] = {
	{ "TimeStamp", FLAG_NONE, TYPE_STRING, collect_scan_result_time_stamp },
	{ "OpClassScanList", FLAG_NONE, TYPE_LIST, collect_scan_result_opclass_scan_list },
	{ "NumberOfOpClassScans", FLAG_NONE, TYPE_UINT8 },
};

static int getnlist(u32 radio_index, void *buf, int len)
{
	u8 opclass_list[5] = {81, 115, 118, 121, 125};
	u8 num_of_opclass_scans = sizeof(opclass_list)/sizeof(u8);
	const struct oper_class_map *oper_class_map;
	u8 channel;
	struct offchan_req offchan_req;
	char ifname[64] = {0};
	int i;

	if (radio_index == 1)
		strcpy(ifname, "wdev1");
	else
		strcpy(ifname, "wdev0");

	memset(&offchan_req, 0, sizeof(offchan_req));
	offchan_req.id = getpid();
	offchan_req.dwell_time = 100;
	for (i = 0; i < num_of_opclass_scans; i++) {
		oper_class_map = get_opclass_channels(opclass_list[i]);
		for (channel = oper_class_map->min_chan; channel <= oper_class_map->max_chan;
				channel+= oper_class_map->inc) {
			offchan_req.channel = channel;
			ioctl_setparam(ifname, WL_PARAM_OFF_CHANNEL_REQ_SEND, (int)&offchan_req);
		}
	}

	sleep(1);
	strncpy(buf, "getnlist", len);
	ioctl_getcmd(ifname, buf, len);

	return 0;
}

int collect_scan_result(struct data_element_container *parent, u32 radio_index)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;

	parent->element = realloc(parent->element, sizeof(scan_result_nodes));
	if (parent->element) {
		parent->num = sizeof(scan_result_nodes)/sizeof(struct data_element);
		nodes = parent->element;
	} else {
		printf("collect scan_result realloc failed with no memory\n");
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, scan_result_nodes, sizeof(scan_result_nodes));

	nlist = calloc(NB_LIST_MAX_NUM, sizeof(struct neighbor_list_entrie_t));
	getnlist(radio_index, nlist,
				NB_LIST_MAX_NUM*sizeof(struct neighbor_list_entrie_t));

	for (i = 0; i < sizeof(scan_result_nodes)/sizeof(struct data_element); i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, nlist);
	}

	free(nlist);
	return 0;
}
