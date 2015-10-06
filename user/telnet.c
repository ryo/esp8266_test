/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#undef TELNET_DEBUG

#ifdef TELNET_DEBUG
#include "printf.h"
#define TELCMDS
#define TELOPTS
#endif
#include "telnet.h"

#include "c_types.h"

#include "netclient.h"


static void send_do(void *, int, int);
static void send_dont(void *, int, int);
static void send_will(void *, int, int);
static void send_wont(void *, int, int);

#define TS_DATA		0	/* base state */
#define TS_IAC		1	/* look for double IAC's */
#define TS_CR		2	/* CR-LF ->'s CR */
#define TS_SB		3	/* throw away begin's... */
#define TS_SE		4	/* ...end's (suboption negotiation) */
#define TS_WILL		5	/* will option negotiation */
#define TS_WONT		6	/* wont " */
#define TS_DO		7	/* do " */
#define TS_DONT		8	/* dont " */

//static int telopt_sent = 0;
//static int telnet_state = TS_DATA;


ICACHE_FLASH_ATTR
void
telnet_init(struct telnet *telnet)
{
	telnet->telopt_sent = 0;
	telnet->telnet_state = TS_DATA;
}

ICACHE_FLASH_ATTR
static void
telnet_init_once(struct telnet *telnet ,void *cookie)
{
	if (telnet->telopt_sent == 0) {
		send_will(cookie, TELOPT_BINARY, 1);
		send_will(cookie, TELOPT_ECHO, 1);
		netout_flush((struct netclient *)cookie);
		telnet->telopt_sent = 1;
	}
}

ICACHE_FLASH_ATTR
static void
send_do(void *cookie, int option, int init)
{
	char buf[3] = { IAC, DO };
	buf[2] = option;
	netout((struct netclient *)cookie, buf, 3);

#ifdef TELNET_DEBUG
	if (TELOPT_OK(option)) {
		printf("> SEND DO %d=%s\n", option, TELOPT(option));
	} else {
		printf("> SEND DO  %d\n", option);
	}
#endif
}

ICACHE_FLASH_ATTR
static void
send_dont(void *cookie, int option, int init)
{
	char buf[3] = { IAC, DONT };
	buf[2] = option;
	netout((struct netclient *)cookie, buf, 3);

#ifdef TELNET_DEBUG
	if (TELOPT_OK(option)) {
		printf("> SEND DONT %d=%s\n", option, TELOPT(option));
	} else {
		printf("> SEND DONT %d\n", option);
	}
#endif
}

ICACHE_FLASH_ATTR
static void
send_will(void *cookie, int option, int init)
{
	char buf[3] = { IAC, WILL };
	buf[2] = option;
	netout((struct netclient *)cookie, buf, 3);

#ifdef TELNET_DEBUG
	if (TELOPT_OK(option)) {
		printf("> SEND WILL %d=%s\n", option, TELOPT(option));
	} else {
		printf("> SEND WILL %d\n", option);
	}
#endif
}

ICACHE_FLASH_ATTR
static void
send_wont(void *cookie, int option, int init)
{
	char buf[3] = { IAC, WONT };
	buf[2] = option;
	netout((struct netclient *)cookie, buf, 3);

#ifdef TELNET_DEBUG
	if (TELOPT_OK(option)) {
		printf("> SEND WONT %d=%s\n", option, TELOPT(option));
	} else {
		printf("> SEND WONT %d\n", option);
	}
#endif
}

ICACHE_FLASH_ATTR
static void
interrupt(void)
{
	/* nothong */
}

ICACHE_FLASH_ATTR
static void
sendbrk(void)
{
	/* nothong */
}

ICACHE_FLASH_ATTR
static void
recv_ayt(void *cookie)
{
	netout((struct netclient *)cookie, "\r\n[Yes]\r\n", 9);
}

ICACHE_FLASH_ATTR
static void
abort_output(void)
{
	/* nothong */
}

ICACHE_FLASH_ATTR
static void
erase_character(void)
{
	/* as 0x7f? */
	/* nothong */
}

ICACHE_FLASH_ATTR
static void
erase_line(void)
{
	/* nothing */
}

ICACHE_FLASH_ATTR
static void
urgent_data(void)
{
	/* nothong */
}

#ifdef TELNET_DEBUG
const char *strstate[] = {
	"DATA", "IAC", "CR", "SB", "SE", "WILL", "WONT", "DO", "DONT", 0
};
#endif

#define	TELNET_INPUT(c)		putchar(c)

ICACHE_FLASH_ATTR
void
telnet_recv(struct telnet *telnet, void *cookie, char *buf, unsigned int len)
{
	int c;

	while (len > 0) {
		c = *buf++ & 0xff;
		len--;

#ifdef TELNET_DEBUG
		if (TELCMD_OK(c)) {
			printf("< state=TS_%s RECV 0x%02x=%s\n", strstate[telnet_state], c, TELCMD(c));
		} else {
			printf("< state=TS_%s RECV 0x%02x\n", strstate[telnet_state], c);
		}
#endif

		switch (telnet->telnet_state) {
		case TS_CR:
			telnet->telnet_state = TS_DATA;
			/* Strip off \n or \0 after a \r */
			if ((c == 0) || (c == '\n')) {
				break;
			}
			/* FALL THROUGH */

		case TS_DATA:
			if (c == IAC) {
				telnet->telnet_state = TS_IAC;
				break;
			}
			/*
			 * We now map \r\n ==> \r for pragmatic reasons.
			 * Many client implementations send \r\n when
			 * the user hits the CarriageReturn key.
			 *
			 * We USED to map \r\n ==> \n, since \r\n says
			 * that we want to be in column 1 of the next
			 * printable line, and \n is the standard
			 * unix way of saying that (\r is only good
			 * if CRMOD is set, which it normally is).
			 */
			if (c == '\r')
				telnet->telnet_state = TS_CR;
			TELNET_INPUT(c);
			break;

		case TS_IAC:
			switch (c) {
			/*
			 * Send the process on the pty side an
			 * interrupt.  Do this with a NULL or
			 * interrupt char; depending on the tty mode.
			 */
			case IP:
				interrupt();
				break;

			case BREAK:
				sendbrk();
				break;

			/*
			 * Are You There?
			 */
			case AYT:
				recv_ayt(cookie);
				break;

			/*
			 * Abort Output
			 */
			case AO:
				abort_output();
				break;

			/*
			 * Erase Character and
			 */
			case EC:
				erase_character();
				break;

			/*
			 * Erase Line
			 */
			case EL:
				erase_line();
				break;

			/*
			 * Check for urgent data...
			 */
			case DM:
				urgent_data();
				break;

			/*
			 * Begin option subnegotiation...
			 */
			case SB:
				telnet->telnet_state = TS_SB;
				continue;

			case WILL:
				telnet->telnet_state = TS_WILL;
				continue;

			case WONT:
				telnet->telnet_state = TS_WONT;
				continue;

			case DO:
				telnet->telnet_state = TS_DO;
				continue;

			case DONT:
				telnet->telnet_state = TS_DONT;
				continue;

			case EOR:
			case xEOF:
				break;

			case SUSP:
			case ABORT:
				/* not supported */
				break;

			case IAC:
				TELNET_INPUT(c);
				break;
			}

			telnet->telnet_state = TS_DATA;
			break;

		case TS_SB:
			if (c == IAC)
				telnet->telnet_state = TS_SE;
			break;

		case TS_SE:
			if (c != SE)
				telnet->telnet_state = TS_SB;
			else
				telnet->telnet_state = TS_DATA;
			break;

		case TS_WILL:
#ifdef TELNET_DEBUG
			if (TELOPT_OK(c)) {
				printf("  WILL %d=%s\n", c, TELOPT(c));
			} else {
				printf("  WILL %d\n", c);
			}
#endif
			switch (c) {
			case TELOPT_ECHO:
			case TELOPT_BINARY:
			case TELOPT_SGA:
				send_do(cookie, c, 0);
				break;
			default:
				send_dont(cookie, c, 0);
				break;
			}
			telnet_init_once(telnet, cookie);

			telnet->telnet_state = TS_DATA;
			continue;

		case TS_WONT:
#ifdef TELNET_DEBUG
			if (TELOPT_OK(c)) {
				printf("  WONT %d=%s\n", c, TELOPT(c));
			} else {
				printf("  WONT %d\n", c);
			}
#endif
			telnet_init_once(telnet, cookie);

			telnet->telnet_state = TS_DATA;
			continue;

		case TS_DO:
#ifdef TELNET_DEBUG
			if (TELOPT_OK(c)) {
				printf("  DO %d=%s\n", c, TELOPT(c));
			} else {
				printf("  DO %d\n", c);
			}
#endif
			switch (c) {
			case TELOPT_ECHO:
			case TELOPT_BINARY:
			case TELOPT_SGA:
				send_will(cookie, c, 0);
				break;
			default:
				send_wont(cookie, c, 0);
				break;
			}
			telnet_init_once(telnet, cookie);

			telnet->telnet_state = TS_DATA;
			continue;

		case TS_DONT:
#ifdef TELNET_DEBUG
			if (TELOPT_OK(c)) {
				printf("  DONT %d=%s\n", c, TELOPT(c));
			} else {
				printf("  DONT %d\n", c);
			}
#endif
			telnet_init_once(telnet, cookie);

			telnet->telnet_state = TS_DATA;
			continue;

		default:
			break;
		}
	}
}  /* end of telrcv */
