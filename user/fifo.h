#ifndef _FIFO_H_
#define _FIFO_H_

struct fifo {
#define FIFO_BUFSIZE	512	/* must be n^2 */
	unsigned int rd;
	unsigned int wr;
	unsigned int len;
	char buf[FIFO_BUFSIZE];
};

static inline unsigned int
fifo_space(struct fifo *fifo)
{
	return FIFO_BUFSIZE - fifo->len;
}

static inline unsigned int
fifo_len(struct fifo *fifo)
{
	return fifo->len;
}

void fifo_init(struct fifo *);
int fifo_putc(struct fifo *, char);
int fifo_getc(struct fifo *);
unsigned int fifo_space(struct fifo *);

char *fifo_getbulk(struct fifo *, unsigned int *);

int fifo_write(struct fifo *, char *, unsigned int);
int fifo_read(struct fifo *, char *, unsigned int);

#endif /* _FIFO_H_ */
