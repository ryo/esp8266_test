/*	$NetBSD: subr_prf.c,v 1.27 2014/08/30 14:24:02 tsutsui Exp $	*/

/*-
 * Copyright (c) 1993
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
 *	@(#)printf.c	8.1 (Berkeley) 6/11/93
 */
#include <stdarg.h>
#include "driver/uart.h"

typedef long long int	intmax_t;
typedef int		intptr_t;
typedef unsigned int	uintptr_t;
typedef int		ssize_t;
typedef unsigned long	u_long;
#define NBBY		8


#define LIBSA_PRINTF_WIDTH_SUPPORT

#define LONG		0x01
#define LLONG		0x02
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
#define ALT		0x04
#define SPACE		0x08
#define LADJUST		0x10
#define SIGN		0x20
#define ZEROPAD		0x40
#define NEGATIVE	0x80
#define KPRINTN(base)	kprintn(put, ul, base, lflag, width)
#define RADJUSTZEROPAD()					\
do {								\
	if ((lflag & (ZEROPAD|LADJUST)) == ZEROPAD) {		\
		while (width-- > 0)				\
			put('0');				\
	}							\
} while (/*CONSTCOND*/0)
#define LADJUSTPAD()						\
do {								\
	if (lflag & LADJUST) {					\
		while (width-- > 0)				\
			put(' ');				\
	}							\
} while (/*CONSTCOND*/0)
#define RADJUSTPAD()						\
do {								\
	if ((lflag & (ZEROPAD|LADJUST)) == 0) {			\
		while (width-- > 0)				\
			put(' ');				\
	}							\
} while (/*CONSTCOND*/0)
#else	/* LIBSA_PRINTF_WIDTH_SUPPORT */
#define KPRINTN(base)	kprintn(put, ul, base)
#define RADJUSTZEROPAD()	/**/
#define LADJUSTPAD()		/**/
#define RADJUSTPAD()		/**/
#endif	/* LIBSA_PRINTF_WIDTH_SUPPORT */

#define KPRINT(base)						\
do {								\
	ul = (lflag & LLONG)					\
	    ? va_arg(ap, unsigned long long)			\
	    : (lflag & LONG)					\
		? va_arg(ap, u_long)				\
		: va_arg(ap, u_int);				\
	KPRINTN(base);						\
} while (/*CONSTCOND*/0)

#define PTRDIFF_T	intptr_t

#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
static void kprintn(int (*)(int), unsigned long long, int, int, int);
#else
static void kprintn(int (*)(int), unsigned long long, int);
#endif
static void kdoprnt(int (*)(int), const char *, va_list);

const char hexdigits[16] = "0123456789abcdef";

static void
kdoprnt(int (*put)(int), const char *fmt, va_list ap)
{
	char *p;
	int ch;
	unsigned long long ul;
	int lflag;
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
	int width;
	char *q;
#endif

	for (;;) {
		while ((ch = *fmt++) != '%') {
			if (ch == '\0')
				return;
			put(ch);
		}
		lflag = 0;
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
		width = 0;
#endif
reswitch:
		switch (ch = *fmt++) {
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
		case '#':
			lflag |= ALT;
			goto reswitch;
		case ' ':
			lflag |= SPACE;
			goto reswitch;
		case '-':
			lflag |= LADJUST;
			goto reswitch;
		case '+':
			lflag |= SIGN;
			goto reswitch;
		case '0':
			lflag |= ZEROPAD;
			goto reswitch;
		case '1': case '2': case '3': case '4': case '5':
		case '6': case '7': case '8': case '9':
			for (;;) {
				width *= 10;
				width += ch - '0';
				ch = *fmt;
				if ((unsigned)ch - '0' > 9)
					break;
				++fmt;
			}
#endif
			goto reswitch;
		case 'l':
			if (*fmt == 'l') {
				++fmt;
				lflag |= LLONG;
			} else
				lflag |= LONG;
			goto reswitch;
		case 'j':
			if (sizeof(intmax_t) == sizeof(long long))
				lflag |= LLONG;
			else if (sizeof(intmax_t) == sizeof(long))
				lflag |= LONG;
			goto reswitch;
		case 't':
			if (sizeof(PTRDIFF_T) == sizeof(long))
				lflag |= LONG;
			goto reswitch;
		case 'z':
			if (sizeof(ssize_t) == sizeof(long))
				lflag |= LONG;
			goto reswitch;
		case 'c':
			ch = va_arg(ap, int);
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
			--width;
#endif
			RADJUSTPAD();
			put(ch & 0xFF);
			LADJUSTPAD();
			break;
		case 's':
			p = va_arg(ap, char *);
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
			for (q = p; *q != '\0'; ++q)
				continue;
			width -= q - p;
#endif
			RADJUSTPAD();
			while ((ch = (unsigned char)*p++))
				put(ch);
			LADJUSTPAD();
			break;
		case 'd':
			ul = (lflag & LLONG) ? va_arg(ap, long long) :
			    (lflag & LONG) ? va_arg(ap, long) : va_arg(ap, int);
			if ((long int)ul < 0) {
				ul = -(long int)ul;
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
				lflag |= NEGATIVE;
#else
				put('-');
#endif
			}
			KPRINTN(10);
			break;
		case 'o':
			KPRINT(8);
			break;
		case 'u':
			KPRINT(10);
			break;
		case 'p':
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
			lflag |= (LONG|ALT);
#else
			put('0');
			put('x');
#endif
			/* FALLTHROUGH */
		case 'x':
			KPRINT(16);
			break;
		default:
			if (ch == '\0')
				return;
			put(ch);
			break;
		}
	}
}

static void
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
kprintn(int (*put)(int), unsigned long long ul, int base, int lflag, int width)
#else
kprintn(int (*put)(int), unsigned long long ul, int base)
#endif
{
					/* hold a INTMAX_T in base 8 */
	char *p, buf[(sizeof(intmax_t) * NBBY / 3) + 1 + 2 /* ALT + SIGN */];
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
	char *q;
#endif

	p = buf;
	do {
		*p++ = hexdigits[ul % base];
	} while (ul /= base);
#ifdef LIBSA_PRINTF_WIDTH_SUPPORT
	q = p;
	if (lflag & ALT && *(p - 1) != '0') {
		if (base == 8) {
			*p++ = '0';
		} else if (base == 16) {
			*p++ = 'x';
			*p++ = '0';
		}
	}
	if (lflag & NEGATIVE)
		*p++ = '-';
	else if (lflag & SIGN)
		*p++ = '+';
	else if (lflag & SPACE)
		*p++ = ' ';
	width -= p - buf;
	if (lflag & ZEROPAD) {
		while (p > q)
			put(*--p);
	}
#endif
	RADJUSTPAD();
	RADJUSTZEROPAD();
	do {
		put(*--p);
	} while (p > buf);
	LADJUSTPAD();
}

int putchar(int);

int
vprintf(const char *fmt, va_list ap)
{
	kdoprnt(putchar, fmt, ap);
	return 0;	/* XXX */
}

int
printf(const char *fmt, ...)
{
	int rc;
	va_list ap;

	va_start(ap, fmt);
	rc = vprintf(fmt, ap);
	va_end(ap);

	return rc;
}

#undef putchar
int
putchar(int c)
{
	if (c == '\n')
		uart_tx_one_char(UART0, '\r');
	uart_tx_one_char(UART0, c);
	return 0;
}

int
puts(const char *str)
{
	while (*str != '\0')
		putchar(*str++);
	putchar('\n');
	return 0;
}
