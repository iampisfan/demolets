#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

#include "httpd.h"
#include "utils.h"
#include "multi_ap.h"
#include "single_ap.h"
#include "debug.h"

struct requestData {
	httpd	*server;
	httpReq	*request;
};

static pthread_mutex_t mutex;
static enum { SINGLE_AP = 0, MULTI_AP = 1 } mode;

extern void _httpd_send404(httpd *server, httpReq *request);

static void usage(void)
{
	fprintf(stderr,"usage: [-h <host IP>] [ -p <port >]\n"
				"\t-s: single-ap, -m: multi-ap\n");
}

static void *handleRequest(void *req)
{
	struct requestData *data = (struct requestData *)req;

	httpdProcessRequest(data->server, data->request);
	httpdEndRequest(data->server, data->request);
	return NULL;
}

static void report_network(httpd *server, httpReq *request)
{
	char *resp = NULL;
	int ret = 0;
	char buf[1024];
	int offset, remain;

	pthread_mutex_lock(&mutex);

	debug("start collect_network\n");

	if (mode)
		ret = collect_de_from_easymesh(GET_DATA_ELEMENT_NETWORK, &resp);
	else
		ret = collect_de_single_ap_network(&resp);

	if (ret)
		_httpd_send404(server, request);
	else {
		offset = 0;
		remain = strlen(resp);
		while (remain > 0) {
			memset(buf, 0, sizeof(buf));
			strncpy(buf, resp + offset, sizeof(buf) - 1);
			httpdPrintf(server, request, "%s", buf);
			offset += sizeof(buf) - 1;
			remain -= sizeof(buf) - 1;
		}
	}

	if (resp)
		free(resp);

	pthread_mutex_unlock(&mutex);
	return;
}

static void report_last_event(httpd *server, httpReq *request)
{
	char *resp = NULL;
	int ret = 0;

	pthread_mutex_lock(&mutex);

	debug("start report_last_event\n");

	if (mode)
		ret = collect_de_from_easymesh(GET_DATA_ELEMENT_LAST_EVENT, &resp);
	else
		ret = collect_de_single_ap_last_event(&resp);

	if (ret)
		_httpd_send404(server, request);
	else
		httpdPrintf(server, request, "%s", resp);

	if (resp)
		free(resp);

	pthread_mutex_unlock(&mutex);
	return;
}

int main(int argc, char *argv[])
{
	httpd *server;
	httpReq *request;
	char *host = NULL;
	int port = 8888;
	pthread_t thread;
	int arg;
	struct requestData data;
	int result;

	openlog("de_agent", 0, LOG_DAEMON);

	mode = MULTI_AP;
	extern char *optarg;
	while ((arg = getopt(argc, argv, "h:p:sm")) != -1) {
		switch (arg) {
		case 'h':
			host = optarg;
			break;

		case 'p':
			port = atoi(optarg);
			break;

		case 's':
			mode = SINGLE_AP;
			break;

		case 'm':
			mode = MULTI_AP;
			break;

		default:
			usage();
			exit(1);
		}
	}

	pthread_mutex_init(&mutex, NULL);

	server = httpdCreate(NULL, port);
	if (NULL == server) {
		perror("cannot create http server\n");
		return -1;
	}
	httpdSetAccessLog(server, stdout);
	httpdSetErrorLog(server, stderr);

	httpdAddCContent(server, "/wifi", "wfa-dataelements:Network", HTTP_TRUE, 
			NULL, report_network);
	httpdAddCContent(server, "/wifi", "wfa-dataelements:Last-Event", HTTP_TRUE, 
			NULL, report_last_event);

	daemon(1, 1);

	if (SINGLE_AP == mode)
		setup_de_single_ap();

	while(1) {
		request = httpdReadRequest(server, NULL, &result);
		if (NULL == request && 0 == result)
			continue;

		if (result < 0)
			continue;

		data.server = server;
		data.request = request;
		pthread_create(&thread, NULL, handleRequest, (void *)&data);
	}

	return 0;
}

