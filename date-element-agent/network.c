#include <stdio.h>
#include <stdlib.h>
#include <libyang/libyang.h>
#include <string.h>

#include "collect.h"
#include "utils.h"

enum index{
	ID,
	NumberOfDevices,
	DeviceList,
	TimeStamp,
	IndexMax,
};

static int collect_id(struct data_element *de, void *arg)
{
	u8 mac[ETH_ALEN] = {0};

	get_interface_macaddr("br-lan", mac);
	fill_de_node(de, mac, 1);

	return 0;
}

static int collect_num_of_devices(struct data_element *de, void *arg)
{
	u16 num = 1;
	if (NULL == de)
		return -1;

	fill_de_node(de, &num, 1);

	return 0;
}

static int collect_device_list(struct data_element *de, void *arg)
{
	struct data_element_list *list_item = NULL;
	struct data_element_container *list_data = NULL;
	struct data_element *key_node = NULL;
	u8 mac[ETH_ALEN] = {0};

	list_item = calloc(1, sizeof(struct data_element_list));
	if (!list_item)
		return -1;

	list_data = calloc(1, sizeof(struct data_element_container));
	if (!list_data)
		return -1;

	key_node = calloc(1, sizeof(struct data_element));
	if (!key_node)
		return -1;

	key_node->name = "ID";
	key_node->type = TYPE_MAC_ADDRESS;
	get_interface_macaddr("br-lan", mac);
	fill_de_node(key_node, mac, 0);

	list_data->num = 1;
	list_data->element = key_node;
	collect_device(list_data);

	list_item->data = list_data;
	fill_de_node(de, list_item, 1);

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

static struct data_element network_nodes[] = {
	[ID] =
		{ "ID", FLAG_NONE, TYPE_MAC_ADDRESS, collect_id },
	[NumberOfDevices] =
		{ "NumberOfDevices", FLAG_NONE, TYPE_UINT16, collect_num_of_devices },
	[DeviceList] =
		{ "DeviceList", FLAG_NONE, TYPE_LIST, collect_device_list },
	/* TimeStamp should be the last one */
	[TimeStamp] =
		{ "TimeStamp", FLAG_NONE, TYPE_STRING, collect_time_stamp }
};

int collect_network(struct data_element_container *parent)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else
		return -1;

	memcpy(nodes, network_nodes, IndexMax*sizeof(struct data_element));

	for (i = 0; i < IndexMax; i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, NULL);
	}

	return 0;
}

int update_network(struct data_element_container *parent)
{
	struct data_element *nodes = parent->element;
	struct data_element *node;
	struct data_element_list *list_item;
	struct data_element_container *list_data;

	node = &nodes[TimeStamp];
	cleanup_de_node(node);
	collect_time_stamp(node, NULL);

	node = &nodes[DeviceList];
	list_item = node->value.list;
	while (list_item) {
		list_data = list_item->data;
		update_device(list_data);
		list_item = list_item->next;
	}
	return 0;
}
