#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/if.h>
#include <errno.h>
#include <time.h>
#include <libubus.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "data_element.h"
#include "utils.h"
#include "wext.h"

int get_interface_macaddr(const char *ifname, u8 *mac_address)
{
	struct ifreq ifr;
	int ret = 0;
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("socket() failed: %s\n", strerror(errno));
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

	ret = ioctl(sock, SIOCGIFHWADDR, &ifr);
	if (ret) {
		printf("get hardware failed\n");
		close(sock);
		return -1;
	}

	memcpy(mac_address, &ifr.ifr_hwaddr.sa_data, ETH_ALEN);

	close(sock);
	return 0;
}

int get_bss_ssid(const char *ifname, char *ssid)
{
#define ESSID_MAX_SIZE	32
	struct iwreq iwr;
	int ret = 0;
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("socket() failed: %s\n", strerror(errno));
		return -1;
	}

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
	iwr.u.essid.pointer = (caddr_t) ssid;
	iwr.u.essid.length  = ESSID_MAX_SIZE + 1;
	iwr.u.essid.flags   = 0;

	ret = ioctl(sock, SIOCGIWESSID, &iwr);
	close(sock);

	return ret;
}

int ioctl_setcmd(const char *ifname, char *cmd_str)
{
	struct iwreq iwr;
	int ret = 0;
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("socket() failed: %s\n", strerror(errno));
		return -1;
	}

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
	iwr.u.data.pointer = cmd_str;
	iwr.u.data.length = strlen(cmd_str);

	ret = ioctl(sock, WL_IOCTL_SETCMD, &iwr);
	close(sock);

	return ret;
}

int ioctl_getcmd(const char *ifname, char *buf, int len)
{
	struct iwreq iwr;
	int ret = 0;
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("socket() failed: %s\n", strerror(errno));
		return -1;
	}

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
	iwr.u.data.pointer = buf;
	iwr.u.data.length = len;

	ret = ioctl(sock, WL_IOCTL_GETCMD, &iwr);
	close(sock);

	return ret;

}

int ioctl_setparam(const char *ifname, int op, int arg)
{
	struct iwreq iwr;
	int ret = 0;
	int sock = -1;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		printf("socket() failed: %s\n", strerror(errno));
		return -1;
	}

	memset(&iwr, 0, sizeof(iwr));
	strncpy(iwr.ifr_name, ifname, IFNAMSIZ);
	iwr.u.mode = op;
	memcpy(iwr.u.name+sizeof(u32), &arg, sizeof(arg));

	ret = ioctl(sock, WL_IOCTL_WL_PARAM, &iwr);
	close(sock);

	return ret;
}

void get_time_ISO8061(char *time_str, int size)
{
	time_t time_utc;
	time_t time_local;
	struct tm tm_local;
	struct tm tm_gmt;
	int time_zone;

	time(&time_utc);
	localtime_r(&time_utc, &tm_local);
	time_local = mktime(&tm_local);
	gmtime_r(&time_utc, &tm_gmt);
	time_zone = tm_local.tm_hour - tm_gmt.tm_hour;

	snprintf(time_str, size, "%04d-%02d-%02dT%02d:%02d:%02d.00000%s%02d:00",
			tm_local.tm_year + 1900,
			tm_local.tm_mon + 1,
			tm_local.tm_mday,
			tm_local.tm_hour,
			tm_local.tm_min,
			tm_local.tm_sec,
			time_zone < 0 ? "" : "+",
			time_zone);
}

static const char alpha_base64_table[64] =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char *encode_base64(const u8 *buf, const u32 size)
{
    u32 a = 0;
    u32 i = 0;
    u8 int63 = 0x3F;
    u8 int255 = 0xFF;
    char *base64_array = NULL;
    u32 base64_size;

    base64_size = ( size/3 + 1 ) * 4 + 1;
    base64_array = (char *)malloc(base64_size);
    memset(base64_array, 0, base64_size);
    
    while (i < size) 
    {
        u8 b0 = buf[i++];
        u8 b1 = (i < size) ? buf[i++] : 0;
        u8 b2 = (i < size) ? buf[i++] : 0;

        base64_array[a++] = alpha_base64_table[(b0 >> 2) & int63];
        base64_array[a++] = alpha_base64_table[((b0 << 4) | ((b1 & int255) >> 4)) & int63];
        base64_array[a++] = alpha_base64_table[((b1 << 2) | ((b2 & int255) >> 6)) & int63];
        base64_array[a++] = alpha_base64_table[b2 & int63];
    }
    switch (size % 3) 
    {
        case 1:
        {
            a = a - 2;
            base64_array[a++] = '=';
            base64_array[a] = '=';
        }
        case 2:
        {
            base64_array[--a] = '=';
        }
        default:
        {
            break;
        }
    }

    return base64_array;
}

