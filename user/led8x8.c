#include "ets_sys.h"
#include "c_types.h"
#include "osapi.h"
#include "gpio.h"


/* SPI compatible */
#define	GPIO_SI		BIT13
#define	GPIO_SCLK	BIT14
#define	GPIO_RCLK	BIT15


static unsigned char num_pattern_mini[10][8] = {
	{	0b00000000,
		0b00000000,
		0b00000111,
		0b00000101,
		0b00000101,
		0b00000101,
		0b00000111,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000001,
		0b00000001,
		0b00000001,
		0b00000001,
		0b00000001,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000111,
		0b00000001,
		0b00000111,
		0b00000100,
		0b00000111,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000111,
		0b00000001,
		0b00000111,
		0b00000001,
		0b00000111,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000101,
		0b00000101,
		0b00000111,
		0b00000001,
		0b00000001,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000111,
		0b00000100,
		0b00000111,
		0b00000001,
		0b00000111,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000111,
		0b00000100,
		0b00000111,
		0b00000101,
		0b00000111,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000111,
		0b00000001,
		0b00000001,
		0b00000001,
		0b00000001,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000111,
		0b00000101,
		0b00000111,
		0b00000101,
		0b00000111,
		0b00000000},
	{	0b00000000,
		0b00000000,
		0b00000111,
		0b00000101,
		0b00000111,
		0b00000001,
		0b00000111,
		0b00000000},
};

static unsigned char num_pattern[10][8] = {
	{	0b00000000,
		0b00111000,
		0b01000100,
		0b01000100,
		0b01000100,
		0b01000100,
		0b01000100,
		0b00111000	},
	{	0b00000000,
		0b00010000,
		0b00110000,
		0b00010000,
		0b00010000,
		0b00010000,
		0b00010000,
		0b00111000	},
	{	0b00000000,
		0b00111000,
		0b01000100,
		0b00001000,
		0b00010000,
		0b00100000,
		0b01000000,
		0b01111100	},
	{	0b00000000,
		0b00111000,
		0b01000100,
		0b00000100,
		0b00011000,
		0b00000100,
		0b01000100,
		0b00111000	},
	{	0b00000000,
		0b00001000,
		0b00011000,
		0b00101000,
		0b01001000,
		0b01111100,
		0b00001000,
		0b00001000	},
	{	0b00000000,
		0b01111100,
		0b01000000,
		0b01000000,
		0b01111000,
		0b00000100,
		0b00000100,
		0b01111000	},
	{	0b00000000,
		0b00111000,
		0b01000100,
		0b01000000,
		0b01111000,
		0b01000100,
		0b01000100,
		0b00111000	},
	{	0b00000000,
		0b01111100,
		0b01000100,
		0b00000100,
		0b00001000,
		0b00001000,
		0b00010000,
		0b00010000	},
	{	0b00000000,
		0b00111000,
		0b01000100,
		0b01000100,
		0b00111000,
		0b01000100,
		0b01000100,
		0b00111000	},
	{	0b00000000,
		0b00111000,
		0b01000100,
		0b01000100,
		0b00111100,
		0b00000100,
		0b01000100,
		0b00111000	}
};

static unsigned char led_matrix[8] = {
#if 0
	0x3c,
	0x42,
	0x81,
	0xa5,
	0x81,
	0xbd,
	0x42,
	0x3c
#endif
#if 1
	0x80,
	0x40,
	0x20,
	0x10,
	0x08,
	0x04,
	0x02,
	0x01
#endif
};

void
led8x8_setpattern(unsigned char data[8])
{
	memcpy(led_matrix, data, 8);
}

void
led8x8_num(int n)
{
	if (n >= 10)
		n = n % 10;

	led8x8_setpattern(num_pattern[n]);
}

void
led8x8_num2(int n)
{
	int i, h, l;

	if (n >= 100)
		n = n % 100;

	h = n / 10;
	l = n % 10;

	for (i = 0; i < 8; i++) {
		led_matrix[i] = (num_pattern_mini[h][i] << 4) +
		    (num_pattern_mini[l][i]);
	}
}

static int led_count;

ICACHE_FLASH_ATTR
static void
gpio_set(int gpio, int on)
{
	if (on)
		gpio_output_set(gpio, 0, gpio, 0);
	else
		gpio_output_set(0, gpio, gpio, 0);
}

#define SI_SET(on)	gpio_set(GPIO_SI, on)
#define SCLK_SET(on)	gpio_set(GPIO_SCLK, on)
#define RCLK_SET(on)	gpio_set(GPIO_RCLK, on)


ICACHE_FLASH_ATTR
void
led8x8_update(void)
{
	int row, i;

	led_count++;
	row = (led_count & 7);

	RCLK_SET(0);

	for (i = 0; i < 8; i++) {
		SI_SET(led_matrix[row] & (0x80 >> i));
		SCLK_SET(1);
		SCLK_SET(0);
	}

	for (i = 0; i < 8; i++) {
		SI_SET(i == row);
		SCLK_SET(1);
		SCLK_SET(0);
	}

	RCLK_SET(1);
}
