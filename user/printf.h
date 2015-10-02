#ifndef _PRINTF_H_
#define _PRINTF_H_

#define __printflike(fmtarg, firstvararg)	\
	__attribute__((__format__ (__printf__, fmtarg, firstvararg)))

#define vsprintf	ets_vsprintf
#define vnsprintf	ets_vnsprintf
#define sprintf		ets_sprintf
extern int printf(const char *__restrict __format, ...);
extern int sprintf(char *, const char *__restrict __format, ...);
extern int xprintf(void *, const char *__restrict __format, ...) __printflike(2, 3);

#endif /* _PRINTF_H_ */
