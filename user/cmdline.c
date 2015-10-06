#include "ets_sys.h"
#include "c_types.h"
#include "cmdline.h"

#define memset	ets_memset
#define memmove	ets_memmove

static char *cmdline_input(struct cmdline *, char);

static void beep(struct cmdline *);
static int appendkillbuffer(struct cmdline *, char, int);
static int inword(struct cmdline *);
static int setmark(struct cmdline *);
static int getmark(struct cmdline *);
static int forward(struct cmdline *, int);
static int backward(struct cmdline *, int);
static int beginning_of_line(struct cmdline *);
static int end_of_line(struct cmdline *);
static int delete(struct cmdline *, int);
static int backspace(struct cmdline *, int);
static int insert(struct cmdline *, char);
static int insertstr(struct cmdline *, char *);
static int forward_word(struct cmdline *);
static int backward_word(struct cmdline *);
static int delete_word(struct cmdline *);
static int backward_delete_word(struct cmdline *);
static int killregion(struct cmdline *);
static int swapmark(struct cmdline *);
static int killtoend(struct cmdline *);
static int killline(struct cmdline *, int);
static int yank(struct cmdline *);
static int addhist(struct cmdline *, char *);
static int prevhist(struct cmdline *);
static int nexthist(struct cmdline *);

ICACHE_FLASH_ATTR
static void
beep(struct cmdline *cmdline)
{
	cmdline->cmdputc(cmdline->cmdputcarg, 0x07);
}

ICACHE_FLASH_ATTR
static void
clearkillbuffer(struct cmdline *cmdline)
{
	cmdline->klen = 0;
	memset(cmdline->killbuf, 0, sizeof(cmdline->killbuf));
}

ICACHE_FLASH_ATTR
static int
appendkillbuffer(struct cmdline *cmdline, char ch, int append)
{
	if (append==0)
		return FALSE;

	if (cmdline->klen >= LINESIZE)
		return FALSE;

	switch (append) {
	case APPENDTOKILL:
		cmdline->killbuf[cmdline->klen++] = ch;
		break;
	case APPENDTOKILLREV:
		memmove(&cmdline->killbuf[1], &cmdline->killbuf[0], cmdline->klen++);
		cmdline->killbuf[0] = ch;
		break;
	default:
		break;
	}

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
inword(struct cmdline *cmdline)
{
	char ch;
	ch = cmdline->line[cmdline->pos];

	if (('0' <= ch && ch <= '9') ||
	   ('A' <= ch && ch <= 'Z') ||
	   ('a' <= ch && ch <= 'z') ||
	   (ch == '_'))
		return TRUE;

	return FALSE;
}

ICACHE_FLASH_ATTR
void
cmdline_init(struct cmdline *cmdline, void (*cmdputc_func)(void *arg, char ch), void *arg)
{
	memset(cmdline, 0, sizeof(struct cmdline));
	cmdline->prompt = ">";
	cmdline->cmdputc = cmdputc_func;
	cmdline->cmdputcarg = arg;
}

ICACHE_FLASH_ATTR
static void
cmdline_puts(struct cmdline *cmdline, const char *str)
{
	char c;

	while ((c = *str++) != '\0')
		cmdline->cmdputc(cmdline->cmdputcarg, c);
}

ICACHE_FLASH_ATTR
void
cmdline_reset(struct cmdline *cmdline)
{
	cmdline->line[0] = '\0';
	cmdline->len = 0;
	cmdline->pos = 0;
	cmdline->esc = 0;
	cmdline->cex = 0;
	cmdline->validmark = 0;
	cmdline->mark = 0;
	cmdline->histcur = -1;

	cmdline_puts(cmdline, cmdline->prompt);
}

ICACHE_FLASH_ATTR
static int
setmark(struct cmdline *cmdline)
{
	cmdline->validmark = 1;
	cmdline->mark = cmdline->pos;
	return TRUE;
}

ICACHE_FLASH_ATTR
static int
getmark(struct cmdline *cmdline)
{
	int r;

	if (cmdline->validmark == 0)
		return -1;

	if ((r = cmdline->mark) < 0)
		r = 0;

	if (r > cmdline->len)
		r = cmdline->len;

	return r;
}

ICACHE_FLASH_ATTR
static int
forward(struct cmdline *cmdline, int n)
{
	while (n-->0) {
		if (cmdline->pos >= cmdline->len)
			return FALSE;

		cmdline->cmdputc(cmdline->cmdputcarg, cmdline->line[cmdline->pos]);
		cmdline->pos++;
	}
	return TRUE;
}

ICACHE_FLASH_ATTR
static int
backward(struct cmdline *cmdline, int n)
{
	while (n-->0) {
		if (cmdline->pos == 0)
			return FALSE;

		cmdline->pos--;
		cmdline->cmdputc(cmdline->cmdputcarg, '\b');
	}
	return TRUE;
}

ICACHE_FLASH_ATTR
static int
beginning_of_line(struct cmdline *cmdline)
{
	return backward(cmdline, cmdline->pos);
}

ICACHE_FLASH_ATTR
static int
end_of_line(struct cmdline *cmdline)
{
	return forward(cmdline, cmdline->len - cmdline->pos);
}

ICACHE_FLASH_ATTR
static int
delete(struct cmdline *cmdline, int append)
{
	int i;
	int rest = cmdline->len - cmdline->pos;

	if (cmdline->len == 0 || rest == 0)
		return FALSE;

	appendkillbuffer(cmdline, cmdline->line[cmdline->pos], append);

	memmove(&cmdline->line[cmdline->pos], &cmdline->line[cmdline->pos + 1], rest - 1);

	for (i = cmdline->pos; i < cmdline->pos + rest - 1; i++)
		cmdline->cmdputc(cmdline->cmdputcarg, cmdline->line[i]);

	cmdline->cmdputc(cmdline->cmdputcarg, ' ');

	for (i = 0; i < rest; i++)
		cmdline->cmdputc(cmdline->cmdputcarg, '\b');

	cmdline->len--;
	cmdline->line[cmdline->len] = '\0';

	if (cmdline->pos < cmdline->mark)
		cmdline->mark--;

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
backspace(struct cmdline *cmdline, int append)
{
	if (backward(cmdline, 1) == FALSE)
		return FALSE;
	return delete(cmdline, append);
}

ICACHE_FLASH_ATTR
static int
insert(struct cmdline *cmdline, char ch)
{
	int i;
	int rest = cmdline->len - cmdline->pos;

	if (cmdline->len >= LINESIZE) {
		beep(cmdline);
		return FALSE;
	}

	memmove(&cmdline->line[cmdline->pos + 1], &cmdline->line[cmdline->pos], rest);
	cmdline->line[cmdline->pos] = ch;

	for (i = cmdline->pos; i <= cmdline->len; i++)
		cmdline->cmdputc(cmdline->cmdputcarg, cmdline->line[i]);

	for (i = 0; i < rest; i++)
		cmdline->cmdputc(cmdline->cmdputcarg, '\b');

	cmdline->pos++;
	cmdline->len++;

	if (cmdline->mark > cmdline->pos)
		cmdline->mark++;

	cmdline->line[cmdline->len] = '\0';

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
insertstr(struct cmdline *cmdline, char *str)
{
	while (*str) {
		if (insert(cmdline, *str++) == FALSE)
			return FALSE;
	}
	return TRUE;
}

ICACHE_FLASH_ATTR
static int
forward_word(struct cmdline *cmdline)
{
	while (inword(cmdline) != FALSE) {
		if (forward(cmdline, 1) == FALSE) {
			return FALSE;
		}
	}

	while (inword(cmdline) == FALSE) {
		if (forward(cmdline, 1) == FALSE) {
			return FALSE;
		}
	}

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
backward_word(struct cmdline *cmdline)
{
	if (backward(cmdline, 1) == FALSE)
		return FALSE;

	while (inword(cmdline) == FALSE) {
		if (backward(cmdline, 1) == FALSE)
			return FALSE;
	}

	while (inword(cmdline) != FALSE) {
		if (backward(cmdline, 1) == FALSE)
			return FALSE;
	}

	return forward(cmdline, 1);
}

ICACHE_FLASH_ATTR
static int
delete_word(struct cmdline *cmdline)
{
	if (cmdline->pos >= cmdline->len)
		return FALSE;

	clearkillbuffer(cmdline);

	while (inword(cmdline) == FALSE) {
		if (delete(cmdline, APPENDTOKILL) == FALSE)
			return FALSE;
	}

	while (inword(cmdline) != FALSE) {
		if (delete(cmdline, APPENDTOKILL) == FALSE)
			return FALSE;
	}

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
backward_delete_word(struct cmdline *cmdline)
{
	if (cmdline->pos == 0)
		return FALSE;

	clearkillbuffer(cmdline);

	for (;;) {
		if (backward(cmdline, 1) == FALSE)
			break;
		if (inword(cmdline) == FALSE) {
			if (delete(cmdline, APPENDTOKILLREV) == FALSE)
				return FALSE;
		} else {
			forward(cmdline, 1);
			break;
		}
	}

	for (;;) {
		if (backward(cmdline, 1) == FALSE)
			break;
		if (inword(cmdline) == TRUE) {
			if (delete(cmdline, APPENDTOKILLREV) == FALSE)
				return FALSE;
		} else {
			forward(cmdline, 1);
			break;
		}
	}

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
killregion(struct cmdline *cmdline)
{
	int i;
	int r;
	if ((r = getmark(cmdline)) < 0)
		return FALSE;

	clearkillbuffer(cmdline);

	r = r - cmdline->pos;
	if (r < 0) {
		for (i = 0; i<-r; i++) {
			backspace(cmdline, APPENDTOKILLREV);
		}
	} else {
		for (i = 0; i < r; i++) {
			delete(cmdline, APPENDTOKILL);
		}
	}

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
swapmark(struct cmdline *cmdline)
{
	int r;
	if ((r = getmark(cmdline)) < 0)
		return FALSE;

	setmark(cmdline);

	r = r - cmdline->pos;
	if (r < 0)
		backward(cmdline,-r);
	else
		forward(cmdline, r);

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
killtoend(struct cmdline *cmdline)
{
	if (cmdline->len == 0)
		return FALSE;

	if (cmdline->pos >= cmdline->len)
		return FALSE;

	clearkillbuffer(cmdline);

	while (delete(cmdline, APPENDTOKILL) != FALSE)
		;

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
killline(struct cmdline *cmdline, int cut)
{
	if (cmdline->len==0)
		return FALSE;

	if (cut)
		clearkillbuffer(cmdline);

	while (forward(cmdline, 1) != FALSE)
		;

	while (backspace(cmdline, cut ? APPENDTOKILLREV : 0) != FALSE)
		;

	return TRUE;
}

ICACHE_FLASH_ATTR
static int
yank(struct cmdline *cmdline)
{
	cmdline->killbuf[cmdline->klen] = '\0';
	return insertstr(cmdline, cmdline->killbuf);
}

#define HISTPTR_FORWARD(cmdline, var)					\
	do {								\
		if ((cmdline)->var >= HISTSIZE - 1) {			\
			(cmdline)->var = 0;				\
		} else {						\
			(cmdline)->var++;				\
		}							\
	} while (0 /* CONSTCOND */)

#define HISTPTR_BACKWARD(cmdline, var)					\
	do {								\
		if ((cmdline)->var == 0) {				\
			(cmdline)->var = HISTSIZE - 1;			\
		} else {						\
			(cmdline)->var--;				\
		}							\
	} while (0 /* CONSTCOND */)

ICACHE_FLASH_ATTR
static void
fetch_from_history(struct cmdline *cmdline)
{
	int i;

	i = cmdline->histcur;
	while (cmdline->histbuf[i] != '\0') {
		insert(cmdline, cmdline->histbuf[i++]);
		if (i >= HISTSIZE - 1)
			i = 0;
	}
}

ICACHE_FLASH_ATTR
static inline int
allspace(const char *str)
{
	for (; *str != '\0'; str++) {
		if (*str != ' ' && *str != '\t')
			return 0;
	}
	return 1;
}

ICACHE_FLASH_ATTR
int
addhist(struct cmdline *cmdline, char *str)
{
	if (cmdline->hist_pending) {
		HISTPTR_BACKWARD(cmdline, histptr);
		HISTPTR_BACKWARD(cmdline, histptr);
		while (cmdline->histbuf[cmdline->histptr] != '\0') {
			HISTPTR_BACKWARD(cmdline, histptr);
		}
		HISTPTR_FORWARD(cmdline, histptr);
		cmdline->hist_pending = 0;
	}

	if (*str == '\0')
		return 0;

	if (allspace(str))
		return 0;

	while (*str != '\0') {
		cmdline->histbuf[cmdline->histptr] = *str++;
		HISTPTR_FORWARD(cmdline, histptr);
	}
	if (cmdline->histbuf[cmdline->histptr] != '\0') {
		int i;

		/* delete oldest history */
		i = cmdline->histptr;
		while (cmdline->histbuf[i] != '\0') {
			cmdline->histbuf[i++] = '\0';
			i %= HISTSIZE;
		}
	}

	HISTPTR_FORWARD(cmdline, histptr);
	cmdline->histcur = -1;

	return 0;
}

ICACHE_FLASH_ATTR
int
prevhist(struct cmdline *cmdline)
{
	if (cmdline->histptr == cmdline->histcur) {
		beep(cmdline);
		return FALSE;
	}

	if (cmdline->histcur < 0) {
		if (!allspace(cmdline->line)) {
			short save;
			save = cmdline->histptr;
			addhist(cmdline, cmdline->line);
			cmdline->hist_pending = 1;
			cmdline->histcur = save;
		} else {
			cmdline->histcur = cmdline->histptr;
		}
	}

	HISTPTR_BACKWARD(cmdline, histcur);
	HISTPTR_BACKWARD(cmdline, histcur);
	if (cmdline->histbuf[cmdline->histcur] == '\0') {
		HISTPTR_FORWARD(cmdline, histcur);
		HISTPTR_FORWARD(cmdline, histcur);
		beep(cmdline);
		return FALSE;
	}

	while (cmdline->histbuf[cmdline->histcur] != '\0') {
		HISTPTR_BACKWARD(cmdline, histcur);
	}
	HISTPTR_FORWARD(cmdline, histcur);

	killline(cmdline, 0);
	fetch_from_history(cmdline);
	return TRUE;
}

ICACHE_FLASH_ATTR
int
nexthist(struct cmdline *cmdline)
{
	int save;

	save = cmdline->histcur;

	if (cmdline->histcur < 0) {
		beep(cmdline);
		return FALSE;
	}

	while (cmdline->histbuf[cmdline->histcur] != '\0') {
		HISTPTR_FORWARD(cmdline, histcur);
	}
	HISTPTR_FORWARD(cmdline, histcur);

	if (cmdline->histptr == cmdline->histcur) {
		cmdline->histcur = save;
		beep(cmdline);
		return FALSE;
	}

	killline(cmdline, 0);
	fetch_from_history(cmdline);
	return TRUE;
}

ICACHE_FLASH_ATTR
static char *
cmdline_input(struct cmdline *cmdline, char ch)
{
	if (cmdline->esc) {
		switch (ch) {
		case 'f':
			forward_word(cmdline);
			break;
		case 'b':
			backward_word(cmdline);
			break;
		case 'd':
			delete_word(cmdline);
			break;
		case 0x08:	/* ^H */
			backward_delete_word(cmdline);
			break;
		default:
			beep(cmdline);
			break;
		}
		cmdline->esc = 0;
	} else if (cmdline->cex) {
		switch (ch) {
		case 0x18:	/* ^X */
			swapmark(cmdline);
			break;
		default:
			beep(cmdline);
			break;
		}
		cmdline->cex = 0;
	} else {
		switch (ch) {
		case 0x1b:	/* ESC */
			cmdline->esc = 1;
			break;
		case 0x00:	/* ^@ */
			setmark(cmdline);
			break;
		case 0x01:	/* ^A */
			beginning_of_line(cmdline);
			break;
		case 0x02:	/* ^B */
			backward(cmdline, 1);
			break;
		case 0x04:	/* ^D */
			delete(cmdline, NOAPPEND);
			break;
		case 0x05:	/* ^E */
			end_of_line(cmdline);
			break;
		case 0x06:	/* ^F */
			forward(cmdline, 1);
			break;
		case 0x08:	/* ^H */
		case 0x7f:	/* ^? */
			backspace(cmdline, NOAPPEND);
			break;
		case 0x0B:	/* ^K */
			killtoend(cmdline);
			break;
		case 0x0E:	/* ^N */
			nexthist(cmdline);
			break;
		case 0x10:	/* ^P */
			prevhist(cmdline);
			break;
		case 0x15:	/* ^U */
			killline(cmdline, 1);
			break;
		case 0x17:	/* ^W */
			killregion(cmdline);
			break;
		case 0x18:	/* ^X */
			cmdline->cex = 1;
			break;
		case 0x19:	/* ^Y */
			yank(cmdline);
			break;
		case 0x0a:
		case 0x0d:
			cmdline->cmdputc(cmdline->cmdputcarg, '\r');
			cmdline->cmdputc(cmdline->cmdputcarg, '\n');
			addhist(cmdline, cmdline->line);
			return cmdline->line;

		default:
			if ((ch < 0x20) || (ch >= 0x7f))
				beep(cmdline);
			else
				insert(cmdline, ch);
			break;
		}
	}

	return NULL;	/* in editing */
}

ICACHE_FLASH_ATTR
static int
execute(char *cmd, struct cmdtbl *cmdtbl, void *arg)
{
#define MAXARGV	16
	char *argv[MAXARGV];
	int argc, i;
	char *p, *q, quote;

	/* parse commandline */
	argc = 0;
	p = cmd;
	quote = 0;
	for (;;) {
		while ((*p == ' ') || (*p == '\t'))
			p++;

		if (*p == '\0')
			break;

		argv[argc++] = p;
		if (argc >= MAXARGV)
			break;

		q = p;
		for (;;) {
			switch (*p) {
			case ' ':
			case '\t':
				if (!quote)
					goto done;
				*q++ = *p++;
				break;
			case '\0':
				goto done;
			case '"':
			case '\'':
				if (quote) {
					if (*p == quote) {
						quote = 0;
						p++;
					} else {
						*q++ = *p++;
					}
				} else {
					quote = *p++;
				}
				break;
			case '\\':
				if (*++p == '\0')
					goto done;
				*q++ = *p++;
				break;

			default:
				*q++ = *p++;
				break;
			}
		}
 done:

		if (*p == '\0') {
			*q++ = '\0';
			break;
		}
		*q++ = *p++ = '\0';
	}

	/* function */
	if (argc >= 1) {
		for (i = 0; ; i++) {
			if (cmdtbl[i].cmd == NULL || cmdtbl[i].func == NULL)
				break;
			if (strcmp(cmdtbl[i].cmd, argv[0]) == 0) {
				return cmdtbl[i].func(arg, argc, argv);
			}
		}
	}

	return -1;
}

ICACHE_FLASH_ATTR
void
cmdline_setprompt(struct cmdline *cmdline, const char *prompt)
{
	cmdline->prompt = prompt;
}

ICACHE_FLASH_ATTR
int
commandline(struct cmdline *cmdline, char ch, struct cmdtbl *cmdtbl, void *arg)
{
	char *p;
	int rc;

	if ((p = cmdline_input(cmdline, ch)) != NULL) {
		rc = 0;
		if (execute(p, cmdtbl, arg) != 0) {
			if (!allspace(p)) {
				cmdline_puts(cmdline, p);
				cmdline_puts(cmdline, ": unknown command\r\n");
				rc = -1;
			}
		}

		cmdline_reset(cmdline);
		return rc;
	}
	return 0;
}
