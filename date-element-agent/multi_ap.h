#ifndef _MULTI_AP_H_
#define _MULTI_AP_H_

#include "type.h"

struct message {
    u8 alid[6];
    u8 macaddr[6];
    unsigned int messagetype;
};

#define GET_DATA_ELEMENT_NETWORK    0xF000
#define GET_DATA_ELEMENT_LAST_EVENT 0xF001
#define CONTROLLER_SOCKET_PATH "/var/run/demo.sock"
#define TIMEOUT 5000

int collect_de_from_easymesh(unsigned int msgtype, char **resp);

#endif
