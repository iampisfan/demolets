#ifndef _DATA_ELEMENT_H_
#define _DATA_ELEMENT_H_

#include "yang.h"
#include "type.h"

enum value_type {
	TYPE_UINT8,
	TYPE_INT8,
	TYPE_UINT16,
	TYPE_UINT32,
	TYPE_STRING,
	TYPE_BOOLEAN,
	TYPE_MAC_ADDRESS,
	TYPE_ARRAY,
	TYPE_CONTAINER,
	TYPE_LIST,
	TYPE_BINARY,
};

struct data_element_array {
	enum value_type type;
	u8 num;
	void *value;
};

struct data_element_container {
	u8 num;
	struct data_element *element;
};

struct data_element_list {
	struct data_element_list *next;
	struct data_element_container *data;
};

struct data_element_binary {
	u8 num;
	byte *bytes;
};

union value_val {
	u8 uint8;
	s8 int8;
	u16 uint16;
	u32 uint32;
	char *string;
	Boolean boolean;
	u8 mac_address[ETH_ALEN];
	struct data_element_array *array;
	struct data_element_binary *binary;
	struct data_element_container *container;
	struct data_element_list *list;
};

struct data_element {
	char *name;
	u32 flags;
	enum value_type type;
	int (*collect)(struct data_element *de, void *arg);
	union value_val value;
};

int fill_de_node(struct data_element *node, void *value, int update_yang);
int cleanup_de_node(struct data_element *node);
int cleanup_de_nodes(struct data_element nodes[], u8 num);
int cleanup_de_container(struct data_element_container **container);
int flush_yang(struct lyd_node *node, struct data_element *nodes, u8 num);

#endif
