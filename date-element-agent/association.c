#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "ieee80211.h"
#include "capabilities.h"
#include "vendor.h"

enum index {
	BSSID,
	MACAddress,
	StatusCode,
	HTCapabilities,
	VHTCapabilities,
	HECapabilities,
	IndexMax
};

static struct data_element assoc_nodes[] = {
	[BSSID] = {
		"AssocData/BSSID", FLAG_NONE, TYPE_MAC_ADDRESS,
	},
	[MACAddress] = {
		"AssocData/MACAddress", FLAG_NONE, TYPE_MAC_ADDRESS,
	},
	[StatusCode] = {
		"AssocData/StatusCode", FLAG_STATIC, TYPE_UINT16, .value.uint16 = 0,
	},
	[HTCapabilities] = {
		"AssocData/HTCapabilities", FLAG_NONE, TYPE_BINARY,
	},
	[VHTCapabilities] = {
		"AssocData/VHTCapabilities", FLAG_NONE, TYPE_BINARY,
	},
	[HECapabilities] = {
		"AssocData/HECapabilities", FLAG_NONE, TYPE_BINARY,
	},
};

int notify_assoc(struct data_element_container *parent, u8 *bss_mac,
		u8 *data, u32 data_len)
{
	int ret;
	struct data_element *nodes;
	struct nlattr *tb[NUM_MWL_VENDOR_ATTR_IE];
	u8 sta_mac[ETH_ALEN] = {0};
	u8 *capa_data = NULL;
	struct IEEEtypes_HT_Element_t ht;
	struct IEEEtypes_vht_cap vht;
	struct ht_vht_capability capabilities;
	union htcap htcap;
	union vhtcap vhtcap;
	struct data_element_binary *binary;

	ret = nla_parse(tb, MAX_MWL_VENDOR_ATTR_IE, (struct nlattr *)data, data_len, NULL);
	if (ret)
		return -1;

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else
		return -1;

	memcpy(nodes, assoc_nodes, IndexMax*sizeof(struct data_element));

	fill_de_node(&nodes[BSSID], bss_mac, 1);
	if (tb[MWL_VENDOR_ATTR_IE_MAC]) {
		memcpy(sta_mac, nla_data(tb[MWL_VENDOR_ATTR_IE_MAC]), ETH_ALEN);
		fill_de_node(&nodes[MACAddress], sta_mac, 1);
	}
	if (tb[MWL_VENDOR_ATTR_IE_CAPA]) {
		capa_data = nla_data(tb[MWL_VENDOR_ATTR_IE_CAPA]);
		while(capa_data[1]) {
			switch (capa_data[0]) {
			case HT_CAP:
				if (capa_data[1] >= (sizeof(struct IEEEtypes_HT_Element_t)-2)) {
					memset(&capabilities, 0, sizeof(capabilities));
					memcpy(&ht, capa_data, sizeof(struct IEEEtypes_HT_Element_t));
					capabilities.htcap.cap = ht.HTCapabilitiesInfo;
					memcpy(&capabilities.htcap.mcs, ht.SupportedMCSset, 16);

					memset(&htcap, 0, sizeof(htcap));
					fill_htcap(&capabilities, &htcap);
					binary = calloc(1, sizeof(struct data_element_binary));
					binary->num = 1;
					binary->bytes = calloc(binary->num, sizeof(byte));
					memcpy(binary->bytes, htcap.bytes, 1);
					fill_de_node(&nodes[HTCapabilities], binary, 1);
				}
				break;
			case VHT_CAP:
				if (capa_data[1] >= (sizeof(struct IEEEtypes_vht_cap)-2)) {
					memset(&capabilities, 0, sizeof(capabilities));
					memcpy(&vht, capa_data, sizeof(struct IEEEtypes_vht_cap));
					capabilities.vhtcap.cap = vht.cap;
					capabilities.vhtcap.vht_mcs.rx_mcs_map = vht.SupportedRxMcsSet & 0xffff;
					capabilities.vhtcap.vht_mcs.rx_highest = (vht.SupportedRxMcsSet >> 16) & 0xffff;
					capabilities.vhtcap.vht_mcs.tx_mcs_map = vht.SupportedTxMcsSet & 0xffff;
					capabilities.vhtcap.vht_mcs.tx_highest = (vht.SupportedTxMcsSet >> 16) & 0xffff;

					memset(&vhtcap, 0, sizeof(vhtcap));
					fill_vhtcap(&capabilities, &vhtcap);
					binary = calloc(1, sizeof(struct data_element_binary));
					binary->num = 6;
					binary->bytes = calloc(binary->num, sizeof(byte));
					memcpy(binary->bytes, vhtcap.bytes, 6);
					fill_de_node(&nodes[VHTCapabilities], binary, 1);
				}
				break;
			default:
				break;
			}
			capa_data += (capa_data[1] + 2);
		}
	}

	return 0;
}