#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <errno.h>

#include "collect.h"
#include "utils.h"
#include "nl80211.h"
#include "capabilities.h"

enum index {
	MACAddress,
	HTCapabilities,
	VHTCapabilities,
	SignalStrength,
	LastConnectTime,
	BytesSent,
	BytesReceived,
	PacketsSent,
	PacketsReceived,
	ErrorsSent,
	ErrorsReceived,
	RetransCount,
	LastDataDownlinkRate,
	LastDataUplinkRate,
	UtilizationReceive,
	UtilizationTransmit,
	TimeStamp,
	NumberOfMeasureReports,
	IndexMax
};

static int collect_time_stamp(struct data_element *de, void *arg)
{
	char time_str[64] = {0};

	if (NULL == de)
		return -1;

	get_time_ISO8061(time_str, sizeof(time_str)-1);
	fill_de_node(de, time_str, 1);

	return 0;
}

static int collect_sta_ht_cap(struct data_element *de, void *arg)
{
	struct ht_vht_capability *capa = arg;
	union htcap ht;
	struct data_element_binary *data;

	data = calloc(1, sizeof(struct data_element_binary));
	if (!data)
		return -1;

	memset(&ht, 0, sizeof(ht));
	fill_htcap(capa, &ht);

	data->num = 1;
	data->bytes = calloc(data->num, sizeof(byte));
	memcpy(data->bytes, ht.bytes, (data->num)*sizeof(byte));

	fill_de_node(de, data, 1);

	return 0;
}

static int collect_sta_vht_cap(struct data_element *de, void *arg)
{
	struct ht_vht_capability *capa = arg;
	union vhtcap vht;
	struct data_element_binary *data;

	data = calloc(1, sizeof(struct data_element_binary));
	if (!data)
		return -1;

	memset(&vht, 0, sizeof(vht));
	fill_vhtcap(capa, &vht);

	data->num = 6;
	data->bytes = calloc(data->num, sizeof(byte));
	memcpy(data->bytes, vht.bytes, (data->num)*sizeof(byte));

	fill_de_node(de, data, 1);

	return 0;
}

static struct data_element sta_nodes[] = {
	[MACAddress] = {
		"MACAddress", FLAG_LIST_KEY, TYPE_MAC_ADDRESS,
	},
	[HTCapabilities] = {
		"HTCapabilities", FLAG_NONE, TYPE_BINARY, NULL,
	},
	[VHTCapabilities] = {
		"VHTCapabilities", FLAG_NONE, TYPE_BINARY, NULL,
	},
	[SignalStrength] = {
		"SignalStrength", FLAG_NONE, TYPE_UINT8,
	},
	[LastConnectTime] = {
		"LastConnectTime", FLAG_NONE, TYPE_UINT32,
	},
	[BytesSent] = {
		"BytesSent", FLAG_NONE, TYPE_UINT32,
	},
	[BytesReceived] = {
		"BytesReceived", FLAG_NONE, TYPE_UINT32,
	},
	[PacketsSent] = {
		"PacketsSent", FLAG_NONE, TYPE_UINT32,
	},
	[PacketsReceived] = {
		"PacketsReceived", FLAG_NONE, TYPE_UINT32,
	},
	[ErrorsSent] = {
		"ErrorsSent", FLAG_NONE, TYPE_UINT32,
	},
	[ErrorsReceived] = {
		"ErrorsReceived", FLAG_NONE, TYPE_UINT32,
	},
	[RetransCount] = {
		"RetransCount", FLAG_NONE, TYPE_UINT32,
	},
	[LastDataDownlinkRate] = {
		"LastDataDownlinkRate", FLAG_NONE, TYPE_UINT32,
	},
	[LastDataUplinkRate] = {
		"LastDataUplinkRate", FLAG_NONE, TYPE_UINT32,
	},
	[UtilizationReceive] = {
		"UtilizationReceive", FLAG_STATIC, TYPE_UINT32, .value.uint32 = 50,
	},
	[UtilizationTransmit] = {
		"UtilizationTransmit", FLAG_STATIC, TYPE_UINT32, .value.uint32 = 50,
	},
	[NumberOfMeasureReports] = {
		"NumberOfMeasureReports", FLAG_STATIC, TYPE_UINT8, .value.uint8 = 0,
	},
	[TimeStamp] = {
		"TimeStamp", FLAG_NONE, TYPE_STRING, collect_time_stamp,
	},
};

static int get_sta_data_cb(struct nl_msg *msg, void *arg)
{
	struct data_element *nodes = arg;
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
	u8 *ies = NULL;
	u8 ies_len;
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
		[NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
		[NL80211_STA_INFO_T_OFFSET] = { .type = NLA_U64 },
		[NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_RX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
		[NL80211_STA_INFO_TX_RETRIES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_FAILED] = { .type = NLA_U32 },
		[NL80211_STA_INFO_STA_FLAGS] =
			{ .minlen = sizeof(struct nl80211_sta_flag_update) },
		[NL80211_STA_INFO_LOCAL_PM] = { .type = NLA_U32},
		[NL80211_STA_INFO_PEER_PM] = { .type = NLA_U32},
		[NL80211_STA_INFO_NONPEER_PM] = { .type = NLA_U32},
		[NL80211_STA_INFO_CHAIN_SIGNAL] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_CHAIN_SIGNAL_AVG] = { .type = NLA_NESTED },
	};
	struct ht_vht_capability *sta_capability;

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	/*
	 * TODO: validate the interface and mac address!
	 * Otherwise, there's a race condition as soon as
	 * the kernel starts sending station notifications.
	 */

	if (!tb[NL80211_ATTR_STA_INFO])
		return NL_SKIP;

	if (nla_parse_nested(sinfo, NL80211_STA_INFO_MAX,
			     tb[NL80211_ATTR_STA_INFO],
			     stats_policy))
		return NL_SKIP;

	if (sinfo[NL80211_STA_INFO_RX_BYTES]) {
		u32 rx_bytes = nla_get_u32(sinfo[NL80211_STA_INFO_RX_BYTES]);
		fill_de_node(&nodes[BytesReceived], &rx_bytes, 1);
	}

	if (sinfo[NL80211_STA_INFO_RX_PACKETS]) {
		u32 rx_packets = nla_get_u32(sinfo[NL80211_STA_INFO_RX_PACKETS]);
		fill_de_node(&nodes[PacketsReceived], &rx_packets, 1);
	}

	if (sinfo[NL80211_STA_INFO_TX_BYTES]) {
		u32 tx_bytes = nla_get_u32(sinfo[NL80211_STA_INFO_TX_BYTES]);
		fill_de_node(&nodes[BytesSent], &tx_bytes, 1);
	}

	if (sinfo[NL80211_STA_INFO_TX_PACKETS]) {
		u32 tx_packets = nla_get_u32(sinfo[NL80211_STA_INFO_TX_PACKETS]);
		fill_de_node(&nodes[PacketsSent], &tx_packets, 1);
	}

	if (sinfo[NL80211_STA_INFO_TX_RETRIES]) {
		u32 tx_retries = nla_get_u32(sinfo[NL80211_STA_INFO_TX_RETRIES]);
		fill_de_node(&nodes[RetransCount], &tx_retries, 1);
	} else {
		u32 tx_retries = 0;
		fill_de_node(&nodes[RetransCount], &tx_retries, 1);
	}

	if (sinfo[NL80211_STA_INFO_TX_FAILED]) {
		u32 tx_failed = nla_get_u32(sinfo[NL80211_STA_INFO_TX_FAILED]);
		fill_de_node(&nodes[ErrorsSent], &tx_failed, 1);
	} else {
		u32 tx_failed = 0;
		fill_de_node(&nodes[ErrorsSent], &tx_failed, 1);
	}

	if (sinfo[NL80211_STA_INFO_SIGNAL]) {
		s8 s8_signal = (s8)nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]);
		u8 u8_signal = 0; 
		if (s8_signal <= -110)
			u8_signal = 0;
		else if (s8_signal > -110 && s8_signal < 0)
			u8_signal = (s8_signal+110)*2;
		else
			u8_signal = 220;

		fill_de_node(&nodes[SignalStrength], &u8_signal, 1);
	}

	if (sinfo[NL80211_STA_INFO_CONNECTED_TIME]) {
		u32 connected_time = nla_get_u32(sinfo[NL80211_STA_INFO_CONNECTED_TIME]);
		fill_de_node(&nodes[LastConnectTime], &connected_time, 1);
		if (connected_time) {
			u32 downlink_rate =
					nodes[BytesReceived].value.uint32*8/(1024*connected_time);
			u32 uplink_rate = 
					nodes[BytesSent].value.uint32*8/(1024*connected_time);
			fill_de_node(&nodes[LastDataDownlinkRate], &downlink_rate, 1);
			fill_de_node(&nodes[LastDataUplinkRate], &uplink_rate, 1);
		}
	}

	if (tb[NL80211_ATTR_IE]) {
		ies = nla_data(tb[NL80211_ATTR_IE]);
		ies_len = nla_len(tb[NL80211_ATTR_IE]);
		if (ies_len >= sizeof(struct ht_vht_capability)) {
			sta_capability = (struct ht_vht_capability *)ies;
			collect_sta_ht_cap(&nodes[HTCapabilities], sta_capability);
			collect_sta_vht_cap(&nodes[VHTCapabilities], sta_capability);
		}
	}
	return NL_SKIP;
}

static int nl80211_get_sta_data(unsigned int bss_ifindex,
				u8 *mac, struct data_element *nodes)
{
	struct nl_msg *msg = nl80211_new_msg(0, NL80211_CMD_GET_STATION);

	if (!msg)
		return -1;

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, bss_ifindex);
	nla_put(msg, NL80211_ATTR_MAC, ETH_ALEN, mac);
	nl80211_send_and_recv(msg, get_sta_data_cb, nodes);

	return 0;

nla_put_failure:
	free(msg);
	return -1;
}

int collect_sta(struct data_element_container *parent, unsigned int bss_ifindex)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;
	struct data_element *key_node = &sta_nodes[0];
	u8 mac[ETH_ALEN] = {0};

	memcpy(mac, parent->element[0].value.mac_address, ETH_ALEN);
	fill_de_node(key_node, mac, 1);

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else {
		printf("collect station realloc elements failed with no memory\n");
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, sta_nodes, IndexMax*sizeof(struct data_element));

	for (i = 0; i < IndexMax; i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, NULL);
	}

	nl80211_get_sta_data(bss_ifindex, mac, nodes);

	return 0;
}

