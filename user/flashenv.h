#ifndef _FLASHENV_H_
#define _FLASHENV_H_

int flashenv_printenv(void *);
int flashenv_setenv(void *, const char *, const char *);
const char *flashenv_getenv(const char *);
int flashenv_save(void);

#endif /* _FLASHENV_H_ */
