/*
 * Copyright (c) 1983, 1993
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
 *
 *	@(#)telnet.h	8.2 (Berkeley) 12/15/93
 */

#ifndef _TELNET_H_
#define _TELNET_H_

/*
 * Definitions for the TELNET protocol.
 */
#define IAC	255		/* interpret as command: */
#define DONT	254		/* you are not to use option */
#define DO	253		/* please, you use option */
#define WONT	252		/* I won't use option */
#define WILL	251		/* I will use option */
#define SB	250		/* interpret as subnegotiation */
#define GA	249		/* you may reverse the line */
#define EL	248		/* erase the current line */
#define EC	247		/* erase the current character */
#define AYT	246		/* are you there */
#define AO	245		/* abort output--but let prog finish */
#define IP	244		/* interrupt process--permanently */
#define BREAK	243		/* break */
#define DM	242		/* data mark--for connect. cleaning */
#define NOP	241		/* nop */
#define SE	240		/* end sub negotiation */
#define EOR	239		/* end of record (transparent mode) */
#define ABORT	238		/* Abort process */
#define SUSP	237		/* Suspend process */
#define xEOF	236		/* End of file: EOF is already used... */

#define SYNCH	242		/* for telfunc calls */

#ifdef TELCMDS
const char *telcmds[] = {
	"EOF", "SUSP", "ABORT", "EOR",
	"SE", "NOP", "DMARK", "BRK", "IP", "AO", "AYT", "EC",
	"EL", "GA", "SB", "WILL", "WONT", "DO", "DONT", "IAC", 0
};
#else
extern const char *telcmds[];
#endif

#define TELCMD_FIRST	xEOF
#define TELCMD_LAST	IAC
#define TELCMD_OK(x)	((unsigned int)(x) <= TELCMD_LAST && \
			 (unsigned int)(x) >= TELCMD_FIRST)
#define TELCMD(x)	telcmds[(x) - TELCMD_FIRST]


/* telnet options */
#define TELOPT_BINARY	0	/* 8-bit data path */
#define TELOPT_ECHO	1	/* echo */
#define TELOPT_RCP	2	/* prepare to reconnect */
#define TELOPT_SGA	3	/* suppress go ahead */
#define TELOPT_NAMS	4	/* approximate message size */
#define TELOPT_STATUS	5	/* give status */
#define TELOPT_TM	6	/* timing mark */
#define TELOPT_RCTE	7	/* remote controlled transmission and echo */
#define TELOPT_NAOL	8	/* negotiate about output line width */
#define TELOPT_NAOP	9	/* negotiate about output page size */
#define TELOPT_NAOCRD	10	/* negotiate about CR disposition */
#define TELOPT_NAOHTS	11	/* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD	12	/* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD	13	/* negotiate about formfeed disposition */
#define TELOPT_NAOVTS	14	/* negotiate about vertical tab stops */
#define TELOPT_NAOVTD	15	/* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD	16	/* negotiate about output LF disposition */
#define TELOPT_XASCII	17	/* extended ascic character set */
#define TELOPT_LOGOUT	18	/* force logout */
#define TELOPT_BM	19	/* byte macro */
#define TELOPT_DET	20	/* data entry terminal */
#define TELOPT_SUPDUP	21	/* supdup protocol */
#define TELOPT_SUPDUPOUTPUT 22	/* supdup output */
#define TELOPT_SNDLOC	23	/* send location */
#define TELOPT_TTYPE	24	/* terminal type */
#define TELOPT_EOR	25	/* end or record */
#define TELOPT_TUID	26	/* TACACS user identification */
#define TELOPT_OUTMRK	27	/* output marking */
#define TELOPT_TTYLOC	28	/* terminal location number */
#define TELOPT_3270REGIME 29	/* 3270 regime */
#define TELOPT_X3PAD	30	/* X.3 PAD */
#define TELOPT_NAWS	31	/* window size */
#define TELOPT_TSPEED	32	/* terminal speed */
#define TELOPT_LFLOW	33	/* remote flow control */
#define TELOPT_LINEMODE	34	/* Linemode option */
#define TELOPT_XDISPLOC	35	/* X Display Location */
#define TELOPT_OLD_ENVIRON 36	/* Old - Environment variables */
#define TELOPT_AUTHENTICATION 37/* Authenticate */
#define TELOPT_ENCRYPT	38	/* Encryption option */
#define TELOPT_NEW_ENVIRON 39	/* New - Environment variables */
#define TELOPT_TN3270E	40	/* RFC2355 - TN3270 Enhancements */
#define TELOPT_CHARSET	42	/* RFC2066 - Charset */
#define TELOPT_COMPORT	44	/* RFC2217 - Com Port Control */
#define TELOPT_KERMIT	47	/* RFC2840 - Kermit */
#define TELOPT_EXOPL	255	/* extended-options-list */

#define	TELOPT_FIRST	TELOPT_BINARY
#define	TELOPT_LAST	TELOPT_KERMIT
#define	TELOPT_OK(x)	((unsigned int)(x) <= TELOPT_LAST)
#define	TELOPT(x)	telopts[(x) - TELOPT_FIRST]

#define	NTELOPTS	(1 + TELOPT_LAST)

#ifdef TELOPTS
const char *telopts[NTELOPTS + 1] = {
	"BINARY", "ECHO", "RCP", "SUPPRESS GO AHEAD", "NAME",
	"STATUS", "TIMING MARK", "RCTE", "NAOL", "NAOP",
	"NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
	"NAOVTD", "NAOLFD", "EXTEND ASCII", "LOGOUT", "BYTE MACRO",
	"DATA ENTRY TERMINAL", "SUPDUP", "SUPDUP OUTPUT",
	"SEND LOCATION", "TERMINAL TYPE", "END OF RECORD",
	"TACACS UID", "OUTPUT MARKING", "TTYLOC",
	"3270 REGIME", "X.3 PAD", "NAWS", "TSPEED", "LFLOW",
	"LINEMODE", "XDISPLOC", "OLD-ENVIRON", "AUTHENTICATION",
	"ENCRYPT", "NEW-ENVIRON", "TN3270E", "CHARSET", "COM-PORT",
	"KERMIT",
	0
};
#else
extern const char *telopts[NTELOPTS + 1];
#endif

struct telnet {
	int telopt_sent;
	int telnet_state;
};

void telnet_init(struct telnet *);
void telnet_recv(struct telnet *, void *, char *, unsigned int);

#endif /* _TELNET_H_ */
