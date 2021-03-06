#include <stdio.h>
#include <string.h>
#include "c_types.h"
#include "fifo.h"

#undef FIFO_DEBUG

#ifdef FIFO_DEBUG
ICACHE_FLASH_ATTR
static void
fifo_dump(struct fifo *fifo)
{
	unsigned int i;

	printf("<fifo ptr=%p rd=0x%x wr=0x%x len=%d>\n",
	    fifo, fifo->rd, fifo->wr, fifo->len);
	for (i = 0; i < FIFO_BUFSIZE; i++) {
		if ((i & 15) == 0) {
			printf("+%02x:", i);
		}

		printf(" %02x", fifo->buf[i] & 0xff);
		if ((i & 15) == 15)
			printf("\n");
	}
	printf("</fifo>\n");
}
#endif

ICACHE_FLASH_ATTR
void
fifo_init(struct fifo *fifo)
{
	memset(fifo, 0, sizeof(*fifo));
}

ICACHE_FLASH_ATTR
int
fifo_putc(struct fifo *fifo, char c)
{
	if (fifo->len >= FIFO_BUFSIZE)
		return 0;

	fifo->buf[fifo->wr] = c;
	fifo->wr = (fifo->wr + 1) & (FIFO_BUFSIZE - 1);
	fifo->len++;

	return 1;
}

ICACHE_FLASH_ATTR
int
fifo_getc(struct fifo *fifo)
{
	int c;

	if (fifo->len == 0)
		return -1;

	c = fifo->buf[fifo->rd] & 0xff;
	fifo->rd = (fifo->rd + 1) & (FIFO_BUFSIZE - 1);

	fifo->len -= 1;
#if 0
	if (fifo->len == 0) {
		fifo->rd = 0;
		fifo->wr = 0;
	}
#endif

	return c;
}

ICACHE_FLASH_ATTR
int
fifo_write(struct fifo *fifo, char *buf, unsigned int len)
{
	unsigned int left, i;

	left = fifo_space(fifo);
	if (left < len)
		len = left;

	for (i = 0; i < len; i++)
		fifo_putc(fifo, *buf++);

	return len;
}

ICACHE_FLASH_ATTR
int
fifo_read(struct fifo *fifo, char *buf, unsigned int len)
{
	unsigned int fifolen, i;

	fifolen = fifo_space(fifo);
	if (fifolen < len)
		len = fifolen;

	for (i = 0; i < len; i++)
		*buf++ = fifo_getc(fifo);;

	return len;
}

ICACHE_FLASH_ATTR
char *
fifo_getbulk(struct fifo *fifo, unsigned int *lenp)
{
	unsigned int r;
	unsigned int len;

	if (fifo->len == 0) {
		*lenp = 0;
		return NULL;
	}

#ifdef FIFO_DEBUG
	fifo_dump(fifo);
#endif

	r = fifo->rd;
	if ((FIFO_BUFSIZE - r) < fifo->len) {
		*lenp = len = FIFO_BUFSIZE - r;
		fifo->len -= len;
		fifo->rd = 0;
		return &fifo->buf[r];
	}

	*lenp = len = fifo->len;
	fifo->rd = 0;
	fifo->wr = 0;
	fifo->len = 0;
	return &fifo->buf[r];
}
