#include <stdio.h>
#include <stdlib.h>

#include "data_element.h"
#include "event.h"
#include "vendor.h"

enum index {
	BSSID,
	MACAddress,
	ReasonCode,
	BytesSent,
	BytesReceived,
	PacketsSent,
	PacketsReceived,
	ErrorsSent,
	ErrorsReceived,
	RetransCount,
	IndexMax
};

static struct data_element disassoc_nodes[] = {
	[BSSID] = {
		"DisassocData/BSSID", FLAG_NONE, TYPE_MAC_ADDRESS,
	},
	[MACAddress] ={
		"DisassocData/MACAddress", FLAG_NONE, TYPE_MAC_ADDRESS,
	},
	[ReasonCode] = {
		"DisassocData/ReasonCode", FLAG_STATIC, TYPE_UINT16, .value.uint16 = 3,
	},
	[BytesSent] = {
		"DisassocData/BytesSent", FLAG_NONE, TYPE_UINT32,
	},
	[BytesReceived] = {
		"DisassocData/BytesReceived", FLAG_NONE, TYPE_UINT32,
	},
	[PacketsSent] = {
		"DisassocData/PacketsSent", FLAG_NONE, TYPE_UINT32,
	},
	[PacketsReceived] = {
		"DisassocData/PacketsReceived", FLAG_NONE, TYPE_UINT32,
	},
	[ErrorsSent] = {
		"DisassocData/ErrorsSent", FLAG_STATIC, TYPE_UINT32, .value.uint32 = 0,
	},
	[ErrorsReceived] = {
		"DisassocData/ErrorsReceived", FLAG_STATIC, TYPE_UINT32, .value.uint32 = 0,
	},
	[RetransCount] = {
		"DisassocData/RetransCount", FLAG_STATIC, TYPE_UINT32, .value.uint32 = 0,
	},
};

int notify_disassoc(struct data_element_container *parent, u8 *bss_mac,
		u8 *data, u32 data_len)
{
	int ret;
	struct data_element *nodes;
	struct nlattr *tb[NUM_MWL_VENDOR_ATTR];
	u8 sta_mac[ETH_ALEN] = {0};
	struct data_element_binary *binary;
	struct mwl_sta_statistics *sta_statistics;

	ret = nla_parse(tb, MAX_MWL_VENDOR_ATTR, (struct nlattr *)data, data_len, NULL);
	if (ret)
		return -1;

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else
		return -1;

	memcpy(nodes, disassoc_nodes, IndexMax*sizeof(struct data_element));
	fill_de_node(&nodes[BSSID], bss_mac, 1);
	if (tb[MWL_VENDOR_ATTR_MAC]) {
		memcpy(sta_mac, nla_data(tb[MWL_VENDOR_ATTR_MAC]), ETH_ALEN);
		fill_de_node(&nodes[MACAddress], sta_mac, 1);
	}
	if (tb[MWL_VENDOR_ATTR_STATISTICS]) {
		sta_statistics = nla_data(tb[MWL_VENDOR_ATTR_STATISTICS]);
		fill_de_node(&nodes[BytesSent], &sta_statistics->tx_bytes, 1);
		fill_de_node(&nodes[BytesReceived], &sta_statistics->tx_bytes, 1);
		fill_de_node(&nodes[PacketsSent], &sta_statistics->tx_packets, 1);
		fill_de_node(&nodes[PacketsReceived], &sta_statistics->rx_packets, 1);
	}

	return 0;

}
