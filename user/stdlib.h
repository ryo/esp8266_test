#ifndef _STDLIB_H_
#define _STDLIB_H_

int isspace(int);
int isupper(int);
int islower(int);
long int strtol(const char *nptr, char **endptr, int base);

#define LONG_MAX	0x7fffffffL
#define LONG_MIN	-0x7fffffffL

#define strchr ets_strchr
char *strchr(const char *s, int c);

#define __UNCONST(_a_) ((void *)(unsigned long)(const void *)(_a_))

#endif /* _STDLIB_H_ */
