#ifndef  _NETCLIENT_H_
#define  _NETCLIENT_H_

#include "telnet.h"
#include "fifo.h"

#define MAXCLIENT	3

struct netclient {
	struct espconn *espconn;
	struct telnet telnet;
	struct fifo fifo_net;
	int fifo_net_sending;
	uint8 remote_ip[4];
	uint16 local_port;
	uint16 remote_port;
};


struct netclient *netclient_connect_espconn(struct espconn *);
void netout(struct netclient *, char *, unsigned int);
void netout_flush(struct netclient *);
struct netclient *netclient_lookup_by_espconn(struct espconn *);

void netout_broadcast_byport(char *, unsigned int, uint16);
void netclient_espconn_reaper(void);

#endif /* _NETCLIENT_H_ */
