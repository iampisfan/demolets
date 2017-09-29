#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct ie_list {
	unsigned char bssid[6];
	unsigned char client_mac[6];
	unsigned char ie_len;
	unsigned char *ie_data;
	struct ie_list *next;
};

struct ie_list* new_node(unsigned char *buf)
{
	struct ie_list *ie = NULL;

	if (!buf)
		return NULL;

	ie = (struct ie_list *)malloc(sizeof(struct ie_list));
	memset(ie, 0, sizeof(struct ie_list));
	memcpy(ie->bssid, buf, sizeof(ie->bssid));
	buf += sizeof(ie->bssid);
	memcpy(ie->client_mac, buf, sizeof(ie->client_mac));
	buf += sizeof(ie->client_mac);
	ie->ie_len = *buf;
	buf += sizeof(unsigned char);
	ie->ie_data = (char *)malloc(ie->ie_len);
	memcpy(ie->ie_data, buf, ie->ie_len);
	ie->next = NULL;

	return ie;
}

void add_node(struct ie_list *head, struct ie_list *node)
{
	struct ie_list *ie = head;

	if (!head || !node)
		return;

	while (ie->next != NULL)
		ie = ie->next;

	ie->next = node;
}

struct ie_list* search_node(struct ie_list *head, char *client_mac)
{
	struct ie_list *ie = head;

	if (!head || !client_mac)
		return NULL;

	while (memcmp(ie->client_mac, client_mac,
				sizeof(ie->client_mac)) != 0) {
		ie = ie->next;
		if (ie == NULL)
			return NULL;
	}

	return ie;
}

void del_node(struct ie_list *head, char *client_mac)
{
	struct ie_list *ie = head;
	struct ie_list *node = NULL;

	if (!head || !client_mac)
		return;

	while (ie->next != NULL) {
		node = ie->next;
		if (memcmp(node->client_mac,
					client_mac, sizeof(node->client_mac)) == 0) {
			ie->next = node->next;
			if (node->ie_data && node->ie_len)
				free(node->ie_data);
			if (node)
				free(node);
		}
		ie = ie->next;
	}
}

int main()
{
	unsigned char data1[] = {0x11,0x22,0x00,0x00,0x00,0x00,
		0x11,0x11,0x11,0x11,0x11,0x11,
		0x01,
		0x56};
	unsigned char data2[] = {0x00,0x00,0x00,0x00,0x00,0x00,
		0xff,0x22,0x25,0x22,0x27,0x22,
		0x03,
		0x56,0x67,0x78};
	unsigned char data3[] = {0x00,0x00,0x00,0x00,0x00,0x00,
		0x33,0x33,0x33,0x33,0x33,0x33,
		0x05,
		0xff,0xff,0xff,0xff,0xff};

	struct ie_list *ie1 = new_node(data1);
	struct ie_list *ie2 = new_node(data2);
	struct ie_list *ie3 = new_node(data3);
	struct ie_list *head = (struct ie_list *)malloc(sizeof(struct ie_list));
	unsigned char client_mac[6] = {0xff,0x22,0x25,0x22,0x27,0x22};

	printf("before add to list, ie2->client_mac = %x:%x:%x:%x:%x:%x\n",ie2->client_mac[0],
			ie2->client_mac[1],ie2->client_mac[2],ie2->client_mac[3],ie2->client_mac[4],ie2->client_mac[5]);
	printf("before add to list, ie2->ie_len = %d\n",ie2->ie_len);
	printf("before add to list, ie2->ie_data = 0x%x,0x%x,0x%x\n",ie2->ie_data[0],ie2->ie_data[1],ie2->ie_data[2]);

	add_node(head, ie1);
	add_node(head, ie2);
	add_node(head, ie3);

	struct ie_list *result = search_node(head, client_mac);
	if (result) {
		printf("search result, result->ie_len = %d\n",result->ie_len);
	}

	del_node(head, client_mac);
	if (search_node(head, client_mac) == NULL)
		printf("del_node result success\n");

	return 0;
}
