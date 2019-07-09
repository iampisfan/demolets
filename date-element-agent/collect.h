#ifndef _COLLECT_H_
#define _COLLECT_H_

#include "data_element.h"

int collect_network(struct data_element_container *parent);
int collect_device(struct data_element_container *parent);
int collect_radio(struct data_element_container *parent);
int collect_bss(struct data_element_container *parent);
int collect_capabilities(struct data_element_container *parent, void *arg, int band);
int collect_sta(struct data_element_container *parent, unsigned int bss_ifindex);
int collect_current_opclass(struct data_element_container *parent, void *arg);
int collect_capable_opclass(struct data_element_container *parent);
int collect_scan_result(struct data_element_container *parent, u32 radio_index);

int update_network(struct data_element_container *parent);
int update_device(struct data_element_container *parent);
int update_radio(struct data_element_container *parent);
int update_bss(struct data_element_container *parent);

#endif
