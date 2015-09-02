#include <stdarg.h>

#include "ip_addr.h"
#include "espconn.h"

#include "syslog.h"
#include "printf.h"

static struct espconn syslog_espconn;
static struct _esp_udp syslog_espconn_udp;

/*
 * simple stupid inet_addr(3)
 *
 *  "1.2.3.4"   -> 1.2.3.4
 *  "1.2.3.4.5" -> 1.2.3.4
 *  "127.1"     -> 127.1.0.0 (XXX)
 *  "123"       -> 123.0.0.0 (XXX)
 */
void
inetaddr(uint8 ip[4], const char *cp)
{
	int i, n;
	for (i = 0; i < 4; i++) {
		for (n = 0; ((*cp >= '0') && (*cp <= '9')); cp++) {
			n = n * 10 + (*cp - '0');
		}
		ip[i] = n;
		if (*cp == '\0')
			break;
		if (*cp != '.')
			break;
		cp++;
	}
}

void
syslog_init(const char *server)
{
	sint8 rc;

	syslog_espconn.type = ESPCONN_UDP;
	syslog_espconn.state = ESPCONN_NONE;
	syslog_espconn.proto.udp = &syslog_espconn_udp;
	syslog_espconn.proto.udp->local_port = espconn_port();
	syslog_espconn.proto.udp->remote_port = 514;	/* syslog */
	inetaddr(syslog_espconn.proto.udp->remote_ip, server);
	rc = espconn_create(&syslog_espconn);
}

void
syslog_send(int facility, int priority, const char *fmt, ...)
{
	va_list ap;
	static char syslogpkt[128];
	char *p;
	sint8 rc;

	sprintf(syslogpkt, "<%d>Jan  1 00:00:00 esp8266 esp8266: ", facility + priority);
	for (p = syslogpkt; *p != '\0'; p++)
		;

	va_start(ap, fmt);
	ets_vsprintf(p, fmt, ap);
	va_end(ap);

	for (p = syslogpkt; *p != '\0'; p++)
		;

	rc = espconn_sent(&syslog_espconn, syslogpkt, p - syslogpkt);
}
