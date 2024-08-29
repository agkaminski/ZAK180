/* ZAK180 Firmaware
 * UART driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_UART_H_
#define DRIVER_UART_H_

#include <stdlib.h>

int uart0_write_poll(const void *buff, size_t bufflen);

int uart0_read_poll(void *buff, size_t bufflen);

int uart0_write(const void *buff, size_t bufflen);

int uart0_read(void *buff, size_t bufflen);

int uart1_write_poll(const void *buff, size_t bufflen);

int uart1_read_poll(void *buff, size_t bufflen);

int uart1_write(const void *buff, size_t bufflen);

int uart1_read(void *buff, size_t bufflen);

void uart_init(void);

#endif
