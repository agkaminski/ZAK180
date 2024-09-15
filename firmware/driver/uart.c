/* ZAK180 Firmaware
 * UART driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <stdint.h>

#include <z180/z180.h>

#include "critical.h"
#include "uart.h"

#define FIFO_SIZE 64

struct fifo {
	uint8_t buff[FIFO_SIZE];
	uint8_t rd, wr;
};

struct uart_ctx {
	struct fifo tx;
	struct fifo rx;
};

static struct uart_ctx uart[2];

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

static int _fifo_push(struct fifo *fifo, uint8_t c, uint8_t overwrite)
{
	if (((fifo->wr + 1) % sizeof(fifo->buff)) == fifo->rd) {
		if (overwrite) {
			++fifo->rd;
		}
		else {
			return -1;
		}
	}

	fifo->buff[fifo->wr] = c;
	fifo->wr = (fifo->wr + 1) % sizeof(fifo->buff);

	return 0;
}

static int fifo_push(struct fifo *fifo, uint8_t c)
{
	_CRITICAL_START;
	int ret = _fifo_push(fifo, c, 0);
	_CRITICAL_END;

	return ret;
}

static int _fifo_pop(struct fifo *fifo, uint8_t *c)
{
	if (fifo->wr == fifo->rd) {
		return -1;
	}

	*c = fifo->buff[fifo->rd];
	fifo->rd = (fifo->rd + 1) % sizeof(fifo->buff);

	return 0;
}

static int fifo_pop(struct fifo *fifo, uint8_t *c)
{
	_CRITICAL_START;
	int ret = _fifo_pop(fifo, c);
	_CRITICAL_END;

	return ret;
}

void uart0_irq_handler(void)
{
	if (uart0_rxready()) {
		(void)_fifo_push(&uart[0].rx, RDR0, 1);
	}

	if (uart0_txready()) {
		uint8_t c;
		if (_fifo_pop(&uart[0].tx, &c) != 0) {
			STAT0 &= ~1;
		}
		else {
			TDR0 = c;
		}
	}
}

void uart1_irq_handler(void)
{
	if (uart1_rxready()) {
		(void)_fifo_push(&uart[1].rx, RDR1, 1);
	}

	if (uart1_txready()) {
		uint8_t c;
		if (_fifo_pop(&uart[1].tx, &c) != 0) {
			STAT1 &= ~1;
		}
		else {
			TDR1 = c;
		}
	}
}

int uart0_write(const void *buff, size_t bufflen, int block)
{
	size_t len;

	for (len = 0; len < bufflen; ++len) {
		int res = fifo_push(&uart[0].tx, ((uint8_t *)buff)[len]);

		STAT0 |= 1;

		if (res != 0) {
			if (!block) {
				break;
			}

			/* TODO reschedule */
		}
	}

	return len;
}

int uart0_write_poll(const void *buff, size_t bufflen)
{
	for (size_t i = 0; i < bufflen; ++i) {
		while (!uart0_txready());
		TDR0 = ((unsigned char *)buff)[i];
	}
	return bufflen;
}

int uart0_read(void *buff, size_t bufflen, int block)
{
	size_t len;

	for (len = 0; len < bufflen; ++len) {
		while (fifo_pop(&uart[0].rx, (uint8_t *)buff + len) < 0) {
			if (!block) {
				break;
			}

			/* TODO reschedule */
		}
	}

	return len;
}

int uart1_write(const void *buff, size_t bufflen, int block)
{
	size_t len;

	for (len = 0; len < bufflen; ++len) {
		int res = fifo_push(&uart[1].tx, ((uint8_t *)buff)[len]);

		STAT1 |= 1;

		if (res != 0) {
			if (!block) {
				break;
			}

			/* TODO reschedule */

		}
	}

	return len;
}

int uart1_write_poll(const void *buff, size_t bufflen)
{
	for (size_t i = 0; i < bufflen; ++i) {
		while (!uart1_txready());
		TDR1 = ((unsigned char *)buff)[i];
	}
	return bufflen;
}

int uart1_read(void *buff, size_t bufflen, int block)
{
	size_t len;

	for (len = 0; len < bufflen; ++len) {
		while (fifo_pop(&uart[1].rx, (uint8_t *)buff + len) < 0) {
			if (!block) {
				break;
			}

			/* TODO reschedule */
		}
	}

	return len;
}

void uart_init(void)
{
	/* Both UARTs @19200 baud 8n1 */
	CNTLB0 = 0x01;
	CNTLB1 = 0x01;
	CNTLA0 = 0x64;
	CNTLA1 = 0x64;

	/* Enable RX interrupts */
	STAT0 = 0x08;
	STAT1 = 0x08;
}
