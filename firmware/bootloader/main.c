/* ZAK180 Firmaware
 * Bootloader main
 * Copyright: Aleksander Kaminski, 2024
 * See LICENSE.md
 */

#include <string.h>
#include <stdio.h>

#include "../driver/uart.h"
#include "../driver/floppy.h"

int putchar(int c)
{
	char t = c;

	uart1_write_poll(&t, 1);

	return 1;
}

int main(void)
{
	uart_init();

	printf("ZAK180 Bootloader rev " VERSION "\r\n");

	floppy_init();

	return 0;
}
