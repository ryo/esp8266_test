#include "c_types.h"
#include "osapi.h"
#include "spi_flash.h"

#include "printf.h"
#include "stdlib.h"
#include "flashenv.h"

#undef DEBUG

#define PRIV_PARAM_START_SEC	0x3c
#define PRIV_PARAM_SAVE			0
#define ENVSIZE	1024

#define FLASHENV_VERSION_STRING	"VERSION"	/* both as magic a number */
#define FLASHENV_VERSION		"2"

static uint32 __environ[ENVSIZE / sizeof(uint32)];
const char __environ_init[] =
     FLASHENV_VERSION_STRING "=" FLASHENV_VERSION "\0"
     "HOSTNAME=esp8266\0"
     "SSID=YourSSID\0"
     "SSID_PASSWD=secret\0"
     "SYSLOG=192.168.0.1\0"
     "SNTP=192.168.0.1\0"
     "TZ=9\0"
     "\0";

#ifdef DEBUG
static void
dump(char *data, unsigned int len)
{
	unsigned int i;

	for (i = 0; i < len; i++) {
		if ((i & 15) == 0)
			printf("%04x:", i);

		printf(" %02x", data[i] & 0xff);

		if ((i & 15) == 15)
			printf("\r\n");
	}
	printf("\r\n");
}
#endif

static inline char *
nextenv(char *env)
{
	if (*env == '\0')
		return NULL;

	while (*env++ != '\0')
		;
	return env;
}


static int
flashenv_load(void)
{
	static int loaded = 0;

	if (loaded)
		return 0;
	loaded = 1;

	spi_flash_read((PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
	    (uint32 *)__environ, sizeof(__environ));

	if (os_strcmp(__environ, FLASHENV_VERSION_STRING "=" FLASHENV_VERSION) != 0) {
		memset(__environ, 0, sizeof(__environ));
		memcpy(__environ, __environ_init, sizeof(__environ_init));
	}

	return 0;
}

int
flashenv_save(void)
{
	spi_flash_erase_sector(PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE);
	spi_flash_write((PRIV_PARAM_START_SEC + PRIV_PARAM_SAVE) * SPI_FLASH_SEC_SIZE,
	    (uint32 *)__environ, sizeof(__environ));

	return 0;
}

int
flashenv_printenv(void *arg)
{
	char *p;

	flashenv_load();

	for (p = (char *)__environ; p != NULL; p = nextenv(p))
		printf("%s\r\n", p);

	return 0;
}

static int
flashenv_match(char *environ, const char *name)
{
	for (;;) {
		if (*name != *environ) {
			if ((*name == '\0') && (*environ == '='))
				return 1;
			break;
		}
		name++;
		environ++;
	}
	return 0;
}

static char *
flashenv_findenv(char *environ, const char *name)
{
	char *p;

	p = environ;

	while (*p != '\0') {
		if (flashenv_match(p, name))
			return p;

		while (*p++ != '\0')
			;
	}

	return NULL;
}

static int
flashenv_deleteenv(char *environ, const char *name)
{
	char *p;
	char *n0, *n;
	unsigned int len, d;

	p = flashenv_findenv(environ, name);
	if (p == NULL)
		return -1;

	n0 = n = nextenv(p);
	while (*n != '\0')
		n = nextenv(n);
	n++;

	len = n - n0;
	memmove(p, n0, len);

	d = n0 - p;
	memset(n0 + len - d, 0, d);

	return 0;
}

static char *
flashenv_addenv(char *environ, unsigned int environ_len, const char *name, const char *value)
{
	char *p, *p0;
	unsigned int left;

	for (p = environ; *p != '\0'; p = nextenv(p))
		;

	left = p - environ;
	if ((strlen(name) + strlen(value) + 2) > left) {
		/* no space left */
		return NULL;
	}

	p0 = p;
	while ((*p++ = *name++) != '\0')
		;
	p[-1] = '=';
	while ((*p++ = *value++) != '\0')
		;
	*p = '\0';

	return p0;
}

int
flashenv_setenv(void *arg, const char *name, const char *value)
{
	flashenv_load();

	if (os_strcmp(name, FLASHENV_VERSION_STRING) == 0)
		return -1;

	flashenv_deleteenv((char *)__environ, name);

	if (value != NULL)
		flashenv_addenv((char *)__environ, sizeof(__environ), name, value);

	flashenv_save();

	return 0;
}

const char *
flashenv_getenv(const char *name)
{
	char *p;

	flashenv_load();
	p = flashenv_findenv((char *)__environ, name);
	if (p != NULL) {
		p = strchr(p, '=');
		return p + 1;
	}

	return p;
}
