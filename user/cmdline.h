#ifndef _COMMANDLINE_H_
#define _COMMANDLINE_H_

struct cmdtbl {
	const char *cmd;
	int (*func)(void *arg, int argc, char *argv[]);
};

#define LINESIZE	127
#define HISTSIZE	256
struct cmdline {
	void (*cmdputc)(void *, char);
	void *cmdputcarg;
	const char *prompt;

	char line[LINESIZE + 1];
	char histbuf[HISTSIZE];
	char killbuf[LINESIZE + 1];
#define NOAPPEND		0x0000
#define APPENDTOKILL	0x0001
#define APPENDTOKILLREV	0x0002
	short len;
	short klen;
	short pos;
	short mark;
	unsigned short histptr;
	short histcur;
	unsigned char hist_pending;
	unsigned char validmark;
	unsigned char esc;
	unsigned char cex;
};

void cmdline_init(struct cmdline *, void (*_func)(void *, char), void *);
void cmdline_reset(struct cmdline *);
void cmdline_setprompt(struct cmdline *, const char *);
void cmdline_putc_handler(struct cmdline *, void (*)(void *, char ), void *);
int commandline(struct cmdline *, char, struct cmdtbl *, void *);

#endif /* _COMMANDLINE_H_ */
