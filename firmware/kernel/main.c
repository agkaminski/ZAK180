/* ZAK180 Firmaware
 * Kernel main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdio.h>

#include "../driver/uart.h"
#include "../driver/vga.h"

#define PAGE_SIZE    (4 * 1024)
#define SCRATCH_SIZE (8 * 1024)

int putchar(int c)
{
	char t = c;

	uart1_write_poll(&t, 1);
	vga_putchar(t);

	return 1;
}

int main(void)
{
	uart_init();
	vga_init();

	printf("Kernel Hello World!\r\n");

	/* Give vblank some time */
	for (volatile uint16_t i = 1; i != 0; ++i);

	return 0;
}
