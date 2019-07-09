#ifndef _UBUS_H_
#define _UBUS_H_

#include <libubus.h>

int ubus_lookup_interface(const char *ifname);
int ubus_get_clients(const char *ifname, void *arg);

#endif
