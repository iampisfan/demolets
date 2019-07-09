#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_element.h"
#include "collect.h"

/* TODO: fill with true value, but not hardcode */

enum index {
	Class,
	MaxTxPower,
	NonOperable,
	NumberOfNonOperChan,
	IndexMax
};

static struct data_element capable_op_class_nodes[] = {
	[Class] = {
		"Class", FLAG_LIST_KEY, TYPE_UINT8,
	},
	[MaxTxPower] = {
		"MaxTxPower", FLAG_STATIC, TYPE_INT8, .value.int8 = 20,
	},
	[NonOperable] = {
		"NonOperable", FLAG_NONE, TYPE_ARRAY,
	},
	[NumberOfNonOperChan] = {
		"NumberOfNonOperChan", FLAG_NONE, TYPE_UINT8,
	},
};

int collect_capable_opclass(struct data_element_container *parent)
{
	struct data_element *nodes = NULL;
	struct data_element *node = NULL;
	struct data_element *key_node = &capable_op_class_nodes[0];
	u8 opclass;
	u8 num_of_nonopchan;
	struct data_element_array *nonopchan;
	u8 *value;

	opclass = parent->element[0].value.uint8;
	fill_de_node(key_node, &opclass, 1);

	parent->element = realloc(parent->element, IndexMax*sizeof(struct data_element));
	if (parent->element) {
		parent->num = IndexMax;
		nodes = parent->element;
	} else {
		printf("collect radio realloc elements failed with no memory\n");
		parent->num = 0;
		return -1;
	}

	memcpy(nodes, capable_op_class_nodes, IndexMax*sizeof(struct data_element));

	switch (opclass) {
	case 81:
	case 84:
		num_of_nonopchan = 2;
		nonopchan = calloc(1, sizeof(struct data_element_array));
		nonopchan->type = TYPE_UINT8;
		nonopchan->num = 2;
		nonopchan->value = calloc(nonopchan->num, sizeof(TYPE_UINT8));
		value = (u8 *)nonopchan->value;
		*value = 12;
		*(value + 1) = 13;
		fill_de_node(&nodes[NonOperable], nonopchan, 1);
		break;
	case 125:
		num_of_nonopchan = 1;
		nonopchan = calloc(1, sizeof(struct data_element_array));
		nonopchan->type = TYPE_UINT8;
		nonopchan->num = 1;
		nonopchan->value = calloc(nonopchan->num, sizeof(TYPE_UINT8));
		value = (u8 *)nonopchan->value;
		*value = 169;
		fill_de_node(&nodes[NonOperable], nonopchan, 1);
		break;
	default:
		num_of_nonopchan = 0;
		nonopchan = calloc(1, sizeof(struct data_element_array));
		nonopchan->type = TYPE_UINT8;
		nonopchan->num = 0;
		nonopchan->value = NULL;
		fill_de_node(&nodes[NonOperable], nonopchan, 1);
		break;
	}
	fill_de_node(&nodes[NumberOfNonOperChan], &num_of_nonopchan, 1);

	return 0;
}
