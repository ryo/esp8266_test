#ifndef _PRINTF_H_
#define _PRINTF_H_

#define vsprintf	ets_vsprintf
#define sprintf		ets_sprintf
extern int printf(const char *__restrict __format, ...);
extern int sprintf(char *, const char *__restrict __format, ...);

#endif /* _PRINTF_H_ */
