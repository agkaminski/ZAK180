/* ZAK180 Firmaware
 * UART driver
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#ifndef DRIVER_UART_H_
#define DRIVER_UART_H_

#include <stdlib.h>

int uart0_write(const void *buff, size_t bufflen, int block);

int uart0_read(void *buff, size_t bufflen, int block);

int uart1_write(const void *buff, size_t bufflen, int block);

int uart1_read(void *buff, size_t bufflen, int block);

void uart_init(void);

#endif
