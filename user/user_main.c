#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "ip_addr.h"
#include "espconn.h"
#include "user_interface.h"
#include "user_config.h"
#include "driver/uart.h"
#include "driver/gpio16.h"

#include "printf.h"
#include "fifo.h"
#include "task.h"
#include "telnet.h"
#include "syslog.h"

#undef NETDEBUG

#define MAX_CONN	4
#define MAX_TXBUFFER	1024
#define SERVER_TIMEOUT	300

static struct espconn serverConn;
static esp_tcp serverTcp;


#define MAXCLIENT	3
struct client {
	struct espconn *espconn;
	struct telnet telnet;
	struct fifo fifo_net;
	int fifo_net_sending;
} clients[MAXCLIENT];

static inline struct client *
allocate_clientinstance(void)
{
	int i;
	for (i = 0; i < MAXCLIENT; i++) {
		if (clients[i].espconn == NULL)
			return &clients[i];
	}
	return NULL;
}

static inline struct client *
espconn2clientinstance(struct espconn *espconn)
{
	int i;
	for (i = 0; i < MAXCLIENT; i++) {
		if (clients[i].espconn == espconn)
			return &clients[i];
	}
	return NULL;
}


unsigned int ledcount = 0;
os_event_t recvTaskQueue[recvTaskQueueLen];

#define MAX_UARTBUFFER (MAX_TXBUFFER/4)
static uint8 uartbuffer[MAX_UARTBUFFER];


static void
led_set(int led0, int led1, int led2, int led3)
{
	if (led0)
		gpio_output_set(BIT14, 0, BIT14, 0);
	else
		gpio_output_set(0, BIT14, BIT14, 0);

	if (led1)
		gpio_output_set(BIT12, 0, BIT12, 0);
	else
		gpio_output_set(0, BIT12, BIT12, 0);

	if (led2) {
		gpio_output_set(BIT13, 0, BIT13, 0);
		gpio_output_set(BIT4, 0, BIT4, 0);
	} else {
		gpio_output_set(0, BIT13, BIT13, 0);
		gpio_output_set(0, BIT4, 0, BIT4);
	}

	if (led3) {
		gpio16_output_set(1);
		gpio_output_set(BIT5, 0, BIT5, 0);
	} else {
		gpio16_output_set(0);
		gpio_output_set(0, BIT5, 0, BIT5);
	}
}

void
netout(void *cookie, char *buf, unsigned int len)
{
	struct client *client;
	char *p;
	unsigned int l;
	sint8 rc;

	client = (struct client *)cookie;

	switch (ledcount++ % 6) {
	case 0:
		led_set(0, 0, 0, 1);
		break;
	case 1:
		led_set(0, 0, 1, 0);
		break;
	case 2:
		led_set(0, 1, 0, 0);
		break;
	case 3:
		led_set(1, 0, 0, 0);
		break;
	case 4:
		led_set(0, 1, 0, 0);
		break;
	case 5:
		led_set(0, 0, 1, 0);
		break;
	default:
		led_set(0, 0, 0, 0);
		break;
	}

	fifo_write(&client->fifo_net, buf, len);
	if (client->fifo_net_sending == 0) {
		client->fifo_net_sending = 1;

		p = fifo_getpkt(&client->fifo_net, &l);
		rc = espconn_sent(client->espconn, p, l);
		if (rc != ESPCONN_OK) {
			espconn_disconnect(client->espconn);
		}
	}
}


static void ICACHE_FLASH_ATTR
recvTask(os_event_t *events)
{
	uint8_t i;

	while (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) {
		WRITE_PERI_REG(0X60000914, 0x73);	//WTD
		uint16 length = 0;
		while ((READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) && (length < MAX_UARTBUFFER))
			uartbuffer[length++] = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;

			/* broadcast serial-input to all telnet clients */
			for (i = 0; i < MAXCLIENT; i++) {
				if (clients[i].espconn != NULL) {
					netout(&clients[i], uartbuffer, length);
					ledcount--;
				}
			}
			ledcount++;
	}

	if(UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	} else if(UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}
	ETS_UART_INTR_ENABLE();
}

static void ICACHE_FLASH_ATTR
serverSentCb(void *arg)
{
	struct espconn *espconn;
	struct client *client;
	char *p;
	unsigned int l;
	sint8 rc;

	espconn = (struct espconn *)arg;
#ifdef NETDEBUG
	printf("%s:%d: %p\n", __func__, __LINE__, espconn);
#endif
	client = espconn2clientinstance(espconn);

	if (fifo_len(&client->fifo_net) == 0) {
		client->fifo_net_sending = 0;
	} else {
		p = fifo_getpkt(&client->fifo_net, &l);
		rc = espconn_sent(espconn, p, l);
		if (rc != ESPCONN_OK) {
			espconn_disconnect(espconn);
		}
	}
}

static void ICACHE_FLASH_ATTR
serverRecvCb(void *arg, char *data, unsigned short len)
{
	struct espconn *espconn;
	struct client *client;

	espconn = (struct espconn *)arg;
#ifdef NETDEBUG
	printf("%s:%d: %p\n", __func__, __LINE__, espconn);
#endif
	client = espconn2clientinstance(espconn);

	if (client != NULL)
		telnet_recv(&client->telnet, client, data, len);
}

static void ICACHE_FLASH_ATTR
serverConnectCb(void *arg)
{
	struct espconn *espconn;
	struct client *client;

	espconn = (struct espconn *)arg;
#ifdef NETDEBUG
	printf("%s:%d: %p\n", __func__, __LINE__, espconn);
#endif
	client = allocate_clientinstance();
	if (client == NULL) {
		espconn_disconnect(espconn);
	} else {
		memset(client, 0, sizeof(*client));

		telnet_init(&client->telnet);
		fifo_init(&client->fifo_net);
		client->fifo_net_sending = 0;
		client->espconn = espconn;

		espconn_regist_recvcb(espconn, serverRecvCb);
		espconn_regist_sentcb(espconn, serverSentCb);
	}

	syslog_send(LOG_DAEMON, LOG_INFO, "connect from %d.%d.%d.%d",
	    espconn->proto.tcp->remote_ip[0],
	    espconn->proto.tcp->remote_ip[1],
	    espconn->proto.tcp->remote_ip[2],
	    espconn->proto.tcp->remote_ip[3]);
}

static void ICACHE_FLASH_ATTR
serverDisconnectCb(void *arg)
{
	/* arg of serverDisconnectCb() is not session's espconn, arg is serverConn */
	struct espconn *espconn;
	struct client *client;
	int i;

	espconn = NULL;
	for (i = 0; i < MAXCLIENT; i++) {
		if (clients[i].espconn == NULL)
			continue;

		if (clients[i].espconn->state == ESPCONN_NONE ||
		    clients[i].espconn->state == ESPCONN_CLOSE) {
			/* found disconnected espconn */
			espconn = clients[i].espconn;

#ifdef NETDEBUG
			printf("%s:%d: %p\n", __func__, __LINE__, espconn);
#endif
			/* reap disconnected client info */
			client = espconn2clientinstance(espconn);
			if (client != NULL)
				client->espconn = NULL;
		}
	}
}

static void ICACHE_FLASH_ATTR
serverReconnectCb(void *arg, sint8 err)
{
	/* XXX */
	serverDisconnectCb(NULL);
}

void
server_init(int port)
{
	serverConn.type = ESPCONN_TCP;
	serverConn.state = ESPCONN_NONE;
	serverTcp.local_port = port;
	serverConn.proto.tcp = &serverTcp;

	espconn_regist_connectcb(&serverConn, serverConnectCb);
	espconn_regist_reconcb(&serverConn, serverReconnectCb);
	espconn_accept(&serverConn);
	espconn_regist_time(&serverConn, SERVER_TIMEOUT, 0);
	espconn_regist_disconcb(&serverConn, serverDisconnectCb);
}

void
wifi_callback(System_Event_t *evt)
{
	switch (evt->event) {
	case EVENT_STAMODE_CONNECTED:
#if 0
		printf("#connect to ssid %s, channel %d\n",
		    evt->event_info.connected.ssid,
		    evt->event_info.connected.channel);
#endif
		break;

	case EVENT_STAMODE_DISCONNECTED:
#if 0
		printf("#disconnect from ssid %s, reason %d\n",
		    evt->event_info.disconnected.ssid,
		    evt->event_info.disconnected.reason);
#endif
		deep_sleep_set_option(0);
		system_deep_sleep(30 * 1000 * 1000);	// 30 seconds
		break;

	case EVENT_STAMODE_GOT_IP:
		printf("#IP:" IPSTR ",MASK:" IPSTR ",GW:" IPSTR,
		    IP2STR(&evt->event_info.got_ip.ip),
		    IP2STR(&evt->event_info.got_ip.mask),
		    IP2STR(&evt->event_info.got_ip.gw));
		printf("\n");

		syslog_send(LOG_DAEMON, LOG_INFO, "boot address=" IPSTR ", mask=" IPSTR ", gateway=" IPSTR,
		    IP2STR(&evt->event_info.got_ip.ip),
		    IP2STR(&evt->event_info.got_ip.mask),
		    IP2STR(&evt->event_info.got_ip.gw));
		break;

	default:
		printf("#wifi_callback: unknown event: %d\n", evt->event);
		break;
	}
}

// UartDev is defined and initialized in rom code.
extern UartDevice UartDev;

char buf[16];

void
user_init(void)
{
	static struct station_config config;

	system_set_os_print(0);
	UartDev.data_bits = EIGHT_BITS;
	UartDev.parity = NONE_BITS;
	UartDev.stop_bits = ONE_STOP_BIT;
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	gpio_init();
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_GPIO13);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO4_U, FUNC_GPIO4);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
	gpio16_output_conf();

	gpio_output_set(BIT12, 0, BIT12, 0);
	gpio_output_set(BIT13, 0, BIT13, 0);
	gpio_output_set(BIT14, 0, BIT14, 0);
	gpio_output_set(BIT4, 0, BIT4, 0);
	gpio_output_set(BIT5, 0, BIT5, 0);
	gpio16_output_set(0);

	wifi_station_set_hostname("esp8266-ryo");
	wifi_set_opmode_current(STATION_MODE);

	config.bssid_set = 0;
	os_memcpy(&config.ssid, SSID, sizeof(SSID));
	os_memcpy(&config.password, SSID_PASSWD, sizeof(SSID_PASSWD));
	wifi_station_set_config(&config);
	wifi_set_event_handler_cb(wifi_callback);

	server_init(23);
	syslog_init(SYSLOG_SERVER);

	system_os_task(recvTask, recvTaskPrio, recvTaskQueue, recvTaskQueueLen);
}
