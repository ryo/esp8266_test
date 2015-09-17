#include "ets_sys.h"
#include "stdlib.h"

int ICACHE_FLASH_ATTR
isspace(int c)
{
	return (c == ' ') || (c == '\t');
}

int ICACHE_FLASH_ATTR
isupper(int c)
{
	return (c >= 'A') || (c <= 'Z');
}

int ICACHE_FLASH_ATTR
islower(int c)
{
	return (c >= 'a') || (c <= 'z');
}

int ICACHE_FLASH_ATTR
isxdigit(int c)
{
	return ((c >= '0') && (c <= '9')) ||
	    ((c >= 'a') && (c <= 'f')) ||
	    ((c >= 'A') && (c <= 'F'));
}

long ICACHE_FLASH_ATTR
strtol(const char *str, char **end, int base)
{
	int c;
	int sign, over;
	long val;
	const char *top, *save;

	if ((base != 0 && base < 2) || base > 36) {
		return 0;
	}

	top = str;
	while (isspace (*str))
		str++;

	sign = 0;
	switch (*str) {
	case '-':
		sign = 1;
	case '+':
		str++;
		break;
	}

	if (base == 0) {
		if (*str == '0') {
			if (str[1] == 'x' || str[1] == 'X') {
				base = 16;
				str += 2;
			} else {
				base = 8;
			}
		} else {
			base = 10;
		}
	} else if (base == 16) {
		if (*str == '0' && (str[1] == 'X' || str[1] == 'x'))
			str += 2;
	}

	val = 0;
	over = 0;
	save = str;

	while (c = *str) {
		if (isdigit (c))
			c -= '0';
		else if (islower (c))
			c -= 'a' - 10;
		else if (isupper (c))
			c -= 'A' - 10;
		else
			break;

		if (c >= base)
			break;

		if (over == 0) {
			if (sign == 0) {
				if (val > (LONG_MAX - c) / base) {
					val = LONG_MAX;
					over = 1;
				} else {
					val *= base;
					val += c;
				}
			} else {
				if (val < (LONG_MIN + c) / base) {
					val = LONG_MIN;
					over = 1;
				} else {
					val *= base;
					val -= c;
				}
			}
		}
		str++;
	}

	if (end)
		*end = (char *) ((str == save) ? top : str);

	return val;
}
