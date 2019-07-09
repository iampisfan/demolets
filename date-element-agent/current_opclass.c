#include <stdio.h>
#include <stdlib.h>

#include "opclass.h"
#include "collect.h"
#include "utils.h"
#include "nl80211.h"

enum index {
	Class,
	Channel,
	TxPower,
	TimeStamp,
	IndexMax
};

static int collect_class(struct data_element *de, void *arg)
{
	struct channel_info *chan_info = arg;

	fill_de_node(de, &chan_info->opclass, 1);
	return 0;
}

static int collect_channel(struct data_element *de, void *arg)
{
	struct channel_info *chan_info = arg;

	fill_de_node(de, &chan_info->channel, 1);
	return 0;
}

static int collect_tx_power(struct data_element *de, void *arg)
{
	struct channel_info *chan_info = arg;

	fill_de_node(de, &chan_info->txpower, 1);
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

static struct data_element current_opclass_nodes[] = {
	[Class] = {
		"Class", FLAG_LIST_KEY, TYPE_UINT8, collect_class,
	},
	[Channel] = {
		"Channel", FLAG_NONE, TYPE_UINT8, collect_channel,
	},
	[TxPower] = {
		"TxPower", FLAG_NONE, TYPE_INT8, collect_tx_power,
	},
	[TimeStamp] = {
		"TimeStamp", FLAG_NONE, TYPE_STRING, collect_time_stamp,
	},
};

int collect_current_opclass(struct data_element_container *parent, void *arg)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;
	struct data_element *key_node = &current_opclass_nodes[0];
	struct channel_info *chan_info = arg;
	int sec_channel, vht;
	
	if (chan_info->center_freq1 > chan_info->freq)
		sec_channel = 1;
	else if (chan_info->center_freq1 < chan_info->freq)
		sec_channel = -1;
	else
		sec_channel = 0;

	switch(chan_info->width) {
	case NL80211_CHAN_WIDTH_80:
		vht = VHT_CHANWIDTH_80MHZ;
		break;
	case NL80211_CHAN_WIDTH_80P80:
		vht = VHT_CHANWIDTH_80P80MHZ;
		break;
	case NL80211_CHAN_WIDTH_160:
		vht = VHT_CHANWIDTH_160MHZ;
		break;
	default:
		vht = VHT_CHANWIDTH_USE_HT;
		break;
	}

	freq_to_channel_opclass(chan_info->freq, sec_channel, vht,
				&chan_info->opclass, &chan_info->channel);

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else {
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, current_opclass_nodes, IndexMax*sizeof(struct data_element));

	for (i = 0; i < IndexMax; i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, chan_info);
	}

	return 0;
}
