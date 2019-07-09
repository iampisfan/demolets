#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>

#include "collect.h"
#include "utils.h"
#include "nl80211.h"
#include "ieee80211.h"
#include "ubus.h"
#include "opclass.h"
#include "vendor.h"

struct radio_arg
{
	u32 index;
	struct data_element *de;
};

enum index {
	ID,
	RadioIndex,
	Enabled,
	Noise,
	Utilization,
	Transmit,
	ReceiveSelf,
	ReceiveOther,
	BSSList,
	NumberOfBSS,
	Capabilities,
	CurrentOperatingClasses,
	NumberOfCurrOpClass,
//	BackhaulSta,
	ScanResultList,
	UnassociatedStaList,
	NumberOfUnassocSta,
	IndexMax
};

static u16 num_of_bss;
static struct ht_vht_capability radio_capa;
static int band;
static struct channel_info chan_info;
static char survey_ifname[64];

static int collect_enabled_state(struct data_element *de, void *arg)
{
	struct radio_arg *radio = arg;
	u32 radio_index = radio->index;
	char buf[128] = {0};
	u8 enabled_state = 0;
	FILE *f;

	snprintf(buf, sizeof(buf), "/sys/class/ieee80211/phy%d/device/enable", radio_index);
	if ((f = fopen(buf, "r")) != NULL) {
		memset(buf, 0, sizeof(buf));
		if (fread(buf, 1, sizeof(buf), f))
			enabled_state = atoi(buf);
		fclose(f);
	}

	fill_de_node(de, &enabled_state, 1);

	return 0;
}

static int dump_bss_cb(struct nl_msg *msg, void *arg)
{
	struct radio_arg *radio = arg;
	u32 radio_index = radio->index;
	struct data_element *de = radio->de;
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct data_element_list *list_item;
	struct data_element_container *list_data;
	struct data_element *key_node;
	const char *ifname;

	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb_msg[NL80211_ATTR_WIPHY] ||
			radio_index != nla_get_u32(tb_msg[NL80211_ATTR_WIPHY]))
		goto out;

	if (!tb_msg[NL80211_ATTR_IFTYPE] ||
			NL80211_IFTYPE_AP != nla_get_u32(tb_msg[NL80211_ATTR_IFTYPE]))
		goto out;

	if (tb_msg[NL80211_ATTR_IFNAME]) {
		ifname = nla_get_string(tb_msg[NL80211_ATTR_IFNAME]);
		if (ubus_lookup_interface(ifname))
			return NL_SKIP;

		list_item = calloc(1, sizeof(struct data_element_list));
		if (!list_item)
			return -1;

		list_data = calloc(1, sizeof(struct data_element_container));
		if (!list_data)
			return -1;

		key_node = calloc(1, sizeof(struct data_element));
		if (!key_node)
			return -1;

		key_node->name = "BSSIfname";
		key_node->type = TYPE_STRING;
		fill_de_node(key_node, (void *)ifname, 0);

		list_data->num = 1;
		list_data->element = key_node;

		list_item->data = list_data;
		fill_de_node(de, list_item, 1);
		num_of_bss++;

		strncpy(survey_ifname, ifname, sizeof(survey_ifname)-1);

		memset(&chan_info, 0, sizeof(chan_info));
		if (tb_msg[NL80211_ATTR_WIPHY_FREQ]) {
			chan_info.freq = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_FREQ]);

			if (tb_msg[NL80211_ATTR_CHANNEL_WIDTH]) {
				chan_info.width = nla_get_u32(tb_msg[NL80211_ATTR_CHANNEL_WIDTH]);
				if (tb_msg[NL80211_ATTR_CENTER_FREQ1])
					chan_info.center_freq1 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ1]);
				if (tb_msg[NL80211_ATTR_CENTER_FREQ2])
					chan_info.center_freq2 = nla_get_u32(tb_msg[NL80211_ATTR_CENTER_FREQ2]);
			}
		}
		if (tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL])
			chan_info.txpower = nla_get_u32(tb_msg[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]);
	}

out:
	return NL_SKIP;
}

static int collect_bss_list(struct data_element *de, void *arg)
{
	struct radio_arg *radio = arg;
	u32 radio_index = radio->index;
	struct nl_msg *msg = nl80211_new_msg(NLM_F_DUMP, NL80211_CMD_GET_INTERFACE);
	struct radio_arg nl_arg;
	struct data_element_list *bss_list;
	struct data_element_container *bss;

	if (!msg) {
		printf("%s: alloc nl_msg failed\n", __func__);
		return -1;
	}

	nl_arg.de = de;
	nl_arg.index = radio_index;

	num_of_bss = 0;
	nl80211_send_and_recv(msg, dump_bss_cb, &nl_arg);

	bss_list = de->value.list;
	while (bss_list) {
		bss = bss_list->data;
		collect_bss(bss);
		bss_list = bss_list->next;
	}

	return 0;
}

static int collect_num_of_bss(struct data_element *de, void *arg)
{
	fill_de_node(de, &num_of_bss, 1);

	return 0;
}

static int collect_capabilities_container(struct data_element *de,void * arg)
{
	struct data_element_container *container =
				calloc(1, sizeof(struct data_element_container));

	de->value.container = container;
	collect_capabilities(container, &radio_capa, band);
	fill_de_node(de, container, 1);

	return 0;
}

static int collect_curr_opclass_list(struct data_element *de,void * arg)
{
	struct data_element_list *list_item = NULL;
	struct data_element_container *list_data = NULL;
	u8 mac[ETH_ALEN] = {0};

	list_item = calloc(1, sizeof(struct data_element_list));
	if (!list_item)
		return -1;

	list_data = calloc(1, sizeof(struct data_element_container));
	if (!list_data)
		return -1;

	collect_current_opclass(list_data, &chan_info);

	list_item->data = list_data;
	fill_de_node(de, list_item, 1);

	return 0;

}

static int collect_num_of_curr_opclass(struct data_element *de,void * arg)
{
	u8 num_of_curr_opclass = 1;

	fill_de_node(de, &num_of_curr_opclass, 1);
	return 0;
}

static int frequency_to_channel(int freq)
{
	/* see 802.11-2007 17.3.8.3.2 and Annex J */
	if (freq == 2484)
		return 14;
	else if (freq < 2484)
		return (freq - 2407) / 5;
	else if (freq >= 4910 && freq <= 4980)
		return (freq - 4000) / 5;
	else if (freq <= 45000) /* DMG band lower limit */
		return (freq - 5000) / 5;
	else if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;
	else
		return 0;
}

static int dump_wiphy_cb(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct nlattr *tb_band[NL80211_BAND_ATTR_MAX + 1];
	struct nlattr *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1];
	static struct nla_policy freq_policy[NL80211_FREQUENCY_ATTR_MAX + 1] = {
		[NL80211_FREQUENCY_ATTR_FREQ] = { .type = NLA_U32 },
		[NL80211_FREQUENCY_ATTR_MAX_TX_POWER] = { .type = NLA_U32 },
	};

	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct ht_vht_capability *capa = &radio_capa;
	struct nlattr *nl_band = NULL;
	struct nlattr *nl_freq = NULL;
	int rem_band, rem_freq;

	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (tb_msg[NL80211_ATTR_WIPHY_BANDS]) {
		nla_for_each_nested(nl_band, tb_msg[NL80211_ATTR_WIPHY_BANDS], rem_band) {
			nla_parse(tb_band, NL80211_BAND_ATTR_MAX, nla_data(nl_band),
				  	nla_len(nl_band), NULL);

			if (tb_band[NL80211_BAND_ATTR_HT_CAPA])
				capa->htcap.cap = nla_get_u16(tb_band[NL80211_BAND_ATTR_HT_CAPA]);

			if (tb_band[NL80211_BAND_ATTR_HT_MCS_SET] &&
					nla_len(tb_band[NL80211_BAND_ATTR_HT_MCS_SET]) >= 16)
				memcpy(&capa->htcap.mcs, nla_data(tb_band[NL80211_BAND_ATTR_HT_MCS_SET]), 16);

			if (tb_band[NL80211_BAND_ATTR_VHT_CAPA])
				capa->vhtcap.cap = nla_get_u32(tb_band[NL80211_BAND_ATTR_VHT_CAPA]);

			if (tb_band[NL80211_BAND_ATTR_VHT_MCS_SET] &&
					nla_len(tb_band[NL80211_BAND_ATTR_VHT_MCS_SET]) >= 8)
				memcpy(&capa->vhtcap.vht_mcs, nla_data(tb_band[NL80211_BAND_ATTR_VHT_MCS_SET]), 8);
		}
	}

	return NL_SKIP;
}

static int protocol_feature_handler(struct nl_msg *msg, void *arg)
{
	uint32_t *feat = arg;
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	if (tb_msg[NL80211_ATTR_PROTOCOL_FEATURES])
		*feat = nla_get_u32(tb_msg[NL80211_ATTR_PROTOCOL_FEATURES]);

	return NL_SKIP;
}

static uint32_t get_nl80211_protocol_features()
{
	uint32_t feat = 0;
	struct nl_msg *msg = nl80211_new_msg(0, NL80211_CMD_GET_PROTOCOL_FEATURES);

	if (!msg)
		return 0;

	if (nl80211_send_and_recv(msg, protocol_feature_handler, &feat) == 0)
		return feat;

	return 0;
}

static int nl80211_dump_wiphy(u32 index, struct data_element *nodes)
{
	u32 feat = 0;
	struct nl_msg *msg = nl80211_new_msg(0, NL80211_CMD_GET_WIPHY);

	if (!msg)
		return -1;

	memset(&radio_capa, 0, sizeof(radio_capa));
	/* TODO: Fix hardcode */
	if (index)
		band = BAND_2G;
	else
		band = BAND_5G;

	feat = get_nl80211_protocol_features();
	if (feat & NL80211_PROTOCOL_FEATURE_SPLIT_WIPHY_DUMP) {
		nla_put_flag(msg, NL80211_ATTR_SPLIT_WIPHY_DUMP);
		nlmsg_hdr(msg)->nlmsg_flags |= NLM_F_DUMP;
	}

	NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, index);
	nl80211_send_and_recv(msg, dump_wiphy_cb, nodes);

	return 0;

nla_put_failure:
	free(msg);
	return -1;
}

static int dump_survey_cb(struct nl_msg *msg, void *arg)
{
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *sinfo[NL80211_SURVEY_INFO_MAX + 1];
	struct data_element *nodes = arg;
	s8 noise;
	u64 time;
	u64 time_busy;
	u64 time_tx;
	u64 time_rx;

	static struct nla_policy survey_policy[NL80211_SURVEY_INFO_MAX + 1] = {
		[NL80211_SURVEY_INFO_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_SURVEY_INFO_NOISE] = { .type = NLA_U8 },
	};

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[NL80211_ATTR_SURVEY_INFO]) {
		fprintf(stderr, "survey data missing!\n");
		return NL_SKIP;
	}

	if (nla_parse_nested(sinfo, NL80211_SURVEY_INFO_MAX,
			     tb[NL80211_ATTR_SURVEY_INFO],
			     survey_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (sinfo[NL80211_SURVEY_INFO_NOISE]) {
		noise = nla_get_u8(sinfo[NL80211_SURVEY_INFO_NOISE]);
		u8 u_noise = noise < 0 ? (noise*-1) : noise;
		fill_de_node(&nodes[Noise], &u_noise, 1);
	}

	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME])
		time = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME]);

	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]) {
		time_busy = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]);
		fill_de_node(&nodes[Utilization], &time_busy, 1);
	}
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_RX]){
		time_rx = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_RX]);
		fill_de_node(&nodes[ReceiveSelf], &time_rx, 1);
	}
	if (sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_TX]) {
		time_tx = nla_get_u64(sinfo[NL80211_SURVEY_INFO_CHANNEL_TIME_TX]);
		fill_de_node(&nodes[Transmit], &time_tx, 1);
	}

	return NL_SKIP;
}

static int nl80211_dump_survey(u32 index, struct data_element *nodes)
{
	u32 radio_index = index;
	struct nl_msg *msg = NULL;
	u32 if_index = 0;

	if (strlen(survey_ifname))
		if_index = if_nametoindex(survey_ifname);

	msg = nl80211_new_msg(NLM_F_DUMP, NL80211_CMD_GET_SURVEY);
	if (!msg)
		return -1;

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, if_index);
	nl80211_send_and_recv(msg, dump_survey_cb, nodes);

	return 0;

nla_put_failure:
	free(msg);
	return -1;
}

static int collect_scan_result_list(struct data_element *de, void *arg)
{
	struct data_element_container *container =
				calloc(1, sizeof(struct data_element_container));
	struct radio_arg *radio = arg;
	u32 radio_index = radio->index;

	de->value.container = container;
	collect_scan_result(container, radio_index);
	fill_de_node(de, container, 1);

	return 0;

}

/* hardcode */
static int collect_unassoc_sta_list(struct data_element *de, void *arg)
{
	struct data_element_list *list_item = NULL;
	struct data_element_container *list_data = NULL;
	struct data_element *data_node = NULL;
	u8 mac[ETH_ALEN] = {0};
	u8 signal = 0;

	list_item = calloc(1, sizeof(struct data_element_list));
	if (!list_item)
		return -1;

	list_data = calloc(1, sizeof(struct data_element_container));
	if (!list_data)
		return -1;

	data_node = calloc(2, sizeof(struct data_element));
	if (!data_node)
		return -1;

	data_node[0].name = "MACAddress";
	data_node[0].flags = FLAG_LIST_KEY;
	data_node[0].type = TYPE_MAC_ADDRESS;
	fill_de_node(&data_node[0], mac, 1);
	data_node[1].name = "SignalStrength";
	data_node[1].flags = FLAG_STATIC;
	data_node[1].type = TYPE_UINT8;
	fill_de_node(&data_node[1], &signal, 1);

	list_data->num = 2;
	list_data->element = data_node;

	list_item->data = list_data;
	fill_de_node(de, list_item, 1);

	return 0;

}

static struct data_element radio_nodes[] = {
	[RadioIndex] = {
		"RadioIndex", FLAG_NONE, TYPE_UINT8
	},
	[ID] = {
		 "ID", FLAG_LIST_KEY, TYPE_MAC_ADDRESS
	},
	[Enabled] = {
		"Enabled", FLAG_NONE,  TYPE_BOOLEAN, collect_enabled_state
	},
	[Noise] = {
		"Noise", FLAG_NONE, TYPE_UINT8
	},
	[Utilization] = {
		"Utilization", FLAG_NONE, TYPE_UINT8
	},
	[Transmit] = {
		"Transmit", FLAG_NONE, TYPE_UINT8
	},
	[ReceiveSelf]  = {
		"ReceiveSelf", FLAG_NONE,  TYPE_UINT8
	},
	[ReceiveOther]  = {
		"ReceiveOther", FLAG_STATIC, TYPE_UINT8, .value.uint8 = 20,
	},
	[BSSList] = {
		"BSSList", FLAG_NONE, TYPE_LIST, collect_bss_list
	},
	[NumberOfBSS] = {
		"NumberOfBSS", FLAG_NONE, TYPE_UINT16, collect_num_of_bss
	},
	[Capabilities] = {
		"Capabilities", FLAG_STATIC, TYPE_CONTAINER, collect_capabilities_container
	},
	[CurrentOperatingClasses] = {
		"CurrentOperatingClasses", FLAG_NONE, TYPE_LIST, collect_curr_opclass_list,
	},
	[NumberOfCurrOpClass] = {
		"NumberOfCurrOpClass", FLAG_NONE, TYPE_UINT8, collect_num_of_curr_opclass,
	},
	[ScanResultList] = {
		"ScanResultList", FLAG_NONE, TYPE_CONTAINER, collect_scan_result_list,
	},
	[UnassociatedStaList] = {
		"UnassociatedStaList", FLAG_NONE, TYPE_LIST, collect_unassoc_sta_list,
	},
	[NumberOfUnassocSta] = {
		"NumberOfUnassocSta", FLAG_STATIC, TYPE_UINT16, .value.uint16 = 1,
	},
};

int collect_radio(struct data_element_container *parent)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;
	struct data_element *key_node = &radio_nodes[0];
	u8 mac[ETH_ALEN] = {0};
	char path[128] = {0};
	char buf[128] = {0};
	FILE *f;
	struct radio_arg arg;

	num_of_bss = 0;
	memset(survey_ifname, 0, sizeof(survey_ifname));

	arg.index = parent->element[0].value.uint32;
	fill_de_node(&radio_nodes[RadioIndex], &arg.index, 0);

	snprintf(path, sizeof(path), "/sys/class/ieee80211/phy%d/macaddress", arg.index);
	if ((f = fopen(path, "r")) != NULL) {
		fgets(buf, sizeof(buf), f);
		fclose(f);
	}
	sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
		&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	fill_de_node(key_node, mac, 1);

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else {
		printf("collect radio realloc elements failed with no memory\n");
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, radio_nodes, IndexMax*sizeof(struct data_element));

	/*
	  * fill Capabilities
	  */
	nl80211_dump_wiphy(arg.index, nodes);

	for (i = 0; i < IndexMax; i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, &arg);
	}

	/*
	  * fill Noise, Utilization, Transmit, ReceiveSelf, ReceiveOther
	  */
	nl80211_dump_survey(arg.index, nodes);

	return 0;
}

int update_radio(struct data_element_container *parent)
{
	struct data_element *nodes = parent->element;
	struct data_element *node;
	struct data_element_list *list_item;
	struct data_element_container *list_data;
	struct radio_arg arg;

	node = &nodes[RadioIndex];
	arg.index = node->value.uint8;

	node = &nodes[BSSList];
	list_item = node->value.list;
	while (list_item) {
		list_data = list_item->data;
		update_bss(list_data);
		list_item = list_item->next;
	}

	node = &nodes[ScanResultList];
	cleanup_de_node(node);
	collect_scan_result_list(node, &arg);

	return 0;
}
