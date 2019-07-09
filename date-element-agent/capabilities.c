#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_element.h"
#include "capabilities.h"
#include "collect.h"
#include "vendor.h"

enum index {
	HTCapabilities,
	VHTCapabilities,
	OperatingClasses,
	NumberOfOpClass,
	IndexMax,
};

static int radio_band;
static int collect_ht_cap(struct data_element *de, void *arg)
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

static int collect_vht_cap(struct data_element *de, void *arg)
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

/* hardcode */
static int collect_opclass_list(struct data_element *de, void *arg)
{
	struct ht_vht_capability *capa = arg;
	u8 num, i, opclass;
	struct data_element_list *list_item;
	struct data_element_container *list_data;
	struct data_element *key_node;

	if (radio_band == BAND_2G) {
		list_item = calloc(1, sizeof(struct data_element_list));
		list_data = calloc(1, sizeof(struct data_element_container));
		key_node = calloc(1, sizeof(struct data_element));
		opclass = 81;
		key_node->name = "Class";
		key_node->type = TYPE_UINT8;
		fill_de_node(key_node, &opclass, 0);
		list_data->num = 1;
		list_data->element = key_node;
		collect_capable_opclass(list_data);
		list_item->data = list_data;
		fill_de_node(de, list_item, 1);

		list_item = calloc(1, sizeof(struct data_element_list));
		list_data = calloc(1, sizeof(struct data_element_container));
		key_node = calloc(1, sizeof(struct data_element));
		opclass = 83;
		key_node->name = "Class";
		key_node->type = TYPE_UINT8;
		fill_de_node(key_node, &opclass, 0);
		list_data->num = 1;
		list_data->element = key_node;
		collect_capable_opclass(list_data);
		list_item->data = list_data;
		fill_de_node(de, list_item, 1);

		list_item = calloc(1, sizeof(struct data_element_list));
		list_data = calloc(1, sizeof(struct data_element_container));
		key_node = calloc(1, sizeof(struct data_element));
		opclass = 84;
		key_node->name = "Class";
		key_node->type = TYPE_UINT8;
		fill_de_node(key_node, &opclass, 0);
		list_data->num = 1;
		list_data->element = key_node;
		collect_capable_opclass(list_data);
		list_item->data = list_data;
		fill_de_node(de, list_item, 1);
	} else {
		num = 16;
		for (i = 0; i < num; i++) {
			list_item = calloc(1, sizeof(struct data_element_list));
			list_data = calloc(1, sizeof(struct data_element_container));
			key_node = calloc(1, sizeof(struct data_element));
			opclass = 115+i;
			key_node->name = "Class";
			key_node->type = TYPE_UINT8;
			fill_de_node(key_node, &opclass, 0);
			list_data->num = 1;
			list_data->element = key_node;
			collect_capable_opclass(list_data);
			list_item->data = list_data;
			fill_de_node(de, list_item, 1);
		}
	}

	return 0;
}

/* hardcode */
static int collect_num_of_opclass(struct data_element *de, void *arg)
{
	struct capabilities *capa = arg;
	u8 num = 0;

	if (radio_band == BAND_2G)
		num = 3;
	else
		num = 16;

	fill_de_node(de, &num, 1);

	return 0;
}

static struct data_element capa_nodes[] = {
	[HTCapabilities] = {
		.name = "HTCapabilities",
		.type = TYPE_BINARY,
		.collect = collect_ht_cap
	},
	[VHTCapabilities] = {
		.name = "VHTCapabilities",
		.type = TYPE_BINARY,
		.collect = collect_vht_cap
	},
	[OperatingClasses] = {
		.name = "OperatingClasses",
		.type = TYPE_LIST,
		.collect = collect_opclass_list,
	},
	[NumberOfOpClass] = {
		.name = "NumberOfOpClass",
		.type = TYPE_UINT8,
		.collect = collect_num_of_opclass,
	},
};

int collect_capabilities(struct data_element_container *parent, void *arg, int band)
{
	int i = 0;
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;
	struct ht_vht_capability *capa = arg;
	radio_band = band;

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else {
		printf("collect capabilities realloc failed with no memory\n");
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, capa_nodes, IndexMax*sizeof(struct data_element));

	for (i = 0; i < IndexMax; i++) {
		node = &nodes[i];
		if (node->collect)
			node->collect(node, capa);
	}

	return 0;
}
