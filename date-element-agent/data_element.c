#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_element.h"
#include "utils.h"

int fill_de_node(struct data_element *node, void *value, int update_yang)
{
	switch (node->type) {
	case TYPE_UINT8:
		node->value.uint8 = *(u8 *)value;
		break;
	case TYPE_INT8:
		node->value.int8 = *(s8 *)value;
		break;
	case TYPE_UINT16:
		node->value.uint16 = *(u16 *)value;
		break;
	case TYPE_UINT32:
		node->value.uint32 = *(u32 *)value;
		break;
	case TYPE_STRING:
		node->value.string = strdup(value);
		node->flags |= FLAG_FREE_VALUE;
		break;
	case TYPE_BOOLEAN:
		node->value.boolean = *(u8 *)value ? TRUE:FALSE;
		break;
	case TYPE_MAC_ADDRESS:
		memcpy(node->value.mac_address, value, ETH_ALEN);
		break;
	case TYPE_BINARY:
		node->value.binary = (struct data_element_binary *)value;
		node->flags |= FLAG_FREE_VALUE;
		break;
	case TYPE_ARRAY:
		node->value.array = (struct data_element_array *)value;
		node->flags |= FLAG_FREE_VALUE;
		break;
	case TYPE_CONTAINER:
		node->value.container = (struct data_element_container *)value;
		node->flags |= FLAG_FREE_VALUE;
		break;
	case TYPE_LIST:
		if (NULL == node->value.list)
			node->value.list = (struct data_element_list *)value;
		else {
			struct data_element_list *prev = node->value.list;
			struct data_element_list *next = node->value.list->next;
			while (next) {
				prev = next;
				next = prev->next;
			}
			prev->next = (struct data_element_list *)value;
		}
		node->flags |= FLAG_FREE_VALUE;
		break;
	default:
		break;
	}

	if (update_yang)
		node->flags |= FLAG_UPDATE_YANG;

	return 0;
}

int cleanup_de_container(struct data_element_container **container)
{
	int i = 0;
	struct data_element_container *de_container = *container;

	for (i = 0; i < de_container->num; i++) {
		cleanup_de_node(&de_container->element[i]);
	}

	free(de_container->element);
	de_container->element = NULL;
	de_container->num = 0;

	free(de_container);
	de_container = NULL;

	return 0;
}

static int cleanup_de_list(struct data_element_list **list)
{
	struct data_element_list *item;
	struct data_element_list *tmp;

	for (item = *list; item;) {
		tmp = item->next;
		cleanup_de_container(&item->data);
		free(item);
		item = tmp;
	}
	*list = NULL;
	
	return 0;
}

int cleanup_de_node(struct data_element *node)
{
	int i = 0;

	if (node->flags & FLAG_FREE_VALUE) {
		switch(node->type) {
		case TYPE_STRING:
			free(node->value.string);
			node->value.string = NULL;
			break;
		case TYPE_BINARY:
			free(node->value.binary->bytes);
			node->value.binary->bytes = NULL;
			free(node->value.binary);
			node->value.binary = NULL;
			break;
		case TYPE_ARRAY:
			free(node->value.array->value);
			node->value.array->value = NULL;
			node->value.array->num = 0;
			free(node->value.array);
			node->value.array = NULL;
			break;
		case TYPE_CONTAINER:
			cleanup_de_container(&node->value.container);
			break;
		case TYPE_LIST:
			cleanup_de_list(&node->value.list);
			break;
		default:
			break;
		}
		node->flags &= ~FLAG_FREE_VALUE;
	}

	return 0;
}

int cleanup_de_nodes(struct data_element nodes[], u8 num)
{
	int i = 0;

	for (i = 0; i < num; i++)
		cleanup_de_node(&nodes[i]);

	return 0;
}

static int get_de_node_value_str(struct data_element *de, char *str, int str_len)
{
	char byte_str[3] = {0};
	char array_str[10] = {0};
	char *dec64_str = NULL;
	int num = 0;

	switch (de->type) {
	case TYPE_UINT8:
		snprintf(str, str_len, "%d", de->value.uint8);
		break;
	case TYPE_INT8:
		snprintf(str, str_len, "%d", de->value.int8);
		break;
	case TYPE_UINT16:
		snprintf(str, str_len, "%d", de->value.uint16);
		break;
	case TYPE_UINT32:
		snprintf(str, str_len, "%ld", de->value.uint32);
		break;
	case TYPE_STRING:
		strncpy(str, de->value.string, str_len-1);
		break;
	case TYPE_BOOLEAN:
		if (de->value.boolean)
			strncpy(str, "true", str_len-1);
		else
			strncpy(str, "false", str_len-1);
		break;
	case TYPE_MAC_ADDRESS:
		snprintf(str, str_len, "%02x:%02x:%02x:%02x:%02x:%02x",
			de->value.mac_address[0], de->value.mac_address[1],
			de->value.mac_address[2], de->value.mac_address[3],
			de->value.mac_address[4], de->value.mac_address[5]);
		break;
	case TYPE_BINARY:
		dec64_str = encode_base64((u8 *)de->value.binary->bytes,
				de->value.binary->num);
		if (dec64_str) {
			strncpy(str, dec64_str, str_len);
			free(dec64_str);
		}
/*
		num = 0;
		byte_str[0] = '0'; byte_str[1] = 'x'; byte_str[2] = '\0';
		while (num < de->value.binary->num) {
			strcat(str, byte_str);
			snprintf(byte_str, 3, "%02x", *((u8 *)de->value.binary->bytes+num));
			num++;
		}
		strcat(str, byte_str);
*/
		break;
	case TYPE_ARRAY:
		if (de->value.array->type == TYPE_UINT8) {
			num = de->value.array->num;
			while (num--) {
				if (de->value.array->num)
					snprintf(array_str, 10, "%d,", *((u8 *)de->value.array->value+num));
				else
					snprintf(array_str, 10, "%d", *((u8 *)de->value.array->value+num));
				strcat(str, array_str);
			}
		}
		break;
	default:
		break;
	}

	return 0;
}

static int flush_yang_container(struct lyd_node *yang,
		char *name, struct data_element_container *de)
{
	struct data_element *key_node = NULL;
	char value_str[128] = {0};
	char list_path[128] = {0};
	struct ly_set *set = NULL;
	struct lyd_node *yang_node;
	int i = 0;

	if (!de->num)
		return 0;

	key_node = &de->element[0];
	if (key_node->flags & FLAG_LIST_KEY) {
		get_de_node_value_str(key_node, value_str, sizeof(value_str));
		snprintf(list_path, sizeof(list_path), "%s[%s=\'%s\']",
				name, key_node->name, value_str);
		update_lyd_node(yang, list_path, NULL);
		set = lyd_get_node(yang, list_path);
		if (set && set->number) {
			yang_node = set->set.d[0];
			flush_yang(yang_node, de->element+1, de->num -1);
		}
		ly_set_free(set);
	} else if (key_node->flags & FLAG_LIST_EMPTY) {
		update_lyd_node(yang, name, NULL);
	} else {
		update_lyd_node(yang, name, NULL);
		set = lyd_get_node(yang, name);
		if (set && set->number) {
			yang_node = set->set.d[0];
			flush_yang(yang_node, de->element, de->num);
		}
		ly_set_free(set);
	}

	return 0;
}
static int flush_yang_list(struct lyd_node *yang, struct data_element *de)
{
	struct data_element_list *list = de->value.list;
	struct data_element_list *item;

	for (item = list; item; item = item->next)
		flush_yang_container(yang, de->name, item->data);

	return 0;
}

int flush_yang(struct lyd_node *yang, struct data_element *nodes, u8 num)
{
	int i = 0;
	struct data_element de;
	char value_str[128] = {0};
	char byte_str[3] = {0};

	for (i = 0; i < num; i++) {
		memset(value_str, 0, sizeof(value_str));
		de = nodes[i];

		if (!(de.flags & (FLAG_UPDATE_YANG | FLAG_STATIC)))
			continue;

		switch (de.type) {
		case TYPE_LIST:
			flush_yang_list(yang, &de);
			break;
		case TYPE_CONTAINER:
			flush_yang_container(yang, de.name, de.value.container);
			break;
		case TYPE_ARRAY:
			if (0 == de.value.array->num) {
				update_lyd_node(yang, de.name, NULL);
				break;
			}
		default:
			get_de_node_value_str(&de, value_str, sizeof(value_str));
			break;
		}

		if (strlen(value_str) || (de.flags & FLAG_ALLOW_EMPTY_STR)) 
			update_lyd_node(yang, de.name, value_str);
	}

	return 0;
}

