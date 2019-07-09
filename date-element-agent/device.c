#include <stdio.h>
#include <stdlib.h>
#include <glob.h>
#include <string.h>

#include "collect.h"

enum index {
	ID,
	RadioList,
	NumberOfRadios,
	CollectionInterval,
	IndexMax,
};
static u16 num_of_radio;

static int collect_radio_list(struct data_element *de, void *arg)
{
	glob_t globbuf;
	int i = 0;
	FILE *f;
	static char buf[128];
	struct data_element_list *list_item ;
	struct data_element_container *list_data;
	struct data_element *key_node;

	glob("/sys/class/ieee80211/*", 0, NULL, &globbuf);

	num_of_radio  = globbuf.gl_pathc;
	if (!num_of_radio)
		return 0;

	for (i = 0; i < num_of_radio; i++) {
		list_item = calloc(1, sizeof(struct data_element_list));
		if (!list_item)
			return -1;

		list_data = calloc(1, sizeof(struct data_element_container));
		if (!list_data)
			return -1;

		key_node = calloc(1, sizeof(struct data_element));
		if (!key_node)
			return -1;

		key_node->name = "RadioIndex";
		key_node->type = TYPE_UINT32;
		snprintf(buf, sizeof(buf), "%s/%s", globbuf.gl_pathv[i], "index");
		if ((f = fopen(buf, "r")) != NULL) {
			memset(buf, 0, sizeof(buf));
			if (fread(buf, 1, sizeof(buf), f)) {
				u32 radio_index = atoi(buf);
				fill_de_node(key_node, &radio_index, 0);
			}
			fclose(f);
		}

		list_data->num = 1;
		list_data->element = key_node;
		collect_radio(list_data);

		list_item->data = list_data;
		fill_de_node(de, list_item, 1);
	}

	globfree(&globbuf);

	return 0;
}

static int collect_num_of_radios(struct data_element *de, void *arg)
{
	fill_de_node(de, &num_of_radio, 1);

	return 0;
}

static int collect_interval(struct data_element *de, void *arg)
{
	u32 interval = COLLECTION_INTERVAL;

	fill_de_node(de, &interval, 1);
	return 0;
}

static struct data_element device_nodes[] = {
	[ID] = {
		"ID", FLAG_LIST_KEY, TYPE_MAC_ADDRESS
	},
	[RadioList] ={
		"RadioList",FLAG_NONE, TYPE_LIST,  collect_radio_list
	},
	[NumberOfRadios] = {
		"NumberOfRadios", FLAG_NONE, TYPE_UINT16, collect_num_of_radios
	},
	[CollectionInterval] = {
		 "CollectionInterval", FLAG_NONE, TYPE_UINT32, collect_interval
	}
};

int collect_device(struct data_element_container *parent)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;
	struct data_element *key_node = &device_nodes[0];

	num_of_radio = 0;
	fill_de_node(key_node, parent->element[0].value.mac_address, 1);

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else {
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, device_nodes, IndexMax*sizeof(struct data_element));

	for (i = 0; i < IndexMax; i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, NULL);
	}

	return 0;
}

int update_device(struct data_element_container *parent)
{
	struct data_element *nodes = parent->element;
	struct data_element *node;
	struct data_element_list *list_item;
	struct data_element_container *list_data;

	node = &nodes[RadioList];
	list_item = node->value.list;
	while (list_item) {
		list_data = list_item->data;
		update_radio(list_data);
		list_item = list_item->next;
	}
	return 0;
}
