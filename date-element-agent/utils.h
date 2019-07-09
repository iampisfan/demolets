#ifndef _UTILS_H_
#define _UTILS_H_

#include "type.h"

#define WL_IOCTL_WL_PARAM (SIOCIWFIRSTPRIV + 0)
#define WL_IOCTL_SETCMD	(SIOCIWFIRSTPRIV + 20)
#define WL_IOCTL_GETCMD	(SIOCIWFIRSTPRIV + 25)
#define WL_PARAM_OFF_CHANNEL_REQ_SEND 86

int  get_interface_macaddr(const char *ifname, u8 *mac_address);
int get_bss_ssid(const char *ifname, char *ssid);
int ioctl_setcmd(const char * ifname, char * cmd_str);
int ioctl_getcmd(const char *ifname, char *buf, int len);
int ioctl_setparam(const char *ifname, int op, int arg);
void get_time_ISO8061(char *time_str, int size);
char *encode_base64(const u8 *buf, const u32 size);

#endif
