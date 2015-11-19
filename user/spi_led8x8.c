#include "driver/spi_register.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "c_types.h"

#include "printf.h"
#include "gpio.h"

#include "spi_led8x8.h"

#define GPIO_SI			BIT13
#define GPIO_SCLK		BIT14
#define GPIO_RCLK		BIT15

static unsigned char led_matrix[8] = {
	0x3c,
	0x42,
	0x81,
	0xa5,
	0x81,
	0xbd,
	0x42,
	0x3c
};

void
spi_led8x8_setpattern(unsigned char data[8])
{
	memcpy(led_matrix, data, 8);
}


ICACHE_FLASH_ATTR
static void
gpio_set(int gpio, int on)
{
	if (on)
		gpio_output_set(gpio, 0, gpio, 0);
	else
		gpio_output_set(0, gpio, gpio, 0);
}

#define SPI	0
#define HSPI	1

ICACHE_FLASH_ATTR
static void
spi_intr(void *arg)
{
	static int nintr = 0;
	int row;
	uint32 reg;

	if (READ_PERI_REG(0x3ff00020) & BIT4) {
		/* SPI interrupt */
		CLEAR_PERI_REG_MASK(SPI_SLAVE(SPI), 0x3ff);

	} else if (READ_PERI_REG(0x3ff00020) & BIT7) {
		/* HSPI interrupt */

		reg = READ_PERI_REG(SPI_SLAVE(HSPI));
		CLEAR_PERI_REG_MASK(SPI_SLAVE(HSPI),
		    SPI_TRANS_DONE_EN |
		    SPI_SLV_WR_STA_DONE_EN |
		    SPI_SLV_RD_STA_DONE_EN |
		    SPI_SLV_WR_BUF_DONE_EN |
		    SPI_SLV_RD_BUF_DONE_EN);
		SET_PERI_REG_MASK(SPI_SLAVE(HSPI), SPI_SYNC_RESET);
		CLEAR_PERI_REG_MASK(SPI_SLAVE(HSPI),
		    SPI_TRANS_DONE |
		    SPI_SLV_WR_STA_DONE |
		    SPI_SLV_RD_STA_DONE |
		    SPI_SLV_WR_BUF_DONE |
		    SPI_SLV_RD_BUF_DONE); 
		SET_PERI_REG_MASK(SPI_SLAVE(HSPI),
		    SPI_TRANS_DONE_EN |
		    SPI_SLV_WR_STA_DONE_EN |
		    SPI_SLV_RD_STA_DONE_EN |
		    SPI_SLV_WR_BUF_DONE_EN |
		    SPI_SLV_RD_BUF_DONE_EN);

		gpio_set(GPIO_RCLK, 1);

		row = ++nintr & 7;
		spi_write((0x8000 >> row) | led_matrix[row], 16);

	} else if (READ_PERI_REG(0x3ff00020) & BIT9) {
		/* I2S interrupt */
	}
}


ICACHE_FLASH_ATTR
void
spi_init(void)
{
	/* GPIO13,14,15 are HSPI mode */
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);
	CLEAR_PERI_REG_MASK(PERIPHS_IO_MUX, BIT9);


	WRITE_PERI_REG(SPI_USER(HSPI),
	    SPI_USR_COMMAND |
	    SPI_CS_SETUP |
	    SPI_CS_HOLD);

#define CLKDIV_N	1
#define CLKDIV_H	(((CLKDIV_N + 1) / 2) - 1)
#define CLKDIV_L	CLKDIV_N

	WRITE_PERI_REG(SPI_CLOCK(HSPI), 
	    ((500 & SPI_CLKDIV_PRE) << SPI_CLKDIV_PRE_S) |
	    ((CLKDIV_N & SPI_CLKCNT_N) << SPI_CLKCNT_N_S) |
	    ((CLKDIV_H & SPI_CLKCNT_H) << SPI_CLKCNT_H_S) |
	    ((CLKDIV_L & SPI_CLKCNT_L) << SPI_CLKCNT_L_S));

	/* set master mode, enable trans done interrupt */
	WRITE_PERI_REG(SPI_SLAVE(HSPI),
	    SPI_TRANS_DONE_EN);


	ETS_SPI_INTR_ATTACH(spi_intr, NULL);
	ETS_SPI_INTR_ENABLE(); 
}

ICACHE_FLASH_ATTR
void
spi_write(uint32_t data, int bitlen)
{
	uint32 reg;

	/* wait SPI available */
	while (READ_PERI_REG(SPI_CMD(HSPI)) & SPI_USR)
		;

	/* write data as SPI command */
	WRITE_PERI_REG(SPI_USER2(HSPI), 
	    (((bitlen - 1) & SPI_USR_COMMAND_BITLEN) << SPI_USR_COMMAND_BITLEN_S) |
	    ((uint32)data));

	gpio_set(GPIO_RCLK, 0);

	/* trigger */
	WRITE_PERI_REG(SPI_CMD(HSPI), SPI_USR);
}

ICACHE_FLASH_ATTR
void
spi_start(void)
{
	/* start spi transfer */
	spi_write(0x8000 | led_matrix[0], 16);

	/* continued by interrupt chain */
}
