#include <stdio.h>
#include <stdlib.h>

#include "ubus.h"
#include "type.h"

struct station {
	u16 num;
	u16 num_max;
	char *mac_addr[10];
};
int ubus_lookup_interface(const char *ifname)
{
	struct ubus_context *ctx;
	char ubus_path[128] = {0};
	int ret = 0;
	u32 ubus_id = 0;

	ctx = ubus_connect(NULL);
	if (!ctx)
		return -1;

	snprintf(ubus_path, sizeof(ubus_path)-1, "hostapd.%s", ifname);
	ret = ubus_lookup_id(ctx, ubus_path, &ubus_id);

	ubus_free(ctx);
	return ret;
}

static void get_clients_cb(struct ubus_request *req, int type, struct blob_attr *msg)
{
	struct station *sta_list;
	struct blob_attr *pos;
	struct blob_attr *attr = blobmsg_data(msg);
	struct blob_attr *clients;
	int rem;

	sta_list = req->priv;
	rem = blobmsg_data_len(msg);
	__blob_for_each_attr(pos, blobmsg_data(msg), rem) {
		if (strcmp(blobmsg_name(pos), "clients") == 0)
			clients = pos;
	}

	rem = blobmsg_data_len(clients);
	__blob_for_each_attr(pos, blobmsg_data(clients), rem) {
		sta_list->mac_addr[sta_list->num] = strdup(blobmsg_name(pos));
		if (sta_list->num++ >= sta_list->num_max)
			break;
	}
}

int ubus_get_clients(const char *ifname, void *arg)
{
	struct ubus_context *ctx;
	char ubus_path[128] = {0};
	int ret = 0;
	u32 ubus_id = 0;

	ctx = ubus_connect(NULL);
	if (!ctx)
		return -1;

	snprintf(ubus_path, sizeof(ubus_path)-1, "hostapd.%s", ifname);
	ret = ubus_lookup_id(ctx, ubus_path, &ubus_id);
	if (ret) {
		ubus_free(ctx);
		return ret;
	}

	return ubus_invoke(ctx, ubus_id, "get_clients", NULL, get_clients_cb,
			arg, 3000);
}

