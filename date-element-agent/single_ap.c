#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "collect.h"
#include "event.h"

struct data_element_container de_network;
struct last_event de_last_event;
u32 start_time;

void setup_de_single_ap()
{
	time_t time_t;
	struct tm *tm;
	time(&time_t);
	tm = gmtime(&time_t);
	start_time = tm->tm_hour*60*60 + tm->tm_min*60 + tm->tm_sec;

	memset(&de_network, 0, sizeof(struct data_element_container));
	nl80211_init_nl_global();
	nl80211_setup_event_monitor();
	run_event_uloop();

	collect_network(&de_network);
	setup_yang_ctx(0);
}

int collect_de_single_ap_network(char **resp)
{
	*resp = calloc(1, 256*1024);
	char *json = NULL;
	int ret = 0;
	struct lyd_node *yang_network;

	yang_network = lyd_new(NULL, yang_ctx->mod, YANG_ROOT_NETWORK);

	update_network(&de_network);
	flush_yang(yang_network, de_network.element, de_network.num);
	lyd_schema_sort(yang_network, 1);
	lyd_print_mem(&json, yang_network, LYD_JSON, LYP_FORMAT);

	if (json) {
		strncpy(*resp, json, 256*1024);
		free(json);
	} else
		ret = -1;

	lyd_free(yang_network);
	yang_network = NULL;

	return ret;
}

int collect_de_single_ap_last_event(char **resp)
{
	*resp = calloc(1, 256*1024);
	char *json = NULL;
	int ret = 0;
	struct lyd_node *yang_last_event;

	if (de_last_event.id == ASSOC_EVENT) {
		yang_last_event = lyd_new(NULL, yang_ctx->mod, YANG_ROOT_ASSOC);
		flush_yang(yang_last_event, de_last_event.assoc->element,
					de_last_event.assoc->num);
	} else if (de_last_event.id == DISASSOC_EVENT) {
		yang_last_event = lyd_new(NULL, yang_ctx->mod, YANG_ROOT_DISASSOC);
		flush_yang(yang_last_event, de_last_event.disassoc->element,
					de_last_event.disassoc->num);
	} else
		return -1;

	lyd_schema_sort(yang_last_event, 1);
	lyd_print_mem(&json, yang_last_event, LYD_JSON, LYP_FORMAT);

	if (json) {
		strncpy(*resp, json, 256*1024);
		free(json);
	} else
		ret = -1;

	lyd_free(yang_last_event);
	yang_last_event = NULL;

	return ret;
}
