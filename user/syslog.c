#include <stdarg.h>

#include "ip_addr.h"
#include "espconn.h"
#include "c_types.h"
#include "osapi.h"
#include "sntp.h"

#include "syslog.h"
#include "printf.h"

static struct espconn syslog_espconn;
static struct _esp_udp syslog_espconn_udp;
static const char *syslog_hostname;

void ICACHE_FLASH_ATTR
syslog_init(const char *server, const char *hostname)
{
	sint8 rc;

	syslog_hostname = hostname;

	syslog_espconn.type = ESPCONN_UDP;
	syslog_espconn.state = ESPCONN_NONE;
	syslog_espconn.proto.udp = &syslog_espconn_udp;
	syslog_espconn.proto.udp->local_port = espconn_port();
	syslog_espconn.proto.udp->remote_port = 514;	/* syslog */
	ipaddr_aton(server, syslog_espconn.proto.udp->remote_ip);
	rc = espconn_create(&syslog_espconn);
}

void ICACHE_FLASH_ATTR
syslog_send(int fac_pri, const char *fmt, ...)
{
	va_list ap;
	static char syslogpkt[128];
	static char timestr[64];
	const char *hostname;
	char *p;
	sint8 rc;

	if ((hostname = syslog_hostname) == NULL)
		hostname = "esp8266";

	os_strcpy(timestr, sntp_get_real_time(sntp_get_current_timestamp()));

	/*
	 * "Thu Sep 03 03:33:33 2015\n"
	 *  0123456789012345678901234
	 */
	if (timestr[8] == '0')
		timestr[8] = ' ';
	timestr[19] = '\0';

	sprintf(syslogpkt, "<%d>%s %s ",
	    fac_pri, &timestr[4], hostname);

	for (p = syslogpkt; *p != '\0'; p++)
		;

	va_start(ap, fmt);
	ets_vsprintf(p, fmt, ap);
	va_end(ap);

	for (p = syslogpkt; *p != '\0'; p++)
		;

	rc = espconn_sent(&syslog_espconn, syslogpkt, p - syslogpkt);
}
