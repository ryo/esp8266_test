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
#undef PRINTFDEBUG

#define USE_SNTP
#define USE_SYSLOG
#define USE_TIMER
#define USE_LED

#ifndef USE_SYSLOG
#define syslog_init(x, y)
#define syslog_send(...)
#endif

#define MAX_TXBUFFER	1024
#define SERVER_TIMEOUT	300

static struct espconn serverConn;
static esp_tcp serverTcp;

#define	HZ	10
static os_timer_t tick_timer;

uint8 wifi_connected;
struct cmdline cmdline;

os_event_t recvTaskQueue[recvTaskQueueLen];

#define MAX_UARTBUFFER (MAX_TXBUFFER/4)
static uint8 uartbuffer[MAX_UARTBUFFER];


ICACHE_FLASH_ATTR
static void
photorelay_ctrl(int sw, int onoff)
{
	if (sw) {
		if (onoff)
			gpio_output_set(BIT4, 0, BIT4, 0);
		else
			gpio_output_set(0, BIT4, 0, BIT4);
	} else {
		if (onoff)
			gpio_output_set(BIT5, 0, BIT5, 0);
		else
			gpio_output_set(0, BIT5, 0, BIT5);
	}
}

ICACHE_FLASH_ATTR
static void
led_set(int led0, int led1, int led2)
{
#ifdef USE_LED
	if (led0)
		gpio_output_set(BIT14, 0, BIT14, 0);
	else
		gpio_output_set(0, BIT14, BIT14, 0);

	if (led1)
		gpio_output_set(BIT12, 0, BIT12, 0);
	else
		gpio_output_set(0, BIT12, BIT12, 0);

	if (led2)
		gpio_output_set(BIT13, 0, BIT13, 0);
	else
		gpio_output_set(0, BIT13, BIT13, 0);
#endif
}

static int ledmode;
#define LEDMODE_OFF	0
#define LEDMODE_ON	1
#define LEDMODE_BLINK	2
#define LEDMODE_KITT	3
#define LEDMODE_KITT2	4
#define LEDMODE_WIFIERR	5
static unsigned int ledcount;

ICACHE_FLASH_ATTR
void
led_mode(int mode)
{
#ifdef USE_LED
	if (ledmode != mode) {
		ledmode = mode;
		switch (ledmode) {
		case LEDMODE_OFF:
			led_set(1, 1, 1);
			break;
		case LEDMODE_ON:
			led_set(1, 1, 1);
			break;
		case LEDMODE_BLINK:
		case LEDMODE_KITT:
		case LEDMODE_KITT2:
		case LEDMODE_WIFIERR:
			break;
		}
	}
#endif
}

ICACHE_FLASH_ATTR
void
led_update(void)
{
#ifdef USE_LED
	switch (ledmode) {
	case LEDMODE_OFF:
	case LEDMODE_ON:
		break;

	case LEDMODE_KITT:
		if (++ledcount >= 4)
			ledcount = 0;

		switch (ledcount) {
		case 0:
			led_set(0, 0, 1);
			break;
		case 1:
			led_set(0, 1, 0);
			break;
		case 2:
			led_set(1, 0, 0);
			break;
		case 3:
			led_set(0, 1, 0);
			break;
		default:
			led_set(0, 0, 0);
			break;
		}
		break;

	case LEDMODE_KITT2:
		if (++ledcount >= 4)
			ledcount = 0;

		switch (ledcount) {
		case 0:
			led_set(1, 1, 0);
			break;
		case 1:
			led_set(1, 0, 1);
			break;
		case 2:
			led_set(0, 1, 1);
			break;
		case 3:
			led_set(1, 0, 1);
			break;
		default:
			led_set(1, 1, 1);
			break;
		}
		break;

	case LEDMODE_BLINK:
		if (++ledcount >= 4)
			ledcount = 0;

		switch (ledcount / 2) {
		case 0:
			led_set(1, 1, 1);
			break;
		case 1:
		default:
			led_set(0, 0, 0);
			break;
		}
		break;

	case LEDMODE_WIFIERR:
		ledcount++;
		if (ledcount < HZ / 2) {
			led_set(1, 0, 1);
		} else if (ledcount < HZ) {
			led_set(0, 1, 0);
		} else {
			ledcount = 0;
		}
		break;
	}
#endif
}

ICACHE_FLASH_ATTR
void
timerhandler(void *arg)
{
#ifdef USE_LED
	led_update();
#endif
}

ICACHE_FLASH_ATTR
static void
cmdlineputc(void *arg, char ch)
{
	if (arg == NULL) {
		putchar(ch);
	} else {
		netout((struct netclient *)arg, &ch, 1);
		netout_flush((struct netclient *)arg);
	}
}

ICACHE_FLASH_ATTR
static void
recvTask(os_event_t *events)
{
	uint8_t i;

	while (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) {
		uint16 length;

		WRITE_PERI_REG(0x60000914, 0x73);	/* touch watchdog timer against below busy loop */
		for (length = 0;
		    (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) &&
		    (length < MAX_UARTBUFFER); length++) {
			uartbuffer[length] = READ_PERI_REG(UART_FIFO(UART0)) & 0xff;
		}

		if (!wifi_connected) {
			/* serial control UI */
			for (i = 0; i < length; i++) {
				commandline(&cmdline, uartbuffer[i], cmdtable, NULL);
			}

		} else {
			netout_broadcast_byport(uartbuffer, length, 23);
		}

//		led_update();
	}


	if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	} else if (UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}
	ETS_UART_INTR_ENABLE();
}

ICACHE_FLASH_ATTR
static void
serverSentCb(void *arg)
{
	struct espconn *espconn;
	struct netclient *netclient;
	char *p;
	unsigned int l;
	sint8 rc;

	espconn = (struct espconn *)arg;
#ifdef NETDEBUG
	printf("%s:%d: %p\n", __func__, __LINE__, espconn);
#endif
	netclient = netclient_lookup_by_espconn(espconn);

	if (fifo_len(&netclient->fifo_net) == 0) {
		netclient->fifo_net_sending = 0;
	} else {
		p = fifo_getbulk(&netclient->fifo_net, &l);
		rc = espconn_sent(espconn, p, l);
		if (rc != ESPCONN_OK) {
			syslog_send(LOG_DAEMON|LOG_DEBUG, "telnet: espconn_sent(continuous): error %d", rc);
			espconn_disconnect(espconn);
		}
	}

#if 0
	{
		struct espconn_packet esppkt;
		espconn_get_packet_info(espconn, &esppkt);
		syslog_send(LOG_DAEMON|LOG_DEBUG, "telnet:"
		    " sent_length=%d"
		    " snd_buf_size=%d"
		    " snd_queuelen=%d"
		    " total_queuelen=%d"
		    " packseqno=%u"
		    " packseq_nxt=%u"
		    " packnum=%u",
		    esppkt.sent_length,
		    esppkt.snd_buf_size,
		    esppkt.snd_queuelen,
		    esppkt.total_queuelen,
		    esppkt.packseqno,
		    esppkt.packseq_nxt,
		    esppkt.packnum);
	}
#endif
}

ICACHE_FLASH_ATTR
static void
serverRecvCb(void *arg, char *data, unsigned short len)
{
	struct espconn *espconn;
	struct netclient *netclient;

	espconn = (struct espconn *)arg;
#ifdef NETDEBUG
	printf("%s:%d: %p\n", __func__, __LINE__, espconn);
#endif
	netclient = netclient_lookup_by_espconn(espconn);
	if (netclient != NULL)
		telnet_recv(&netclient->telnet, netclient, data, len);
}

ICACHE_FLASH_ATTR
static void
serverDisconnectCb(void *arg)
{
	/* arg of serverDisconnectCb() is not session's espconn, arg is serverConn */
	netclient_espconn_reaper();
}

ICACHE_FLASH_ATTR
static void
serverReconnectCb(void *arg, sint8 err)
{
	/* arg of serverReconnectCb() is not session's espconn */
	syslog_send(LOG_DAEMON|LOG_ERR, "telnet: reconnect");
}

ICACHE_FLASH_ATTR
static void
serverConnectCb(void *arg)
{
	struct espconn *espconn;
	struct netclient *netclient;
	static int once = 0;

	espconn = (struct espconn *)arg;
#ifdef NETDEBUG
	printf("%s:%d: %p\n", __func__, __LINE__, espconn);
#endif

	netclient = netclient_connect_espconn(espconn);
	if (netclient == NULL) {
		espconn_disconnect(espconn);
	} else {
		espconn_regist_recvcb(espconn, serverRecvCb);
		espconn_regist_sentcb(espconn, serverSentCb);
		espconn_regist_reconcb(espconn, serverReconnectCb);
		espconn_regist_disconcb(espconn, serverDisconnectCb);

		syslog_send(LOG_DAEMON|LOG_INFO, "telnet: connection from %d.%d.%d.%d:%d",
		    espconn->proto.tcp->remote_ip[0],
		    espconn->proto.tcp->remote_ip[1],
		    espconn->proto.tcp->remote_ip[2],
		    espconn->proto.tcp->remote_ip[3],
		    espconn->proto.tcp->remote_port);
		syslog_send(LOG_DAEMON|LOG_DEBUG, "debug: heap=%d", system_get_free_heap_size());
	}
}

ICACHE_FLASH_ATTR
const char *
strflashsize(enum flash_size_map size)
{
	switch (size) {
	case FLASH_SIZE_4M_MAP_256_256:
		return "4M/256/256";
	case FLASH_SIZE_2M:
		return "2M";
	case FLASH_SIZE_8M_MAP_512_512:
		return "8M/512/512";
	case FLASH_SIZE_16M_MAP_512_512:
		return "16M/512/512";
	case FLASH_SIZE_32M_MAP_512_512:
		return "32M/512/512";
	case FLASH_SIZE_16M_MAP_1024_1024:
		return "16M/1024/1024";
	case FLASH_SIZE_32M_MAP_1024_1024:
		return "32M/1024/1024";
	default:
		return "unknown";
	}
}

ICACHE_FLASH_ATTR
void
server_init(int port)
{
	serverConn.type = ESPCONN_TCP;
	serverConn.state = ESPCONN_NONE;
	serverTcp.local_port = port;
	serverConn.proto.tcp = &serverTcp;

	espconn_regist_connectcb(&serverConn, serverConnectCb);
	espconn_accept(&serverConn);
	espconn_regist_time(&serverConn, SERVER_TIMEOUT, 0);
}

ICACHE_FLASH_ATTR
void
wifi_callback(System_Event_t *evt)
{
	switch (evt->event) {
	case EVENT_STAMODE_CONNECTED:
#ifdef PRINTFDEBUG
		printf("#connect to ssid %s, channel %d\n",
		    evt->event_info.connected.ssid,
		    evt->event_info.connected.channel);
#endif
		break;

	case EVENT_STAMODE_DISCONNECTED:
		wifi_connected = 0;
		led_mode(LEDMODE_WIFIERR);
#ifdef PRINTFDEBUG
		printf("#disconnect from ssid %s, reason %d\n",
		    evt->event_info.disconnected.ssid,
		    evt->event_info.disconnected.reason);
#endif
		break;

	case EVENT_STAMODE_GOT_IP:
		wifi_connected = 1;
		led_mode(LEDMODE_KITT);
#ifdef PRINTFDEBUG
		printf("#IP:" IPSTR ",MASK:" IPSTR ",GW:" IPSTR "\n",
		    IP2STR(&evt->event_info.got_ip.ip),
		    IP2STR(&evt->event_info.got_ip.mask),
		    IP2STR(&evt->event_info.got_ip.gw));
#endif
		syslog_send(LOG_DAEMON|LOG_INFO, "system: address=" IPSTR ", mask=" IPSTR ", gateway=" IPSTR,
		    IP2STR(&evt->event_info.got_ip.ip),
		    IP2STR(&evt->event_info.got_ip.mask),
		    IP2STR(&evt->event_info.got_ip.gw));


		/* syslog reset info (for debug) */
		{
			struct rst_info *rstinfo;
			char *reasonstr;

			rstinfo = system_get_rst_info();

			switch (rstinfo->reason) {
			case REASON_DEFAULT_RST:	reasonstr = "REASON_DEFAULT_RST";
			case REASON_WDT_RST:		reasonstr = "REASON_WDT_RST";
			case REASON_EXCEPTION_RST:	reasonstr = "REASON_EXCEPTION_RST";
			case REASON_SOFT_WDT_RST:	reasonstr = "REASON_SOFT_WDT_RST";
			case REASON_SOFT_RESTART:	reasonstr = "REASON_SOFT_RESTART";
			case REASON_DEEP_SLEEP_AWAKE:	reasonstr = "REASON_DEEP_SLEEP_AWAKE";
			case REASON_EXT_SYS_RST:	reasonstr = "REASON_EXT_SYS_RST";
			default:			reasonstr = "???";
			}

			syslog_send(LOG_DAEMON|LOG_INFO, "system: reset info reason=%s, exccause=0x%x, epc1=0x%x, epc2=0x%x, epc3=0x%x, excvaddr=0x%x, depc=0x%x",
			    reasonstr,
			    rstinfo->exccause,
			    rstinfo->epc1,
			    rstinfo->epc2,
			    rstinfo->epc3,
			    rstinfo->excvaddr,
			    rstinfo->depc);
		}

#ifdef PRINTFDEBUG
		printf("#boot: Mhz=%d, chip_id=0x%08x, ver=%d, flash=%d, heap=%d",
		    system_get_cpu_freq(),
		    system_get_chip_id(),
		    system_get_boot_version(),
		    system_get_flash_size_map(),
		    system_get_free_heap_size());
#endif

		syslog_send(LOG_DAEMON|LOG_INFO, "system: cpu-freq=%dMHz chip-id=0x%08x, boot-version=%d, flash-size-map=%d(%s)",
		    system_get_cpu_freq(),
		    system_get_chip_id(),
		    system_get_boot_version(),
		    system_get_flash_size_map(),
		    strflashsize(system_get_flash_size_map()));

		syslog_send(LOG_DAEMON|LOG_INFO, "system: sdk-version=%s, heap=%d",
		    system_get_sdk_version(),
		    system_get_free_heap_size());

		break;

	default:
		if (evt == NULL) {
#ifdef PRINTFDEBUG
			printf("#wifi_callback: unknown event: evt\n");
#endif
			syslog_send(LOG_DAEMON|LOG_DEBUG, "debug: wifi_callback: unknown event");
		} else {
#ifdef PRINTFDEBUG
			printf("#wifi_callback: unknown event: %d\n", evt->event);
#endif
			syslog_send(LOG_DAEMON|LOG_DEBUG, "debug: wifi_callback: unknown event: %d", evt->event);
		}
		break;
	}
}

ICACHE_FLASH_ATTR
void
gpio_setup()
{
	gpio_init();

#ifdef USE_LED
	/* for LED */
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	gpio_output_set(BIT12, 0, BIT12, 0);
	gpio_output_set(BIT13, 0, BIT13, 0);
	gpio_output_set(BIT14, 0, BIT14, 0);
#endif

	/* for PhotoRelay Switch */
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	gpio_output_set(BIT4, 0, BIT4, 0);
	gpio_output_set(BIT5, 0, BIT5, 0);
}

ICACHE_FLASH_ATTR
void
sntp_setup(void)
{
#ifdef USE_SNTP
	int tz;
	const char *p;

	p = flashenv_getenv("TZ");
	if (p != NULL) {
		tz = strtol(p, NULL, 10);
	} else {
		tz = 0;
	}
	sntp_set_timezone(tz);

	if ((p = flashenv_getenv("SNTP")) != NULL)
		sntp_setservername(0, p);

	sntp_init();
#endif /* USE_SNTP */
}

ICACHE_FLASH_ATTR
void
user_init(void)
{
	const char *hostname, *p;

	/* UartDev is defined and initialized in rom code. */
	extern UartDevice UartDev;
	static struct station_config config;

	system_update_cpu_freq(SYS_CPU_160MHZ);

	system_set_os_print(0);
	UartDev.data_bits = EIGHT_BITS;
	UartDev.parity = NONE_BITS;
	UartDev.stop_bits = ONE_STOP_BIT;
	uart_init(BIT_RATE_115200, BIT_RATE_115200);


	/* GPIO, LED */
	gpio_setup();
#ifdef USE_LED
	led_set(1, 1, 1);
	led_mode(LEDMODE_BLINK);
#endif


	/* WiFi configuration */
	if ((hostname = flashenv_getenv("HOSTNAME")) == NULL)
		hostname = "esp8266";

	wifi_station_set_hostname(__UNCONST(hostname));
	wifi_set_opmode_current(STATION_MODE);

	config.bssid_set = 0;
	p = flashenv_getenv("SSID");
	if (p != NULL)
		os_memcpy(&config.ssid, p, strlen(p));
	p = flashenv_getenv("SSID_PASSWD");
	if (p != NULL)
	os_memcpy(&config.password, p, strlen(p));
	wifi_station_set_config(&config);
	wifi_set_event_handler_cb(wifi_callback);


	/* for CUI (serial) */
	cmdline_init(&cmdline, cmdlineputc, NULL);
	cmdline_setprompt(&cmdline, "ESP8266>");
	cmdline_reset(&cmdline);


	/* network */
	server_init(23);
	if ((p = flashenv_getenv("SYSLOG")) != NULL)
		syslog_init(p, hostname);
	sntp_setup();


#ifdef USE_TIMER
	/* timer for LED */
	os_timer_disarm(&tick_timer);
	os_timer_setfn(&tick_timer, timerhandler, NULL);
	os_timer_arm(&tick_timer, 1000 / HZ, 1);
#endif


	/* triggered from uart0_rx_intr_handler */
	system_os_task(recvTask, recvTaskPrio, recvTaskQueue, recvTaskQueueLen);
}
