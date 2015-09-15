#include "ets_sys.h"
#include "user_interface.h"

#include "printf.h"
#include "cmdline.h"
#include "flashenv.h"

static int command_d(void *, int, char *[]);
static int command_setenv(void *, int, char *[]);
static int command_printenv(void *, int, char *[]);
static int command_reboot(void *, int, char *[]);
static int command_help(void *, int, char *[]);


struct cmdtbl cmdtable[] = {
	{	"d",		command_d	},
	{	"setenv",	command_setenv	},
	{	"printenv",	command_printenv},

	{	"reboot",	command_reboot	},

	{	"help",		command_help	},
	{	"?",		command_help	},

	{	NULL,	NULL		}
};

static int
command_reboot(void *arg, int argc, char *argv[])
{
	system_restart();
	return 0;
}


static int
command_d(void *arg, int argc, char *argv[])
{
	unsigned char *address;
	unsigned int size, i;

	if (argc != 3) {
		printf("usage: d <address> <size>\r\n");
		return 0;
	}
	address = (unsigned char *)strtol(argv[1], NULL, 16);
	size = strtol(argv[2], NULL, 16);

	for (i = 0; i < size; i++) {
		if ((i & 15) == 0)
			printf("0x%p", address);

		printf(" %02x", *address);
		address++;

		if ((i & 15) == 15)
			printf("\r\n");
	}
	printf("\r\n");

	return 0;
}

static int
command_setenv(void *arg, int argc, char *argv[])
{
	if (argc == 1) {
		flashenv_printenv(arg);
	} else if (argc == 2) {
		flashenv_setenv(arg, argv[1], NULL);
	} else if (argc == 3) {
		flashenv_setenv(arg, argv[1], argv[2]);
	} else {
		printf(
		    "usage: setenv <varname> <value>\r\n"
		    "    set environment variable <name> to <value>\r\n"
		    "usage: setenv <varname>\r\n"
		    "    delete environment variable <name>\r\n"
		);
	}
	return 0;
}

static int
command_printenv(void *arg, int argc, char *argv[])
{
	flashenv_printenv(arg);
	return 0;
}

static int
command_help(void *arg, int argc, char *argv[])
{
	printf("d		dump memory\r\n"
	       "\r\n"
	       "setenv		set environment variable\r\n"
	       "printenv	show environment variables\r\n"
	       "\r\n"
	       "reboot		restart system\r\n"
	);
	return 0;
}

