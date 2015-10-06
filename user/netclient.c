#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "driver/uart.h"

#include "printf.h"
#include "fifo.h"
#include "task.h"
#include "telnet.h"
#include "syslog.h"
#include "cmdline.h"
#include "command.h"
#include "flashenv.h"
#include "stdlib.h"
#include "netclient.h"

#undef NETDEBUG

struct netclient netclients[MAXCLIENT];

static void netclient_init_espconn(struct netclient *, struct espconn *);
static struct netclient *netclient_alloc_instance(void);

ICACHE_FLASH_ATTR
static void
netclient_init_espconn(struct netclient *netclient, struct espconn *espconn)
{
	memset(netclient, 0, sizeof(*netclient));

	telnet_init(&netclient->telnet);
	fifo_init(&netclient->fifo_net);
	netclient->fifo_net_sending = 0;
	netclient->espconn = espconn;

	/* copy tcp info from espconn */
	memcpy(netclient->remote_ip, espconn->proto.tcp->remote_ip, sizeof(netclient->remote_ip));
	netclient->remote_port = espconn->proto.tcp->remote_port;
	netclient->local_port = espconn->proto.tcp->local_port;
}

ICACHE_FLASH_ATTR
static struct netclient *
netclient_alloc_instance(void)
{
	int i;
	for (i = 0; i < MAXCLIENT; i++) {
		if (netclients[i].espconn == NULL)
			return &netclients[i];
	}
	return NULL;
}

ICACHE_FLASH_ATTR
struct netclient *
netclient_connect_espconn(struct espconn *espconn)
{
	struct netclient *netclient;
	netclient = netclient_alloc_instance();
	if (netclient != NULL) {
		netclient_init_espconn(netclient, espconn);
	}
	return netclient;
}

ICACHE_FLASH_ATTR struct netclient *
netclient_lookup_by_espconn(struct espconn *espconn)
{
	int i;
	for (i = 0; i < MAXCLIENT; i++) {
		if (netclients[i].espconn == espconn)
			return &netclients[i];
	}
	return NULL;
}

ICACHE_FLASH_ATTR void
netout(struct netclient *netclient, char *buf, unsigned int len)
{
	fifo_write(&netclient->fifo_net, buf, len);
}

ICACHE_FLASH_ATTR void
netout_flush(struct netclient *netclient)
{
	char *p;
	unsigned int l;
	sint8 rc;

	if (fifo_len(&netclient->fifo_net) == 0)
		return;

	if (netclient->fifo_net_sending == 0) {
		netclient->fifo_net_sending = 1;

		p = fifo_getbulk(&netclient->fifo_net, &l);
		rc = espconn_sent(netclient->espconn, p, l);
		if (rc != ESPCONN_OK) {
			syslog_send(LOG_DAEMON|LOG_DEBUG, "telnet: espconn_sent: error %d", rc);
			espconn_disconnect(netclient->espconn);
		}
	}
}

ICACHE_FLASH_ATTR void
netout_broadcast_byport(char *buf, unsigned int len, uint16 serverport)
{
	int i;

	/* broadcast serial data to all telnet clients */
	for (i = 0; i < MAXCLIENT; i++) {
		if ((netclients[i].espconn != NULL) &&
		    (netclients[i].local_port == serverport)) {
			netout(&netclients[i], buf, len);
			netout_flush(&netclients[i]);
		}
	}
}

ICACHE_FLASH_ATTR void
netclient_espconn_reaper(void)
{
	struct espconn *espconn;
	struct netclient *netclient;
	int i;

	for (i = 0; i < MAXCLIENT; i++) {
		if (netclients[i].espconn == NULL)
			continue;

		if (netclients[i].espconn->state == ESPCONN_NONE ||
		    netclients[i].espconn->state == ESPCONN_CLOSE) {
			/* found disconnected espconn */
			espconn = netclients[i].espconn;

#ifdef NETDEBUG
			printf("%s:%d: %p\n", __func__, __LINE__, espconn);
#endif
			/* reap disconnected client info */
			netclient = netclient_lookup_by_espconn(espconn);
			if (netclient != NULL) {
				syslog_send(LOG_DAEMON|LOG_INFO, "telnet: disconnect from %d.%d.%d.%d:%d",
				    netclient->remote_ip[0],
				    netclient->remote_ip[1],
				    netclient->remote_ip[2],
				    netclient->remote_ip[3],
				    netclient->remote_port);
				netclient->espconn = NULL;
			}
		}
	}
}
