/* ZAK180 Firmaware
 * UART driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdbool.h>

#include <z180/z180.h>

#include "uart.h"

static unsigned char uart0_txready(void)
{
	return STAT0 & (1 << 1);
}

static unsigned char uart1_txready(void)
{
	return STAT1 & (1 << 1);
}

static unsigned char uart0_rxready(void)
{
	return STAT0 & (1 << 7);
}

static unsigned char uart1_rxready(void)
{
	return STAT1 & (1 << 7);
}

int uart0_write_poll(const void *buff, size_t bufflen)
{
	for (size_t i = 0; i < bufflen; ++i) {
		while (!uart0_txready());
		TDR0 = ((unsigned char *)buff)[i];
	}
	return bufflen;
}

int uart0_read_poll(void *buff, size_t bufflen)
{
	/* TODO */
	(void)buff;
	(void)bufflen;
	return -1;
}

int uart0_write(const void *buff, size_t bufflen)
{
	/* TODO */
	(void)buff;
	(void)bufflen;
	return -1;
}

int uart0_read(void *buff, size_t bufflen)
{
	/* TODO */
	(void)buff;
	(void)bufflen;
	return -1;
}

int uart1_write_poll(const void *buff, size_t bufflen)
{
	for (size_t i = 0; i < bufflen; ++i) {
		while (!uart1_txready());
		TDR1 = ((unsigned char *)buff)[i];
	}
	return bufflen;
}

int uart1_read_poll(void *buff, size_t bufflen)
{
	/* TODO */
	(void)buff;
	(void)bufflen;
	return -1;
}

int uart1_write(const void *buff, size_t bufflen)
{
	/* TODO */
	(void)buff;
	(void)bufflen;
	return -1;
}

int uart1_read(void *buff, size_t bufflen)
{
	/* TODO */
	(void)buff;
	(void)bufflen;
	return -1;
}

void uart_init(void)
{
	/* Both UARTs @19200 baud 8n1 */
	CNTLB0 = 0x01;
	CNTLB1 = 0x01;
	CNTLA0 = 0x64;
	CNTLA1 = 0x64;

	/* Enable interrupts */
	//STAT0 = 0x09;
	//STAT1 = 0x09;
}
